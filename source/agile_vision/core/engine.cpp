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
#include <algorithm>
#include "process.h"
#include "spdlog/spdlog.h"

AGV_NAMESPACE_BEGIN

Engine::Engine()
{
    std::thread(&Engine::updateWaitThreadProc, this).detach();
}

Engine::~Engine()
{
    {
        std::lock_guard<std::mutex> lk(access_wait_mutex_);
        stop_update_wait_ = true;
    }
    check_wait_cv_.notify_one();
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
    work_table_[wk_id]->process->runContext().engine = this;
    work_table_[wk_id]->process->run(); //sync running...
    {
        std::lock_guard<std::mutex> lk(access_wait_mutex_);
        finish_work_id_ = wk_id;
    }
    check_wait_cv_.notify_one();
}

void Engine::updateWaitThreadProc()
{
    while(!stop_update_wait_){
        std::unique_lock<std::mutex> lk(access_wait_mutex_);
        check_wait_cv_.wait(lk, [this]{ return stop_update_wait_ || finish_work_id_ != null_id; });
        if(stop_update_wait_){
            lk.unlock();
            break;
        }
        {
            //check
            for(auto& kv : wait_table_){
                if(kv.second->status_dict.find(finish_work_id_) == kv.second->status_dict.end())
                    continue;
                kv.second->status_dict[finish_work_id_] = true;
            }
            lk.unlock();            
        }
        std::vector<wtid_t> ids_to_del;
        for(auto& kv : wait_table_){
            auto& status_dict = kv.second->status_dict;
            if(std::all_of(status_dict.begin(), 
                            status_dict.end(),
                            [](const auto& it){return it.second;}
                            )){
                (kv.second->cb)(kv.first);
                ids_to_del.push_back(kv.first); //mark finished
            }
        }
        if(!ids_to_del.empty()){ //to remove finished items
            for(auto id : ids_to_del){
                wait_table_.erase(id);
            }
        }     
        finish_work_id_ = null_id;   
    };
}

wkid_t Engine::createWork(ProcessPtr p)
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

Engine::wtid_t Engine::createWait(wkid_list works, finish_callback cb)
{
    if(works.empty() || (bool)cb == false)
        return null_id;
    bool all_valid = std::all_of(works.begin(), works.end(),
        [this](const auto& id){
            return this->work_table_.find(id) != this->work_table_.end();
        }
    );
    if(!all_valid){
        spdlog::error("Exist invalid work id in passed id list!");
        return null_id;
    }
    auto new_wait_id = wait_id_seed_.next();
    auto new_wait = std::make_shared<Wait>();
    wait_table_[new_wait_id] = new_wait;
    new_wait->cb = cb;
    for(auto id : works)
        new_wait->status_dict.insert({id, false});
    return new_wait_id;
}

void Engine::syncRunProcess(ProcessPtr p)
{
    if(p){
        p->runContext().work_id = null_id;
        p->runContext().async_run = false;
        p->run();
    }
}

AGV_NAMESPACE_END
