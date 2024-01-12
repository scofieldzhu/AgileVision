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
#include "spdlog/spdlog.h"

using namespace ratel;
using std::filesystem::path;

AGV_NAMESPACE_BEGIN

ToolFactory::ToolFactory()
{
}

ToolFactory::~ToolFactory()
{
    unloadAll();
}

bool ToolFactory::loadToolFile(const path& filepath)
{
    if(loader_dict_.contains(filepath)){
        spdlog::warn("The tool file:{} already loaded!", filepath.string());
        return true;
    }
    auto loader = std::make_unique<DynamicLibraryLoader>();
    if(!loader->load(filepath.string())){
        spdlog::error("Load tool file:{} failed!", filepath.string());
        return false;
    }
    DllInfo di;    
    di.get_tool_clsid_func = loader->getFunction<GetToolClsIdFunc>("GetToolClsID");
    di.create_tool_func  = loader->getFunction<CreateToolInstanceFunc>("CreateToolInstance");
    di.destroy_tool_func = loader->getFunction<DestroyToolInstanceFunc>("DestroyToolInstance");
    di.loader = std::move(loader);
    if(di){
        di.cls_iid = (di.get_tool_clsid_func)();
        loader_dict_[filepath] = std::move(di);
        return true;
    }
    spdlog::error("Invalid tool file:{} failed!", filepath.string());
    return false;
}

ToolSPtr ToolFactory::createTool(const std::string& clsid, const std::string& iid) const
{
    for(const auto& kv : loader_dict_){
        if(kv.second.cls_iid == clsid){
            const auto& di = kv.second;
            ToolPtr new_instance = (di.create_tool_func)(iid.c_str());
            ToolSPtr new_tool(new_instance, di.destroy_tool_func);
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

AGV_NAMESPACE_END