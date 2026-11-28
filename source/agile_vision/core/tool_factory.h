/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: tool_factory.h  
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

#ifndef __agile_vision_core_tool_factory_h__
#define __agile_vision_core_tool_factory_h__

#include <functional>
#include <map>
#include <filesystem>
#include "agile_vision/core/core_typedef.h"
#include "agile_vision/core/tool_metadata.h"
#include "agile_vision/system/dynamic_library_loader.h"

namespace agv::core {

    class AGV_CORE_API ToolFactory 
    {
    public:
        using CreatorFunc = std::function<ToolSPtr(const std::string&)>;
        /// @brief 注册进程内工具类型，供反序列化重建；同名注册覆盖旧实现。
        bool registerCreator(const std::string& clsid, CreatorFunc creator);
        /// @brief 注册工具元数据（UI 配置界面数据源）；meta.clsid 为空时自动用 clsid 填充。
        bool registerMetadata(const std::string& clsid, ToolMetadata meta);
        /// @brief 查询工具元数据：显式注册 > DLL v2 元数据 > 临时实例反射。
        bool getToolMeta(const std::string& clsid, ToolMetadata* out)const;
        std::vector<ToolMetadata> getAllToolMetas()const;
        /// @brief 查询工具 ABI 版本；0 表示未注册或未加载。
        int getToolAbiVersion(const std::string& clsid)const;
        bool loadToolFile(const std::filesystem::path& filepath);
        ToolSPtr createTool(const std::string& clsid, const std::string& iid)const;
        void unloadToolFile(const std::filesystem::path& filepath);
        void unloadAll();
        ToolFactory();
        ~ToolFactory();

    private:
        using GetToolClsIdFunc = const char*(*)();
        using CreateToolInstanceFunc = ToolPtr(*)(const char*);
        using DestroyToolInstanceFunc = void(*)(ToolPtr);
        using DllLoaderPtr = std::shared_ptr<agv::DynamicLibraryLoader>;
        struct DllInfo
        {
            DllLoaderPtr loader = nullptr;
            std::string cls_iid;
            int abi_version = kToolAbiV1;
            ToolMetadata metadata;
            GetToolClsIdFunc get_tool_clsid_func = nullptr;
            CreateToolInstanceFunc create_tool_func = nullptr;
            DestroyToolInstanceFunc destroy_tool_func = nullptr;
            operator bool()const{
                return loader && get_tool_clsid_func && create_tool_func && destroy_tool_func;
            }
        };
        std::map<std::filesystem::path, DllInfo> loader_dict_;
        std::map<std::string, CreatorFunc> registered_creators_;
        std::map<std::string, ToolMetadata> registered_metadata_;
    };

}

#endif
