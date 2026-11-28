/*
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *
 *   File: process_runner.h
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

#ifndef __agile_vision_core_process_runner_h__
#define __agile_vision_core_process_runner_h__

#include "agile_vision/core/core_typedef.h"
#include "agile_vision/core/engine.h"

namespace agv::core {

    /// @brief 子过程执行入口，统一同步/异步提交与等待，避免各工具重复拼接 engine 调用。
    /// 宿主进程的 runContext().engine 提供执行引擎；所有接口在参数无效时安全返回。
    class AGV_CORE_API ProcessRunner
    {
    public:
        /// @brief 同步执行子过程，阻塞至完成。返回执行结果。
        static bool runSync(ProcessPtr host, ProcessPtr child);

        /// @brief 异步提交子过程，返回工作 ID；失败返回 kNullId。
        /// 成功时把工作 ID 写回 child 自身的 runContext，便于后续 wait/cancel。
        static wkid_t runAsync(ProcessPtr host, ProcessPtr child);

        /// @brief 等待子过程完成。已完成返回 kOk，从未提交返回 kNotFound。
        static Engine::WaitResult wait(ProcessPtr host, ProcessPtr child, std::chrono::milliseconds timeout);

        /// @brief 从宿主所属根进程按 iid 递归查找子过程。
        static ProcessPtr findChild(ProcessPtr host, const std::string& iid);
    };

}

#endif
