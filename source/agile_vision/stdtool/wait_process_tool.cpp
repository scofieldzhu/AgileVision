/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: wait_process_tool.cpp  
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

#include "wait_process_tool.h"
#include "agile_vision/core/process.h"
#include "agile_vision/core/procedure.h"
#include "agile_vision/core/process_runner.h"
#include "agile_vision/basic/log_service.h"

namespace agv::stdtool {
    using namespace agv::core;

    WaitProcessTool::WaitProcessTool(const std::string& iid)
        :Tool(iid)
    {
        auto prop_process_iid_list = std::make_shared<PropPin>(DataSpec::DynamicArray(DataType::kString));
        addPin(PK_P_ProcessIIDList, prop_process_iid_list);
        auto max_wait_time_prop = std::make_shared<PropPin>(DataSpec::SingleFloat());
        max_wait_time_prop->mutableDataBuffer().setFloatValue(2.0);
        addPin(PK_P_MaxWaitTimeSeconds, max_wait_time_prop);
    }

    WaitProcessTool::~WaitProcessTool()
    {
    }

    bool WaitProcessTool::requestOutputData()
    {
        auto iid_str_list = getPropPin(PK_P_ProcessIIDList)->dataBuffer().getAllStringValues();
        if(iid_str_list.empty()){
            AGV_LOG_WARN("No any process iid list bind!");
            return true;
        }
        auto joined_process = joinedProcess();
        if(joined_process == nullptr){
            AGV_LOG_ERROR("This tool doesn't join any process at all!");
            return false;
        }
        if(std::find(iid_str_list.begin(), iid_str_list.end(), joined_process->iid()) != iid_str_list.end()){
            AGV_LOG_ERROR("Process iid list contain self!");
            return false;
        }
        float timeout = getPropPin(PK_P_MaxWaitTimeSeconds)->mutableDataBuffer().getFloatValue().value();
        const auto timeout_ms = std::chrono::milliseconds(static_cast<int>(timeout * 1000));
        for(const auto& iid_str : iid_str_list){
            auto cur_process = ProcessRunner::findChild(joined_process, iid_str);
            if(cur_process == nullptr){
                AGV_LOG_ERROR("Invalid process iid:{}", iid_str);
                return false;
            }
            auto wait_result = ProcessRunner::wait(joined_process, cur_process, timeout_ms);
            if(wait_result != Engine::WaitResult::kOk){
                AGV_LOG_ERROR("Wait process[{}] time out!", iid_str);
                return false;
            }
        }
        return true;
    }

    std::string WaitProcessTool::getClsGuid() const
    {
        return "70eec671-9f3c-4b15-ad7a-c38b44c4c7da";
    }

}


