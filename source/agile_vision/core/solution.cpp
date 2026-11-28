/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: solution.cpp  
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

#include "solution.h"
#include <ranges>
#include "procedure.h"
#include "engine.h"
#include "tool_factory.h"
#include "serialize_policy.h"
#include "serialization_frame.h"
#include "agile_vision/basic/log_service.h"

namespace agv {
    using Procedure = agv::core::Procedure;

    template <>
    struct DefaultCreator<Procedure*>
    {
        static Procedure* Create()
        {
            // iid 为反序列化占位值，随后由 Procedure::loadBytes 覆盖。
            return new Procedure("");
        }
    };

}

namespace agv::core {

    Solution::Solution(size_t max_run_thread_number)
        :engine_(std::make_unique<Engine>(max_run_thread_number))
    {
    }

    Solution::~Solution()
    {
    }

    AgvBytes Solution::serializeToBytes() const
    {
        AgvBytes payload;
        detail::writeU32(payload, static_cast<uint32_t>(procedure_list_.size()));
        for(const auto& procedure : procedure_list_){
            const auto bytes = procedure->serializeToBytes();
            detail::writeBytes(payload, bytes.data(), bytes.size());
        }
        AgvBytes out;
        detail::writeFrameHeader(out, detail::kSolutionFrameMagic, static_cast<uint32_t>(payload.size()));
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }

    size_t Solution::loadBytes(ConsAgvBytePtr buffer, size_t size)
    {
        return loadBytes(buffer, size, nullptr);
    }

    size_t Solution::loadBytes(ConsAgvBytePtr buffer, size_t size, ToolFactory* factory)
    {
        if(buffer == nullptr || size < 12){
            return 0;
        }
        auto cur = buffer;
        size_t left = size;
        uint32_t payload_size = 0;
        if(!detail::readFrameHeader(cur, left, detail::kSolutionFrameMagic, payload_size)){
            return 0;
        }
        uint32_t procedure_count = 0;
        if(!detail::readU32(cur, left, procedure_count)){
            return 0;
        }
        std::vector<ProcedureUPtr> restored;
        restored.reserve(procedure_count);
        for(uint32_t i = 0; i < procedure_count; ++i){
            const char* data = nullptr;
            size_t data_size = 0;
            if(!detail::readBytes(cur, left, data, data_size)){
                return 0;
            }
            auto procedure = std::make_unique<Procedure>("");
            if(procedure->loadBytes(data, data_size, factory) == 0){
                return 0;
            }
            restored.push_back(std::move(procedure));
        }
        removeAll();
        procedure_list_ = std::move(restored);
        return size - left;
    }

    void Solution::run()
    {
        run(true);
    }

    void Solution::run(bool async_run)
    {
        for(auto& p : procedure_list_){
            if(p->activeRun()){
                p->runContext().async_run = async_run;
                p->runContext().engine = engine_.get();
                p->run();
            }
        }
    }

    bool Solution::waitAll(std::chrono::milliseconds timeout)
    {
        using Clock = std::chrono::steady_clock;
        const auto deadline = Clock::now() + timeout;
        bool all_done = true;
        for(auto& p : procedure_list_){
            if(!p->activeRun()){
                continue;
            }
            const auto wid = p->runContext().work_id;
            if(wid == kNullId){
                continue;
            }
            const auto remain = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now());
            const auto status = engine_->wait(wid, remain < std::chrono::milliseconds(0) ? std::chrono::milliseconds(0) : remain);
            if(status != Engine::WaitResult::kOk){
                AGV_LOG_WARN("Wait work:{} of procedure:{} failed with status:{}", wid, p->iid(), static_cast<int>(status));
                all_done = false;
            }
        }
        return all_done;
    }

    void Solution::cancelAll(bool wait_infinite)
    {
        for(auto& p : procedure_list_){
            if(!p->activeRun()){
                continue;
            }
            const auto wid = p->runContext().work_id;
            if(wid == kNullId){
                continue;
            }
            engine_->cancel(wid, wait_infinite);
        }
    }

    bool Solution::getWorkResult(const std::string& iid, WorkResult* out) const
    {
        const auto proc = findProcedure(iid);
        if(proc == nullptr || out == nullptr){
            return false;
        }
        return engine_->getResult(proc->runContext().work_id, out);
    }

    Procedure *Solution::createProcedure(const std::string &iid)
    {
        if(findProcedure(iid)){
            AGV_LOG_WARN("Procedure with iid:{} already exists!", iid);
            return nullptr;
        }
        auto procedure = std::make_unique<Procedure>(iid);
        procedure_list_.push_back(std::move(procedure));
        return procedure_list_.back().get();
    }

    ConstProcedurePtr Solution::findProcedure(const std::string& iid) const
    {
        auto it = std::find_if(procedure_list_.begin(), procedure_list_.end(), [&iid](auto& it){
            return (*it).iid() == iid;
        });
        return it != procedure_list_.end() ? (*it).get() : nullptr;
    }

    void Solution::removeProcedure(const std::string &iid)
    {
        procedure_list_.erase(
            std::remove_if(procedure_list_.begin(), procedure_list_.end(), [&iid](auto& it){
                return (*it).iid() == iid;
            }),
            procedure_list_.end()
        );
    }

    void Solution::removeProcedure(const_iterator pos)
    {
        procedure_list_.erase(pos);
    }

    void Solution::removeAll()
    {
        procedure_list_.clear();
    }

}