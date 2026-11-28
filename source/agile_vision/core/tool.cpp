/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: tool.cpp  
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

#include "tool.h"
#include "process.h"
#include "procedure.h"
#include "relationship_network.h"
#include "serialize_policy.h"
#include "serialization_frame.h"
#include "agile_vision/basic/dbg_tracker.h"
#include "agile_vision/basic/log_service.h"
using namespace agv;

namespace agv::core {

    Tool::Tool(const std::string &iid)
        : iid_(iid)
    {
        auto status_pin = std::make_shared<OutputPin>(DataSpec::SingleInt());
        addPin(kStatusPinKey, status_pin);
    }

    Tool::~Tool()
    {
    }

    bool Tool::requestOutputData()
    {
        return false;
    }

    void Tool::setJoinedProcess(ProcessPtr p)
    {
        joined_process_ = p;
    }

    // bool Tool::setPinConnection(const PinKey& consume_key, ToolPtr producer, const PinKey& produce_key, unsigned int data_location)
    // {
    //     auto procedure = belongedProcedure();
    //     if(procedure == nullptr){
    //         AGV_LOG_WARN("Connection will break with consume key:{}", consume_key);
    //         //??? how to do?
    //         return true;
    //     }       
    //     auto consume_pin = this->getToolPin(consume_key);
    //     if(consume_pin == nullptr){
    //         AGV_LOG_ERROR("No such pin exists with name:{}!", consume_key);
    //         return false;
    //     }
    //     if(consume_pin->canReferenceData()){
    //         AGV_LOG_ERROR("This pin type:{} cannot consume any data!", (int)consume_pin->getPinType());
    //         return false;
    //     }
    //     auto rsn = belongedProcedure()->tool_relationships_;
    //     if(!rsn->existTool(this))
    //         rsn->addTool(this);
    //     if(producer && !rsn->existTool(producer))
    //         rsn->addTool(producer);
    //     ToolLinkage tl;
    //     tl.producer = producer;
    //     tl.produce_pin_key = produce_key;
    //     tl.data_location = data_location;
    //     tl.consumer = this;
    //     tl.consume_pin_key = consume_key;
    //     rsn->makeRelationship(tl);
    //     return true;
    // }

    std::vector<PinMetadata> Tool::pinMetadata() const
    {
        std::vector<PinMetadata> metadata_list;
        metadata_list.reserve(tool_pin_dict_.size());
        for(const auto& kv : tool_pin_dict_){
            const auto* pin = kv.second.get();
            if(pin == nullptr){
                continue;
            }
            PinMetadata meta;
            meta.key = std::string(kv.first);
            meta.pin_type = pin->getPinType();
            const auto& spec = pin->dataSpec();
            meta.data_type = spec.major_type;
            meta.subtype = spec.subtype;
            meta.array_size = spec.arry_size;
            meta.alias = std::string(pin->alias());
            meta.deprecated = pin->deprecated();
            const auto* input = dynamic_cast<const InputPin*>(pin);
            meta.optional = input != nullptr && input->optional();
            metadata_list.push_back(std::move(meta));
        }
        return metadata_list;
    }

    AgvBytes Tool::serializeToBytes() const
    {
        detail::TopologyContext ctx;
        return serializeTree(ctx);
    }

