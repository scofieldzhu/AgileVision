/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: process.h  
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

#ifndef __agile_vision_core_process_h__
#define __agile_vision_core_process_h__

#include <atomic>
#include <mutex>
#include "agile_vision/core/run_context.h"
#include "agile_vision/core/serialization_frame.h"
#include "agile_vision/basic/time_util.h"

namespace agv::core {

    class ToolFactory;

    class AGV_CORE_API Process
    {
    public:
        using ToolList = std::vector<ToolSPtr>;
        using iterator = ToolList::iterator;
        using const_iterator = ToolList::const_iterator;
        iterator begin(){ return tools_.begin(); }
        const_iterator begin()const{ return tools_.begin(); }
        iterator end(){ return tools_.end(); }
        const_iterator end()const{ return tools_.end(); }
        AgvBytes serializeToBytes()const;
        size_t loadBytes(ConsAgvBytePtr buffer, size_t size);
        /// @brief 从完整拓扑帧重建工具并重连 Pin；factory 为空时只能复用已存在的同 iid 工具。
        size_t loadBytes(ConsAgvBytePtr buffer, size_t size, ToolFactory* factory);
        /// @brief 递归加载入口：同时重建工具内的嵌套子过程树。
        size_t loadBytes(ConsAgvBytePtr buffer, size_t size, ToolFactory* factory, detail::TopologyContext& ctx);
        bool run();
        bool runInTopologicalOrder();
        /// @brief 按依赖分层并行执行：同层工具并发，层间屏障等待全部完成。
        /// 同层工具互不依赖，因此不会同时写同一 DataBuffer；跨阶段读取由屏障建立 happens-before。
        bool runInParallelLayers();
        RunContext& runContext(){ return run_context_; }
        const RunContext& runContext()const{ return run_context_; }    
        void appendTool(ToolSPtr t);
        void insertTool(const_iterator pos, ToolSPtr t);
        ToolPtr findTool(const AgvString& name, bool recursive)const;
        const_iterator findTool(ToolPtr t)const;
        bool existsTool(ToolPtr t)const;
        void removeTool(ToolPtr t);
        void removeTool(const_iterator pos);
        void removeTool(iterator pos);
        auto sizeOfTools()const{ return tools_.size(); }
        void setInterrupt(bool s);
        void setAlias(const AgvString& s);
        const AgvString& alias()const{ return alias_; }
        const std::string& iid()const{ return iid_; }
        ProcessManager* manager(){ return manager_; }
        const ProcessManager* manager()const{ return manager_; }
        ProcessPtr findMutableChildProcess(const std::string& iid, bool recursive);
        ConstProcessPtr findChildProcess(const std::string& iid, bool recursive)const;
        Process* getMutableRootProcess();
        const Process* getRootProcess()const;
        explicit Process(const std::string& iid);
        ~Process();

    private:
        friend class ProcessManager;
        friend class Tool;
        AgvBytes serializeToBytes(detail::TopologyContext& ctx)const;
        void setManager(ProcessManager* t);
        bool runTools(const std::vector<ToolPtr>& ordered_tools);
        ProcessManager* manager_ = nullptr;
        std::string iid_;
        AgvString alias_{"null"};
        ToolList tools_;
        RunContext run_context_;
        agv::Timestamp timestamp_ = {0};
        std::atomic_bool interrupt_ = false;
        std::mutex run_mutex_;
    };

}

#endif

