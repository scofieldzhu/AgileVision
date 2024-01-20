/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: global_variable.cpp  
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

#include "global_variable.h"
#include "ratel/geometry/element_proxy.hpp"
#include "ratel/geometry/proxy_combine.hpp"
#include "ratel/basic/string_proxy.h"
using namespace ratel;

AGV_NAMESPACE_BEGIN

namespace{
    using IntProxy = ElementProxy<int>;
}

GlobalVariable::GlobalVariable(DataType type)
    :data_type_(type)
{
}

GlobalVariable::~GlobalVariable()
{
}

AgvBytes GlobalVariable::serializeToBytes() const
{
    
    
    return AgvBytes();
}

size_t GlobalVariable::loadBytes(ConsAgvBytePtr buffer, size_t size)
{
    return size_t();
}

void GlobalVariable::setBool(bool s)
{
    if(data_type_ == DT_BOOLEAN)
        value_ = std::make_any<bool>(s);
}

std::optional<bool> GlobalVariable::getBoolValue() const
{
    return data_type_ == DT_BOOLEAN ? std::optional<bool>(std::any_cast<bool>(value_)) : std::nullopt;
}

void GlobalVariable::setInt(int a)
{
    if(data_type_ == DT_INT)
        value_ = std::make_any<int>(a);
}

std::optional<int> GlobalVariable::getIntValue() const
{
    return data_type_ == DT_INT ? std::optional<int>(std::any_cast<int>(value_)) : std::nullopt;
}

void GlobalVariable::setFloat(float f)
{
    if(data_type_ == DT_FLOAT)
        value_ = std::make_any<float>(f);
}

std::optional<float> GlobalVariable::getFloatValue() const
{
    return data_type_ == DT_FLOAT ? std::optional<float>(std::any_cast<float>(value_)) : std::nullopt;
}

void GlobalVariable::setString(const std::string& s)
{
    if(data_type_ == DT_STRING)
        value_ = std::make_any<std::string>(s);
}

std::optional<std::string> GlobalVariable::getStringValue() const
{
    return data_type_ == DT_STRING ? std::optional<std::string>(std::any_cast<std::string>(value_)) : std::nullopt;
}

AGV_NAMESPACE_END