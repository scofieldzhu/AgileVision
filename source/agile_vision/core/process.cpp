/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: process.cpp  
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

#include "process.h"
#include "tool.h"
#include "ratel/geometry/geometry.h"
using namespace ratel;

AGV_NAMESPACE_BEGIN
namespace {
    using StrPxEP = ElementProxy<StringProxy>;
    using ByteVecPx = VecProxy<agv_byte>;
    using PinDictProxy = DictProxy<std::string, ByteVecPx>;
}

Process::Process(const std::string &iid)
    :iid_(iid)
{
}

Process::~Process()
{
}

AgvBytes Process::serializeToBytes() const
{
    StrPxEP str_ep(StringProxy(iid_.c_str()));
    auto bv = str_ep.serializeToBytes();
    str_ep.mutableElement() = alias_;
    auto bv_n = str_ep.serializeToBytes();
    std::copy(bv_n.begin(), bv_n.end(), std::back_inserter(bv));
    for(auto& t : tools_){
        auto bv_t = t->serializeToBytes();
        std::copy(bv_t.begin(), bv_t.end(), std::back_inserter(bv));
    }
    return bv;
}

size_t Process::loadBytes(ConsAgvBytePtr buffer, size_t size)
{
    if(buffer == nullptr)
        return 0;
    auto cur_data = buffer;
    auto left_size = size;
    StrPxEP str_ep;
    auto finish_bytes = str_ep.loadBytes(cur_data, left_size);
    if(finish_bytes == 0)
        return 0;
    iid_ = str_ep.element().stdStr();
    cur_data += finish_bytes;
    left_size -= finish_bytes;
    finish_bytes = str_ep.loadBytes(cur_data, left_size);
    if(finish_bytes == 0)
        return 0;
    alias_ = str_ep.element().stdStr();
    cur_data += finish_bytes;
    left_size -= finish_bytes;
    for(auto& t : tools_){
        finish_bytes = t->loadBytes(cur_data, left_size);
        if(finish_bytes == 0)
            return 0;
        cur_data += finish_bytes;
        left_size -= finish_bytes;
    }
    return size - left_size;
}

bool Process::run()
{
    bool s = true;
    run_context_.is_running = true;
    for(auto& t : tools_)
        s = s && t->run();
    run_context_.is_running = false;
    return s;
}

void Process::appendTool(ToolPtr t)
{
    if(t && !existsTool(t.get()))
        tools_.push_back(t);
}

void Process::insertTool(const_iterator pos, ToolPtr t)
{
    if(t && !existsTool(t.get()))
        tools_.insert(pos, t);
}

Tool* Process::findTool(const AgvString& name, bool recursive) const
{
    if(!tools_.empty()){
        auto it = std::find_if(tools_.begin(), tools_.end(), [name](const auto& item){ return item->name() == name; });
        if(it != tools_.end())
            return (*it).get();
    }
    return nullptr;
}

Process::const_iterator Process::findTool(Tool *t) const
{
    if(t && !tools_.empty())
        return std::find_if(tools_.begin(), tools_.end(), [t](const auto& item){ return item.get() == t; });
    return end();
}

bool Process::existsTool(Tool *t) const
{
    return t && !tools_.empty() && std::find_if(tools_.begin(), tools_.end(), [t](const auto& item){ return item.get() == t; }) != tools_.end();
}

void Process::removeTool(Tool* t)
{
    auto it = findTool(t);
    if(it != end())
        tools_.erase(it);
}

void Process::removeTool(const_iterator pos)
{
    tools_.erase(pos);
}

void Process::removeTool(iterator pos)
{
    tools_.erase(pos);
}

AGV_NAMESPACE_END
