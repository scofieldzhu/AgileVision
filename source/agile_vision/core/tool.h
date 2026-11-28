/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: tool.h  
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

#ifndef __agile_vision_core_tool_h__
#define __agile_vision_core_tool_h__

#include <cstdint>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "agile_vision/core/input_pin.h"
#include "agile_vision/core/output_pin.h"
#include "agile_vision/core/tool_metadata.h"
#include "agile_vision/core/prop_pin.h"
#include "agile_vision/core/process_manager.h"
#include "agile_vision/core/tool_linkage.h"
#include "agile_vision/core/serialization_frame.h"

namespace agv::core {

    class ToolFactory;

    /// @brief 反序列化后待由 Process 统一重连的输入引脚绑定描述。
    struct PinBinding
    {
        std::string consume_pin_key;
        std::string producer_iid;
        std::string produce_pin_key;
        uint32_t data_location = 0;
    };

    class AGV_CORE_API Tool 
    {
    public:
        static constexpr PinKey kStatusPinKey = "Status";
        virtual AgvBytes serializeToBytes()const;
        /// @brief 递归序列化入口：附带子过程树，ctx 用于深度/节点防护。
        AgvBytes serializeTree(detail::TopologyContext& ctx)const;
        virtual size_t loadBytes(ConsAgvBytePtr buffer, size_t size);
        /// @brief 加载完整拓扑帧；绑定关系输出到 bindings，由所属 Process 统一重连。
        size_t loadTopologyBytes(ConsAgvBytePtr buffer, size_t size, std::vector<PinBinding>* bindings);
        /// @brief 递归加载入口：解析 Pin 与子过程段，子过程在此重建到本 Tool 的 ProcessManager。
        size_t loadTopologyBytes(ConsAgvBytePtr buffer, size_t size, std::vector<PinBinding>* bindings, ToolFactory* factory, detail::TopologyContext& ctx);
        /// @brief 反序列化时按需补齐派生类构造阶段未创建的引脚。
        bool restorePin(const PinKey& key, PinType type, const DataSpec& spec);
        bool setPinConnection(const PinKey& consume_key, const ProduceInfo& pi);
        ToolLinkageList collectPinLinkages()const;
        /// @brief 反射本工具全部 Pin 规格，供 UI 自动生成配置界面。
        std::vector<PinMetadata> pinMetadata()const;
        bool run();
        const OutputPin* getOutputPin(const PinKey& key)const;
        const InputPin* getInputPin(const PinKey& key)const;
        PropPin* getPropPin(const PinKey& key);
        const PropPin* getPropPin(const PinKey& key)const;
        const ToolPin* getToolPin(const PinKey& key)const;
        const std::string& iid()const{ return iid_; }
        void setName(const AgvString& str);
        const auto& name()const{ return name_; }
        virtual std::string getClsGuid()const = 0;
        const ProcessManager* getProcessManager()const{ return child_process_manager_.get(); }
        ProcessManager* getMutableProcessManager(){ return child_process_manager_.get(); }
        ProcessPtr joinedProcess(){ return joined_process_; }
        ConstProcessPtr joinedProcess()const { return joined_process_; }
        virtual Tool* clone()const;
        Tool& operator=(const Tool&) = delete;
        explicit Tool(const std::string& iid);
        Tool(const Tool&) = delete;
        Tool(Tool&& rhs) = default;
        virtual ~Tool();

    protected:
        using ToolPinSPtr = std::shared_ptr<ToolPin>;
        using ToolPinDict = std::map<PinKey, ToolPinSPtr>;
        OutputPin* getOutputPin(const PinKey& key);
        InputPin* getInputPin(const PinKey& key);    
        ToolPin* getToolPin(const PinKey& key);
        bool checkPinDataCompatible()const;
        void addPin(const PinKey& key, ToolPinSPtr pin);
        void autoCreateProcesManager();    
        virtual bool requestOutputData() = 0;

    private:
        friend class Process;
        void setJoinedProcess(ProcessPtr p);
        std::unique_ptr<ProcessManager> child_process_manager_;
        std::string iid_;
        AgvString name_ = "unnamed";
        ToolPinDict tool_pin_dict_;
        // 反序列化补建引脚时持有 key 字符串，保证字典中的 string_view 长期有效。
        std::list<std::string> owned_pin_keys_;
        ProcessPtr joined_process_ = nullptr;
    };

}

#endif