    AgvBytes Tool::serializeTree(detail::TopologyContext& ctx) const
    {
        if(ctx.depth >= detail::kMaxTreeDepth || ctx.node_count >= detail::kMaxTreeNodes){
            AGV_LOG_ERROR("Tool tree depth/node limit exceeded at iid:{}", iid_);
            return AgvBytes();
        }
        ++ctx.node_count;
        AgvBytes payload;
        detail::writeString(payload, getClsGuid());
        detail::writeString(payload, iid_);
        detail::writeString(payload, name_);
        std::vector<const ToolPin*> pins;
        pins.reserve(tool_pin_dict_.size());
        for(const auto& kv : tool_pin_dict_){
            if(kv.second != nullptr){
                pins.push_back(kv.second.get());
            }
        }
        detail::writeU32(payload, static_cast<uint32_t>(pins.size()));
        for(const auto* pin : pins){
            AgvString key;
            for(const auto& kv : tool_pin_dict_){
                if(kv.second.get() == pin){
                    key = AgvString(kv.first);
                    break;
                }
            }
            detail::writeString(payload, key);
            detail::writeU8(payload, static_cast<uint8_t>(pin->getPinType()));
            const auto spec_bytes = pin->dataSpec().serializeToBytes();
            detail::writeBytes(payload, spec_bytes.data(), spec_bytes.size());
            detail::writeString(payload, pin->alias());
            detail::writeU8(payload, pin->deprecated() ? 1 : 0);
            const auto* input = dynamic_cast<const InputPin*>(pin);
            detail::writeU8(payload, input != nullptr && input->optional() ? 1 : 0);
            const auto* prop = dynamic_cast<const PropPin*>(pin);
            const auto prop_bytes = prop != nullptr ? prop->serializeToBytes() : AgvBytes();
            detail::writeBytes(payload, prop_bytes.data(), prop_bytes.size());
        }
        const auto linkages = collectPinLinkages();
        detail::writeU32(payload, static_cast<uint32_t>(linkages.size()));
        for(const auto& linkage : linkages){
            detail::writeString(payload, std::string(linkage.consume_pin_key));
            detail::writeString(payload, linkage.producer != nullptr ? linkage.producer->iid() : std::string());
            detail::writeString(payload, std::string(linkage.produce_pin_key));
            detail::writeU32(payload, linkage.data_location);
        }
        // 可选子过程段：带独立 magic，旧帧无此段也能按原有字段解析。
        const auto* manager = getProcessManager();
        const bool has_children = manager != nullptr && manager->size() > 0;
        detail::writeU32(payload, detail::kChildProcessSegmentMagic);
        detail::writeU8(payload, has_children ? 1 : 0);
        if(has_children){
            detail::writeString(payload, manager->activeProcessIid());
            detail::writeU32(payload, static_cast<uint32_t>(manager->size()));
            ++ctx.depth;
            for(const auto& child : *manager){
                const auto child_bytes = child == nullptr ? AgvBytes() : child->serializeToBytes(ctx);
                if(child_bytes.empty()){
                    return AgvBytes();
                }
                detail::writeBytes(payload, child_bytes.data(), child_bytes.size());
            }
            --ctx.depth;
        }
        AgvBytes out;
        detail::writeFrameHeader(out, detail::kToolFrameMagic, static_cast<uint32_t>(payload.size()));
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }

    size_t Tool::loadBytes(ConsAgvBytePtr buffer, size_t size)
    {
        return loadTopologyBytes(buffer, size, nullptr);
    }

    size_t Tool::loadTopologyBytes(ConsAgvBytePtr buffer, size_t size, std::vector<PinBinding>* bindings)
    {
        detail::TopologyContext ctx;
        return loadTopologyBytes(buffer, size, bindings, nullptr, ctx);
    }

    bool Tool::restorePin(const PinKey& key, PinType type, const DataSpec& spec)
    {
        if(getToolPin(key) != nullptr){
            return true;
        }
        std::shared_ptr<ToolPin> pin;
        if(type == PinType::kInput){
            pin = std::make_shared<InputPin>(spec);
        }else if(type == PinType::kOutput){
            pin = std::make_shared<OutputPin>(spec);
        }else if(type == PinType::kProp){
            pin = std::make_shared<PropPin>(spec);
        }else{
            return false;
        }
        owned_pin_keys_.push_back(std::string(key));
        addPin(std::string_view(owned_pin_keys_.back()), pin);
        return true;
    }

