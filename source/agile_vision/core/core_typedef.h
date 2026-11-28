/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: core_typedef.h  
 *   Copyright (c) 2023-2024 scofieldzhu
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

#ifndef __agile_vision_core_typedef_h__
#define __agile_vision_core_typedef_h__

#include <memory>
#include <string_view>
#include "agile_vision/basic/image_data.h"
#include "agile_vision/core/core_export.h"

namespace agv {

    // ===== 公共基础类型：位于 agv 根，供各模块子空间直接访问 =====

    enum class DataType
    {
        kUnk,
        kInt,
        kFloat,
        kString,
        kBytes,
        kImage
    };

    inline bool isFundamentalType(DataType t)
    {
        return t == DataType::kInt || t == DataType::kFloat || t == DataType::kImage;
    }

    inline unsigned int getFundamentalTypeSize(DataType t)
    {
        if(t == DataType::kInt){
            return sizeof(int);
        }
        if(t == DataType::kFloat){
            return sizeof(float);
        }
        if(t == DataType::kImage){
            return sizeof(ImageData);
        }
        return 0;
    }

    enum class PinType
    {
        kInput,
        kOutput,
        kProp
    };

    using PinKey = std::string_view;

    constexpr uint32_t kNullId = 0; // null id for wkid_t\wtid_t
    using wkid_t = uint32_t; //work id for process in engine
    using wkid_list = std::vector<wkid_t>;

    // ===== core 模块类的前向声明：位于 agv::core 子空间 =====

    namespace core {

        class DataBuffer;
        struct DataSpec;

        class Tool;
        using ToolSPtr = std::shared_ptr<Tool>;
        using ToolPtr = Tool*;
        using ConstToolPtr = const Tool*;

        class Process;
        using ProcessPtr = Process*;
        using ConstProcessPtr = const Process*;

        class ProcessManager;

        class Procedure;
        using ProcedurePtr = Procedure*;
        using ConstProcedurePtr = const Procedure*;

        class OutputPin;
        class InputPin;
        class PropPin;
        class ToolPin;
        class Engine;

    } // namespace core
} // namespace agv

#endif
