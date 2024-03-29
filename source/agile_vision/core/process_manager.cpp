/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: process_manager.cpp  
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

#include "process_manager.h"
/*
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *
 *   File: process_manager.cpp
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
#include "process_manager.h"
#include "process.h"

AGV_NAMESPACE_BEGIN

ProcessManager::ProcessManager(ToolPtr owner)
    :owner_(owner)
{
}

ProcessManager::~ProcessManager()
{
}

void ProcessManager::insertProcess(const_iterator pos, ProcessSPtr p)
{
    if(p && std::find(process_list_.begin(), process_list_.end(), p) == process_list_.end()){
        process_list_.insert(pos, p);
        p->setManager(this);
    }
}

ConstProcessPtr ProcessManager::findProcess(const std::string& iid, bool recursive) const
{
    return const_cast<ProcessManager*>(this)->findProcess(iid, recursive);
}

ProcessPtr ProcessManager::findProcess(const std::string& iid, bool recursive)
{
    for(auto& p : process_list_){
        auto dst_p = p->findMutableChildProcess(iid, recursive);
        if(dst_p)
            return dst_p;
    }
    return nullptr;
}

void ProcessManager::appendProcess(ProcessSPtr p)
{
    if(p && std::find(process_list_.begin(), process_list_.end(), p) == process_list_.end()){
        process_list_.push_back(p);
        p->setManager(this);
    }
}

void ProcessManager::removeProcess(const std::string& iid)
{
    auto it = std::find_if(process_list_.begin(), process_list_.end(), [&iid](auto it){ return (*it).iid() == iid; });
    if(it != process_list_.end()){
        (*it)->setManager(nullptr);
        process_list_.erase(it);
    }
}

void ProcessManager::removeProcess(iterator pos)
{
    if(pos != process_list_.end()){
        process_list_.erase(pos);
        (*pos)->setManager(nullptr);
    }
}

void ProcessManager::setActiveProcessIID(const std::string& iid)
{
    active_process_iid_ = iid;
}

ConstProcessPtr ProcessManager::getActiveProcess() const
{
    return const_cast<ProcessManager*>(this)->getActiveProcess();
}

ProcessPtr ProcessManager::getActiveProcess()
{
    if(active_process_iid_.empty())
        return nullptr;
    auto it = std::find_if(process_list_.begin(), process_list_.end(), [this](auto item){ 
        return (*item).iid() == active_process_iid_; 
    });
    return it != process_list_.end() ? (*it).get() : nullptr;
}

AGV_NAMESPACE_END
