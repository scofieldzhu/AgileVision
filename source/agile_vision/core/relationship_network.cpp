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

#include <queue>
#include <unordered_map>
#include "relationship_network.h"
#include "tool.h"
#include "process.h"
#include "process_manager.h"

namespace agv::core {

    namespace
    {
        void collectTools(ConstProcessPtr proc, bool recursive, std::vector<ConstToolPtr>& out)
        {
            if(proc == nullptr){
                return;
            }
            for(const auto& tool : *proc){
                if(tool == nullptr){
                    continue;
                }
                out.push_back(tool.get());
                if(!recursive){
                    continue;
                }
                const auto* manager = tool->getProcessManager();
                if(manager == nullptr){
                    continue;
                }
                for(const auto& child : *manager){
                    collectTools(child.get(), true, out);
                }
            }
        }
    }

    void RelationshipNetwork::buildFrom(ConstProcessPtr root)
    {
        buildFrom(root, true);
    }

    void RelationshipNetwork::buildFrom(ConstProcessPtr root, bool recursive)
    {
        dg_.clear();
        vertex_tools_.clear();
        std::vector<ConstToolPtr> tools;
        collectTools(root, recursive, tools);
        for(const auto* tool : tools){
            addTool(tool);
        }
        for(const auto* tool : tools){
            for(const auto& linkage : tool->collectPinLinkages()){
                makeRelationship(linkage);
            }
        }
    }

    bool RelationshipNetwork::topologicalSort(std::vector<ConstToolPtr>& out_order) const
    {
        out_order.clear();
        if(vertex_tools_.empty()){
            return true;
        }
        std::unordered_map<std::string, size_t> in_degree;
        for(const auto* tool : vertex_tools_){
            in_degree[tool->iid()] = 0;
        }
        for(const auto* tool : vertex_tools_){
            ToolLinkageList consume_links;
            getToolRelationships(tool, nullptr, &consume_links);
            const auto it = in_degree.find(tool->iid());
            if(it != in_degree.end()){
                it->second = consume_links.size();
            }
        }
        std::queue<std::string> ready;
        for(const auto* tool : vertex_tools_){
            if(in_degree[tool->iid()] == 0){
                ready.push(tool->iid());
            }
        }
        while(!ready.empty()){
            const auto cur_key = ready.front();
            ready.pop();
            VertexData vd;
            dg_.getVertexData(cur_key, vd);
            if(vd.t != nullptr){
                out_order.push_back(vd.t);
            }
            DGType::ArcInfoList out_arcs, in_arcs;
            dg_.getVertexArcs(cur_key, out_arcs, in_arcs);
            for(const auto& arc : out_arcs){
                auto degree_it = in_degree.find(arc.target_vertex);
                if(degree_it == in_degree.end()){
                    continue;
                }
                --degree_it->second;
                if(degree_it->second == 0){
                    ready.push(arc.target_vertex);
                }
            }
        }
        return out_order.size() == vertex_tools_.size();
    }

    bool RelationshipNetwork::topologicalLevels(std::vector<std::vector<ToolPtr>>& out_levels) const
    {
        out_levels.clear();
        if(vertex_tools_.empty()){
            return true;
        }
        std::unordered_map<std::string, size_t> in_degree;
        for(const auto* tool : vertex_tools_){
            in_degree[tool->iid()] = 0;
        }
        for(const auto* tool : vertex_tools_){
            ToolLinkageList consume_links;
            getToolRelationships(tool, nullptr, &consume_links);
            auto it = in_degree.find(tool->iid());
            if(it != in_degree.end()){
                it->second = consume_links.size();
            }
        }
        std::vector<std::string> ready;
        for(const auto* tool : vertex_tools_){
            if(in_degree[tool->iid()] == 0){
                ready.push_back(tool->iid());
            }
        }
        size_t visited = 0;
        while(!ready.empty()){
            std::vector<ToolPtr> level;
            level.reserve(ready.size());
            std::vector<std::string> next;
            for(const auto& cur_key : ready){
                VertexData vd;
                dg_.getVertexData(cur_key, vd);
                if(vd.t != nullptr){
                    // 图是派生只读视图，执行时需要可调用的工具指针。
                    level.push_back(const_cast<ToolPtr>(vd.t));
                }
                DGType::ArcInfoList out_arcs, in_arcs;
                dg_.getVertexArcs(cur_key, out_arcs, in_arcs);
                for(const auto& arc : out_arcs){
                    auto degree_it = in_degree.find(arc.target_vertex);
                    if(degree_it == in_degree.end()){
                        continue;
                    }
                    --degree_it->second;
                    if(degree_it->second == 0){
                        next.push_back(arc.target_vertex);
                    }
                }
            }
            visited += level.size();
            out_levels.push_back(std::move(level));
            ready.swap(next);
        }
        return visited == vertex_tools_.size();
    }

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

    void RelationshipNetwork::addTool(ConstToolPtr t)
    {
        if(t == nullptr || existTool(t)){
            return;
        }
        VertexData vd;
        vd.t = t;
        dg_.addVertex(t->iid(), vd);
        vertex_tools_.push_back(t);
    }

    bool RelationshipNetwork::existTool(ConstToolPtr t) const
    {
        return dg_.existVertex(t->iid());
    }

    void RelationshipNetwork::makeRelationship(const ToolLinkage& linkage)
    {
        if(!linkage.isValid()){
            return;
        }
        ArcData ad;
        ad.produce_pin_key = linkage.produce_pin_key;
        ad.data_location = linkage.data_location;
        ad.consume_pin_key = linkage.consume_pin_key;
        dg_.addArc(linkage.producer->iid(), linkage.consumer->iid(), ad);
    }

    void RelationshipNetwork::breakRelationship()
    {
        dg_.clear();
        vertex_tools_.clear();
    }

    bool RelationshipNetwork::getToolRelationships(ConstToolPtr t, ToolLinkageList* produce_linkages, ToolLinkageList* consume_linkages) const
    {
        if(t == nullptr || !dg_.existVertex(t->iid())){
            return false;
        }
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
                consume_linkages->push_back(std::move(tl));
            }
        }
        return true;
    }

}
