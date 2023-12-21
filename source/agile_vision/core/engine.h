 
/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: engine.h  
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

#ifndef __engine_h__
#define __engine_h__

#include <thread>
#include "ratel/basic/notifier.hpp"
#include "ratel/basic/id_seed.hpp"
#include "agile_vision/core/core_base_def.h"
#include "agile_vision/core/core_export.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API Engine 
{
public:    
    static constexpr uint32_t null_id = 0;
    using wkid_t = uint32_t;
    using wkid_list = std::vector<wkid_t>;
    wkid_t createWork(ProcessPtr p);
    void commitWork(wkid_t w);
    void stopWork(wkid_t w);
    bool isValidWork(wkid_t w)const;
    bool isActiveWork(wkid_t w)const;
    void destroyWork(wkid_t w);
    bool existsWork(wkid_t w)const;
    using wtid_t = uint32_t;
    wtid_t createWait(wkid_list works);
    void destroyWait(wtid_t wait);
    using wait_notifier = ratel::Notifier<wtid_t>;
    wait_notifier& waitNotifier(){ return wait_notifier_; }
    Engine();
    ~Engine();

private:
    bool isValidWork(Process* p)const;
    int locateWorkPos(wkid_t w)const;
    void makeActiveWork(wkid_t w)const;
    void runThreadProc(wkid_t);
    enum Status
    {
        STATUS_IDLE,
        STATUS_RUNNING
    };
    struct Work
    {
        ProcessPtr process;
        std::thread thread;
        Status status = STATUS_IDLE;
    };
    using WorkPtr = std::shared_ptr<Work>;
    std::map<wkid_t, WorkPtr> work_table_;
    struct Wait 
    {
        std::vector<wkid_t> target_wids;
    };
    using WaitPtr = std::shared_ptr<Wait>;
    std::map<wtid_t, WaitPtr> wait_table_;
    wkid_list active_work_ids_;
    ratel::IdSeed<wkid_t> work_id_seed_{1};
    ratel::IdSeed<wtid_t> wait_id_seed_{1};
    wait_notifier wait_notifier_;
    std::mutex active_works_mutex_;
};

AGV_NAMESPACE_END

#endif
