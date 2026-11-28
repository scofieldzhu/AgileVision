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

#include <atomic>
#include <chrono>
#include <future>
#include <map>
#include <string>
#include "agile_vision/basic/id_seed.hpp"
#include "agile_vision/basic/thread_pool.h"
#include "agile_vision/core/core_typedef.h"

namespace agv::core {

    /// @brief 一次工作执行的可观测结果，由 Engine 的回调与结果查询共同承载。
    struct WorkResult
    {
        wkid_t work_id = kNullId;
        bool success = false;
        bool cancelled = false;
        std::string error;
        std::chrono::milliseconds elapsed{0};
    };

    class AGV_CORE_API Engine 
    {
    public:        
        using FinishCallback = std::function<void(wkid_t)>; // 旧回调签名，保留兼容
        using ResultCallback = std::function<void(const WorkResult&)>;
        wkid_t get(ProcessPtr p)const;
        wkid_t commit(ProcessPtr p, FinishCallback cb);
        wkid_t commit(ProcessPtr p, ResultCallback cb);
        void cancel(wkid_t w, bool wait_infinite);
        enum class WaitResult{
            kOk,        // 已完成（等待命中或提交后已结束）
            kTimeout,   // 仍在运行且超过等待时长
            kNotFound,  // 工作从未提交
            kUnk
        };
        WaitResult wait(wkid_t w, std::chrono::milliseconds timeout);
        /// @brief 查询已完成工作的结果；未找到时返回 false，不消费结果。
        bool getResult(wkid_t w, WorkResult* out)const;
        bool exist(wkid_t w)const;
        bool syncRunProcess(ProcessPtr p);
        explicit Engine(size_t max_thread_number);
        ~Engine();

    private:
        friend struct RunThreadTask;
        struct Work
        {
            ProcessPtr process = nullptr;
            FinishCallback finish_callback; // 旧回调，仅在没有结果回调时触发
            ResultCallback result_callback;
            std::atomic_bool cancelled{false};
            std::promise<WorkResult> prm;
            std::future<WorkResult> fut;
        };
        using WorkSPtr = std::shared_ptr<Work>;
        using WorkTableType = std::map<wkid_t, WorkSPtr>;
        using ResultTableType = std::map<wkid_t, WorkResult>;
        void recordResult(wkid_t w, const WorkResult& result); // 调用方须持 running_work_table_mutex_
        WorkTableType running_work_table_;
        ResultTableType completed_results_;
        static constexpr size_t kMaxCompletedResults = 4096;
        mutable std::mutex running_work_table_mutex_;
        agv::IdSeed<wkid_t> work_id_seed_{0};
        // 线程池最后声明：析构时先 stop/join，确保在途任务结束前工作表与锁仍有效
        agv::ThreadPool thread_pool_;
    };

}

#endif
