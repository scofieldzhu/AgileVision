/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: engine.cpp  
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

#include "engine.h"
#include <algorithm>
#include "process.h"
#include "spdlog/spdlog.h"
#include "ratel/basic/dbg_tracker.h"

AGV_NAMESPACE_BEGIN

#define AUTO_LOCK_RUNNING_WORK_TABLE std::lock_guard __auto_lk__(running_work_table_mutex_);

Engine::Engine(size_t max_thread_number)
    :thread_pool_(max_thread_number)
{
    assert(max_thread_number); // 1 at least!
}

Engine::~Engine()
{
}

struct RunThreadTask
{
    void operator()()
    {
        _AUTO_FUNC_TRACK_
        this_engine->running_work_table_[wk_id]->process->runContext().engine = this_engine;
        this_engine->running_work_table_[wk_id]->process->run(); //sync running...
        {
            std::lock_guard __auto_lk__(this_engine->running_work_table_mutex_);
            if(this_engine->running_work_table_[wk_id]->callback)
                this_engine->running_work_table_[wk_id]->callback(wk_id);
            this_engine->running_work_table_[wk_id]->prm.set_value(wk_id);
            this_engine->running_work_table_.erase(wk_id);
        }
    }
    Engine* this_engine = nullptr;
    wkid_t wk_id = null_id;
};

wkid_t Engine::get(Process* proc) const
{
    if(proc){
        auto it = std::find_if(running_work_table_.begin(), 
                               running_work_table_.end(),
                               [proc](const auto& pair){
                                 return pair.second->process.get() == proc;
                               });
        if(it != running_work_table_.end()){
            return (*it).first;
        }
    }
    return null_id;
}

wkid_t Engine::commit(ProcessPtr p, finish_callback cb)
{    
    if(p == nullptr || get(p.get()) != null_id){
        spdlog::error("Null process pointer or already running!");
        return null_id;
    } 
    auto new_wid = work_id_seed_.next();
    auto work = std::make_shared<Work>();
    work->process = p;
    work->callback = cb;
    work->fut = work->prm.get_future();
    {
        AUTO_LOCK_RUNNING_WORK_TABLE
        running_work_table_.insert({new_wid, work});   
    }    
    //push to thread pool!
    RunThreadTask rtt{this, new_wid};
    ratel::ThreadPool::task_func task_func = rtt;
    thread_pool_.enqueue(std::move(task_func));
    return new_wid;
}

void Engine::cancel(wkid_t w, bool wait_infinite)
{
    if(!exist(w)){
        spdlog::error("No running work:{} exists!", w);
        return;
    }
    running_work_table_[w]->process->setInterrupt(true);
    if(wait_infinite)
        running_work_table_[w]->fut.wait();
}

Engine::WaitResult Engine::wait(wkid_t w, std::chrono::milliseconds timeout)
{
    if(!exist(w)){
        spdlog::error("Aleady done:{}", w);
        return WaitResult::kOk;
    }
    auto fut_status = running_work_table_[w]->fut.wait_for(timeout);
    if(fut_status == std::future_status::ready)
        return WaitResult::kOk;
    if(fut_status == std::future_status::timeout)
        return WaitResult::kTimeout;
    return WaitResult::kUnk;
}

bool Engine::exist(wkid_t w) const
{
    return running_work_table_.find(w) != running_work_table_.end();
}

bool Engine::syncRunProcess(ProcessPtr p)
{
    if(p == nullptr || get(p.get()) != null_id){
        spdlog::error("This process:{} is null or in runnning state!", p->alias());
        return false;
    }
    p->runContext().work_id = null_id;
    p->runContext().async_run = false;
    p->run();
    return true;
}

AGV_NAMESPACE_END
