/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: solution.h  
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
#ifndef __solution_h__
#define __solution_h__

#include "agile_vision/core/core_base_def.h"
#include "agile_vision/core/core_export.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API Solution 
{
    using ProcedureList = std::vector<std::unique_ptr<Procedure>>;
public:
    using iterator = ProcedureList::iterator;
    using const_iterator = ProcedureList::const_iterator;
    iterator begin(){ return procedure_list_.begin(); }
    const_iterator begin()const{ return procedure_list_.begin(); }
    iterator end(){ return procedure_list_.end(); }
    const_iterator end()const{ return procedure_list_.end(); }
    Procedure* createProcedure(const std::string& iid);
    const Procedure* findProcedure(const std::string& iid)const;
    void removeProcedure(const std::string& iid);
    void removeProcedure(const_iterator pos);
    auto numberOfProcedures()const{ return procedure_list_.size(); }
    Solution& operator=(const Solution&) = delete;
    Solution();
    Solution(const Solution&) = delete;
    ~Solution();

private:
    using ProcedurePtr = std::unique_ptr<Procedure>;
    std::vector<ProcedurePtr> procedure_list_;
};

AGV_NAMESPACE_END

#endif

