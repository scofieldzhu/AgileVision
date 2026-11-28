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
#include "agile_vision/geometry/element_proxy.hpp"
#include "agile_vision/geometry/proxy_combine.hpp"
#include "agile_vision/basic/string_proxy.h"
#include "agile_vision/basic/portable_stream.h"
#include <cstring>
using namespace agv;

namespace agv::core {

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
        AgvBytes payload;
        agv::detail::writeU32(payload, static_cast<uint32_t>(data_type_));
        if(data_type_ == DT_BOOLEAN){
            const auto* val = std::any_cast<bool>(&value_);
            agv::detail::writeU8(payload, val != nullptr && *val ? 1u : 0u);
        }else if(data_type_ == DT_INT)
        {
            const auto* val = std::any_cast<int>(&value_);
            agv::detail::writeScalar(payload, val == nullptr ? 0 : *val);
        }else if(data_type_ == DT_FLOAT)
        {
            const auto* val = std::any_cast<float>(&value_);
            agv::detail::writeScalar(payload, val == nullptr ? 0.0f : *val);
        }else if(data_type_ == DT_STRING)
        {
            const auto* val = std::any_cast<std::string>(&value_);
            const std::string str_value = val == nullptr ? std::string() : *val;
            StringProxy proxy;
            proxy = str_value;
            agv::detail::appendBytes(payload, proxy.serializeToBytes());
        }
        AgvBytes framed;
        agv::detail::writeFramedBytes(framed, agv::detail::kGlobalVariableMagic, payload);
        return framed;
    }

    size_t GlobalVariable::loadBytes(ConsAgvBytePtr buffer, size_t size)
    {
        ConsAgvBytePtr payload = nullptr;
        size_t payload_size = 0;
        if(!agv::detail::readFramedBytes(buffer, size, agv::detail::kGlobalVariableMagic, payload, payload_size)){
            return 0;
        }
        ConsAgvBytePtr cur = payload;
        size_t left = payload_size;
        uint32_t type_value = 0;
        if(!agv::detail::readU32(cur, left, type_value) || type_value > static_cast<uint32_t>(DT_STRING)){
            return 0;
        }
        const DataType loaded_type = static_cast<DataType>(type_value);
        if(loaded_type == DT_BOOLEAN){
            uint8_t bool_value = 0;
            if(!agv::detail::readU8(cur, left, bool_value) || bool_value > 1u){
                return 0;
            }
            value_ = std::make_any<bool>(bool_value != 0u);
        }else if(loaded_type == DT_INT)
        {
            int int_value = 0;
            if(!agv::detail::readScalar(cur, left, int_value)){
                return 0;
            }
            value_ = std::make_any<int>(int_value);
        }else if(loaded_type == DT_FLOAT)
        {
            float float_value = 0.0f;
            if(!agv::detail::readScalar(cur, left, float_value)){
                return 0;
            }
            value_ = std::make_any<float>(float_value);
        }else
        {
            StringProxy proxy;
            const auto str_used = proxy.loadBytes(cur, left);
            if(str_used == 0 || str_used > left){
                return 0;
            }
            cur += str_used;
            left -= str_used;
            value_ = std::make_any<std::string>(proxy.stdStr());
        }
        if(left != 0){
            return 0;
        }
        data_type_ = loaded_type;
        return agv::detail::kPortableHeaderSize + payload_size;
    }

    void GlobalVariable::setBool(bool s)
    {
        if(data_type_ == DT_BOOLEAN){
            value_ = std::make_any<bool>(s);
        }
    }

    std::optional<bool> GlobalVariable::getBoolValue() const
    {
        if(data_type_ != DT_BOOLEAN){
            return std::nullopt;
        }
        const auto* val = std::any_cast<bool>(&value_);
        return val != nullptr ? std::optional<bool>(*val) : std::nullopt;
    }

    void GlobalVariable::setInt(int a)
    {
        if(data_type_ == DT_INT){
            value_ = std::make_any<int>(a);
        }
    }

    std::optional<int> GlobalVariable::getIntValue() const
    {
        if(data_type_ != DT_INT){
            return std::nullopt;
        }
        const auto* val = std::any_cast<int>(&value_);
        return val != nullptr ? std::optional<int>(*val) : std::nullopt;
    }

    void GlobalVariable::setFloat(float f)
    {
        if(data_type_ == DT_FLOAT){
            value_ = std::make_any<float>(f);
        }
    }

    std::optional<float> GlobalVariable::getFloatValue() const
    {
        if(data_type_ != DT_FLOAT){
            return std::nullopt;
        }
        const auto* val = std::any_cast<float>(&value_);
        return val != nullptr ? std::optional<float>(*val) : std::nullopt;
    }

    void GlobalVariable::setString(const std::string& s)
    {
        if(data_type_ == DT_STRING){
            value_ = std::make_any<std::string>(s);
        }
    }

    std::optional<std::string> GlobalVariable::getStringValue() const
    {
        if(data_type_ != DT_STRING){
            return std::nullopt;
        }
        const auto* val = std::any_cast<std::string>(&value_);
        return val != nullptr ? std::optional<std::string>(*val) : std::nullopt;
    }

}