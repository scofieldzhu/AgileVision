/*
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *
 *   File: tool_metadata.cpp
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

#include "tool_metadata.h"

namespace agv::core {

    bool isToolAbiVersionSupported(int version)
    {
        return version >= kToolAbiV1 && version <= kToolAbiCurrent;
    }

    bool copyToolMetadata(const AgvToolMetaC* c_meta, ToolMetadata* out)
    {
        if(c_meta == nullptr || out == nullptr){
            return false;
        }
        if(!isToolAbiVersionSupported(c_meta->abi_version)){
            return false;
        }
        // clsid 是工具身份，缺失即拒绝；pin_count 必须有上限，防止畸形插件导致超大分配。
        if(c_meta->clsid == nullptr || c_meta->pin_count > kToolMetaMaxPins){
            return false;
        }
        if(c_meta->pin_count > 0 && c_meta->pins == nullptr){
            return false;
        }
        ToolMetadata result;
        result.abi_version = c_meta->abi_version;
        result.clsid = c_meta->clsid;
        result.name = c_meta->name != nullptr ? c_meta->name : "";
        result.pins.reserve(c_meta->pin_count);
        for(uint32_t i = 0; i < c_meta->pin_count; ++i){
            const auto& c_pin = c_meta->pins[i];
            if(c_pin.key == nullptr){
                return false;
            }
            PinMetadata pin;
            pin.key = c_pin.key;
            pin.pin_type = static_cast<PinType>(c_pin.pin_type);
            pin.data_type = static_cast<DataType>(c_pin.data_type);
            pin.subtype = c_pin.subtype;
            pin.array_size = c_pin.array_size;
            pin.alias = c_pin.alias != nullptr ? c_pin.alias : "";
            pin.deprecated = c_pin.deprecated != 0;
            pin.optional = c_pin.optional != 0;
            result.pins.push_back(std::move(pin));
        }
        *out = std::move(result);
        return true;
    }

}
