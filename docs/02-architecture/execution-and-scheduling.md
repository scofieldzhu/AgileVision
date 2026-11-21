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
                           ├─ 构造 Work{process, callback, promise, future}
                           ├─ 插入 running_work_table_
                           └─ 投递 RunThreadTask 到 ThreadPool
                                ├─ process->runContext().engine = this
                                ├─ process->run()
                                ├─ callback(wkid) → prm.set_value(wkid)
                                └─ 从 running_work_table_ 移除该 Work
```

## 4.2 Engine 接口

定义于 `core/engine.h/.cpp`。

| 接口 | 说明 |
| --- | --- |
| `wkid_t commit(ProcessPtr p, FinishCallback cb)` | 异步提交过程执行，分配工作 ID，投递线程池任务，立即返回 |
| `bool syncRunProcess(ProcessPtr p)` | 同步直接调用 `p->run()`，返回执行结果 |
| `wkid_t get(ProcessPtr p) const` | 查询某过程当前的工作 ID，未找到返回 `kNullId` |
| `bool exist(wkid_t w) const` | 判断工作是否仍在运行表中 |
| `void cancel(wkid_t w, bool wait_infinite)` | 协作式取消：设置目标 Process 的 `interrupt_` 标志 |
| `WaitResult wait(wkid_t w, chrono::milliseconds timeout)` | 等待工作完成，返回 `kOk / kTimeout / kUnk` |
| `using FinishCallback = std::function<void(wkid_t)>` | 完成回调签名 |

关键成员：`ThreadPool thread_pool_`、`map<wkid_t, WorkSPtr> running_work_table_` + 互斥量、`IdSeed<wkid_t> work_id_seed_{0}`。`friend struct RunThreadTask` 允许任务访问私有表。

## 4.3 工作 ID 与生命周期

- `wkid_t = uint32_t`，空值 `kNullId = 0`，由 `IdSeed<wkid_t>`（basic 模块的 header-only 无符号整型 ID 生成器）顺序分配，达到最大值时回绕到初始值。
- `Work` 内持 `std::promise<wkid_t>` 与 `std::future<wkid_t>`，供 `wait()` 阻塞等待完成信号。
- `exist()` 通过查询 `running_work_table_` 判断工作是否仍在运行。
- 工作完成后 `RunThreadTask` 持锁触发 callback、置 promise 值并从表中移除。

## 4.4 协作式中断

`Engine::cancel(w, wait_infinite)` 不强制终止线程，而是设置目标 `Process` 的 `interrupt_ = true`。`Process::run()` 在每个 Tool 执行间隙检查该标志，为 `true` 则跳出循环并复位。这是一种安全的协作式取消，避免跨线程强杀带来的资源不一致。

## 4.5 同步与异步模式

| 模式 | 触发 | 特征 |
| --- | --- | --- |
| 同步 | `Procedure::run()` 检查 `run_context_.async_run == false` | 直接调用 `Engine::syncRunProcess`，阻塞至执行完成 |
| 异步 | `run_context_.async_run == true`（Solution::run 默认注入） | `Engine::commit` 投递线程池，返回 `wkid`，可通过 `wait/cancel/exist` 管理 |

`RunContext` 是运行时信息下传的载体：Solution 填入 `engine` 与 `async_run`，Procedure 据此决定提交方式，`RunThreadTask` 再回填 `engine` 给 Process。`test_solution.cpp` 演示了异步场景下把 `work_id` 写回过程上下文的做法。

## 4.6 线程池

`basic/thread_pool.h/.cpp` 的 `agv::ThreadPool` 是固定 worker 数的任务队列：

- 构造 `explicit ThreadPool(size_t number)` 启动 number 个 worker。
- `enqueue(task_func&&)` 把 `std::function<void()>` 入队并 `cv_.notify_one()`。
- worker 循环 `cv_.wait`，条件为“停止 或 有任务”，取队首执行。
- 析构置 `stop_ = true`、`notify_all()` 并 `join()` 全部线程。

## 4.7 工具执行的状态机

每个 `Tool::run()` 内部：

1. Status Pin 置 0（成功）。
2. `checkPinDataCompatible()`：遍历 Pin 字典，跳过非 Input 与 optional Pin；必选 InputPin 必须已绑定 producer，且 `DataSpec::compatibleWith()` 校验通过。失败则 Status=1 并返回 false。
3. `requestOutputData()`：派生类实现具体算法，经 InputPin 读取上游数据、写 OutputPin。失败则 Status=2 并返回 false。
4. 返回 true。

`Process::run()` 用 `s = s && t->run()` 聚合所有工具结果，任一工具失败则最终返回 false，但已执行的工具不会被回滚。

## 4.8 并发要点

- `Process::run_mutex_` 保证同一 Process 不会被并发执行。
- `Engine::running_work_table_mutex_` 保护工作表的增删查。
- `IdSeed` 非线程安全，仅在 Engine 内部顺序分配 ID。
- `Timestamp`（basic）基于 `std::atomic_uint64_t`，用于跨线程时间戳/序号。
- 跨线程 ID/工作号统一使用 `wkid_t` 与原子计数，符合编码规范 §13。