    size_t Tool::loadTopologyBytes(ConsAgvBytePtr buffer, size_t size, std::vector<PinBinding>* bindings,
                                   ToolFactory* factory, detail::TopologyContext& ctx)
    {
        if(buffer == nullptr || size < 12){
            return 0;
        }
        auto cur = buffer;
        size_t left = size;
        uint32_t payload_size = 0;
        if(!detail::readFrameHeader(cur, left, detail::kToolFrameMagic, payload_size)){
            return 0;
        }
        std::string clsid;
        std::string loaded_iid;
        std::string loaded_name;
        if(!detail::readString(cur, left, clsid)){
            return 0;
        }
        if(!detail::readString(cur, left, loaded_iid)){
            return 0;
        }
        if(!detail::readString(cur, left, loaded_name)){
            return 0;
        }
        if(clsid != getClsGuid()){
            AGV_LOG_ERROR("Tool clsid mismatch, expected:{} got:{}", getClsGuid(), clsid);
            return 0;
        }
        iid_ = loaded_iid;
        name_ = loaded_name;
        uint32_t pin_count = 0;
        if(!detail::readU32(cur, left, pin_count)){
            return 0;
        }
        for(uint32_t i = 0; i < pin_count; ++i){
            std::string key;
            std::string alias;
            uint8_t type = 0;
            uint8_t deprecated = 0;
            uint8_t optional = 0;
            const char* spec_data = nullptr;
            const char* prop_data = nullptr;
            size_t spec_size = 0;
            size_t prop_size = 0;
            if(!detail::readString(cur, left, key)){
                return 0;
            }
            if(!detail::readU8(cur, left, type)){
                return 0;
            }
            if(!detail::readBytes(cur, left, spec_data, spec_size)){
                return 0;
            }
            if(!detail::readString(cur, left, alias)){
                return 0;
            }
            if(!detail::readU8(cur, left, deprecated)){
                return 0;
            }
            if(!detail::readU8(cur, left, optional)){
                return 0;
            }
            if(!detail::readBytes(cur, left, prop_data, prop_size)){
                return 0;
            }
            DataSpec spec;
            if(spec_size == 0 || spec.loadBytes(spec_data, spec_size) == 0){
                return 0;
            }
            const auto pin_type = static_cast<PinType>(type);
            if(!restorePin(key, pin_type, spec)){
                return 0;
            }
            auto* pin = getToolPin(key);
            if(pin == nullptr){
                return 0;
            }
            pin->setAlias(alias);
            pin->setDeprecated(deprecated != 0);
            if(auto* input = dynamic_cast<InputPin*>(pin); input != nullptr){
                input->setOptional(optional != 0);
            }
            if(auto* prop = dynamic_cast<PropPin*>(pin); prop != nullptr && prop_size > 0){
                if(prop->loadBytes(prop_data, prop_size) == 0){
                    return 0;
                }
            }
        }
        uint32_t link_count = 0;
        if(!detail::readU32(cur, left, link_count)){
            return 0;
        }
        if(bindings != nullptr){
            bindings->clear();
            bindings->reserve(link_count);
        }
        for(uint32_t i = 0; i < link_count; ++i){
            PinBinding binding;
            if(!detail::readString(cur, left, binding.consume_pin_key)){
                return 0;
            }
            if(!detail::readString(cur, left, binding.producer_iid)){
                return 0;
            }
            if(!detail::readString(cur, left, binding.produce_pin_key)){
                return 0;
            }
            if(!detail::readU32(cur, left, binding.data_location)){
                return 0;
            }
            if(bindings != nullptr){
                bindings->push_back(std::move(binding));
            }
        }
        // 可选子过程段：旧帧读完 linkage 后 left 为 0，新帧则带 magic 前缀。
        if(left >= 5){
            uint32_t magic = 0;
            if(!detail::readU32(cur, left, magic)){
                return 0;
            }
            if(magic != detail::kChildProcessSegmentMagic){
                return 0;
            }
            uint8_t has_children = 0;
            if(!detail::readU8(cur, left, has_children)){
                return 0;
            }
            if(has_children != 0){
                if(ctx.depth >= detail::kMaxTreeDepth){
                    AGV_LOG_ERROR("Child process tree depth limit exceeded at tool:{}", iid_);
                    return 0;
                }
                std::string active_iid;
                uint32_t child_count = 0;
                if(!detail::readString(cur, left, active_iid) || !detail::readU32(cur, left, child_count)){
                    return 0;
                }
                if(child_count > detail::kMaxTreeNodes){
                    return 0;
                }
                std::vector<AgvBytes> child_frames;
                child_frames.reserve(child_count);
                for(uint32_t i = 0; i < child_count; ++i){
                    const char* data = nullptr;
                    size_t data_size = 0;
                    if(!detail::readBytes(cur, left, data, data_size)){
                        return 0;
                    }
                    child_frames.emplace_back(data, data + data_size);
                }
                auto* manager = getMutableProcessManager();
                if(manager == nullptr){
                    autoCreateProcesManager();
                    manager = getMutableProcessManager();
                }
                if(manager == nullptr){
                    return 0;
                }
                manager->clear();
                ctx.depth = ctx.depth + 1;
                for(const auto& frame : child_frames){
                    auto child = std::make_shared<Process>("");
                    if(child->loadBytes(frame.data(), frame.size(), factory, ctx) == 0){
                        return 0;
                    }
                    manager->appendProcess(std::move(child));
                }
                manager->setActiveProcessIid(active_iid);
            }
        }
        return size - left;
    }

