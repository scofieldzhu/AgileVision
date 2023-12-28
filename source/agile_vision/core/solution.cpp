/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: solution.cpp  
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

#include "solution.h"
/*
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *
 *   File: solution.cpp
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
#include "solution.h"
#include "procedure.h"
#include "spdlog/spdlog.h"

AGV_NAMESPACE_BEGIN

Solution::Solution()
{
}

Solution::~Solution()
{
}

void Solution::run(Engine *e)
{
    if(e){
        for(auto& p : procedure_list_){
            if(p->activeRun())
                p->run(e);
        }
    }
}

Procedure *Solution::createProcedure(const std::string &iid)
{
    if(findProcedure(iid)){
        SPDLOG_WARN("Procedure with iid:{} already exists!", iid);
        return nullptr;
    }
    auto procedure = std::make_unique<Procedure>(iid);
    procedure_list_.push_back(std::move(procedure));
    return procedure.get();
}

const Procedure* Solution::findProcedure(const std::string& iid) const
{
    auto it = std::find_if(procedure_list_.begin(), procedure_list_.end(), [&iid](auto& it){
        return (*it).iid() == iid;
    });
    return it != procedure_list_.end() ? (*it).get() : nullptr;
}

void Solution::removeProcedure(const std::string &iid)
{
    procedure_list_.erase(
        std::remove_if(procedure_list_.begin(), procedure_list_.end(), [&iid](auto& it){
            return (*it).iid() == iid;
        }),
        procedure_list_.end()
    );
}

void Solution::removeProcedure(const_iterator pos)
{
    procedure_list_.erase(pos);
}

AGV_NAMESPACE_END