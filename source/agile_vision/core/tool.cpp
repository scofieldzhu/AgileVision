/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: tool.cpp  
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

#include "tool.h"
#include "procedure.h"
#include "relationship_network.h"
#include "data_spec.h"
#include "input_pin.h"
#include "output_pin.h"
#include "spdlog/spdlog.h"

AGV_NAMESPACE_BEGIN

Tool::Tool(const std::string& iid)
    :iid_(iid)
{
}

Tool::~Tool()
{
}

bool Tool::requestOutputData()
{
    return false;
}

bool Tool::setInputConnection(const PinKey& consume_key, Tool* producer, const PinKey& produce_key, unsigned int data_location)
{
    auto procedure = belongedProcedure();
    if(procedure == nullptr){
        SPDLOG_ERROR("No procedure object exists!");
        return false;
    }        
    auto rsn = belongedProcedure()->tool_relationships_;
    if(!rsn->existTool(this))
        rsn->addTool(this);
    if(producer && !rsn->existTool(producer))
        rsn->addTool(producer);
    ToolLinkage tl;
    tl.producer = producer;
    tl.produce_pin_key = produce_key;
    tl.data_location = data_location;
    tl.consumer = this;
    tl.consum_pin_key = consume_key;
    rsn->makeRelationship(tl);
    return true;
}

bool Tool::run()
{
    auto procedure = belongedProcedure();
    if(procedure == nullptr){
        SPDLOG_ERROR("No procedure object exists!");
        return false;
    }
    if(!checkPinDataCompatible()){
        SPDLOG_ERROR("CheckPinDataCompatible failed!");
        return false;
    }
    if(!requestOutputData()){
        SPDLOG_ERROR("RequestOutputData failed!");
        return false;
    }
    return true;
}

const OutputPin* Tool::getOutputPin(const PinKey &key) const
{
    for(const auto& kv : output_pin_dict_){
        if(key == kv.first && !kv.second->deprecated())
            return kv.second.get();
    }
    return nullptr;
}

const InputPin* Tool::getInputPin(const PinKey &key) const
{
    for(const auto& kv : input_pin_dict_){
        if(key == kv.first && !kv.second->deprecated())
            return kv.second.get();
    }
    return nullptr;
}

bool Tool::checkPinDataCompatible() const
{
    auto rsn = belongedProcedure()->tool_relationships_;
    ToolLinkageList produce_linkages;
    rsn->getToolRelationships(this, &produce_linkages, nullptr);
    for(const auto& tl : produce_linkages){
        auto producer = tl.producer;
        auto output_pin = producer->getOutputPin(tl.produce_pin_key);
        auto input_pin = tl.consumer->getInputPin(tl.consum_pin_key);
        if(output_pin == nullptr || input_pin == nullptr){
            SPDLOG_ERROR("Invalid pin with key:{}", tl.produce_pin_key);
            return false;
        }
        if(!output_pin->dataSpec().compatibleWith(input_pin->dataSpec())){
            SPDLOG_ERROR("Output pin:{} data spec is not compatile with input pin:{} data spec!", tl.produce_pin_key, tl.consum_pin_key);
            return false;
        }
        if(input_pin->optional() || input_pin->deprecated()){
            continue;
        }
        // other stuffs:
        //...
    }
    return true;
}

void Tool::addOutputPin(const PinKey& key, OutputPinPtr pin)
{
}

void Tool::addInputPin(const PinKey& key, InputPinPtr pin)
{
}

Procedure* Tool::belongedProcedure()
{
    return nullptr;
}

const Procedure* Tool::belongedProcedure()const
{
    return nullptr;
}

AGV_NAMESPACE_END
