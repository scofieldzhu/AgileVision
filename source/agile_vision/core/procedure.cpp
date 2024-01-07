/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: procedure.cpp  
 *   Copyright (c) 2023-2023 scofieldzhu
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
#include "ratel/geometry/geometry.h"
#include "spdlog/spdlog.h"

using namespace ratel;

AGV_NAMESPACE_BEGIN

namespace{
    using StrPxEP = ElementProxy<StringProxy>;
    using CombineA = ProxyCombine<StrPxEP, StrPxEP>;
    using CombineB = ProxyCombine<CombineA, Process>;
}

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
    CombineA ca;
    ca.proxyA().mutableElement() = iid_;
    ca.proxyB().mutableElement() = alias_;
    auto bv = ca.serializeToBytes();
    auto bv_r = root_->serializeToBytes();
    std::copy(bv_r.begin(), bv_r.end(), std::back_inserter(bv));
    return bv;
}

size_t Procedure::loadBytes(ConsAgvBytePtr buffer, size_t size)
{
    CombineA ca;
    auto finish_bytes = ca.loadBytes(buffer, size);
    if(finish_bytes == 0)
        return 0;
    auto left_size = size - finish_bytes;
    iid_ = ca.proxyA().element().stdStr();
    alias_ = ca.proxyB().element().stdStr();
    finish_bytes = root_->loadBytes(buffer + finish_bytes, left_size);
    if(finish_bytes == 0)
        return 0;
    left_size -= finish_bytes;
    return size - left_size;
}

void Procedure::setAlias(const AgvString &str)
{
    alias_ = str;
}

void Procedure::run()
{
    if(run_context_.engine ==  nullptr){
        spdlog::error("No engine object attached!");
        return;
    }
    auto cur_engine = run_context_.engine;
    if(run_context_.async_run){ 
        run_context_.work_id = cur_engine->createWork(root_);
        cur_engine->commitWork(run_context_.work_id);
    }else{
        cur_engine->syncRunProcess(root_);
    }
}

void Procedure::setActiveRun(bool s)
{
    active_run_ = s;
}

AGV_NAMESPACE_END

