# core 模块

框架核心层，命名空间 `agv::core`（公共类型在 `agv` 根），编译为 `agile_vision.core` SHARED 库，PUBLIC 依赖 basic。

## 文件清单（34 个）

| 分类 | 文件 |
| --- | --- |
| 基础定义 | `core_typedef.h`、`core_export.h`、`serialize_policy.h`、`run_context.h`、`consume_info.h`、`produce_info.h`、`tool_linkage.h` |
| 数据层 | `data_spec.h/.cpp`、`data_buffer.h/.cpp`、`global_variable.h/.cpp` |
| Pin 层 | `tool_pin.h/.cpp`、`input_pin.h/.cpp`、`output_pin.h/.cpp`、`prop_pin.h/.cpp` |
| 工具层 | `tool.h/.cpp`、`tool_factory.h/.cpp` |
| 流程层 | `process.h/.cpp`、`procedure.h/.cpp`、`solution.h/.cpp`、`process_manager.h/.cpp` |
| 调度层 | `engine.h/.cpp`、`relationship_network.h/.cpp` |

## 核心类职责速查

| 类 | 关键职责 | 详见 |
| --- | --- | --- |
| `Solution` | 顶层容器，持有 Engine 与多个 Procedure，`run()` 为运行入口 | [领域模型](../02-architecture/domain-model.md) |
| `Procedure` | 持有一个根 Process，决定同步/异步提交 | 同上 |
| `Process` | 持有 Tool 列表，`run()` 按序执行并支持中断 | 同上 |
| `Engine` | 线程池调度，commit/syncRun/wait/cancel/exist | [执行引擎](../02-architecture/execution-and-scheduling.md) |
| `Tool` | 抽象工具基类，Status Pin 状态机，Pin 兼容校验 | [领域模型](../02-architecture/domain-model.md) |
| `ToolFactory` | 动态加载工具 DLL 并创建/销毁实例 | 同上 |
| `ProcessManager` | Tool 的子过程容器，实现递归树 | 同上 |
| `ToolPin` 及派生 | 数据输入/输出/属性端口 | [数据流与引脚](../02-architecture/dataflow-and-pins.md) |
| `DataSpec`/`DataBuffer` | 数据规格与存储 | 同上 |
| `GlobalVariable` | `std::any` 全局变量（DT_BOOLEAN/INT/FLOAT/STRING） | — |
| `RelationshipNetwork` | 工具依赖有向图，支持判环 | [数据流与引脚](../02-architecture/dataflow-and-pins.md) |
| `RunContext` | 运行时上下文下传 | [执行引擎](../02-architecture/execution-and-scheduling.md) |

## 复合结构与工具函数

`core_typedef.h`（`namespace agv` 根）定义：

- `enum class DataType`：`kUnk/kInt/kFloat/kString/kBytes/kImage`。
- `enum class PinType`：`kInput/kOutput/kProp`。
- `using PinKey = std::string_view`、`wkid_t = uint32_t`、`wkid_list`、`kNullId = 0`。
- `isFundamentalType(DataType)`、`getFundamentalTypeSize(DataType)`。
- core 各类的前向声明与指针别名（`ToolSPtr/ToolPtr/ProcessPtr` 等）放在 `agv::core`。

## 值得关注的实现细节

1. **递归组合**：Process → Tool → ProcessManager → Process 形成树，`findChildProcess(iid, recursive)` 递归下钻，`getRootProcess()` 向上回溯。
2. **插件化**：Tool 抽象 + ToolFactory 动态加载，DLL 销毁函数作 shared_ptr 删除器。
3. **弱引用数据流**：Pin 间通过裸指针互连，共享 DataBuffer，低拷贝。
4. **协作式中断**：Engine::cancel 设置 Process::interrupt_，由 Process::run 检查。
5. **编译期可切换序列化**：`SerializePolicy` 当前走二进制协议，PB 后端预留。

## 已发现的问题

- `relationship_network.cpp` 中 `if(consume_linkages)` 分支曾误写 `produce_linkages->push_back`，已修正。
- `Process::setManager` 仅 `friend ProcessManager` 可访问，反向指针维护需谨慎。
- 全局变量的序列化为空实现；Pin 连接关系未持久化。详见 [序列化体系](../02-architecture/serialization.md) 与 [已知问题](../06-reference/known-issues.md)。