    bool Tool::setPinConnection(const PinKey &consume_key, const ProduceInfo &pi)
    {
        auto consume_pin = getInputPin(consume_key);
        if(consume_pin == nullptr){
            AGV_LOG_ERROR("No such consume key:{} exists!", consume_key);
            return false;
        }
        const auto& old_info = consume_pin->produceInfo();
        if(old_info){
            old_info.pin->removeConsumePin(consume_pin);
        }
        consume_pin->unbind();
        if(pi.isNull()){ //disconnect pin relationship.
            return true;
        }
        consume_pin->bindProduceInfo(pi);
        // 在生产者的输出引脚上登记消费者引脚，而非生产者自身
        pi.pin->addConsumeInfo({consume_pin});
        return true;
    }

    ToolLinkageList Tool::collectPinLinkages() const
    {
        ToolLinkageList linkages;
        for(const auto& kv : tool_pin_dict_){
            const auto* input = dynamic_cast<const InputPin*>(kv.second.get());
            if(input == nullptr || input->deprecated()){
                continue;
            }
            const auto& produce = input->produceInfo();
            ConstToolPtr producer = produce.isNull() ? nullptr : produce.pin->ownerTool();
            if(producer == nullptr){
                continue;
            }
            ToolLinkage linkage;
            linkage.consumer = this;
            linkage.consume_pin_key = kv.first;
            linkage.data_location = produce.data_location;
            linkage.producer = producer;
            for(const auto& producer_kv : producer->tool_pin_dict_){
                if(producer_kv.second.get() == produce.pin){
                    linkage.produce_pin_key = producer_kv.first;
                    break;
                }
            }
            linkages.push_back(std::move(linkage));
        }
        return linkages;
    }

    bool Tool::run()
    {
        auto status_pin = getOutputPin(kStatusPinKey);
        status_pin->mutableDataBuffer().setIntValue(0);
        if(!checkPinDataCompatible()){
            AGV_LOG_ERROR("CheckPinDataCompatible failed!");
            status_pin->mutableDataBuffer().setIntValue(1);
            return false;
        }
        if(!requestOutputData()){
            AGV_LOG_ERROR("RequestOutputData failed!");
            status_pin->mutableDataBuffer().setIntValue(2);
            return false;
        }
        return true;
    }

    template <class PinCls, PinType Type>
    PinCls* obtainPinObject(const std::map<PinKey, std::shared_ptr<ToolPin>>& dict, const PinKey &key)
    {
        for(const auto& kv : dict){
            auto& cur_pin = kv.second;
            const auto& cur_key = kv.first;
            if(key == kv.first && cur_pin->getPinType() == Type && !kv.second->deprecated()){
                return dynamic_cast<PinCls*>(cur_pin.get());
            }
        }
        return nullptr;
    }

