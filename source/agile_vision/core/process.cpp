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

AGV_NAMESPACE_BEGIN
Process::Process(const std::string &iid)
    :iid_(iid)
{
}

Process::~Process()
{
}

bool Process::run()
{
    bool s = true;
    for(auto& t : tools_){
        s = s && t->run();
    }
    return s;
}

void Process::insertTool(const_iterator pos, ToolPtr t)
{
    if(!existsTool(t.get()))
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
