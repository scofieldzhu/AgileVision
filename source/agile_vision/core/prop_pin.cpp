/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: prop_pin.cpp  
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

#include "prop_pin.h"

AGV_NAMESPACE_BEGIN

PropPin::PropPin(const DataSpec &ds)
    :ToolPin(ds),
    data_buffer_(ds)
{
}

PropPin::~PropPin()
{
}

AgvBytes PropPin::serializeToBytes() const
{
    return data_buffer_.serializeToBytes();
}

size_t PropPin::loadBytes(ConsAgvBytePtr buffer, size_t size)
{
    return data_buffer_.loadBytes(buffer, size);
}

void PropPin::bindProduceInfo(const ProduceInfo &ci)
{
    produce_info_ = ci;
}

void PropPin::unbind()
{
    produce_info_.pin = nullptr;
}

PinType PropPin::getPinType() const
{
    return PinType::kProp;
}

AGV_NAMESPACE_END
