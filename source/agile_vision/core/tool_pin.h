/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: tool_pin.h  
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

#ifndef __tool_pin_h__
#define __tool_pin_h__

#include "agile_vision/core/data_spec.h"
#include "agile_vision/core/core_export.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API ToolPin
{
public:
    // virtual AgvBytes serializeToBytes()const;
    // virtual size_t loadBytes(ConsAgvBytePtr buffer, size_t size);
    const auto& dataSpec()const{ return ds_; }
    const auto& alias()const{ return alias_; }
    void setAlias(const AgvString& s){ alias_ = s; }
    void setDeprecated(bool s){ deprecated_ = s; }
    bool deprecated()const{ return deprecated_; }
    virtual PinType getPinType()const = 0;
    ToolPin(const DataSpec& ds);
    virtual ~ToolPin();

private:
    DataSpec ds_;
    AgvString alias_{"null"};
    bool deprecated_ = false;
};

AGV_NAMESPACE_END

#endif