/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: procedure.h  
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

#ifndef __procedure_h__
#define __procedure_h__

#include "agile_vision/core/core_export.h"
#include "agile_vision/core/process.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API Procedure
{
public:
    AgvBytes serializeToBytes()const;
    size_t loadBytes(ConsAgvBytePtr buffer, size_t size);
    ProcessPtr mutableRoot(){ return root_.get(); }
    ConstProcessPtr root()const{ return root_.get(); }
    void setAlias(const AgvString& str);
    const auto& alias()const{ return alias_; }
    const auto& iid()const{ return iid_; }
    void run();
    auto& runContext(){ return run_context_; }
    const auto& runContext()const{ return run_context_; }
    void setActiveRun(bool s);
    bool activeRun()const{ return active_run_; }
    Procedure(const std::string& iid);
    ~Procedure();

private:
    bool active_run_ = true;
    RunContext run_context_;
    std::string iid_;
    AgvString alias_{"unnamed"};
    std::shared_ptr<Process> root_;
};

AGV_NAMESPACE_END

#endif