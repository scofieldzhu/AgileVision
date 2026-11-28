/*
 *   AgileVision smoke test: self-contained, non-interactive checks for the
 *   framework's critical serialization and data-buffer invariants.
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

#include <cstdio>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <future>
#include <mutex>
#include <vector>
#include <stdexcept>
#include "agile_vision/geometry/line.hpp"
#include "agile_vision/geometry/circle.hpp"
#include "agile_vision/geometry/vec_proxy.hpp"
#include "agile_vision/geometry/array_x.hpp"
#include "agile_vision/geometry/dict_proxy.hpp"
#include "agile_vision/basic/portable_stream.h"
#include "agile_vision/core/data_spec.h"
#include "agile_vision/core/data_buffer.h"
#include "agile_vision/core/global_variable.h"
#include "agile_vision/core/tool.h"
#include "agile_vision/core/process.h"
#include "agile_vision/core/procedure.h"
#include "agile_vision/core/process_runner.h"
#include "agile_vision/core/tool_factory.h"
#include "agile_vision/core/tool_metadata.h"
#include "agile_vision/core/solution.h"
#include "agile_vision/core/relationship_network.h"
#include "agile_vision/core/produce_info.h"
#include "agile_vision/core/input_pin.h"
#include "agile_vision/core/output_pin.h"

namespace {

    int g_failures = 0;

    void check(bool ok, const char* case_name)
    {
        if(!ok){
            std::printf("[FAIL] %s\n", case_name);
            ++g_failures;
        } else
        {
            std::printf("[ OK ] %s\n", case_name);
        }
    }

    void testLineRoundTrip()
    {
        agv::Line2 src(agv::Point2{1, 2}, agv::Point2{3, 4});
        auto bytes = src.serializeToBytes();
        agv::Line2 dst(agv::Point2{0, 0}, agv::Point2{0, 0});
        auto used = dst.loadBytes(bytes.data(), bytes.size());
        const bool size_ok = used == bytes.size();
        const bool p1_ok = dst.p1()[0] == 1 && dst.p1()[1] == 2;
        const bool p2_ok = dst.p2()[0] == 3 && dst.p2()[1] == 4;
        check(size_ok && p1_ok && p2_ok, "Line2 serialize/load round-trip");
    }

    void testCircleRoundTrip()
    {
        agv::Circle<int> src(agv::ArrayX<int, 2>{5, 6}, 7.5f);
        auto bytes = src.serializeToBytes();
        agv::Circle<int> dst(agv::ArrayX<int, 2>{0, 0}, 0.0f);
        auto used = dst.loadBytes(bytes.data(), bytes.size());
        const bool size_ok = used == bytes.size();
        const bool center_ok = dst.center()[0] == 5 && dst.center()[1] == 6;
        const bool radius_ok = dst.radius() == 7.5f;
        check(size_ok && center_ok && radius_ok, "Circle serialize/load round-trip");
    }

    void testDataSpecRoundTrip()
    {
        agv::core::DataSpec src = agv::core::DataSpec::StaticArray(agv::DataType::kFloat, 3);
        src.subtype = 9;
        auto bytes = src.serializeToBytes();
        agv::core::DataSpec dst;
        auto used = dst.loadBytes(bytes.data(), bytes.size());
        const bool size_ok = used == bytes.size();
        const bool fields_ok = dst.major_type == agv::DataType::kFloat && dst.subtype == 9 && dst.arry_size == 3;
        check(size_ok && fields_ok, "DataSpec loadBytes writes back members");
    }

    void testDataBufferStringBound()
    {
        // 定长为 2 的字符串缓冲写入 5 个元素，必须截断而不越界。
        agv::core::DataBuffer buf(agv::core::DataSpec::StaticArray(agv::DataType::kString, 2));
        agv::AgvMultiString values = {"a", "b", "c", "d", "e"};
        buf.setStringValue(values);
        auto stored = buf.getAllStringValues();
        check(stored.size() == 2, "DataBuffer fixed-size string write is bounded");
    }

    void testMalformedVecInput()
    {
        // 只有 count 字段且声明超大元素数的畸形输入必须被拒绝（返回 0），不得超大分配。
        unsigned int huge_count = 0xFFFFFFFFu;
        std::vector<char> malformed(sizeof(huge_count));
        std::memcpy(malformed.data(), &huge_count, sizeof(huge_count));
        agv::VecProxy<int> proxy;
        auto used = proxy.loadBytes(malformed.data(), malformed.size());
        check(used == 0, "VecProxy rejects malformed oversized count");
    }

    void testGlobalVariableRoundTrip()
    {
        agv::core::GlobalVariable int_var(agv::core::GlobalVariable::DT_INT);
        int_var.setInt(20261125);
        auto int_bytes = int_var.serializeToBytes();
        agv::core::GlobalVariable int_dst(agv::core::GlobalVariable::DT_BOOLEAN);
        auto int_used = int_dst.loadBytes(int_bytes.data(), int_bytes.size());
        const bool int_ok = int_used == int_bytes.size() && int_dst.getIntValue() == 20261125;
        check(int_ok, "GlobalVariable int serialize/load round-trip");

        agv::core::GlobalVariable str_var(agv::core::GlobalVariable::DT_STRING);
        str_var.setString("agile-vision");
        auto str_bytes = str_var.serializeToBytes();
        agv::core::GlobalVariable str_dst(agv::core::GlobalVariable::DT_INT);
        auto str_used = str_dst.loadBytes(str_bytes.data(), str_bytes.size());
        const bool str_ok = str_used == str_bytes.size() && str_dst.getStringValue() == "agile-vision";
        check(str_ok, "GlobalVariable string serialize/load round-trip");

        agv::core::GlobalVariable bool_var(agv::core::GlobalVariable::DT_BOOLEAN);
        bool_var.setBool(true);
        auto bool_bytes = bool_var.serializeToBytes();
        agv::core::GlobalVariable bool_dst(agv::core::GlobalVariable::DT_FLOAT);
        auto bool_used = bool_dst.loadBytes(bool_bytes.data(), bool_bytes.size());
        const bool bool_ok = bool_used == bool_bytes.size() && bool_dst.getBoolValue() == true;
        check(bool_ok, "GlobalVariable bool serialize/load round-trip");

        agv::core::GlobalVariable empty_var(agv::core::GlobalVariable::DT_FLOAT);
        const bool empty_ok = !empty_var.getFloatValue().has_value();
        check(empty_ok, "GlobalVariable getter returns nullopt when value not set");
    }

    void testGlobalVariableMalformed()
    {
        const unsigned int bad_type = 99;
        std::vector<char> bad(sizeof(bad_type));
        std::memcpy(bad.data(), &bad_type, sizeof(bad_type));
        agv::core::GlobalVariable var(agv::core::GlobalVariable::DT_INT);
        check(var.loadBytes(bad.data(), bad.size()) == 0, "GlobalVariable rejects invalid data type");
        check(var.loadBytes(bad.data(), 0) == 0, "GlobalVariable rejects truncated header");
        check(var.loadBytes(nullptr, bad.size()) == 0, "GlobalVariable rejects null buffer");
    }

    void testArrayXNegativeIndex()
    {
        agv::ArrayX<int, 3> arr = {1, 2, 3};
        bool threw = false;
        try
        {
            (void)arr[-1];
        }
        catch(const std::out_of_range&)
        {
            threw = true;
        }
        check(threw, "ArrayX rejects negative index");
    }

    // 可控结果的测试工具：记录执行次数，用于验证失败工具之后的工具仍会执行。
    class TestTool : public agv::core::Tool
    {
    public:
        TestTool(const std::string& iid, bool result)
            : agv::core::Tool(iid),
              result_(result)
        {
        }
        std::string getClsGuid()const override{ return "test-tool"; }
        int runCount()const{ return run_count_; }
        void addTestPin(const agv::PinKey& key, std::shared_ptr<agv::core::ToolPin> pin){ addPin(key, pin); }
        agv::core::OutputPin* outputPin(const agv::PinKey& key){ return getOutputPin(key); }
        agv::core::InputPin* inputPin(const agv::PinKey& key){ return getInputPin(key); }
        agv::core::ProcessManager* ensureChildManager()
        {
            autoCreateProcesManager();
            return getMutableProcessManager();
        }
        void setOrderLog(std::vector<std::string>* log, std::mutex* mutex)
        {
            order_log_ = log;
            order_mutex_ = mutex;
        }
    protected:
        bool requestOutputData()override
        {
            ++run_count_;
            if(order_log_ != nullptr && order_mutex_ != nullptr){
                std::lock_guard lk(*order_mutex_);
                order_log_->push_back(iid());
            }
            return result_;
        }
    private:
        bool result_ = true;
        int run_count_ = 0;
        std::vector<std::string>* order_log_ = nullptr;
        std::mutex* order_mutex_ = nullptr;
    };

    void testProcessRunsAllToolsOnFailure()
    {
        // 首个工具失败不得短路跳过后续工具（原 s = s && t->run() 的行为）。
        auto fail_tool = std::make_shared<TestTool>("fail", false);
        auto ok_tool = std::make_shared<TestTool>("ok", true);
        agv::core::Process proc("p");
        proc.appendTool(fail_tool);
        proc.appendTool(ok_tool);
        const bool run_result = proc.run();
        check(!run_result && fail_tool->runCount() == 1 && ok_tool->runCount() == 1,
              "Process runs all tools even if an earlier one fails");
    }

    void testInterruptFlagAtomic()
    {
        // 置位中断后 run 应跳过全部工具并复位标志，便于下一轮正常执行。
        auto tool = std::make_shared<TestTool>("t", true);
        agv::core::Process proc("p_int");
        proc.appendTool(tool);
        proc.setInterrupt(true);
        check(proc.run() && tool->runCount() == 0, "Interrupt skips remaining tools");
        check(proc.run() && tool->runCount() == 1, "Interrupt flag resets after run");
    }

    void testDataflowCycleDetection()
    {
        // A -> B -> A 的环必须被判出；断开一条边后应无环。
        const auto spec = agv::core::DataSpec::SingleInt();
        auto tool_a = std::make_shared<TestTool>("a", true);
        auto tool_b = std::make_shared<TestTool>("b", true);
        tool_a->addTestPin("out", std::make_shared<agv::core::OutputPin>(spec));
        tool_b->addTestPin("in", std::make_shared<agv::core::InputPin>(spec));
        agv::core::Procedure procedure("cyc");
        procedure.mutableRoot()->appendTool(tool_a);
        procedure.mutableRoot()->appendTool(tool_b);
        // B 的输出回连 B 的输入形成自环，用于验证环检测。
        tool_b->addTestPin("self_out", std::make_shared<agv::core::OutputPin>(spec));
        agv::core::ProduceInfo self_pi;
        self_pi.pin = tool_b->outputPin("self_out");
        check(tool_b->setPinConnection("in", self_pi), "Self loop connection established");
        check(procedure.checkDataflowCycle(), "Procedure detects dataflow cycle");
        tool_b->setPinConnection("in", agv::core::ProduceInfo{});
        check(!procedure.checkDataflowCycle(), "Procedure passes acyclic dataflow");
    }

    void testTopologicalOrder()
    {
        // 构造 A -> B -> C 链，但按 C、A、B 的顺序插入工具，验证拓扑序恢复依赖顺序。
        const auto spec = agv::core::DataSpec::SingleInt();
        auto tool_a = std::make_shared<TestTool>("topo_a", true);
        auto tool_b = std::make_shared<TestTool>("topo_b", true);
        auto tool_c = std::make_shared<TestTool>("topo_c", true);
        // A 作为链头只需输出引脚；B、C 各需输入引脚。避免未绑定的必选输入导致兼容校验失败。
        tool_a->addTestPin("out", std::make_shared<agv::core::OutputPin>(spec));
        tool_b->addTestPin("out", std::make_shared<agv::core::OutputPin>(spec));
        tool_b->addTestPin("in", std::make_shared<agv::core::InputPin>(spec));
        tool_c->addTestPin("in", std::make_shared<agv::core::InputPin>(spec));
        agv::core::ProduceInfo a_to_b;
        a_to_b.pin = tool_a->outputPin("out");
        check(tool_b->setPinConnection("in", a_to_b), "A->B connection for topo test");
        agv::core::ProduceInfo b_to_c;
        b_to_c.pin = tool_b->outputPin("out");
        check(tool_c->setPinConnection("in", b_to_c), "B->C connection for topo test");

        agv::core::Process proc("topo_root");
        proc.appendTool(tool_c);
        proc.appendTool(tool_a);
        proc.appendTool(tool_b);
        agv::core::RelationshipNetwork network;
        network.buildFrom(&proc, false);
        check(!network.checkCycle(), "Topological graph is acyclic");
        std::vector<agv::core::ConstToolPtr> order;
        const bool sorted_ok = network.topologicalSort(order);
        const bool size_ok = order.size() == 3;
        int pos_a = -1, pos_b = -1, pos_c = -1;
        for(size_t i = 0; i < order.size(); ++i){
            if(order[i]->iid() == "topo_a"){
                pos_a = (int)i;
            }
            if(order[i]->iid() == "topo_b"){
                pos_b = (int)i;
            }
            if(order[i]->iid() == "topo_c"){
                pos_c = (int)i;
            }
        }
        check(sorted_ok && size_ok && pos_a < pos_b && pos_b < pos_c,
              "Topological sort restores producer-before-consumer order");
        check(proc.runInTopologicalOrder(), "Process runs tools in topological order");
    }

    void testTopologicalSortRejectsCycle()
    {
        const auto spec = agv::core::DataSpec::SingleInt();
        auto tool_a = std::make_shared<TestTool>("cyc_a", true);
        auto tool_b = std::make_shared<TestTool>("cyc_b", true);
        for(auto* tool : {tool_a.get(), tool_b.get()}){
            tool->addTestPin("out", std::make_shared<agv::core::OutputPin>(spec));
            tool->addTestPin("in", std::make_shared<agv::core::InputPin>(spec));
        }
        agv::core::ProduceInfo a_to_b;
        a_to_b.pin = tool_a->outputPin("out");
        tool_b->setPinConnection("in", a_to_b);
        agv::core::ProduceInfo b_to_a;
        b_to_a.pin = tool_b->outputPin("out");
        tool_a->setPinConnection("in", b_to_a);
        agv::core::Process proc("cyc_root");
        proc.appendTool(tool_a);
        proc.appendTool(tool_b);
        agv::core::RelationshipNetwork network;
        network.buildFrom(&proc, false);
        std::vector<agv::core::ConstToolPtr> order;
        check(!network.topologicalSort(order), "Topological sort rejects cyclic graph");
        check(!proc.runInTopologicalOrder(), "Process refuses topological run on cycle");
        check(!proc.runInParallelLayers(), "Process refuses parallel run on cycle");
    }

    void testProcessRunnerSyncAndFind()
    {
        agv::core::Engine engine(2);
        agv::core::Process host("runner_host");
        host.runContext().engine = &engine;
        agv::core::Process child("runner_child");
        auto tool = std::make_shared<TestTool>("runner_tool", true);
        child.appendTool(tool);

        check(agv::core::ProcessRunner::runSync(&host, &child), "ProcessRunner sync run succeeds");
        check(tool->runCount() == 1, "ProcessRunner sync run executes child tool");
        check(agv::core::ProcessRunner::runAsync(nullptr, &child) == agv::kNullId,
              "ProcessRunner rejects null host");
    }

    void testSyncRunProcessPropagatesFailure()
    {
        agv::core::Engine engine(1);
        agv::core::Process fail_proc("sync_fail");
        fail_proc.appendTool(std::make_shared<TestTool>("sync_fail_tool", false));
        check(!engine.syncRunProcess(&fail_proc), "syncRunProcess propagates tool failure");

        agv::core::Process ok_proc("sync_ok");
        ok_proc.appendTool(std::make_shared<TestTool>("sync_ok_tool", true));
        check(engine.syncRunProcess(&ok_proc), "syncRunProcess reports success");
    }

    void testWorkResultCallbackAndQuery()
    {
        agv::core::Engine engine(2);
        agv::core::Process proc("async_result");
        proc.appendTool(std::make_shared<TestTool>("async_result_tool", true));
        std::promise<agv::core::WorkResult> captured;
        auto captured_future = captured.get_future();
        const auto wid = engine.commit(&proc, [&captured](const agv::core::WorkResult& result){
            captured.set_value(result);
        });
        check(wid != agv::kNullId, "Engine commit returns work id");
        check(engine.wait(wid, std::chrono::seconds(2)) == agv::core::Engine::WaitResult::kOk,
              "Engine wait reports completed work");
        const auto result = captured_future.get();
        check(result.work_id == wid && result.success && !result.cancelled && result.error.empty(),
              "Result callback carries work result");

        agv::core::WorkResult queried;
        check(engine.getResult(wid, &queried) && queried.work_id == wid && queried.success,
              "Completed result is queryable after work table cleanup");
        check(engine.wait(999999u, std::chrono::milliseconds(0)) == agv::core::Engine::WaitResult::kNotFound,
              "Engine wait distinguishes never-submitted work");
    }

    void testSolutionAggregation()
    {
        agv::core::Solution solution(2);
        auto* procedure = solution.createProcedure("agg_proc");
        check(procedure != nullptr, "Solution creates procedure for aggregation");
        procedure->setActiveRun(true);
        procedure->mutableRoot()->appendTool(std::make_shared<TestTool>("agg_tool", true));
        solution.run();
        check(solution.waitAll(std::chrono::seconds(2)), "Solution waitAll completes submitted work");
        agv::core::WorkResult result;
        check(solution.getWorkResult("agg_proc", &result) && result.success,
              "Solution exposes aggregated work result");
        check(!solution.getWorkResult("unknown_proc", &result), "Solution rejects unknown procedure result");
    }

    void testDataBufferSnapshotIsolation()
    {
        agv::core::DataBuffer buf(agv::core::DataSpec::StaticArray(agv::DataType::kInt, 2));
        buf.setIntValue(7, 0);
        const auto old_version = buf.version();
        auto snap = buf.snapshot();
        buf.setIntValue(9, 0);
        check(snap.getIntValue(0).value_or(-1) == 7 && buf.getIntValue(0).value_or(-1) == 9,
              "DataBuffer snapshot stays stable after source write");
        check(snap.version() == old_version && buf.version() > old_version,
              "DataBuffer version advances on write");
    }

    void testParallelLayersRespectDependencies()
    {
        const auto spec = agv::core::DataSpec::SingleInt();
        auto tool_a = std::make_shared<TestTool>("par_a", true);
        auto tool_b = std::make_shared<TestTool>("par_b", true);
        auto tool_c = std::make_shared<TestTool>("par_c", true);
        auto tool_d = std::make_shared<TestTool>("par_d", true);
        // A -> B -> C 构成依赖链；D 无依赖，应与 A 处于同一层。
        tool_a->addTestPin("out", std::make_shared<agv::core::OutputPin>(spec));
        tool_b->addTestPin("out", std::make_shared<agv::core::OutputPin>(spec));
        tool_b->addTestPin("in", std::make_shared<agv::core::InputPin>(spec));
        tool_c->addTestPin("in", std::make_shared<agv::core::InputPin>(spec));
        agv::core::ProduceInfo a_to_b;
        a_to_b.pin = tool_a->outputPin("out");
        tool_b->setPinConnection("in", a_to_b);
        agv::core::ProduceInfo b_to_c;
        b_to_c.pin = tool_b->outputPin("out");
        tool_c->setPinConnection("in", b_to_c);

        std::vector<std::string> order;
        std::mutex order_mutex;
        for(auto* tool : {tool_a.get(), tool_b.get(), tool_c.get(), tool_d.get()}){
            tool->setOrderLog(&order, &order_mutex);
        }
        agv::core::Process proc("par_root");
        // 故意打乱插入顺序，验证顺序由依赖而非配置顺序决定。
        proc.appendTool(tool_c);
        proc.appendTool(tool_d);
        proc.appendTool(tool_b);
        proc.appendTool(tool_a);

        agv::core::RelationshipNetwork network;
        network.buildFrom(&proc, false);
        std::vector<std::vector<agv::core::ToolPtr>> levels;
        check(network.topologicalLevels(levels) && levels.size() == 3,
              "Topological layering groups independent tools");
        check(proc.runInParallelLayers(), "Process runs all tools in parallel layers");
        const auto pos = [&order](const std::string& iid){
            auto it = std::find(order.begin(), order.end(), iid);
            return it == order.end() ? -1 : static_cast<int>(it - order.begin());
        };
        check(pos("par_a") >= 0 && pos("par_a") < pos("par_b") && pos("par_b") < pos("par_c"),
              "Parallel layers respect producer-before-consumer order");
        check(pos("par_d") >= 0, "Independent tool runs in parallel schedule");
    }

    void testParallelLayersPropagateFailure()
    {
        auto tool_a = std::make_shared<TestTool>("pf_a", false);
        auto tool_b = std::make_shared<TestTool>("pf_b", true);
        agv::core::Process proc("pf_root");
        proc.appendTool(tool_a);
        proc.appendTool(tool_b);
        check(!proc.runInParallelLayers() && tool_a->runCount() == 1 && tool_b->runCount() == 1,
              "Parallel layers run all tools and aggregate failure");
    }

    void testTopologySerializationRoundTrip()
    {
        const auto spec = agv::core::DataSpec::SingleInt();
        auto tool_a = std::make_shared<TestTool>("ser_a", true);
        auto tool_b = std::make_shared<TestTool>("ser_b", true);
        tool_a->addTestPin("out", std::make_shared<agv::core::OutputPin>(spec));
        tool_a->addTestPin("count", std::make_shared<agv::core::PropPin>(spec));
        tool_a->getPropPin("count")->mutableDataBuffer().setIntValue(42);
        tool_b->addTestPin("in", std::make_shared<agv::core::InputPin>(spec));
        agv::core::ProduceInfo pi;
        pi.pin = tool_a->outputPin("out");
        check(tool_b->setPinConnection("in", pi), "Pin connection before serialization");

        agv::core::Process src("ser_root");
        src.setAlias("serialized");
        src.appendTool(tool_a);
        src.appendTool(tool_b);
        const auto bytes = src.serializeToBytes();

        agv::core::ToolFactory factory;
        check(factory.registerCreator("test-tool", [](const std::string& iid){
            return std::make_shared<TestTool>(iid, true);
        }), "ToolFactory registers in-process creator");

        agv::core::Process restored("ser_restored");
        check(restored.loadBytes(bytes.data(), bytes.size(), &factory) == bytes.size(),
              "Process loads full topology frame");
        check(restored.sizeOfTools() == 2, "Process rebuilds tools from clsid");

        agv::core::ToolPtr rebuilt_a = nullptr;
        agv::core::ToolPtr rebuilt_b = nullptr;
        for(auto it = restored.begin(); it != restored.end(); ++it){
            if((*it)->iid() == "ser_a"){
                rebuilt_a = it->get();
            }
            if((*it)->iid() == "ser_b"){
                rebuilt_b = it->get();
            }
        }
        check(rebuilt_a != nullptr && rebuilt_b != nullptr, "Rebuilt tools keep their iids");
        check(restored.alias() == "serialized", "Process alias survives round-trip");

        auto* test_b = dynamic_cast<TestTool*>(rebuilt_b);
        const auto* rebound_pin = test_b != nullptr ? test_b->inputPin("in") : nullptr;
        const bool rebound = rebound_pin != nullptr && rebound_pin->produceInfo().pin != nullptr;
        check(rebound, "Input pin rebound to producer after load");
        check(test_b != nullptr && test_b->inputPin("in")->produceInfo().pin->ownerTool()->iid() == "ser_a",
              "Connection points to rebuilt producer");

        const auto* test_a = dynamic_cast<TestTool*>(rebuilt_a);
        check(test_a != nullptr && test_a->getPropPin("count")->dataBuffer().getIntValue().value_or(-1) == 42,
              "Prop pin value survives round-trip");
    }

    void testSolutionTopologyRoundTrip()
    {
        agv::core::Solution src(2);
        auto* procedure = src.createProcedure("sol_proc");
        procedure->setAlias("demo");
        procedure->mutableRoot()->appendTool(std::make_shared<TestTool>("sol_tool", true));
        const auto bytes = src.serializeToBytes();

        agv::core::ToolFactory factory;
        factory.registerCreator("test-tool", [](const std::string& iid){
            return std::make_shared<TestTool>(iid, true);
        });
        agv::core::Solution restored(2);
        check(restored.loadBytes(bytes.data(), bytes.size(), &factory) == bytes.size(),
              "Solution loads topology frame");
        check(restored.numberOfProcedures() == 1, "Solution restores procedure count");
        const auto* loaded_proc = restored.findProcedure("sol_proc");
        check(loaded_proc != nullptr && loaded_proc->alias() == "demo", "Procedure identity survives round-trip");
        check(loaded_proc != nullptr && loaded_proc->root()->sizeOfTools() == 1,
              "Procedure root tool rebuilt");
    }

    void testChildProcessTreeRoundTrip()
    {
        auto parent_tool = std::make_shared<TestTool>("tree_parent", true);
        auto* level1_manager = parent_tool->ensureChildManager();
        check(level1_manager != nullptr, "Tool creates child process manager");
        auto child_process = std::make_shared<agv::core::Process>("tree_child");
        child_process->setAlias("child-alias");
        auto child_tool = std::make_shared<TestTool>("tree_child_tool", true);
        auto* level2_manager = child_tool->ensureChildManager();
        auto grandchild = std::make_shared<agv::core::Process>("tree_grandchild");
        grandchild->appendTool(std::make_shared<TestTool>("tree_grandchild_tool", true));
        level2_manager->appendProcess(grandchild);
        child_process->appendTool(child_tool);
        level1_manager->appendProcess(child_process);
        level1_manager->setActiveProcessIid("tree_child");

        agv::core::Process root("tree_root");
        root.appendTool(parent_tool);
        const auto bytes = root.serializeToBytes();
        check(!bytes.empty(), "Root process with nested tree serializes");

        agv::core::ToolFactory factory;
        factory.registerCreator("test-tool", [](const std::string& iid){
            return std::make_shared<TestTool>(iid, true);
        });
        agv::core::Process restored("tree_restored");
        check(restored.loadBytes(bytes.data(), bytes.size(), &factory) == bytes.size(),
              "Nested tree frame loads");
        check(restored.sizeOfTools() == 1, "Root tool rebuilt");
        const auto* rebuilt_parent = dynamic_cast<TestTool*>(restored.begin()->get());
        check(rebuilt_parent != nullptr && rebuilt_parent->getProcessManager() != nullptr,
              "Parent tool owns rebuilt child manager");
        if(rebuilt_parent == nullptr || rebuilt_parent->getProcessManager() == nullptr){
            return;
        }
        const auto* rebuilt_manager = rebuilt_parent->getProcessManager();
        check(rebuilt_manager->size() == 1, "Child process rebuilt");
        check(rebuilt_manager->activeProcessIid() == "tree_child", "Active child iid survives round-trip");
        if(rebuilt_manager->begin() == rebuilt_manager->end()){
            return;
        }
        const agv::core::Process* rebuilt_child = rebuilt_manager->begin()->get();
        check(rebuilt_child->iid() == "tree_child", "Child process identity survives");
        check(rebuilt_child->alias() == "child-alias", "Child process alias survives");
        check(rebuilt_child->manager() == rebuilt_manager, "Child process manager back-pointer correct");
        check(rebuilt_child->sizeOfTools() == 1, "Child process tool rebuilt");
        if(rebuilt_child->begin() == rebuilt_child->end()){
            return;
        }
        const auto* rebuilt_child_tool = dynamic_cast<TestTool*>(rebuilt_child->begin()->get());
        check(rebuilt_child_tool != nullptr && rebuilt_child_tool->getProcessManager() != nullptr,
              "Child tool owns rebuilt grandchild manager");
        if(rebuilt_child_tool == nullptr || rebuilt_child_tool->getProcessManager() == nullptr){
            return;
        }
        const auto* child_tool_manager = rebuilt_child_tool->getProcessManager();
        check(child_tool_manager->size() == 1, "Grandchild process rebuilt");
        if(child_tool_manager->begin() == child_tool_manager->end()){
            return;
        }
        const agv::core::Process* rebuilt_grandchild = child_tool_manager->begin()->get();
        check(rebuilt_grandchild->iid() == "tree_grandchild", "Grandchild identity survives two-level nesting");
        check(rebuilt_grandchild->sizeOfTools() == 1, "Grandchild tool rebuilt");
    }

    void testChildProcessTreeDepthLimit()
    {
        // 自底向上构造超过深度上限的工具/子过程链，序列化应返回空帧拒绝生成不可加载文件。
        auto leaf = std::make_shared<TestTool>("depth_leaf", true);
        auto current = leaf;
        for(int i = 0; i < agv::core::detail::kMaxTreeDepth + 2; ++i){
            auto wrapper = std::make_shared<TestTool>("depth_" + std::to_string(i), true);
            auto proc = std::make_shared<agv::core::Process>("depth_proc_" + std::to_string(i));
            proc->appendTool(current);
            wrapper->ensureChildManager()->appendProcess(proc);
            current = wrapper;
        }
        agv::core::Process root("depth_root");
        root.appendTool(current);
        check(root.serializeToBytes().empty(), "Over-deep child tree is rejected on serialize");
    }

    void testTopologyFrameRejectsMalformed()
    {
        agv::core::Solution solution(2);
        std::vector<char> truncated(8, 0);
        check(solution.loadBytes(truncated.data(), truncated.size(), nullptr) == 0,
              "Frame rejects truncated header");

        auto bytes = solution.serializeToBytes();
        bytes[4] = 99; // 版本字段低字节
        check(solution.loadBytes(bytes.data(), bytes.size(), nullptr) == 0,
              "Frame rejects unsupported version");
        bytes[4] = 1;
        bytes[0] = 0; // 魔数字段低字节
        check(solution.loadBytes(bytes.data(), bytes.size(), nullptr) == 0,
              "Frame rejects bad magic");
    }

    void testPortableFrameHeaderAndEndian()
    {
        // 已知整数的载荷字节必须按小端排列，帧头携带可识别的魔数与版本。
        agv::ArrayX<int, 2> src = {0x01020304, -2};
        auto bytes = src.serializeToBytes();
        check(bytes.size() == agv::detail::kPortableHeaderSize + 2 * sizeof(int),
              "ArrayX frame uses fixed 12-byte header");
        const auto* p = reinterpret_cast<const unsigned char*>(bytes.data());
        const bool magic_ok = p[0] == 0x41 && p[1] == 0x52 && p[2] == 0x52 && p[3] == 0x58;
        const bool version_ok = p[4] == 1 && p[5] == 0 && p[6] == 0 && p[7] == 0;
        const bool little_ok = p[12] == 0x04 && p[13] == 0x03 && p[14] == 0x02 && p[15] == 0x01;
        check(magic_ok && version_ok && little_ok, "ArrayX frame stores little-endian payload");
    }

    void testPortableFrameRejectsCorruption()
    {
        agv::StringProxy src("agile-vision");
        auto bytes = src.serializeToBytes();

        auto bad_magic = bytes;
        bad_magic[0] = 0;
        agv::StringProxy dst_magic;
        check(dst_magic.loadBytes(bad_magic.data(), bad_magic.size()) == 0, "StringProxy rejects bad magic");

        auto bad_version = bytes;
        bad_version[4] = 9;
        agv::StringProxy dst_version;
        check(dst_version.loadBytes(bad_version.data(), bad_version.size()) == 0, "StringProxy rejects bad version");

        auto with_tail = bytes;
        with_tail.push_back(0);
        agv::StringProxy dst_tail;
        const auto used = dst_tail.loadBytes(with_tail.data(), with_tail.size());
        check(used == bytes.size() && dst_tail.stdStr() == "agile-vision",
              "StringProxy consumes exactly one frame");

        auto truncated = bytes;
        truncated.resize(agv::detail::kPortableHeaderSize - 1);
        agv::StringProxy dst_truncated;
        check(dst_truncated.loadBytes(truncated.data(), truncated.size()) == 0, "StringProxy rejects truncated header");
    }

    void testVecProxyRejectsOversizedCountInFrame()
    {
        agv::ByteVec frame;
        agv::detail::writeU32(frame, agv::detail::kVecProxyMagic);
        agv::detail::writeU32(frame, agv::detail::kPortableStreamVersion);
        agv::detail::writeU32(frame, 4);
        agv::detail::writeU32(frame, 0xFFFFFFFFu);
        agv::VecProxy<int> proxy;
        check(proxy.loadBytes(frame.data(), frame.size()) == 0, "VecProxy rejects oversized count inside valid frame");
    }

    void testDictProxyNestedFrameRoundTrip()
    {
        agv::DictProxy<int, std::string> src;
        src.mutableData()[1] = "one";
        src.mutableData()[2] = "two";
        auto bytes = src.serializeToBytes();
        agv::DictProxy<int, std::string> dst;
        const auto used = dst.loadBytes(bytes.data(), bytes.size());
        const auto& data = dst.data();
        check(used == bytes.size() && data.size() == 2 && data.at(1) == "one" && data.at(2) == "two",
              "DictProxy frame survives nested StringProxy payload");
    }

    void testDataBufferPortablePayload()
    {
        // DataBuffer 载荷布局：[DataSpec 帧][基础段长度][基础段][字节表帧][value_size]。
        agv::core::DataBuffer buf(agv::core::DataSpec::StaticArray(agv::DataType::kInt, 1));
        buf.setIntValue(0x01020304, 0);
        auto bytes = buf.serializeToBytes();
        const auto* p = reinterpret_cast<const unsigned char*>(bytes.data());
        const bool little_ok = p[40] == 0x04 && p[41] == 0x03 && p[42] == 0x02 && p[43] == 0x01;
        agv::core::DataBuffer restored(agv::core::DataSpec::StaticArray(agv::DataType::kInt, 1));
        const auto used = restored.loadBytes(bytes.data(), bytes.size());
        const int value = restored.getIntValue(0).value_or(0);
        check(little_ok && used == bytes.size() && value == 0x01020304,
              "DataBuffer encodes integer payload little-endian");
    }

    void testDataBufferImageMetadataOnly()
    {
        agv::ImageData img{};
        img.type = agv::ImageData::DT_FLOAT;
        img.channel = 3;
        img.bits = 32;
        img.width = 640;
        img.height = 480;
        img.size = 640u * 480u * 3u * 4u;
        int local_pixel = 0;
        img.data = &local_pixel;

        agv::core::DataBuffer buf(agv::core::DataSpec::StaticArray(agv::DataType::kImage, 1));
        buf.setImageValue(img, 0);
        auto bytes = buf.serializeToBytes();
        agv::core::DataBuffer restored(agv::core::DataSpec::StaticArray(agv::DataType::kImage, 1));
        const auto used = restored.loadBytes(bytes.data(), bytes.size());
        const auto loaded = restored.getImageValue(0);
        const bool meta_ok = loaded.has_value() && loaded->type == img.type;
        const bool dims_ok = loaded.has_value() && loaded->channel == 3 && loaded->bits == 32 && loaded->width == 640;
        const bool size_ok = loaded.has_value() && loaded->height == 480 && loaded->size == img.size;
        check(used == bytes.size() && meta_ok && dims_ok && size_ok && loaded->data == nullptr,
              "ImageData frame keeps metadata and drops process-local pointer");
    }

    void testGlobalVariablePortableFrame()
    {
        agv::core::GlobalVariable var(agv::core::GlobalVariable::DT_INT);
        var.setInt(0x01020304);
        auto bytes = var.serializeToBytes();
        const auto* p = reinterpret_cast<const unsigned char*>(bytes.data());
        const bool magic_ok = p[0] == 0x47 && p[1] == 0x4C && p[2] == 0x42 && p[3] == 0x56;
        const bool little_ok = p[16] == 0x04 && p[17] == 0x03 && p[18] == 0x02 && p[19] == 0x01;
        agv::core::GlobalVariable dst(agv::core::GlobalVariable::DT_STRING);
        const auto used = dst.loadBytes(bytes.data(), bytes.size());
        check(magic_ok && little_ok && used == bytes.size() && dst.getIntValue() == 0x01020304,
              "GlobalVariable frame uses little-endian payload");
    }

    void testAbiVersionNegotiation()
    {
        check(agv::core::isToolAbiVersionSupported(agv::core::kToolAbiV1), "Tool ABI accepts legacy v1");
        check(agv::core::isToolAbiVersionSupported(agv::core::kToolAbiCurrent), "Tool ABI accepts current version");
        check(!agv::core::isToolAbiVersionSupported(agv::core::kToolAbiCurrent + 1), "Tool ABI rejects future version");
        check(!agv::core::isToolAbiVersionSupported(0), "Tool ABI rejects version zero");
    }

    void testToolMetadataCopy()
    {
        agv::core::AgvPinMetaC c_pins[2] = {};
        c_pins[0].key = "in";
        c_pins[0].pin_type = static_cast<int32_t>(agv::PinType::kInput);
        c_pins[0].data_type = static_cast<int32_t>(agv::DataType::kFloat);
        c_pins[0].array_size = 3;
        c_pins[0].alias = "source";
        c_pins[0].optional = 1;
        c_pins[1].key = "gain";
        c_pins[1].pin_type = static_cast<int32_t>(agv::PinType::kProp);

        agv::core::AgvToolMetaC c_meta;
        c_meta.abi_version = agv::core::kToolAbiCurrent;
        c_meta.clsid = "meta-tool";
        c_meta.name = "meta tool";
        c_meta.pin_count = 2;
        c_meta.pins = c_pins;
        agv::core::ToolMetadata meta;
        check(agv::core::copyToolMetadata(&c_meta, &meta), "C ABI metadata copies to C++ value");
        check(meta.clsid == "meta-tool" && meta.name == "meta tool", "Metadata preserves identity");
        check(meta.pins.size() == 2, "Metadata preserves pin count");
        const auto& first = meta.pins[0];
        check(first.key == "in" && first.pin_type == agv::PinType::kInput, "Pin key and type survive copy");
        check(first.data_type == agv::DataType::kFloat && first.array_size == 3, "Pin spec survives copy");
        check(first.alias == "source" && first.optional && !first.deprecated, "Pin flags survive copy");
        // 立即拷贝语义：插件可复用 C 缓冲，已拷贝的 C++ 对象不受影响。
        c_pins[0].key = "mutated";
        check(meta.pins[0].key == "in", "Copied metadata is detached from C buffer");

        c_meta.pin_count = agv::core::kToolMetaMaxPins + 1;
        check(!agv::core::copyToolMetadata(&c_meta, &meta), "Metadata rejects oversized pin count");
        c_meta.pin_count = 1;
        c_meta.pins = nullptr;
        check(!agv::core::copyToolMetadata(&c_meta, &meta), "Metadata rejects null pin array");
        c_meta.pins = c_pins;
        c_meta.abi_version = agv::core::kToolAbiCurrent + 1;
        check(!agv::core::copyToolMetadata(&c_meta, &meta), "Metadata rejects unsupported version");
        c_meta.abi_version = agv::core::kToolAbiCurrent;
        c_meta.clsid = nullptr;
        check(!agv::core::copyToolMetadata(&c_meta, &meta), "Metadata rejects missing clsid");
    }

    void testToolFactoryMetadataDiscovery()
    {
        auto tool = std::make_shared<TestTool>("meta-probe", true);
        const auto spec = agv::core::DataSpec::StaticArray(agv::DataType::kFloat, 2);
        tool->addTestPin("in", std::make_shared<agv::core::InputPin>(spec));
        tool->addTestPin("gain", std::make_shared<agv::core::PropPin>(spec));
        const auto reflected = tool->pinMetadata();
        check(reflected.size() == 3, "Tool reflects all pins including Status");
        bool has_input = false;
        bool has_prop = false;
        bool has_output = false;
        for(const auto& pin : reflected){
            has_input = has_input || pin.pin_type == agv::PinType::kInput;
            has_prop = has_prop || pin.pin_type == agv::PinType::kProp;
            has_output = has_output || pin.pin_type == agv::PinType::kOutput;
        }
        check(has_input && has_prop && has_output, "Reflected metadata covers input/prop/output");

        agv::core::ToolFactory factory;
        factory.registerCreator("test-tool", [](const std::string& iid){
            return std::make_shared<TestTool>(iid, true);
        });
        agv::core::ToolMetadata discovered;
        check(factory.getToolMeta("test-tool", &discovered), "Factory discovers metadata via temporary instance");
        check(discovered.clsid == "test-tool" && !discovered.pins.empty(), "Discovered metadata has identity and pins");
        check(factory.getToolAbiVersion("test-tool") == agv::core::kToolAbiCurrent, "Registered tool reports current ABI");
        check(factory.getToolAbiVersion("missing") == 0, "Unknown tool reports zero ABI");

        agv::core::ToolMetadata explicit_meta;
        explicit_meta.name = "explicit name";
        explicit_meta.pins.push_back(reflected.front());
        check(factory.registerMetadata("test-tool", explicit_meta), "Factory accepts explicit metadata");
        agv::core::ToolMetadata queried;
        check(factory.getToolMeta("test-tool", &queried), "Factory returns explicit metadata");
        check(queried.name == "explicit name" && queried.clsid == "test-tool", "Explicit metadata overrides reflection");
        const auto all_metas = factory.getAllToolMetas();
        check(all_metas.size() == 1, "Factory enumerates unique tool metadata");
    }

}

int main()
{
    testLineRoundTrip();
    testCircleRoundTrip();
    testDataSpecRoundTrip();
    testDataBufferStringBound();
    testMalformedVecInput();
    testGlobalVariableRoundTrip();
    testGlobalVariableMalformed();
    testArrayXNegativeIndex();
    testProcessRunsAllToolsOnFailure();
    testInterruptFlagAtomic();
    testDataflowCycleDetection();
    testTopologicalOrder();
    testTopologicalSortRejectsCycle();
    testProcessRunnerSyncAndFind();
    testSyncRunProcessPropagatesFailure();
    testWorkResultCallbackAndQuery();
    testSolutionAggregation();
    testDataBufferSnapshotIsolation();
    testParallelLayersRespectDependencies();
    testParallelLayersPropagateFailure();
    testTopologySerializationRoundTrip();
    testSolutionTopologyRoundTrip();
    testTopologyFrameRejectsMalformed();
    testPortableFrameHeaderAndEndian();
    testPortableFrameRejectsCorruption();
    testVecProxyRejectsOversizedCountInFrame();
    testDictProxyNestedFrameRoundTrip();
    testDataBufferPortablePayload();
    testDataBufferImageMetadataOnly();
    testGlobalVariablePortableFrame();
    testChildProcessTreeRoundTrip();
    testChildProcessTreeDepthLimit();
    testAbiVersionNegotiation();
    testToolMetadataCopy();
    testToolFactoryMetadataDiscovery();
    if(g_failures != 0){
        std::printf("SMOKE TEST FAILED: %d case(s)\n", g_failures);
        return 1;
    }
    std::printf("SMOKE TEST PASSED\n");
    return 0;
}