    const OutputPin* Tool::getOutputPin(const PinKey &key) const
    {
        return obtainPinObject<OutputPin, PinType::kOutput>(tool_pin_dict_, key);
    }

    const InputPin* Tool::getInputPin(const PinKey &key) const
    {
        return obtainPinObject<InputPin, PinType::kInput>(tool_pin_dict_, key);
    }

    const PropPin* Tool::getPropPin(const PinKey &key) const
    {
        return obtainPinObject<PropPin, PinType::kProp>(tool_pin_dict_, key);
    }

    OutputPin* Tool::getOutputPin(const PinKey &key)
    {
        return obtainPinObject<OutputPin, PinType::kOutput>(tool_pin_dict_, key);
    }

    InputPin* Tool::getInputPin(const PinKey &key) 
    {
        return obtainPinObject<InputPin, PinType::kInput>(tool_pin_dict_, key);
    }

    PropPin* Tool::getPropPin(const PinKey &key) 
    {
        return obtainPinObject<PropPin, PinType::kProp>(tool_pin_dict_, key);
    }

    ToolPin* Tool::getToolPin(const PinKey& key) 
    {
        ToolPinDict::iterator cit = tool_pin_dict_.find(key);
        return cit != tool_pin_dict_.end() ? (*cit).second.get() : nullptr;
    }

    // bool Tool::checkPinDataCompatible() const
    // {
    //     auto rsn = belongedProcedure()->tool_relationships_;
    //     ToolLinkageList produce_linkages;
    //     rsn->getToolRelationships(this, &produce_linkages, nullptr);
    //     for(const auto& tl : produce_linkages){
    //         auto producer = tl.producer;
    //         auto output_pin = producer->getOutputPin(tl.produce_pin_key);
    //         auto input_pin = tl.consumer->getInputPin(tl.consume_pin_key);
    //         if(output_pin == nullptr || input_pin == nullptr){
    //             AGV_LOG_ERROR("Invalid pin with key:{}", tl.produce_pin_key);
    //             return false;
    //         }
    //         if(!output_pin->dataSpec().compatibleWith(input_pin->dataSpec())){
    //             AGV_LOG_ERROR("Output pin:{} data spec is not compatile with input pin:{} data spec!", tl.produce_pin_key, tl.consume_pin_key);
    //             return false;
    //         }
    //         if(input_pin->optional() || input_pin->deprecated()){
    //             continue;
    //         }
    //         // other stuffs:
    //         //...
    //     }
    //     return true;
    // }

    bool Tool::checkPinDataCompatible() const
    {
        for(auto& kv : tool_pin_dict_){
            auto pin = kv.second;
            if(pin->getPinType() != PinType::kInput || dynamic_cast<InputPin*>(pin.get())->optional()){
                continue;
            }
            auto input_pin = dynamic_cast<InputPin*>(pin.get());
            auto produce_info = input_pin->produceInfo();
            if(produce_info.isNull()){
                AGV_LOG_WARN("No producer bind at input pin:{}", kv.first);
                return false;
            }
            const auto& output_spec = produce_info.pin->dataSpec();
            const auto& input_spec = input_pin->dataSpec();
            if(!input_spec.compatibleWith(output_spec)){
                AGV_LOG_WARN("Producer pin's data spec is not compatible with consumer pin's:{}", kv.first);
                return false;
            }
            //other stuffs
            //...
        }
        return true;
    }

    void Tool::addPin(const PinKey& key, ToolPinSPtr pin)
    {
        if(tool_pin_dict_.find(key) == tool_pin_dict_.end()){
            pin->setOwnerTool(this);
            tool_pin_dict_[key] = pin;
        }
    }

    void Tool::autoCreateProcesManager()
    {
        if(child_process_manager_ == nullptr){
            child_process_manager_ = std::make_unique<ProcessManager>(this);
        }
    }

    Tool* Tool::clone()const
    {
        // 默认不支持克隆：基类无法通用复制派生行为与引脚连接，由需要的派生类重写。
        return nullptr;
    }

}
