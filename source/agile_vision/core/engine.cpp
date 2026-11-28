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
                if(it == this_engine->running_work_table_.end()){
                    return;
                }
                work = it->second;
            }
            using Clock = std::chrono::steady_clock;
            const auto started = Clock::now();
            WorkResult result;
            result.work_id = wk_id;
            // 任务异常不得逸出线程池工作线程，否则触发 terminate 且 promise 永不满足
            try
            {
                work->process->runContext().engine = this_engine;
                result.success = work->process->run();
            }
            catch(const std::exception& e)
            {
                result.error = e.what();
                AGV_LOG_ERROR("Async process run failed, work id:{}, error:{}", wk_id, result.error);
            }
            catch(...)
            {
                result.error = "unknown exception";
                AGV_LOG_ERROR("Async process run failed with unknown exception, work id:{}", wk_id);
            }
            result.cancelled = work->cancelled.load();
            result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started);
            // 先满足 promise 再执行回调，避免回调内调用 wait() 时等待自身而死锁
            try
            {
                work->prm.set_value(result);
            }
            catch(const std::future_error&)
            {
            }
            // 回调异常同样不得中断结果登记与工作表清理
            try
            {
                if(work->result_callback){
                    work->result_callback(result);
                }else if(work->finish_callback){
                    work->finish_callback(wk_id);
                }
            }
            catch(const std::exception& e)
            {
                AGV_LOG_ERROR("Async work callback failed, work id:{}, error:{}", wk_id, e.what());
            }
            catch(...)
            {
                AGV_LOG_ERROR("Async work callback failed with unknown exception, work id:{}", wk_id);
            }
            std::lock_guard lk(this_engine->running_work_table_mutex_);
            this_engine->recordResult(wk_id, result);
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
        // 旧签名适配为结果回调，保持既有调用方无需改动。
        if(!cb){
            return commit(p, ResultCallback());
        }
        return commit(p, [cb](const WorkResult& result){ cb(result.work_id); });
    }

    wkid_t Engine::commit(ProcessPtr p, ResultCallback cb)
    {
        if(p == nullptr){
            AGV_LOG_ERROR("Null process pointer!");
            return kNullId;
        }
        auto work = std::make_shared<Work>();
        work->process = p;
        work->result_callback = cb;
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
        work->cancelled.store(true);
        work->process->setInterrupt(true);
        if(wait_infinite){
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
                // 结果表只在完成后登记且不会因查询而消费，可据此区分“已完成”与“从未提交”。
                return completed_results_.find(w) != completed_results_.end() ? WaitResult::kOk : WaitResult::kNotFound;
            }
            work = it->second;
        }
        auto fut_status = work->fut.wait_for(timeout);
        if(fut_status == std::future_status::ready){
            return WaitResult::kOk;
        }
        if(fut_status == std::future_status::timeout){
            return WaitResult::kTimeout;
        }
        return WaitResult::kUnk;
    }

    bool Engine::getResult(wkid_t w, WorkResult* out) const
    {
        if(out == nullptr){
            return false;
        }
        std::lock_guard lk(running_work_table_mutex_);
        auto it = completed_results_.find(w);
        if(it == completed_results_.end()){
            return false;
        }
        *out = it->second;
        return true;
    }

    void Engine::recordResult(wkid_t w, const WorkResult& result)
    {
        completed_results_[w] = result;
        while(completed_results_.size() > kMaxCompletedResults){
            completed_results_.erase(completed_results_.begin());
        }
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
        return p->run();
    }

}
