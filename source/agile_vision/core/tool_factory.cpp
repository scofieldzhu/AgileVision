/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: tool_factory.cpp  
 *   Copyright (c) 2024-2024 scofieldzhu
 *  
 *   MIT License
 *  
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *  
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *  
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include "tool_factory.h"
#include "agile_vision/basic/log_service.h"
#include "agile_vision/core/tool.h"
#include <algorithm>

using namespace agv;
using std::filesystem::path;

namespace agv::core {

    ToolFactory::ToolFactory()
    {
    }

    ToolFactory::~ToolFactory()
    {
        unloadAll();
    }

    bool ToolFactory::registerCreator(const std::string& clsid, CreatorFunc creator)
    {
        if(clsid.empty() || !creator){
            AGV_LOG_ERROR("Invalid tool creator for clsid:{}", clsid);
            return false;
        }
        registered_creators_[clsid] = std::move(creator);
        return true;
    }

    bool ToolFactory::registerMetadata(const std::string& clsid, ToolMetadata meta)
    {
        if(clsid.empty()){
            AGV_LOG_ERROR("Empty clsid for tool metadata");
            return false;
        }
        if(meta.clsid.empty()){
            meta.clsid = clsid;
        }else if(meta.clsid != clsid){
            AGV_LOG_ERROR("Metadata clsid:{} mismatches registration key:{}", meta.clsid, clsid);
            return false;
        }
        if(meta.abi_version == kToolAbiV1){
            meta.abi_version = kToolAbiCurrent;
        }
        registered_metadata_[clsid] = std::move(meta);
        return true;
    }

    bool ToolFactory::getToolMeta(const std::string& clsid, ToolMetadata* out) const
    {
        if(out == nullptr){
            return false;
        }
        const auto explicit_meta = registered_metadata_.find(clsid);
        if(explicit_meta != registered_metadata_.end()){
            *out = explicit_meta->second;
            return true;
        }
        for(const auto& kv : loader_dict_){
            if(kv.second.cls_iid == clsid && kv.second.abi_version >= kToolAbiV2){
                *out = kv.second.metadata;
                return true;
            }
        }
        // 无显式元数据时创建临时实例反射 Pin 清单；探针 iid 不参与运行。
        auto probe = createTool(clsid, "__meta_probe__");
        if(probe == nullptr){
            return false;
        }
        ToolMetadata meta;
        meta.abi_version = kToolAbiCurrent;
        meta.clsid = clsid;
        meta.name = probe->name();
        meta.pins = probe->pinMetadata();
        *out = std::move(meta);
        return true;
    }

    std::vector<ToolMetadata> ToolFactory::getAllToolMetas() const
    {
        std::vector<std::string> clsids;
        for(const auto& kv : registered_creators_){
            clsids.push_back(kv.first);
        }
        for(const auto& kv : registered_metadata_){
            if(std::find(clsids.begin(), clsids.end(), kv.first) == clsids.end()){
                clsids.push_back(kv.first);
            }
        }
        for(const auto& kv : loader_dict_){
            if(std::find(clsids.begin(), clsids.end(), kv.second.cls_iid) == clsids.end()){
                clsids.push_back(kv.second.cls_iid);
            }
        }
        std::vector<ToolMetadata> result;
        result.reserve(clsids.size());
        for(const auto& clsid : clsids){
            ToolMetadata meta;
            if(getToolMeta(clsid, &meta)){
                result.push_back(std::move(meta));
            }
        }
        return result;
    }

    int ToolFactory::getToolAbiVersion(const std::string& clsid) const
    {
        if(registered_creators_.contains(clsid) || registered_metadata_.contains(clsid)){
            return kToolAbiCurrent;
        }
        for(const auto& kv : loader_dict_){
            if(kv.second.cls_iid == clsid){
                return kv.second.abi_version;
            }
        }
        return 0;
    }

