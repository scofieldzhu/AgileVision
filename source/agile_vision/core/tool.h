/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: tool.h  
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

#ifndef __tool_h__
#define __tool_h__

#include <map>
#include "agile_vision/core/core_base_def.h"
#include "agile_vision/core/core_export.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API Tool
{
public:
    bool setPinConnection(const PinKey& consume_key, Tool* producer, const PinKey& produce_key, unsigned int data_location = 0);
    bool run();
    const OutputPin* getOutputPin(const PinKey& key)const;
    const InputPin* getInputPin(const PinKey& key)const;
    const PropPin* getPropPin(const PinKey& key)const;
    const ToolPin* getToolPin(const PinKey& key)const;
    const std::string& iid()const{ return iid_; }
    Procedure* belongedProcedure();
    const Procedure* belongedProcedure()const;
    void setName(const AgvString& str);
    const auto& name()const{ return name_; }
    Tool(const std::string& iid);
    virtual ~Tool();

protected:
    bool checkPinDataCompatible()const;
    void addPin(const PinKey& key, ToolPinPtr pin);
    virtual bool requestOutputData() = 0;

private:
    const std::string iid_;
    AgvString name_ = "unnamed";
    using ToolPinDict = std::map<PinKey, ToolPinPtr>;
    ToolPinDict tool_pin_dict_;
};

AGV_NAMESPACE_END

#endif