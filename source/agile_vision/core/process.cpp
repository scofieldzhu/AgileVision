/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: process.cpp  
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
#include <future>
#include <ranges>
#include "process.h"
#include "tool.h"
#include "tool_factory.h"
#include "relationship_network.h"
#include "serialize_policy.h"
#include "serialization_frame.h"
#include "agile_vision/geometry/geometry.h"
#include "agile_vision/basic/dbg_tracker.h"
#include "agile_vision/basic/log_service.h"
using namespace agv;

namespace agv::core {
    Process::Process(const std::string &iid)
        : iid_(iid)
    {
    }

    Process::~Process()
    {
    }

    void Process::setManager(ProcessManager* t)
    {
        manager_ = t;
    }

    void Process::setAlias(const AgvString& s)
    {
        alias_ = s;
    }

    AgvBytes Process::serializeToBytes() const
    {
        detail::TopologyContext ctx;
        return serializeToBytes(ctx);
    }

    AgvBytes Process::serializeToBytes(detail::TopologyContext& ctx) const
    {
        if(ctx.depth >= detail::kMaxTreeDepth || ctx.node_count >= detail::kMaxTreeNodes){
            AGV_LOG_ERROR("Process tree depth/node limit exceeded at iid:{}", iid_);
            return AgvBytes();
        }
        ++ctx.node_count;
        AgvBytes payload;
        detail::writeString(payload, iid_);
        detail::writeString(payload, alias_);
        std::vector<ToolSPtr> valid_tools;
        valid_tools.reserve(tools_.size());
        for(const auto& tool : tools_){
            if(tool != nullptr){
                valid_tools.push_back(tool);
            }
        }
        detail::writeU32(payload, static_cast<uint32_t>(valid_tools.size()));
        for(const auto& tool : valid_tools){
            const auto bytes = tool->serializeTree(ctx);
            if(bytes.empty()){
                return AgvBytes();
            }
            detail::writeBytes(payload, bytes.data(), bytes.size());
        }
        AgvBytes out;
        detail::writeFrameHeader(out, detail::kProcessFrameMagic, static_cast<uint32_t>(payload.size()));
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }

    size_t Process::loadBytes(ConsAgvBytePtr buffer, size_t size)
    {
        return loadBytes(buffer, size, nullptr);
    }

    size_t Process::loadBytes(ConsAgvBytePtr buffer, size_t size, ToolFactory* factory)
    {
        detail::TopologyContext ctx;
        return loadBytes(buffer, size, factory, ctx);
    }

