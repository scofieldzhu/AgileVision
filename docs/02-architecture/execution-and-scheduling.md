# 执行引擎与调度

## 4.1 运行主链路

```
Solution::run()
  └─ 对每个 activeRun() 的 Procedure：
       ├─ runContext().async_run = true
       └─ runContext().engine = engine_.get()
            └─ Procedure::run()
                 ├─ 异步：engine->commit(root_.get(), finish_cb) → 记录 work_id
                 └─ 同步：engine->syncRunProcess(root_.get())
                      └─ Engine::commit：
                           ├─ work_id_seed_.next() 分配 wkid
                           ├─ 构造 Work{process, callback(s), promise<WorkResult>, future<WorkResult>}
                           ├─ 插入 running_work_table_
                           └─ 投递 RunThreadTask 到 ThreadPool
                                ├─ process->runContext().engine = this
                                ├─ process->run() → success
                                ├─ 组装 WorkResult{work_id, success, cancelled, error, elapsed}
                                ├─ prm.set_value(result) → result_callback(result)
                                ├─ 登记 completed_results_[wkid]
                                └─ 从 running_work_table_ 移除该 Work
```

## 4.2 Engine 接口

定义于 `core/engine.h/.cpp`。

| 接口 | 说明 |
| --- | --- |
| `wkid_t commit(ProcessPtr p, ResultCallback cb)` | 异步提交过程执行，分配工作 ID，投递线程池任务，立即返回；回调收到完整结果 |
| `wkid_t commit(ProcessPtr p, FinishCallback cb)` | 旧签名兼容重载，内部适配为结果回调后仅回传 wkid |
| `bool syncRunProcess(ProcessPtr p)` | 同步直接调用 `p->run()`，返回真实执行结果（不再恒为 true） |
| `wkid_t get(ProcessPtr p) const` | 查询某过程当前的工作 ID，未找到返回 `kNullId` |
| `bool exist(wkid_t w) const` | 判断工作是否仍在运行表中 |
| `void cancel(wkid_t w, bool wait_infinite)` | 协作式取消：置 `interrupt_` 并标记 Work 的 `cancelled` |
| `WaitResult wait(wkid_t w, chrono::milliseconds timeout)` | 等待工作完成，返回 `kOk / kTimeout / kNotFound / kUnk` |
| `bool getResult(wkid_t w, WorkResult* out) const` | 查询已完成工作的结果；不消费结果 |
| `using FinishCallback = std::function<void(wkid_t)>` | 旧完成回调签名，保留兼容 |
| `using ResultCallback = std::function<void(const WorkResult&)>` | 结果回调签名，承载成功/失败与耗时 |

关键成员：`ThreadPool thread_pool_`、`map<wkid_t, WorkSPtr> running_work_table_`、`map<wkid_t, WorkResult> completed_results_`（上限 4096 条，超出淘汰最早记录）+ 同一把互斥量、`IdSeed<wkid_t> work_id_seed_{0}`。`friend struct RunThreadTask` 允许任务访问私有表。

### 4.2.1 WorkResult 结果契约

`WorkResult{work_id, success, cancelled, error, elapsed}` 由 `promise<WorkResult>`、`ResultCallback` 与 `getResult()` 三条路径共同承载：

- `success` 取自 `Process::run()` 的返回值；派发异常时为 false；协作取消导致的提前跳出仍按 Process 自身返回值判定（当前为 true）。
- `cancelled` 由 `Engine::cancel()` 写入 Work 的原子标志，任务结束时读取。
- `error` 记录异常文本，无异常时为空。
- `elapsed` 基于 `steady_clock` 统计执行耗时（毫秒）。

`WaitResult::kNotFound` 专门用于区分“从未提交”与“已完成但工作表已清理”：后者可从 `completed_results_` 查到并仍返回 `kOk`。`FinishCallback` 重载保留兼容，没有结果回调时才触发。回调异常会被捕获记录，不影响结果登记与工作表清理。

## 4.3 工作 ID 与生命周期

- `wkid_t = uint32_t`，空值 `kNullId = 0`，由 `IdSeed<wkid_t>`（basic 模块的 header-only 无符号整型 ID 生成器）顺序分配，达到最大值时回绕到初始值。
- `Work` 内持 `std::promise<WorkResult>` 与 `std::future<WorkResult>`，供 `wait()` 阻塞等待完成信号。
- `exist()` 通过查询 `running_work_table_` 判断工作是否仍在运行。
- 工作完成后 `RunThreadTask` 先置 promise 值、再触发回调（避免回调内 `wait()` 等自身），最后持锁登记结果并从运行表移除。

## 4.4 协作式中断

`Engine::cancel(w, wait_infinite)` 不强制终止线程，而是设置目标 `Process` 的 `interrupt_ = true`，同时把对应 `Work` 的 `cancelled` 原子标志置位，使完成回调可区分“正常结束”与“被取消”。`Process::run()` 在每个 Tool 执行间隙检查该标志，为 `true` 则跳出循环并复位。这是一种安全的协作式取消，避免跨线程强杀带来的资源不一致。

