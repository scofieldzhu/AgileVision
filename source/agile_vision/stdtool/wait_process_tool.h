/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: wait_process_tool.h  
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

#ifndef __wait_process_tool_h__
#define __wait_process_tool_h__

#include "agile_vision/core/tool.h"
#include "agile_vision/stdtool/stdtool_export.h"

AGV_NAMESPACE_BEGIN

/// @brief Block current thread to wait target asynchronous process execution finish!
class AGV_STDTOOL_API WaitProcessTool : public Tool 
{
public:
    static constexpr PinKey PK_P_ProcessIIDList = "ProcessIIDList";
    static constexpr PinKey PK_P_MaxWaitTimeSeconds = "MaxWaitTimeSeconds";
    std::string getClsGuid() const override;
    WaitProcessTool(const std::string& iid);
    ~WaitProcessTool();

private:
    bool requestOutputData() override;

};

AGV_NAMESPACE_END

#endif