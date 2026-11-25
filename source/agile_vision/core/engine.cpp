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
#include "agile_vision/basic/log_service.h"
#include "agile_vision/basic/dbg_tracker.h"

namespace agv::core {

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
            // 先在锁内取出 Work 副本，后续通过副本访问，避免并发查表与 erase 竞争
            Engine::WorkSPtr work;
            {
                std::lock_guard lk(this_engine->running_work_table_mutex_);
                auto it = this_engine->running_work_table_.find(wk_id);
                if(it == this_engine->running_work_table_.end())
                {
                    return;
                }
                work = it->second;
            }
            // 任务异常不得逸出线程池工作线程，否则触发 terminate 且 promise 永不满足
            try
            {
                work->process->runContext().engine = this_engine;
                work->process->run(); //sync running...
                // 先满足 promise 再执行回调，避免回调内调用 wait() 时等待自身而死锁
                work->prm.set_value(wk_id);
                if(work->callback)
                {
                    work->callback(wk_id);
                }
            }
            catch(...)
            {
                AGV_LOG_ERROR("Async process run failed, work id:{}!", wk_id);
                try
                {
                    work->prm.set_value(wk_id);
                }
                catch(const std::future_error&)
                {
                }
            }
            std::lock_guard lk(this_engine->running_work_table_mutex_);
            this_engine->running_work_table_.erase(wk_id);
        }
        Engine* this_engine = nullptr;
        wkid_t wk_id = kNullId;
    };

    wkid_t Engine::get(ProcessPtr proc) const
    {
        if(proc){
            std::lock_guard lk(running_work_table_mutex_);
            auto it = std::find_if(running_work_table_.begin(), 
                                   running_work_table_.end(),
                                   [proc](const auto& pair){
                                     return pair.second->process == proc;
                                   });
            if(it != running_work_table_.end()){
                return (*it).first;
            }
        }
        return kNullId;
    }

    wkid_t Engine::commit(ProcessPtr p, FinishCallback cb)
    {    
        if(p == nullptr){
            AGV_LOG_ERROR("Null process pointer!");
            return kNullId;
        }
        auto work = std::make_shared<Work>();
        work->process = p;
        work->callback = cb;
        work->fut = work->prm.get_future();
        std::lock_guard lk(running_work_table_mutex_);
        // 检查与插入在同一临界区内完成，避免重复提交与 id 生成竞态
        const bool already_running = std::any_of(
            running_work_table_.begin(), 
            running_work_table_.end(),
            [p](const auto& pair){ 
                return pair.second->process == p;
            }
        );
        if(already_running){
            AGV_LOG_ERROR("Process already running!");
            return kNullId;
        }
        const auto new_wid = work_id_seed_.next();
        running_work_table_.insert({new_wid, work});
        //push to thread pool!
        RunThreadTask rtt{this, new_wid};
        agv::ThreadPool::task_func task_func = rtt;
        thread_pool_.enqueue(std::move(task_func));
        return new_wid;
    }

    void Engine::cancel(wkid_t w, bool wait_infinite)
    {
        // 在锁内一次性取出 Work 副本，避免 exist() 与 operator[] 之间的竞态
        Engine::WorkSPtr work;
        {
            std::lock_guard lk(running_work_table_mutex_);
            auto it = running_work_table_.find(w);
            if(it == running_work_table_.end()){
                AGV_LOG_ERROR("No running work:{} exists!", w);
                return;
            }
            work = it->second;
        }
        work->process->setInterrupt(true);
        if(wait_infinite)
        {
            work->fut.wait();
        }
    }

    Engine::WaitResult Engine::wait(wkid_t w, std::chrono::milliseconds timeout)
    {
        Engine::WorkSPtr work;
        {
            std::lock_guard lk(running_work_table_mutex_);
            auto it = running_work_table_.find(w);
            if(it == running_work_table_.end()){
                AGV_LOG_ERROR("Aleady done:{}", w);
                return WaitResult::kOk;
            }
            work = it->second;
        }
        auto fut_status = work->fut.wait_for(timeout);
        if(fut_status == std::future_status::ready)
        {
            return WaitResult::kOk;
        }
        if(fut_status == std::future_status::timeout)
        {
            return WaitResult::kTimeout;
        }
        return WaitResult::kUnk;
    }

    bool Engine::exist(wkid_t w) const
    {
        std::lock_guard lk(running_work_table_mutex_);
        return running_work_table_.find(w) != running_work_table_.end();
    }

    bool Engine::syncRunProcess(ProcessPtr p)
    {
        if(p == nullptr){
            AGV_LOG_ERROR("Null process pointer!");
            return false;
        }
        if(get(p) != kNullId){
            AGV_LOG_ERROR("This process:{} is in running state!", p->alias());
            return false;
        }
        p->runContext().work_id = kNullId;
        p->runContext().async_run = false;
        p->run();
        return true;
    }

}
