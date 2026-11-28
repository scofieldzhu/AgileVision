/*
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *
 *   File: process_runner.cpp
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

#include "process_runner.h"
#include "process.h"
#include "agile_vision/basic/log_service.h"

namespace agv::core {

    namespace
    {
        Engine* hostEngine(ProcessPtr host)
        {
            return host != nullptr ? host->runContext().engine : nullptr;
        }
    }

    bool ProcessRunner::runSync(ProcessPtr host, ProcessPtr child)
    {
        auto* engine = hostEngine(host);
        if(engine == nullptr || child == nullptr){
            AGV_LOG_ERROR("Invalid host engine or child process for sync run!");
            return false;
        }
        return engine->syncRunProcess(child);
    }

    wkid_t ProcessRunner::runAsync(ProcessPtr host, ProcessPtr child)
    {
        auto* engine = hostEngine(host);
        if(engine == nullptr || child == nullptr){
            AGV_LOG_ERROR("Invalid host engine or child process for async run!");
            return kNullId;
        }
        const auto work_id = engine->commit(child, Engine::FinishCallback());
        if(work_id != kNullId){
            // 直接写入 child 自身的上下文，避免按值拷贝后写入副本导致 work_id 丢失。
            child->runContext().work_id = work_id;
        }
        return work_id;
    }

    Engine::WaitResult ProcessRunner::wait(ProcessPtr host, ProcessPtr child, std::chrono::milliseconds timeout)
    {
        auto* engine = hostEngine(host);
        if(engine == nullptr || child == nullptr){
            AGV_LOG_ERROR("Invalid host engine or child process for wait!");
            return Engine::WaitResult::kUnk;
        }
        auto work_id = engine->get(child);
        if(work_id == kNullId){
            // 运行表可能已清理；用上下文记录的工作号查询完成结果，从未提交则返回 kNotFound。
            work_id = child->runContext().work_id;
            if(work_id == kNullId){
                return Engine::WaitResult::kNotFound;
            }
        }
        return engine->wait(work_id, timeout);
    }

    ProcessPtr ProcessRunner::findChild(ProcessPtr host, const std::string& iid)
    {
        if(host == nullptr){
            return nullptr;
        }
        return host->getMutableRootProcess()->findMutableChildProcess(iid, true);
    }

}
