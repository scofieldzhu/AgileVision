/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: relationship_network.h  
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

#ifndef __relationship_network_h__
#define __relationship_network_h__

#include "ratel/basic/directed_graph.hpp"
#include "agile_vision/core/tool_linkage.h"
#include "agile_vision/core/core_export.h"

AGV_NAMESPACE_BEGIN

class AGV_CORE_API RelationshipNetwork
{
public:
    bool checkCycle()const;
    void addTool(ConstToolPtr t);
    bool existTool(ConstToolPtr t)const;
    void makeRelationship(const ToolLinkage& linkage);
    void breakRelationship();
    bool getToolRelationships(ConstToolPtr t, ToolLinkageList* produce_linkages, ToolLinkageList* consume_linkages)const;
    RelationshipNetwork();
    ~RelationshipNetwork();

private:
    struct VertexData{
        ConstToolPtr t = nullptr;
    };
    struct ArcData{
        AgvString produce_pin_key;
        unsigned int data_location = 0;
        AgvString consume_pin_key;
    };
    using DGType = ratel::DirectedGraph<VertexData, ArcData, std::string>;
    DGType dg_;
};

AGV_NAMESPACE_END

#endif