    bool ToolFactory::loadToolFile(const path& filepath)
    {
        if(loader_dict_.contains(filepath)){
            AGV_LOG_WARN("The tool file:{} already loaded!", filepath.string());
            return true;
        }
        auto loader = std::make_unique<DynamicLibraryLoader>();
        if(!loader->load(filepath.string())){
            AGV_LOG_ERROR("Load tool file:{} failed!", filepath.string());
            return false;
        }
        DllInfo di;    
        di.get_tool_clsid_func = loader->getFunction<GetToolClsIdFunc>("GetToolClsID");
        di.create_tool_func  = loader->getFunction<CreateToolInstanceFunc>("CreateToolInstance");
        di.destroy_tool_func = loader->getFunction<DestroyToolInstanceFunc>("DestroyToolInstance");
        auto get_abi_version = loader->getFunction<GetToolAbiVersionFunc>("GetToolABIVersion");
        auto get_meta = loader->getFunction<GetToolMetaFunc>("GetToolMeta");
        di.loader = std::move(loader);
        if(!di){
            AGV_LOG_ERROR("Invalid tool file:{} failed!", filepath.string());
            return false;
        }
        di.cls_iid = (di.get_tool_clsid_func)();
        // 旧 DLL 不导出版本符号，按 v1 兼容接入。
        if(get_abi_version != nullptr){
            di.abi_version = static_cast<int>(get_abi_version());
            if(!isToolAbiVersionSupported(di.abi_version)){
                AGV_LOG_ERROR("Unsupported tool ABI version:{} in file:{}", di.abi_version, filepath.string());
                return false;
            }
        }
        if(di.abi_version >= kToolAbiV2){
            const auto* c_meta = get_meta != nullptr ? get_meta() : nullptr;
            if(!copyToolMetadata(c_meta, &di.metadata) || di.metadata.clsid != di.cls_iid){
                AGV_LOG_ERROR("Invalid tool metadata in file:{}", filepath.string());
                return false;
            }
            if(di.metadata.abi_version != di.abi_version){
                AGV_LOG_ERROR("Tool metadata ABI:{} mismatches declared ABI:{}", di.metadata.abi_version, di.abi_version);
                return false;
            }
        }
        loader_dict_[filepath] = std::move(di);
        return true;
    }

    ToolSPtr ToolFactory::createTool(const std::string& clsid, const std::string& iid) const
    {
        const auto registered = registered_creators_.find(clsid);
        if(registered != registered_creators_.end()){
            auto tool = (registered->second)(iid);
            if(tool == nullptr){
                AGV_LOG_ERROR("Registered creator of clsid:{} returned null!", clsid);
            }
            return tool;
        }
        for(const auto& kv : loader_dict_){
            if(kv.second.cls_iid == clsid){
                const auto& di = kv.second;
                ToolPtr new_instance = (di.create_tool_func)(iid.c_str());
                if(new_instance == nullptr){
                    AGV_LOG_ERROR("Create tool instance of clsid:{} failed!", clsid);
                    return nullptr;
                }
                // 删除器捕获 loader 的 shared_ptr，使 DLL 生命周期不短于由其创建的 Tool：
                // 即使工厂先 unload 或析构，DLL 也会保留到最后一个 Tool 销毁后才真正卸载，
                // 避免调用已卸载代码页中的 destroy 函数。
                auto loader = di.loader;
                auto destroy = di.destroy_tool_func;
                ToolSPtr new_tool(new_instance, [loader, destroy](ToolPtr p){
                    if(p != nullptr && destroy != nullptr){
                        destroy(p);
                    }
                });
                return new_tool;
            }
        }
        return nullptr;
    }

    void ToolFactory::unloadToolFile(const path& filepath)
    {
        for(auto& kv : loader_dict_){
            if(kv.first == filepath){
                auto& di = kv.second;
                di.loader = nullptr;
                loader_dict_.erase(filepath);
                break;
            }
        }
    }

    void ToolFactory::unloadAll()
    {
        for(auto& kv : loader_dict_){
            auto& di = kv.second;
            di.loader = nullptr;
        }
        loader_dict_.clear();
    }

}