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
#include "ratel/geometry/geometry.h"
#include "spdlog/spdlog.h"

using namespace ratel;

AGV_NAMESPACE_BEGIN

namespace{
    using StrPxEP = StringProxy;
    using CombineA = ProxyCombine<StringProxy, StringProxy, false>;
    using CombineB = ProxyCombine<CombineA, Process, true>;
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
    ca.proxyA() = iid_;
    ca.proxyB() = alias_;
    CombineB cb(ca, *root_);
    return cb.serializeToBytes();
}

size_t Procedure::loadBytes(ConsAgvBytePtr buffer, size_t size)
{
    CombineA ca;
    CombineB cb(ca, *root_);
    auto finish_bytes = cb.loadBytes(buffer, size);
    if(finish_bytes == 0)
        return 0;
    iid_ = ca.proxyA().stdStr();
    alias_ = ca.proxyB().stdStr();
    return finish_bytes;
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
        auto finish_func = [this](wkid_t w){
            spdlog::trace("Procedure:{} root run done!", this->alias_);
        };
        run_context_.work_id = cur_engine->commit(root_.get(), finish_func);
        if(run_context_.work_id == null_id){
            spdlog::error("Engine commit root process:{} work failed!", root_->iid());
        }
    }else{
        if(!cur_engine->syncRunProcess(root_.get())){
            spdlog::error("Engine syncRunProcess root process:{} work failed!", root_->iid());
        }
    }
}

void Procedure::setActiveRun(bool s)
{
    active_run_ = s;
}

AGV_NAMESPACE_END

