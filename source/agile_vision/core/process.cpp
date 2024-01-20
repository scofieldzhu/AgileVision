/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: process.cpp  
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
#include <ranges>
#include "process.h"
#include "tool.h"
#include "serialize_policy.h"
#include "ratel/geometry/geometry.h"
#include "ratel/basic/dbg_tracker.h"
using namespace ratel;

AGV_NAMESPACE_BEGIN
namespace
{
#ifdef SERIALIZER_RATEL
    using ByteVecPx = VecProxy<agv_byte>;
    using PinDictProxy = DictProxy<std::string, ByteVecPx>;
    using StrPcType = ProxyCombine<StringProxy, StringProxy, true>;
#endif
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
#ifdef SERIALIZER_RATEL    
    StringProxy sp(iid_);
    StringProxy sp1(alias_);    
    StrPcType spc(sp, sp1);
    std::vector<Tool*> ptr_list;
    auto transform_view = tools_ | std::views::transform([](auto& t){ return t.get();});
    std::ranges::copy(transform_view, std::back_inserter(ptr_list));
    using TVP = VecProxy<Tool*, true>;
    TVP pp(ptr_list);
    using PC = ProxyCombine<StrPcType, TVP, true>;
    PC pc(spc, pp);
    return pc.serializeToBytes();
#endif

}

size_t Process::loadBytes(ConsAgvBytePtr buffer, size_t size)
{
#ifdef SERIALIZER_RATEL  
    StringProxy sp;
    StringProxy sp1;    
    StrPcType spc(sp, sp1);
    auto finish_bytes = spc.loadBytes(buffer, size);
    if(finish_bytes == 0)
        return 0;
    iid_ = sp.stdStr();
    alias_ = sp1.stdStr();
    auto cur_data = buffer + finish_bytes;
    auto left_size = size - finish_bytes;
    for(auto& t : tools_){
        finish_bytes = t->loadBytes(cur_data, left_size);
        if(finish_bytes == 0)
            return 0;
        cur_data += finish_bytes;
        left_size -= finish_bytes;
    }
    return size - left_size;
#endif
}

bool Process::run()
{
    std::lock_guard __auto_lock(run_mutex_);
    bool s = true;
    run_context_.is_running = true;
    for(auto& t : tools_){
        if(interrupt_)
            break;
        s = s && t->run();
    }        
    run_context_.is_running = false;
    interrupt_ = false;
    return s;
}

void Process::appendTool(ToolSPtr t)
{
    if(t && !existsTool(t.get())){
        tools_.push_back(t);
        t->setJoinedProcess(this);
    }
}

void Process::insertTool(const_iterator pos, ToolSPtr t)
{
    if(t && !existsTool(t.get())){
        tools_.insert(pos, t);
        t->setJoinedProcess(this);
    }
}

ToolPtr Process::findTool(const AgvString& name, bool recursive) const
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

void Process::removeTool(ToolPtr t)
{
    auto it = findTool(t);
    if(it != end()){
        t->setJoinedProcess(nullptr);
        tools_.erase(it);
    }        
}

void Process::removeTool(const_iterator pos)
{
    if(pos != end()){
        auto t = *pos;
        t->setJoinedProcess(nullptr);
        tools_.erase(pos);
    }
}

void Process::removeTool(iterator pos)
{
    if(pos != end()){
        auto t = *pos;
        t->setJoinedProcess(nullptr);
        tools_.erase(pos);
    }
}

void Process::setInterrupt(bool s)
{
    interrupt_ = s;
}

AGV_NAMESPACE_END
