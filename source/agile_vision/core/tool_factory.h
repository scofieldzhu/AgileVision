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

#ifndef __tool_factory_h__
#define __tool_factory_h__

#include <map>
#include <filesystem>
#include "agile_vision/core/core_export.h"
#include "agile_vision/core/core_base_def.h"
#include "ratel/system/dynamic_library_loader.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API ToolFactory 
{
public:
    bool loadToolFile(const std::filesystem::path& filepath);
    ToolPtr createTool(const std::string& clsid, const std::string& iid)const;
    void unloadToolFile(const std::filesystem::path& filepath);
    void unloadAll();
    ToolFactory();
    ~ToolFactory();

private:
    using GetToolClsIdFunc = const char*(*)();
    using CreateToolInstanceFunc = Tool*(*)(const char*);
    using DestroyToolInstanceFunc = void(*)(Tool*);
    using DllLoaderPtr = std::shared_ptr<ratel::DynamicLibraryLoader>;
    struct DllInfo
    {
        DllLoaderPtr loader = nullptr;
        std::string cls_iid;
        GetToolClsIdFunc get_tool_clsid_func = nullptr;
        CreateToolInstanceFunc create_tool_func = nullptr;
        DestroyToolInstanceFunc destroy_tool_func = nullptr;
        operator bool()const{
            return loader && get_tool_clsid_func && create_tool_func && destroy_tool_func;
        }
    };
    std::map<std::filesystem::path, DllInfo> loader_dict_;
};

AGV_NAMESPACE_END

#endif
