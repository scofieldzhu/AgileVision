/* 
 *  Ratel is a application framework, which provides some convenient librarys
 *  for for those c++ developers pursuing fast-developement.
 *  
 *  File: thread_pool.cpp 
 *  Copyright (c) 2024-2024 scofieldzhu
 *  
 *  MIT License
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include "thread_pool.h"
#include "agile_vision/basic/log_service.h"

namespace agv {

    ThreadPool::ThreadPool(size_t number)
    {
        for(auto i = 0; i < number; ++i){
            workers_.emplace_back([this]
            {
                while(!this->stop_.load(std::memory_order_acquire)){
                    task_func active_task;
                    {
                        std::unique_lock lk(mutex_);
                        cv_.wait(lk, [this]{
                            return this->stop_.load(std::memory_order_acquire) || !this->pending_tasks_.empty();
                        });
                        if(this->stop_.load(std::memory_order_acquire)){
                            break;
                        }
                        active_task = std::move(this->pending_tasks_.front());
                        this->pending_tasks_.pop();
                    }
                    // 隔离任务异常，避免工作线程 terminate 导致整个进程退出。
                    try
                    {
                        active_task();
                    }
                    catch(const std::exception& e)
                    {
                        AGV_LOG_ERROR("ThreadPool task threw exception: {}", e.what());
                    }
                    catch(...)
                    {
                        AGV_LOG_ERROR("ThreadPool task threw unknown exception!");
                    }
                }
            });
        }
    }

    ThreadPool::~ThreadPool()
    {
        stop_.store(true, std::memory_order_release);
        cv_.notify_all();
        for(std::thread& w : workers_){
            if(w.joinable()){
                w.join();
            }
        }
    }

    void ThreadPool::enqueue(task_func&& t)
    {
        if(stop_.load(std::memory_order_acquire)){
            AGV_LOG_WARN("Enqueue rejected: thread pool is stopping!");
            return;
        }
        {
            std::lock_guard<std::mutex> lk(mutex_);
            pending_tasks_.emplace(std::move(t));
        }
        cv_.notify_one();
    }

}