/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: global_variable.h  
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

#ifndef __global_variable_h__
#define __global_variable_h__

#include <any>
#include <string>
#include <optional>
#include "agile_vision/core/core_export.h"
#include "agile_vision/basic/base_type_def.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API GlobalVariable final
{
public:
    enum DataType
    {
        DT_BOOLEAN,
        DT_INT,
        DT_FLOAT,
        DT_STRING
    };
    AgvBytes serializeToBytes()const;
    size_t loadBytes(ConsAgvBytePtr buffer, size_t size);
    void setBool(bool s);
    std::optional<bool> getBoolValue()const;
    void setInt(int a);
    std::optional<int> getIntValue()const;
    void setFloat(float f);
    std::optional<float> getFloatValue()const;
    void setString(const std::string& s);
    std::optional<std::string> getStringValue()const;
    auto dataType()const{ return data_type_; }
    explicit GlobalVariable(DataType type);
    ~GlobalVariable();

private:
    DataType data_type_;
    std::any value_;
};

AGV_NAMESPACE_END

#endif