    size_t Process::loadBytes(ConsAgvBytePtr buffer, size_t size, ToolFactory* factory, detail::TopologyContext& ctx)
    {
        if(buffer == nullptr || size < 12){
            return 0;
        }
        if(ctx.depth >= detail::kMaxTreeDepth || ctx.node_count >= detail::kMaxTreeNodes){
            return 0;
        }
        ++ctx.node_count;
        auto cur = buffer;
        size_t left = size;
        uint32_t payload_size = 0;
        if(!detail::readFrameHeader(cur, left, detail::kProcessFrameMagic, payload_size)){
            return 0;
        }
        std::string loaded_iid;
        std::string loaded_alias;
        if(!detail::readString(cur, left, loaded_iid) || !detail::readString(cur, left, loaded_alias)){
            return 0;
        }
        uint32_t tool_count = 0;
        if(!detail::readU32(cur, left, tool_count)){
            return 0;
        }
        std::vector<AgvBytes> tool_frames;
        tool_frames.reserve(tool_count);
        for(uint32_t i = 0; i < tool_count; ++i){
            const char* data = nullptr;
            size_t data_size = 0;
            if(!detail::readBytes(cur, left, data, data_size)){
                return 0;
            }
            tool_frames.emplace_back(data, data + data_size);
        }
        std::vector<ToolSPtr> new_tools;
        std::vector<std::vector<PinBinding>> all_bindings;
        new_tools.reserve(tool_frames.size());
        all_bindings.reserve(tool_frames.size());
        for(const auto& frame : tool_frames){
            if(frame.size() < 12 + 4){
                return 0;
            }
            // 工具帧的 payload 前两个字段是 clsid 与 iid，先解析出来用于工厂重建。
            auto meta_cur = frame.data() + 12;
            size_t meta_left = frame.size() - 12;
            std::string clsid;
            std::string tool_iid;
            if(!detail::readString(meta_cur, meta_left, clsid) || !detail::readString(meta_cur, meta_left, tool_iid)){
                return 0;
            }
            ToolSPtr tool = nullptr;
            if(factory != nullptr){
                tool = factory->createTool(clsid, tool_iid);
            }
            if(tool == nullptr){
                for(const auto& existing : tools_){
                    if(existing != nullptr && existing->iid() == tool_iid){
                        tool = existing;
                        break;
                    }
                }
            }
            if(tool == nullptr){
                AGV_LOG_ERROR("Rebuild tool:{} of clsid:{} failed!", tool_iid, clsid);
                return 0;
            }
            std::vector<PinBinding> bindings;
            if(tool->loadTopologyBytes(frame.data(), frame.size(), &bindings, factory, ctx) == 0){
                return 0;
            }
            new_tools.push_back(std::move(tool));
            all_bindings.push_back(std::move(bindings));
        }
        const auto find_tool = [&new_tools](const std::string& tool_iid) -> ToolPtr{
            for(const auto& tool : new_tools){
                if(tool->iid() == tool_iid){
                    return tool.get();
                }
            }
            return nullptr;
        };
        for(size_t i = 0; i < new_tools.size(); ++i){
            for(const auto& binding : all_bindings[i]){
                auto* producer = find_tool(binding.producer_iid);
                if(producer == nullptr){
                    AGV_LOG_ERROR("Producer:{} of pin:{} not found!", binding.producer_iid, binding.consume_pin_key);
                    return 0;
                }
                const auto* output = producer->getOutputPin(binding.produce_pin_key);
                if(output == nullptr){
                    AGV_LOG_ERROR("Produce pin:{} of tool:{} not found!", binding.produce_pin_key, binding.producer_iid);
                    return 0;
                }
                ProduceInfo pi;
                pi.pin = const_cast<OutputPin*>(output);
                pi.data_location = binding.data_location;
                if(!new_tools[i]->setPinConnection(binding.consume_pin_key, pi)){
                    return 0;
                }
            }
        }
        iid_ = loaded_iid;
        alias_ = loaded_alias;
        tools_ = std::move(new_tools);
        for(auto& tool : tools_){
            tool->setJoinedProcess(this);
        }
        return size - left;
    }

    bool Process::runTools(const std::vector<ToolPtr>& ordered_tools)
    {
        std::lock_guard __auto_lock(run_mutex_);
        bool all_succeeded = true;
        run_context_.is_running = true;
        for(auto* t : ordered_tools){
            if(t == nullptr){
                continue;
            }
            if(interrupt_.load()){
                break;
            }
            // 先执行再聚合：直接写 s = s && t->run() 会因短路跳过失败工具之后的所有工具。
            if(!t->run()){
                all_succeeded = false;
            }
        }
        run_context_.is_running = false;
        interrupt_.store(false);
        return all_succeeded;
    }

    bool Process::run()
    {
        std::vector<ToolPtr> ordered_tools;
        ordered_tools.reserve(tools_.size());
        for(const auto& t : tools_){
            ordered_tools.push_back(t.get());
        }
        return runTools(ordered_tools);
    }

    bool Process::runInTopologicalOrder()
    {
        RelationshipNetwork network;
        network.buildFrom(this, false);
        if(network.checkCycle()){
            AGV_LOG_ERROR("Dataflow cycle detected in process:{}, refuse topological run!", alias_);
            return false;
        }
        std::vector<ConstToolPtr> sorted_tools;
        network.topologicalSort(sorted_tools);
        std::vector<ToolPtr> ordered_tools;
        ordered_tools.reserve(sorted_tools.size());
        for(const auto* tool : sorted_tools){
            ordered_tools.push_back(const_cast<ToolPtr>(tool));
        }
        return runTools(ordered_tools);
    }

