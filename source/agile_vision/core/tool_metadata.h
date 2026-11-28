/*
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *
 *   File: tool_metadata.h
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

#ifndef __agile_vision_core_tool_metadata_h__
#define __agile_vision_core_tool_metadata_h__

#include <cstdint>
#include <string>
#include <vector>
#include "agile_vision/core/core_export.h"
#include "agile_vision/core/core_typedef.h"
#include "agile_vision/core/data_spec.h"

namespace agv::core {

    /// @brief 插件 ABI 主版本：1 为旧三函数（无元数据），2 增加版本协商与 Pin 清单。
    constexpr int kToolAbiV1 = 1;
    constexpr int kToolAbiV2 = 2;
    constexpr int kToolAbiCurrent = kToolAbiV2;
    constexpr uint32_t kToolMetaMaxPins = 256;

    /// @brief 一个 Pin 的 C ABI 描述：纯 POD，字符串指针由插件持有，调用方需立即拷贝。
    struct AgvPinMetaC
    {
        const char* key = nullptr;
        int32_t pin_type = 0;
        int32_t data_type = 0;
        uint32_t subtype = 0;
        uint32_t array_size = 0;
        const char* alias = nullptr;
        int32_t deprecated = 0;
        int32_t optional = 0;
    };

    /// @brief 一个工具的 C ABI 元数据；pins 数组由插件持有，长度受 kToolMetaMaxPins 限制。
    struct AgvToolMetaC
    {
        int32_t abi_version = kToolAbiV1;
        const char* clsid = nullptr;
        const char* name = nullptr;
        uint32_t pin_count = 0;
        const AgvPinMetaC* pins = nullptr;
    };

    /// @brief C++ 侧值语义 Pin 元数据，供 UI 自动生成配置界面使用。
    struct PinMetadata
    {
        std::string key;
        PinType pin_type = PinType::kProp;
        DataType data_type = DataType::kUnk;
        unsigned int subtype = 0;
        unsigned int array_size = 1;
        std::string alias;
        bool deprecated = false;
        bool optional = false;
    };

    /// @brief C++ 侧值语义工具元数据。
    struct ToolMetadata
    {
        int abi_version = kToolAbiV1;
        std::string clsid;
        std::string name;
        std::vector<PinMetadata> pins;
    };

    /// @brief 插件 DLL 可选导出：int32_t GetToolABIVersion(void)。缺失时视为 v1。
    using GetToolAbiVersionFunc = int32_t(*)();
    /// @brief 插件 DLL 可选导出（v2 必需）：const AgvToolMetaC* GetToolMeta(void)。
    using GetToolMetaFunc = const AgvToolMetaC*(*)();

    /// @brief 判断宿主是否支持插件声明的 ABI 版本。
    AGV_CORE_API bool isToolAbiVersionSupported(int version);
    /// @brief 将 C ABI 元数据立即拷贝为值语义结构；畸形输入返回 false。
    AGV_CORE_API bool copyToolMetadata(const AgvToolMetaC* c_meta, ToolMetadata* out);

}

#endif
