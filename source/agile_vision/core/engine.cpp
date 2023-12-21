 /*
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *
 *   File: engine.cpp
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
#include "engine.h"
#include "process.h"
#include "spdlog/spdlog.h"

AGV_NAMESPACE_BEGIN

Engine::Engine()
{
}

Engine::~Engine()
{
}

bool Engine::isValidWork(Process* p) const
{
    for(const auto& kv : work_table_){
        if(kv.second->process.get() == p)
            return true;
    }
    return false;
}

void Engine::makeActiveWork(wkid_t w) const
{
}

void Engine::runThreadProc(wkid_t wk_id)
{
    work_table_[wk_id]->process->run(); //sync running...
    
}

Engine::wkid_t Engine::createWork(ProcessPtr p)
{
    if(p == nullptr || isValidWork(p.get()))
        return null_id;    
    auto work = std::make_shared<Work>();
    work->process = p;
    auto new_wid = work_id_seed_.next();
    work_table_.insert({new_wid, work});
    return new_wid;
}

void Engine::commitWork(wkid_t wid)
{
    if(!isValidWork(wid) || isActiveWork(wid))
        return;
    std::thread th(&Engine::runThreadProc, this, wid);
    work_table_[wid]->thread = std::move(th);
    makeActiveWork(wid);
}

void Engine::stopWork(wkid_t w)
{
}

bool Engine::isValidWork(wkid_t w) const
{
    return work_table_.find(w) != work_table_.end();
}

bool Engine::isActiveWork(wkid_t w) const
{
    return false;
}

void Engine::destroyWork(wkid_t w)
{
}

bool Engine::existsWork(wkid_t w) const
{
    return false;
}

AGV_NAMESPACE_END
