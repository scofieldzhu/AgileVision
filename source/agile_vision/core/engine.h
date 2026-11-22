/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: engine.h  
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

#ifndef __agile_vision_core_engine_h__
#define __agile_vision_core_engine_h__

#include <map>
#include <future>
#include "agile_vision/basic/id_seed.hpp"
#include "agile_vision/basic/thread_pool.h"
#include "agile_vision/core/core_typedef.h"

namespace agv::core {

    class AGV_CORE_API Engine 
    {
    public:        
        using FinishCallback = std::function<void(wkid_t)>;
        wkid_t get(ProcessPtr p)const;
        wkid_t commit(ProcessPtr p, FinishCallback cb);
        void cancel(wkid_t w, bool wait_infinite);
        enum class WaitResult{
            kOk, //result waited or already done!
            kTimeout,
            kUnk
        };
        WaitResult wait(wkid_t w, std::chrono::milliseconds timeout);
        bool exist(wkid_t w)const;
        bool syncRunProcess(ProcessPtr p);
        explicit Engine(size_t max_thread_number);
        ~Engine();

    private:
        friend struct RunThreadTask;
        agv::ThreadPool thread_pool_;
        struct Work
        {
            ProcessPtr process = nullptr;
            FinishCallback callback; //for async notify
            std::promise<wkid_t> prm;
            std::future<wkid_t> fut;
        };
        using WorkSPtr = std::shared_ptr<Work>;
        using WorkTableType = std::map<wkid_t, WorkSPtr>;
        WorkTableType running_work_table_;
        std::mutex running_work_table_mutex_;
        agv::IdSeed<wkid_t> work_id_seed_{0};
        std::condition_variable rem_work_cv_;
        std::mutex rem_work_mutex_;
        wkid_t rem_work_id_ = kNullId;
        bool exit_rem_task_ = false;
    };

}

#endif
