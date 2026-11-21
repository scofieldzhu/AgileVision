# stdtool 模块

标准工具集，命名空间 `agv::stdtool`，编译为 `agile_vision.stdtool` SHARED 库，PUBLIC 依赖 basic + core。导出宏 `AGV_STDTOOL_API`。

> 重要：本模块中**仅 2 个工具类有实际实现**，其余 5 个为只有 MIT 许可证头的占位文件（无任何类定义）。不可将占位文件视为可用功能。

## 已实现类

### StringProcessTool（`string_process_tool.h/.cpp`）

继承 `agv::core::Tool`，字符串触发器。

| 项 | 内容 |
| --- | --- |
| GUID | `6A571A8E-4C5B-4500-A5E7-6CB506DB1BDF` |
| 构造 | `autoCreateProcesManager()`；输入 Pin `ReferenceString`（SingleString）；属性 Pin `PK_P_ProcessIIDList`、`PK_P_TriggerStringList`、`PK_P_TriggerModeList`（均为动态数组） |
| 功能 | 读取 `ReferenceString`，与 `TriggerStringList` 逐项匹配；命中后按 `TriggerModeList` 决定以同步（`Engine::syncRunProcess`）或异步（`Engine::commit`，work_id 写回 process 上下文）方式运行 `process_iid_list` 指定流程 |
| 校验 | 三个列表长度一致性校验，失败记日志 |

### WaitProcessTool（`wait_process_tool.h/.cpp`）

继承 `agv::core::Tool`，阻塞等待异步过程结束。

| 项 | 内容 |
| --- | --- |
| GUID | `70eec671-9f3c-4b15-ad7a-c38b44c4c7da` |
| 构造 | 属性 Pin `PK_P_ProcessIIDList`（string 动态数组）、`PK_P_MaxWaitTimeSeconds`（SingleFloat，默认 2.0 秒） |
| 功能 | 校验列表非空且不含自身 iid；对每个 iid 经 `findMutableChildProcess` 定位进程，`Engine::get` 取 work_id（`kNullId` 视为已完成跳过），`Engine::wait(wid, 毫秒超时)` 等待；超时记 `LOG_ERROR` 并返回 false |

## 占位类（未实现）

以下文件仅有 28 行 MIT 许可证头，无类定义，属“已规划、未实现”：

| 文件 | 按命名的预期职责 |
| --- | --- |
| `async_process.h/.cpp` | `AsyncProcess` 异步过程工具 |
| `break_tool.h/.cpp` | `BreakTool` |
| `condition_process_tool.h/.cpp` | `ConditionProcessTool` 条件流程工具 |
| `loop_process_tool.h/.cpp` | `LoopProcessTool` 循环流程工具 |
| `sync_procedure_tool.h/.cpp` | `SyncProcedureTool` 同步流程工具 |

## 与 core 的关系

已实现的工具类继承 `agv::core::Tool`，必须实现两个纯虚函数：

- `getClsGuid()` — 返回类 GUID，供 `ToolFactory` 按 clsid 创建。
- `requestOutputData()` — 实现算法，经 Pin 读写数据。

基类提供的关键接口：`addPin`、`autoCreateProcesManager`、`checkPinDataCompatible`、`getInputPin/getOutputPin/getPropPin`、`getMutableProcessManager`。工具类依赖 core 的 `DataSpec/DataBuffer/InputPin/PropPin/ProcessManager/Process/Engine` 与 basic 的 `LOG_*` 日志。

## 扩展示例

新增一个标准工具的步骤见 [扩展指南](../05-development/extension-guide.md)；`StringProcessTool` 是“属性 Pin 配置 + 子过程调度”的参考范例，`WaitProcessTool` 是“异步工作等待”的参考范例。
