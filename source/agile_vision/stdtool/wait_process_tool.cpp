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

AGV_NAMESPACE_BEGIN

WaitProcessTool::WaitProcessTool(const std::string& iid)
    :Tool(iid)
{
    auto prop_process_iid_list = std::make_shared<PropPin>(DataSpec::DynamicArray(DataType::kString));
    addPin(PK_P_ProcessIIDList, prop_process_iid_list);
}

WaitProcessTool::~WaitProcessTool()
{
}

bool WaitProcessTool::requestOutputData()
{
    //joinedProcess()->root
    return false;
}

std::string WaitProcessTool::getClsGuid() const
{
    return "70eec671-9f3c-4b15-ad7a-c38b44c4c7da";
}

AGV_NAMESPACE_END


