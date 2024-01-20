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
#include "procedure.h"
#include "relationship_network.h"
#include "serialize_policy.h"
#include "ratel/basic/dbg_tracker.h"
#include "spdlog/spdlog.h"
using namespace ratel;

AGV_NAMESPACE_BEGIN

namespace 
{
#ifdef SERIALIZER_RATEL
    using ByteVecPx = VecProxy<agv_byte>;
    using PinDictProxy = DictProxy<std::string, ByteVecPx>;
    using StrProCb = ProxyCombine<StringProxy, StringProxy, true>;
    using FinalCb = ProxyCombine<StrProCb, PinDictProxy, true>;
#endif
}

Tool::Tool(const std::string& iid)
    :iid_(iid)
{
    auto status_pin = std::make_shared<OutputPin>(DataSpec::SingleInt());
    addPin(PK_O_Status, status_pin);
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
//         spdlog::warn("Connection will break with consume key:{}", consume_key);
//         //??? how to do?
//         return true;
//     }       
//     auto consume_pin = this->getToolPin(consume_key);
//     if(consume_pin == nullptr){
//         spdlog::error("No such pin exists with name:{}!", consume_key);
//         return false;
//     }
//     if(consume_pin->canReferenceData()){
//         spdlog::error("This pin type:{} cannot consume any data!", (int)consume_pin->getPinType());
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

AgvBytes Tool::serializeToBytes() const
{
#ifdef SERIALIZER_RATEL
    StringProxy sp1(iid_);
    StringProxy sp2(name_);
    StrProCb pc1(sp1, sp2); 
    PinDictProxy pdp;
    for(const auto& kv : tool_pin_dict_){
        auto pin = kv.second;
        if(pin->getPinType() == PinType::kProp){
            std::string key(kv.first);
            ByteVecPx bvp;
            bvp.mutableData() = dynamic_cast<PropPin*>(pin.get())->serializeToBytes();
            pdp.mutableData().insert({std::move(key), std::move(bvp)});
        }
    }
    FinalCb fpc(pc1, pdp);
    return fpc.serializeToBytes();
#endif
}

size_t Tool::loadBytes(ConsAgvBytePtr buffer, size_t size)
{
#ifdef SERIALIZER_RATEL    
    StringProxy sp1;
    StringProxy sp2;
    StrProCb pc1(sp1, sp2); 
    PinDictProxy pdp;
    FinalCb fpc(pc1, pdp);
    auto finish_bytes = fpc.loadBytes(buffer, size);
    if(finish_bytes == 0)
        return 0;
    iid_ = sp1.stdStr();
    name_ = sp2.stdStr();
    for(const auto& kv : pdp.data()){
        PinKey key = kv.first;
        const auto& bv = kv.second.data();
        auto ppin = getPropPin(key);
        if(ppin == nullptr){
            spdlog::warn("Prop pin:{} not exists!", key);
            continue;
        }
        if(ppin->loadBytes(bv.data(), bv.size()) == 0){
            return 0; // exception occur!
        }
    }
    return finish_bytes;
#endif
}

bool Tool::setPinConnection(const PinKey &consume_key, const ProduceInfo &pi)
{
    auto consume_pin = getInputPin(consume_key);
    if(consume_pin == nullptr){
        spdlog::error("No such consume key:{} exists!", consume_key);
        return false;
    }
    const auto& old_info = consume_pin->produceInfo();
    if(old_info)
        old_info.pin->removeConsumePin(consume_pin);
    consume_pin->unbind();
    if(pi.isNull())//disconnect pin relationship.        
        return true;
    consume_pin->bindProduceInfo(pi);
    pi.pin->addConsumeInfo({pi.pin});
    return false;
}

bool Tool::run()
{
    auto status_pin = getOutputPin(PK_O_Status);
    status_pin->mutableDataBuffer().setIntValue(0);
    if(!checkPinDataCompatible()){
        spdlog::error("CheckPinDataCompatible failed!");
        status_pin->mutableDataBuffer().setIntValue(1);
        return false;
    }
    if(!requestOutputData()){
        spdlog::error("RequestOutputData failed!");
        status_pin->mutableDataBuffer().setIntValue(2);
        return false;
    }
    return true;
}

template <class PinCls, PinType Type>
PinCls* ObtainPinObject(const std::map<PinKey, std::shared_ptr<ToolPin>>& dict, const PinKey &key)
{
    for(const auto& kv : dict){
        auto& cur_pin = kv.second;
        const auto& cur_key = kv.first;
        if(key == kv.first && cur_pin->getPinType() == Type && !kv.second->deprecated())
            return dynamic_cast<PinCls*>(cur_pin.get());
    }
    return nullptr;
}

const OutputPin* Tool::getOutputPin(const PinKey &key) const
{
    return ObtainPinObject<OutputPin, PinType::kOutput>(tool_pin_dict_, key);
}

const InputPin* Tool::getInputPin(const PinKey &key) const
{
    return ObtainPinObject<InputPin, PinType::kInput>(tool_pin_dict_, key);
}

const PropPin* Tool::getPropPin(const PinKey &key) const
{
    return ObtainPinObject<PropPin, PinType::kProp>(tool_pin_dict_, key);
}

OutputPin* Tool::getOutputPin(const PinKey &key)
{
    return ObtainPinObject<OutputPin, PinType::kOutput>(tool_pin_dict_, key);
}

InputPin* Tool::getInputPin(const PinKey &key) 
{
    return ObtainPinObject<InputPin, PinType::kInput>(tool_pin_dict_, key);
}

PropPin* Tool::getPropPin(const PinKey &key) 
{
    return ObtainPinObject<PropPin, PinType::kProp>(tool_pin_dict_, key);
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
//             spdlog::error("Invalid pin with key:{}", tl.produce_pin_key);
//             return false;
//         }
//         if(!output_pin->dataSpec().compatibleWith(input_pin->dataSpec())){
//             spdlog::error("Output pin:{} data spec is not compatile with input pin:{} data spec!", tl.produce_pin_key, tl.consume_pin_key);
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
        if(pin->getPinType() != PinType::kInput || dynamic_cast<InputPin*>(pin.get())->optional())
            continue;
        auto input_pin = dynamic_cast<InputPin*>(pin.get());
        auto produce_info = input_pin->produceInfo();
        if(produce_info.isNull()){
            spdlog::warn("No producer bind at input pin:{}", kv.first);
            return false;
        }
        const auto& output_spec = produce_info.pin->dataSpec();
        const auto& input_spec = input_pin->dataSpec();
        if(!input_spec.compatibleWith(output_spec)){
            spdlog::warn("Producer pin's data spec is not compatible with consumer pin's:{}", kv.first);
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
        tool_pin_dict_[key] = pin;
    }
}

AGV_NAMESPACE_END