`interrupt_` 声明为 `std::atomic_bool`：`cancel()` 由调用方线程写入、`run()` 在工作线程读取，普通 bool 会构成数据竞争（未定义行为）。复位由 `run()` 自身完成，保证下一轮执行不受上一轮取消影响。

## 4.5 同步与异步模式

| 模式 | 触发 | 特征 |
| --- | --- | --- |
| 同步 | `Procedure::run()` 检查 `run_context_.async_run == false` | 直接调用 `Engine::syncRunProcess`，阻塞至执行完成 |
| 异步 | `run_context_.async_run == true`（Solution::run 默认注入） | `Engine::commit` 投递线程池，返回 `wkid`，可通过 `wait/cancel/exist` 管理 |

### 4.5.1 Process 的三种执行入口

| 入口 | 语义 |
| --- | --- |
| `run()` | 按配置顺序串行执行，向后兼容 |
| `runInTopologicalOrder()` | 按派生依赖图的拓扑序串行执行 |
| `runInParallelLayers()` | 同层工具并行、层间屏障等待；分层由 `topologicalLevels()` 给出 |

三者共享"失败不短路 + 原子中断复位"的结果语义；并行入口额外隔离工具异常并保证层间 happens-before。默认入口不变，并行是显式可选能力。

`RunContext` 是运行时信息下传的载体：Solution 填入 `engine` 与 `async_run`，Procedure 据此决定提交方式，`RunThreadTask` 再回填 `engine` 给 Process。`test_solution.cpp` 演示了异步场景下把 `work_id` 写回过程上下文的做法。

`Solution` 提供批量聚合接口：`run(bool async_run)`（默认 `run()` 仍为异步，语义不变）、`waitAll(timeout)`（超时上限对整批生效）、`cancelAll(wait_infinite)`、`getWorkResult(procedure_iid, out)`。这样外部可以等待、取消并读取每个流程的执行结果，而不必直接访问 Engine 私有工作表。`Procedure::run()` 的异步回调改为按值捕获流程别名，避免流程先于回调析构时的悬垂引用。

## 4.6 线程池

`basic/thread_pool.h/.cpp` 的 `agv::ThreadPool` 是固定 worker 数的任务队列：

- 构造 `explicit ThreadPool(size_t number)` 启动 number 个 worker。
- `enqueue(task_func&&)` 把 `std::function<void()>` 入队并 `cv_.notify_one()`。
- worker 循环 `cv_.wait`，条件为“停止 或 有任务”，取队首执行。
- 析构置 `stop_ = true`、`notify_all()` 并 `join()` 全部线程。

## 4.6.1 子过程执行入口 ProcessRunner

`core/process_runner.h/.cpp` 提供子过程执行的统一封装，避免各编排工具重复拼接 engine 调用：

| 接口 | 说明 |
| --- | --- |
| `runSync(host, child)` | 经宿主 `runContext().engine` 同步执行子过程 |
| `runAsync(host, child)` | 异步提交子过程并把 work_id 写回子过程自身的 `runContext` |
| `wait(host, child, timeout)` | 按 work_id 等待；已完成返回 kOk，从未提交返回 kNotFound |
| `findChild(host, iid)` | 沿宿主根进程递归按 iid 查找子过程 |

`stdtool` 的 `StringProcessTool` 与 `WaitProcessTool` 已改用该入口；这同时修复了前者原先 `auto proc_ctx = process->runContext()` 按值拷贝后写 work_id 到副本的缺陷。

## 4.7 工具执行的状态机

每个 `Tool::run()` 内部：

1. Status Pin 置 0（成功）。
2. `checkPinDataCompatible()`：遍历 Pin 字典，跳过非 Input 与 optional Pin；必选 InputPin 必须已绑定 producer，且 `DataSpec::compatibleWith()` 校验通过。失败则 Status=1 并返回 false。
3. `requestOutputData()`：派生类实现具体算法，经 InputPin 读取上游数据、写 OutputPin。失败则 Status=2 并返回 false。
4. 返回 true。

`Process::run()` 按序调用每个 `Tool::run()`，并以显式循环聚合结果：即使某个工具失败，其后的工具仍会继续执行，最终返回 false；失败工具不会被短路跳过。这一语义由冒烟用例 `Process runs all tools even if an earlier one fails` 守护。已执行的工具不会被回滚。

## 4.8 并发要点

- `Process::run_mutex_` 保证同一 Process 不会被并发执行。
- `Engine::running_work_table_mutex_` 保护工作表的增删查。
- `IdSeed` 非线程安全，仅在 Engine 内部顺序分配 ID。
- `Timestamp`（basic）基于 `std::atomic_uint64_t`，用于跨线程时间戳/序号。
- 跨线程 ID/工作号统一使用 `wkid_t` 与原子计数，符合编码规范 §13。
