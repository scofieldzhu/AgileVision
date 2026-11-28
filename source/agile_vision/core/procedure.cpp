/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: procedure.cpp  
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

#include "procedure.h"
#include "engine.h"
#include "tool_factory.h"
#include "relationship_network.h"
#include "serialization_frame.h"
#include "agile_vision/geometry/geometry.h"
#include "agile_vision/basic/log_service.h"

using namespace agv;

namespace agv::core {

    Procedure::Procedure(const std::string &iid)
        : iid_(iid),
          root_(std::make_shared<Process>(iid)) // same iid with procedure's
    {
    }

    Procedure::~Procedure()
    {
    }

    AgvBytes Procedure::serializeToBytes() const
    {
        AgvBytes payload;
        detail::writeString(payload, iid_);
        detail::writeString(payload, alias_);
        detail::writeU8(payload, active_run_ ? 1 : 0);
        const auto process_bytes = root_->serializeToBytes();
        detail::writeBytes(payload, process_bytes.data(), process_bytes.size());
        AgvBytes out;
        detail::writeFrameHeader(out, detail::kProcedureFrameMagic, static_cast<uint32_t>(payload.size()));
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }

    size_t Procedure::loadBytes(ConsAgvBytePtr buffer, size_t size)
    {
        return loadBytes(buffer, size, nullptr);
    }

    size_t Procedure::loadBytes(ConsAgvBytePtr buffer, size_t size, ToolFactory* factory)
    {
        if(buffer == nullptr || size < 12){
            return 0;
        }
        auto cur = buffer;
        size_t left = size;
        uint32_t payload_size = 0;
        if(!detail::readFrameHeader(cur, left, detail::kProcedureFrameMagic, payload_size)){
            return 0;
        }
        std::string loaded_iid;
        std::string loaded_alias;
        uint8_t active = 0;
        const char* process_data = nullptr;
        size_t process_size = 0;
        if(!detail::readString(cur, left, loaded_iid)){
            return 0;
        }
        if(!detail::readString(cur, left, loaded_alias)){
            return 0;
        }
        if(!detail::readU8(cur, left, active)){
            return 0;
        }
        if(!detail::readBytes(cur, left, process_data, process_size)){
            return 0;
        }
        if(root_->loadBytes(process_data, process_size, factory) == 0){
            return 0;
        }
        iid_ = loaded_iid;
        alias_ = loaded_alias;
        active_run_ = active != 0;
        return size - left;
    }

    void Procedure::setAlias(const AgvString &str)
    {
        alias_ = str;
    }

    bool Procedure::checkDataflowCycle() const
    {
        RelationshipNetwork network;
        network.buildFrom(root_.get());
        return network.checkCycle();
    }

    void Procedure::run()
    {
        if(checkDataflowCycle()){
            AGV_LOG_ERROR("Dataflow cycle detected in procedure:{}, refuse to run!", alias_);
            return;
        }
        if(run_context_.engine ==  nullptr){
            AGV_LOG_ERROR("No engine object attached!");
            return;
        }
        auto cur_engine = run_context_.engine;
        if(run_context_.async_run){
            // 按值捕获别名，避免 Procedure 先于异步回调析构时 this 悬垂。
            const std::string proc_alias = alias_;
            auto finish_func = [proc_alias](const WorkResult& result){
                if(result.success){
                    AGV_LOG_TRACE("Procedure:{} root run done, work:{}!", proc_alias, result.work_id);
                }else{
                    AGV_LOG_ERROR("Procedure:{} root run failed, work:{}, error:{}!", proc_alias, result.work_id, result.error);
                }
            };
            run_context_.work_id = cur_engine->commit(root_.get(), finish_func);
            if(run_context_.work_id == kNullId){
                AGV_LOG_ERROR("Engine commit root process:{} work failed!", root_->iid());
            }
        }else{
            if(!cur_engine->syncRunProcess(root_.get())){
                AGV_LOG_ERROR("Engine syncRunProcess root process:{} work failed!", root_->iid());
            }
        }
    }

    void Procedure::setActiveRun(bool s)
    {
        active_run_ = s;
    }

}

