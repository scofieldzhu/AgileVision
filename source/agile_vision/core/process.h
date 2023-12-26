/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: process.h  
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

#ifndef __process_h__
#define __process_h__

#include "agile_vision/core/run_context.h"
#include "agile_vision/core/core_export.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API Process
{
public:
    using iterator = ToolList::iterator;
    using const_iterator = ToolList::const_iterator;
    iterator begin(){ return tools_.begin(); }
    const_iterator begin()const{ return tools_.begin(); }
    iterator end(){ return tools_.end(); }
    const_iterator end()const{ return tools_.end(); }
    AgvBytes serializeToBytes()const;
    size_t loadBytes(ConsAgvBytePtr buffer, size_t size);
    RunContext& runContext(){ return run_context_; }
    const RunContext& runContext()const{ return run_context_; }
    bool run();
    void insertTool(const_iterator pos, ToolPtr t);
    Tool* findTool(const AgvString& name, bool recursive)const;
    const_iterator findTool(Tool* t)const;
    bool existsTool(Tool* t)const;
    void removeTool(Tool* t);
    void removeTool(const_iterator pos);
    void removeTool(iterator pos);
    auto sizeOfTools()const{ return tools_.size(); }
    void setAlias(const AgvString& s);
    const AgvString& alias()const{ return alias_; }
    const std::string& iid()const{ return iid_; }
    Process(const std::string& iid);
    ~Process();

private:
    std::string iid_;
    AgvString alias_{"null"};
    ToolList tools_;
    RunContext run_context_;
};

AGV_NAMESPACE_END

#endif

