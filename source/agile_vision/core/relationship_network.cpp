/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: relationship_network.cpp  
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

#include "relationship_network.h"
#include "tool.h"

AGV_NAMESPACE_BEGIN

RelationshipNetwork::RelationshipNetwork()
{
}

RelationshipNetwork::~RelationshipNetwork()
{
}

bool RelationshipNetwork::checkCycle() const
{
    return dg_.checkCycle();
}

void RelationshipNetwork::addTool(const Tool *t)
{
    if(t == nullptr)
        return;    
    VertexData vd;
    vd.t = t;
    dg_.addVertex(t->iid(), vd);
}

bool RelationshipNetwork::existTool(const Tool *t) const
{
    return dg_.existVertex(t->iid());
}

void RelationshipNetwork::makeRelationship(const ToolLinkage& linkage)
{
    if(!linkage.isValid())
        return;
    ArcData ad;
    ad.produce_pin_key = linkage.produce_pin_key;
    ad.data_location = linkage.data_location;
    ad.consume_pin_key = linkage.consume_pin_key;
    dg_.addArc(linkage.producer->iid(), linkage.consumer->iid(), ad);
}

bool RelationshipNetwork::getToolRelationships(const Tool* t, ToolLinkageList* produce_linkages, ToolLinkageList* consume_linkages) const
{
    if(t == nullptr || !dg_.existVertex(t->iid()))
        return false;
    DGType::ArcInfoList out_arc_infos, in_arc_infos;
    dg_.getVertexArcs(t->iid(), out_arc_infos, in_arc_infos);
    if(produce_linkages){
        for(const auto& out_arc_info : out_arc_infos){
            ToolLinkage tl;
            tl.producer = t;
            tl.produce_pin_key = out_arc_info.data.produce_pin_key;
            tl.data_location = out_arc_info.data.data_location;
            VertexData vd;
            dg_.getVertexData(out_arc_info.target_vertex, vd);
            tl.consumer = vd.t;
            tl.consume_pin_key = out_arc_info.data.consume_pin_key;
            produce_linkages->push_back(std::move(tl));
        }
    }
    if(consume_linkages){
        for(const auto& in_arc_info : in_arc_infos){
            ToolLinkage tl;
            VertexData vd;
            dg_.getVertexData(in_arc_info.source_vertex, vd);
            tl.producer = vd.t;
            tl.produce_pin_key = in_arc_info.data.produce_pin_key;
            tl.data_location = in_arc_info.data.data_location;
            tl.consumer = t;
            tl.consume_pin_key = in_arc_info.data.consume_pin_key;
            produce_linkages->push_back(std::move(tl));
        }
    }
    return true;
}

AGV_NAMESPACE_END
