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
        AgvBytes bytes(kUIntSize, 0);
        const unsigned int type_value = static_cast<unsigned int>(data_type_);
        std::memcpy(bytes.data(), &type_value, kUIntSize);

        if(data_type_ == DT_BOOLEAN)
        {
            const auto* val = std::any_cast<bool>(&value_);
            const unsigned char bool_value = val != nullptr && *val ? 1u : 0u;
            bytes.push_back(static_cast<Byte>(bool_value));
        }
        else if(data_type_ == DT_INT)
        {
            const auto* val = std::any_cast<int>(&value_);
            const int int_value = val == nullptr ? 0 : *val;
            const size_t offset = bytes.size();
            bytes.resize(offset + kIntSize);
            std::memcpy(bytes.data() + offset, &int_value, kIntSize);
        }
        else if(data_type_ == DT_FLOAT)
        {
            const auto* val = std::any_cast<float>(&value_);
            const float float_value = val == nullptr ? 0.0f : *val;
            const size_t offset = bytes.size();
            bytes.resize(offset + kFloatSize);
            std::memcpy(bytes.data() + offset, &float_value, kFloatSize);
        }
        else if(data_type_ == DT_STRING)
        {
            const auto* val = std::any_cast<std::string>(&value_);
            const std::string str_value = val == nullptr ? std::string() : *val;
            StringProxy proxy;
            proxy = str_value;
            const auto str_bytes = proxy.serializeToBytes();
            bytes.insert(bytes.end(), str_bytes.begin(), str_bytes.end());
        }
        return bytes;
    }

    size_t GlobalVariable::loadBytes(ConsAgvBytePtr buffer, size_t size)
    {
        if(buffer == nullptr || size < kUIntSize)
        {
            return 0;
        }
        unsigned int type_value = 0;
        std::memcpy(&type_value, buffer, kUIntSize);
        if(type_value > static_cast<unsigned int>(DT_STRING))
        {
            return 0;
        }
        const DataType loaded_type = static_cast<DataType>(type_value);
        size_t consumed = kUIntSize;

        if(loaded_type == DT_BOOLEAN)
        {
            if(size - consumed < sizeof(unsigned char))
            {
                return 0;
            }
            unsigned char bool_value = 0;
            std::memcpy(&bool_value, buffer + consumed, sizeof(unsigned char));
            consumed += sizeof(unsigned char);
            if(bool_value > 1u)
            {
                return 0;
            }
            value_ = std::make_any<bool>(bool_value != 0u);
        }
        else if(loaded_type == DT_INT)
        {
            if(size - consumed < kIntSize)
            {
                return 0;
            }
            int int_value = 0;
            std::memcpy(&int_value, buffer + consumed, kIntSize);
            consumed += kIntSize;
            value_ = std::make_any<int>(int_value);
        }
        else if(loaded_type == DT_FLOAT)
        {
            if(size - consumed < kFloatSize)
            {
                return 0;
            }
            float float_value = 0.0f;
            std::memcpy(&float_value, buffer + consumed, kFloatSize);
            consumed += kFloatSize;
            value_ = std::make_any<float>(float_value);
        }else
        {
            StringProxy proxy;
            const auto str_used = proxy.loadBytes(buffer + consumed, size - consumed);
            if(str_used == 0)
            {
                return 0;
            }
            consumed += str_used;
            value_ = std::make_any<std::string>(proxy.stdStr());
        }
        data_type_ = loaded_type;
        return consumed;
    }

    void GlobalVariable::setBool(bool s)
    {
        if(data_type_ == DT_BOOLEAN)
        {
            value_ = std::make_any<bool>(s);
        }
    }

    std::optional<bool> GlobalVariable::getBoolValue() const
    {
        if(data_type_ != DT_BOOLEAN)
        {
            return std::nullopt;
        }
        const auto* val = std::any_cast<bool>(&value_);
        return val != nullptr ? std::optional<bool>(*val) : std::nullopt;
    }

    void GlobalVariable::setInt(int a)
    {
        if(data_type_ == DT_INT)
        {
            value_ = std::make_any<int>(a);
        }
    }

    std::optional<int> GlobalVariable::getIntValue() const
    {
        if(data_type_ != DT_INT)
        {
            return std::nullopt;
        }
        const auto* val = std::any_cast<int>(&value_);
        return val != nullptr ? std::optional<int>(*val) : std::nullopt;
    }

    void GlobalVariable::setFloat(float f)
    {
        if(data_type_ == DT_FLOAT)
        {
            value_ = std::make_any<float>(f);
        }
    }

    std::optional<float> GlobalVariable::getFloatValue() const
    {
        if(data_type_ != DT_FLOAT)
        {
            return std::nullopt;
        }
        const auto* val = std::any_cast<float>(&value_);
        return val != nullptr ? std::optional<float>(*val) : std::nullopt;
    }

    void GlobalVariable::setString(const std::string& s)
    {
        if(data_type_ == DT_STRING)
        {
            value_ = std::make_any<std::string>(s);
        }
    }

    std::optional<std::string> GlobalVariable::getStringValue() const
    {
        if(data_type_ != DT_STRING)
        {
            return std::nullopt;
        }
        const auto* val = std::any_cast<std::string>(&value_);
        return val != nullptr ? std::optional<std::string>(*val) : std::nullopt;
    }

}