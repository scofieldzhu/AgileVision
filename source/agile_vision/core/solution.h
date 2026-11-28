/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: solution.h  
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

#ifndef __agile_vision_core_solution_h__
#define __agile_vision_core_solution_h__

#include <chrono>
#include <string>
#include "agile_vision/core/run_context.h"
#include "agile_vision/core/core_export.h"

namespace agv::core {

    struct WorkResult;
    class ToolFactory;

    class AGV_CORE_API Solution
    {
    public:
        using ProcedureUPtr = std::unique_ptr<Procedure>;
        using ProcedureList = std::vector<ProcedureUPtr>;
        using iterator = ProcedureList::iterator;
        using const_iterator = ProcedureList::const_iterator;
        iterator begin(){ return procedure_list_.begin(); }
        const_iterator begin()const{ return procedure_list_.begin(); }
        iterator end(){ return procedure_list_.end(); }
        const_iterator end()const{ return procedure_list_.end(); }
        AgvBytes serializeToBytes()const;
        size_t loadBytes(ConsAgvBytePtr buffer, size_t size);
        /// @brief 从拓扑帧重建全部流程；factory 提供工具类型的创建能力。
        size_t loadBytes(ConsAgvBytePtr buffer, size_t size, ToolFactory* factory);
        void run();
        /// @brief 按指定模式启动所有激活流程；默认 run() 保持异步行为。
        void run(bool async_run);
        /// @brief 等待所有已提交流程完成，超时上限对整批生效；无已提交工作时返回 true。
        bool waitAll(std::chrono::milliseconds timeout);
        /// @brief 协作式取消所有仍在运行的流程。
        void cancelAll(bool wait_infinite);
        /// @brief 查询指定流程最近一次已完成工作的结果。
        bool getWorkResult(const std::string& iid, WorkResult* out)const;
        ProcedurePtr createProcedure(const std::string& iid);
        ConstProcedurePtr findProcedure(const std::string& iid)const;
        void removeProcedure(const std::string& iid);
        void removeProcedure(const_iterator pos);
        void removeAll();
        auto numberOfProcedures()const{ return procedure_list_.size(); }
        Solution& operator=(const Solution&) = delete;

        explicit Solution(size_t max_run_thread_number);
        Solution(const Solution&) = delete;
        ~Solution();

    private:
        std::unique_ptr<Engine> engine_;
        RunContext run_context_;    
        std::vector<ProcedureUPtr> procedure_list_;
    };

}

#endif

