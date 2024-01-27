/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: process_manager.h  
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

#ifndef __process_manager_h__
#define __process_manager_h__

#include "agile_vision/core/core_base_def.h"
#include "agile_vision/core/core_export.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API ProcessManager 
{
public:
    using ProcessSPtr = std::shared_ptr<Process>;
    using ProcessList = std::vector<ProcessSPtr>;
    using iterator = ProcessList::iterator;
    using const_iterator = ProcessList::const_iterator;
    iterator begin(){ return process_list_.begin(); }
    const_iterator begin()const{ return process_list_.begin(); }
    iterator end(){ return process_list_.end(); }
    const_iterator end()const{ return process_list_.end(); }
    void insertProcess(const_iterator pos, ProcessSPtr p);
    ProcessPtr findProcess(const std::string& iid, bool recursive);
    ConstProcessPtr findProcess(const std::string& iid, bool recursive)const;
    void appendProcess(ProcessSPtr p);
    void removeProcess(const std::string& iid);
    void removeProcess(iterator pos);
    auto size()const{ return process_list_.size(); }
    void setActiveProcessIID(const std::string& iid);
    const auto& activeProcessIID()const{ return active_process_iid_; }
    ConstProcessPtr getActiveProcess()const;
    ProcessPtr getActiveProcess();
    ToolPtr owner(){ return owner_; }
    ConstToolPtr owner()const{ return owner_; }
    ProcessManager(ToolPtr owner);
    ~ProcessManager();

private:    
    ToolPtr owner_;
    ProcessList process_list_;
    std::string active_process_iid_;
};

AGV_NAMESPACE_END

#endif