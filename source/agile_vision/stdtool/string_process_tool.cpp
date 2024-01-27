/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: string_process_tool.cpp  
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

#include "string_process_tool.h"
#include "agile_vision/core/process.h"
#include "agile_vision/core/process_manager.h"
#include "agile_vision/core/engine.h"
#include "spdlog/spdlog.h"

AGV_NAMESPACE_BEGIN

StringProcessTool::StringProcessTool(const std::string& iid)
    :Tool(iid)
{
    autoCreateProcesManager();
    auto new_input_pin = std::make_shared<InputPin>(DataSpec::SingleString());
    addPin("ReferenceString", new_input_pin);
    auto prop_process_iid_list = std::make_shared<PropPin>(DataSpec::DynamicArray(DataType::kString));
    addPin(PK_P_ProcessIIDList, prop_process_iid_list);
    auto prop_trigger_string_list = std::make_shared<PropPin>(DataSpec::DynamicArray(DataType::kString));
    addPin(PK_P_TriggerStringList, prop_trigger_string_list);
    auto prop_trigger_mode_list = std::make_shared<PropPin>(DataSpec::DynamicArray(DataType::kInt));
    addPin(PK_P_TriggerModeList, prop_trigger_mode_list);
}

StringProcessTool::~StringProcessTool()
{
}

std::string StringProcessTool::getClsGuid() const
{
    return "6A571A8E-4C5B-4500-A5E7-6CB506DB1BDF";
}

bool StringProcessTool::requestOutputData()
{
    auto referenced_string_pin = getInputPin("ReferenceString")->produceInfo().pin;
    auto process_iid_list_pin = getPropPin(PK_P_ProcessIIDList);
    auto trigger_string_list_pin = getPropPin(PK_P_TriggerStringList);
    auto trigger_mode_list_pin = getPropPin(PK_P_TriggerModeList);
    if(process_iid_list_pin->dataBuffer().valueSize() != trigger_string_list_pin->dataBuffer().valueSize() && 
       trigger_string_list_pin->dataBuffer().valueSize() != trigger_mode_list_pin->dataBuffer().valueSize()){
        spdlog::error("The sizes of process iid list, trigger string list and trigger mode list are not equal with each other:[{},{},{}]!", 
            process_iid_list_pin->dataBuffer().valueSize(), 
            trigger_string_list_pin->dataBuffer().valueSize(),
            trigger_mode_list_pin->dataBuffer().valueSize());
        return false;
    }
    AgvString cur_trigger_string = referenced_string_pin->dataBuffer().getStringValue();
    for(unsigned int i = 0; i < trigger_string_list_pin->dataBuffer().valueSize(); ++i){
        std::string cur_string = trigger_string_list_pin->dataBuffer().getStringValue(i);
        auto trigger_mode = trigger_mode_list_pin->dataBuffer().getIntValue(i).value();
        if(cur_string == cur_trigger_string){
            const std::string process_iid =  process_iid_list_pin->dataBuffer().getStringValue(i);
            auto process = getMutableProcessManager()->findProcess(process_iid, false);
            if(process == nullptr){
                spdlog::error("No process object exists with iid:{}", process_iid);
                return false;
            }
            auto proc_engine = joinedProcess()->runContext().engine;
            if(trigger_mode){ //sync mode;
                proc_engine->syncRunProcess(process); 
            }else{ //async mode
                auto proc_ctx = process->runContext();
                proc_ctx.work_id = proc_engine->commit(process, Engine::finish_callback());
                if(proc_ctx.work_id == null_id){
                    spdlog::error("Commit process:{} work failed!", process->alias());
                    return false;
                }
            }
            return true;
        }
    }
    spdlog::error("No trigger string matched with input string:{}", cur_trigger_string);
    return false;
}

AGV_NAMESPACE_END

