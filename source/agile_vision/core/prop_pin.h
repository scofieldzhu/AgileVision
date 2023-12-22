/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: prop_pin.h  
 *   Copyright (c) 2023-2023 scofieldzhu
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

#ifndef __prop_pin_h__
#define __prop_pin_h__

#include "agile_vision/core/tool_pin.h"
#include "agile_vision/core/produce_info.h"
#include "agile_vision/core/data_buffer.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API PropPin : public ToolPin
{
public:
    bool loadProps(const AgvBytes& data);
    void serializeProps(AgvBytes& data)const;
    void bindProduceInfo(const ProduceInfo& pi);
    void unbind();
    const auto& produceInfo()const{ return produce_info_; }
    DataBuffer& mutableDataBuffer(){ return data_buffer_; }
    const DataBuffer& dataBuffer()const{ return data_buffer_; }
    PinType getPinType()const override;
    PropPin(const DataSpec& ds);
    ~PropPin();

private:
    DataBuffer data_buffer_;
    ProduceInfo produce_info_;
};

AGV_NAMESPACE_END

#endif