    bool Process::runInParallelLayers()
    {
        std::lock_guard __auto_lock(run_mutex_);
        RelationshipNetwork network;
        network.buildFrom(this, false);
        if(network.checkCycle()){
            AGV_LOG_ERROR("Dataflow cycle detected in process:{}, refuse parallel run!", alias_);
            return false;
        }
        std::vector<std::vector<ToolPtr>> levels;
        if(!network.topologicalLevels(levels)){
            AGV_LOG_ERROR("Topological layering failed in process:{}!", alias_);
            return false;
        }
        bool all_succeeded = true;
        run_context_.is_running = true;
        for(const auto& level : levels){
            if(interrupt_.load()){
                break;
            }
            std::vector<std::future<bool>> results;
            results.reserve(level.size());
            for(auto* tool : level){
                if(tool == nullptr){
                    continue;
                }
                try
                {
                    results.push_back(std::async(std::launch::async, [tool](){
                        try
                        {
                            return tool->run();
                        }
                        catch(const std::exception& e)
                        {
                            AGV_LOG_ERROR("Tool run failed with exception: {}", e.what());
                            return false;
                        }
                        catch(...)
                        {
                            AGV_LOG_ERROR("Tool run failed with unknown exception");
                            return false;
                        }
                    }));
                }
                catch(const std::exception& e)
                {
                    AGV_LOG_ERROR("Start tool task failed: {}", e.what());
                    all_succeeded = false;
                }
            }
            // 屏障：本层全部完成后才进入下一层，建立跨阶段的 happens-before。
            for(auto& future : results){
                bool ok = false;
                try
                {
                    ok = future.get();
                }
                catch(const std::exception& e)
                {
                    AGV_LOG_ERROR("Parallel tool task failed: {}", e.what());
                }
                catch(...)
                {
                    AGV_LOG_ERROR("Parallel tool task failed with unknown exception");
                }
                if(!ok){
                    all_succeeded = false;
                }
            }
        }
        run_context_.is_running = false;
        interrupt_.store(false);
        return all_succeeded;
    }

    void Process::appendTool(ToolSPtr t)
    {
        if(t && !existsTool(t.get())){
            tools_.push_back(t);
            t->setJoinedProcess(this);
        }
    }

    void Process::insertTool(const_iterator pos, ToolSPtr t)
    {
        if(t && !existsTool(t.get())){
            tools_.insert(pos, t);
            t->setJoinedProcess(this);
        }
    }

    ToolPtr Process::findTool(const AgvString& name, bool recursive) const
    {
        if(!tools_.empty()){
            auto it = std::find_if(tools_.begin(), tools_.end(), [name](const auto& item){ return item->name() == name; });
            if(it != tools_.end()){
                return (*it).get();
            }
        }
        return nullptr;
    }

    Process::const_iterator Process::findTool(Tool *t) const
    {
        if(t && !tools_.empty()){
            return std::find_if(tools_.begin(), tools_.end(), [t](const auto& item){ return item.get() == t; });
        }
        return end();
    }

    bool Process::existsTool(Tool *t) const
    {
        return t && !tools_.empty() && std::find_if(tools_.begin(), tools_.end(), [t](const auto& item){ return item.get() == t; }) != tools_.end();
    }

    void Process::removeTool(ToolPtr t)
    {
        auto it = findTool(t);
        if(it != end()){
            t->setJoinedProcess(nullptr);
            tools_.erase(it);
        }        
    }

    void Process::removeTool(const_iterator pos)
    {
        if(pos != end()){
            auto t = *pos;
            t->setJoinedProcess(nullptr);
            tools_.erase(pos);
        }
    }

    void Process::removeTool(iterator pos)
    {
        if(pos != end()){
            auto t = *pos;
            t->setJoinedProcess(nullptr);
            tools_.erase(pos);
        }
    }

    void Process::setInterrupt(bool s)
    {
        interrupt_.store(s);
    }

    ProcessPtr Process::findMutableChildProcess(const std::string& iid, bool recursive)
    {
        return const_cast<ProcessPtr>(const_cast<const Process*>(this)->findChildProcess(iid, recursive));
    }

    ConstProcessPtr Process::findChildProcess(const std::string &iid, bool recursive) const
    {
        if(!recursive){
            return this->iid() == iid ? this : nullptr;
        }
        for(auto t : tools_){
            auto mgr = t->getMutableProcessManager();
            if(mgr){
                auto target_proc = mgr->findProcess(iid, recursive);
                if(target_proc){
                    return target_proc;
                }
            }
        }
        return nullptr;
    }

    Process *Process::getMutableRootProcess()
    {
        return const_cast<Process*>(const_cast<const Process*>(this)->getRootProcess());
    }

    const Process* Process::getRootProcess() const
    {
        if(manager_ == nullptr){
            return this;
        }
        auto parent = manager_->owner()->joinedProcess();
        while(parent){
            if(parent->manager_ == nullptr){
                return parent;
            }
            parent = parent->manager_->owner()->joinedProcess();
        }
        return nullptr;
    }

}
