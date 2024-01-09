/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: output_pin.cpp  
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

#include "output_pin.h"

AGV_NAMESPACE_BEGIN

OutputPin::OutputPin(const DataSpec& ds)
    :ToolPin(ds),
    data_buffer_(ds)
{
}

void OutputPin::addConsumeInfo(const ConsumeInfo& inf)
{
    if(inf && std::find_if( consume_info_list_.begin(), 
                            consume_info_list_.end(), 
                            [&inf](const auto& it){ return inf == (it); }) == consume_info_list_.end()){
        consume_info_list_.push_back(inf);
    }
}

void OutputPin::removeConsumePin(ToolPin* consume_pin)
{
    if(consume_pin){
        auto it = std::remove_if(consume_info_list_.begin(), consume_info_list_.end(), [&consume_pin](const auto& it){ return consume_pin == it.pin; });
        consume_info_list_.erase(it, consume_info_list_.end());
    }
}

PinType OutputPin::getPinType() const
{
    return PinType::kOutput;
}

OutputPin::~OutputPin()
{

}
AGV_NAMESPACE_END
