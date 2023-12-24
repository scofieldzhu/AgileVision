/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: string_process_tool.cpp  
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

#include "string_process_tool.h"
#include "agile_vision/core/input_pin.h"
#include "agile_vision/core/output_pin.h"
#include "agile_vision/core/process.h"
#include "agile_vision/core/process_manager.h"
#include "spdlog/spdlog.h"

AGV_NAMESPACE_BEGIN

StringProcessTool::StringProcessTool(const std::string& iid)
    :Tool(iid)
{
    child_process_manager_ = std::make_shared<ProcessManager>();
    auto new_input_pin = std::make_shared<InputPin>(DataSpec::SingleString());
    addPin("ReferenceString", new_input_pin);
}

StringProcessTool::~StringProcessTool()
{
}

void StringProcessTool::setProcessTriggerString(const std::string& process_iid, const AgvString& trigger_str)
{
    trigger_string_dict_[process_iid] = trigger_str;
}

AgvString StringProcessTool::getProcessTriggerString(const std::string& process_iid) const
{
    auto it = trigger_string_dict_.find(process_iid);
    if(it != trigger_string_dict_.end())
        return (*it).second;
    return "";
}

std::string StringProcessTool::getClsGuid() const
{
    return "6A571A8E-4C5B-4500-A5E7-6CB506DB1BDF";
}

bool StringProcessTool::requestOutputData()
{
    auto referenced_string_pin = getInputPin("ReferenceString")->produceInfo().pin;
    AgvString cur_trigger_string = referenced_string_pin->dataBuffer().getStringValue();
    for(const auto& kv : trigger_string_dict_){
        if(kv.second == cur_trigger_string){
            const std::string process_iid =  kv.first;
            auto process = child_process_manager_->findProcess(process_iid);
            if(process == nullptr){
                SPDLOG_ERROR("No process object exists with iid:{}", process_iid);
                return false;
            }
            process->run(); //sync run;
            return true;
        }
    }
    SPDLOG_ERROR("No trigger string matched with input string:{}", cur_trigger_string);
    return false;
}

AGV_NAMESPACE_END

