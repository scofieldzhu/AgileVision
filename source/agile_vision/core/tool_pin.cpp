/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: tool_pin.cpp  
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

#include "tool_pin.h"
#include "ratel/geometry/element_proxy.hpp"
using namespace ratel;

namespace {
    using StringEP = ElementProxy<StringProxy>;
    using IntEP = ElementProxy<int>;
}

AGV_NAMESPACE_BEGIN

ToolPin::ToolPin(const DataSpec& ds)
    :ds_(ds)
{}

ToolPin::~ToolPin()
{}

// AgvBytes ToolPin::serializeToBytes() const
// {
//     auto bv_ds = ds_.serializeToBytes();
//     StringProxy sp(alias_);
//     StringEP str_ep(sp);
//     auto bv_s = str_ep.serializeToBytes();
//     IntEP i_ep((int)deprecated_);
//     auto bv_i = i_ep.serializeToBytes();
//     AgvBytes bv(bv_ds.size() + bv_s.size() + bv_i.size(), 0);
//     auto cur_data = bv.data();
//     memcpy(cur_data, bv_ds.data(), bv_ds.size());
//     cur_data += bv_ds.size();
//     memcpy(cur_data, bv_s.data(), bv_s.size());
//     cur_data += bv_s.size();
//     memcpy(cur_data, bv_i.data(), bv_i.size());
//     cur_data += bv_i.size();
//     return bv;
// }

// size_t ToolPin::loadBytes(ConsAgvBytePtr buffer, size_t size)
// {
//     if(buffer == nullptr)
//         return 0;
//     auto cur_data = buffer;
//     auto left_size = size;
//     auto finish_bytes = ds_.loadBytes(cur_data, left_size);
//     if(finish_bytes == 0)
//         return 0;
//     cur_data += finish_bytes;
//     left_size -= finish_bytes;
//     StringEP str_ep;
//     finish_bytes = str_ep.loadBytes(cur_data, left_size);
//     if(finish_bytes == 0)
//         return 0;
//     alias_ = str_ep.element().stdStr();
//     cur_data += finish_bytes;
//     left_size -= finish_bytes;
//     IntEP i_ep;
//     finish_bytes = i_ep.loadBytes(cur_data, left_size);
//     if(finish_bytes == 0)
//         return 0;
//     deprecated_ = (bool)i_ep.element();
//     cur_data += finish_bytes;
//     left_size -= finish_bytes;
//     return size - left_size;
// }

AGV_NAMESPACE_END