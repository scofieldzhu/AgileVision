/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: core_base_def.h  
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

#ifndef __core_base_def_h__
#define __core_base_def_h__

#include <memory>
#include <string_view>
#include "agile_vision/basic/image_data.h"

AGV_NAMESPACE_BEGIN

enum class DataType
{
    kUnk,
    kInt,
    kFloat,
    kString,
    kBytes,
    kImage
};

inline bool IsFundamentalType(DataType t)
{
    return t == DataType::kInt || t == DataType::kFloat || t == DataType::kImage;
}

inline unsigned int GetFundamentalTypeSize(DataType t)
{
    if(t == DataType::kInt)
        return sizeof(int);
    if(t == DataType::kFloat)
        return sizeof(float);
    if(t == DataType::kImage)
        return sizeof(ImageData);
    return 0;
}

enum class PinType
{
    kInput,
    kOutput,
    kProp
};

class DataBuffer;
using DataBufferPtr = std::shared_ptr<DataBuffer>;

struct DataSpec;

using PinKey = std::string_view;

class Tool;
using ToolPtr = std::shared_ptr<Tool>;
using ToolList = std::vector<ToolPtr>;

class Process;
using ProcessPtr = std::shared_ptr<Process>;

class ProcessManager;

class Procedure;
using ProcedurePtr = std::shared_ptr<Procedure>;

class OutputPin;
using OutputPinPtr = std::shared_ptr<OutputPin>;

class InputPin;
using InputPinPtr = std::shared_ptr<InputPin>;

class PropPin;
using PropPinPtr = std::shared_ptr<PropPin>;

class ToolPin;
using ToolPinPtr = std::shared_ptr<ToolPin>;

class Engine;
constexpr uint32_t null_id = 0; // null id for wkid_t\wtid_t
using wkid_t = uint32_t; //work id for process in engine
using wkid_list = std::vector<wkid_t>;

AGV_NAMESPACE_END

#endif
