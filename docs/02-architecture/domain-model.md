# 领域模型

## 3.1 模型总览

框架的核心领域概念自上而下为：**方案（Solution）→ 流程（Procedure）→ 过程（Process）→ 工具（Tool）→ 引脚（ToolPin）**。其中 Process 与 Tool 通过 ProcessManager 形成可递归的树结构，Tool 之间通过 Pin 形成数据流图。

| 概念 | 类名 | 命名空间 | 语义 |
| --- | --- | --- | --- |
| 方案 | `Solution` | `agv::core` | 顶层容器与运行入口，持有一个 Engine 与多个 Procedure |
| 流程 | `Procedure` | `agv::core` | 一个可执行流程单元，持有一个根 Process |
| 过程 | `Process` | `agv::core` | 基本执行单元，持有多个 Tool，可嵌套子 Process |
| 工具 | `Tool` | `agv::core` | 抽象算法工具，由插件 DLL 实现，持有 Pin 字典与子过程管理器 |
| 引脚 | `ToolPin` / `InputPin` / `OutputPin` / `PropPin` | `agv::core` | 工具的数据输入、输出与属性端口 |
| 数据规格 | `DataSpec` | `agv::core` | 描述 Pin 的主类型、子类型、数组形态 |
| 数据缓冲 | `DataBuffer` | `agv::core` | 按 DataSpec 存储实际数据，Pin 间共享的数据载体 |
| 引擎 | `Engine` | `agv::core` | 基于线程池的同步/异步调度器 |
| 子过程容器 | `ProcessManager` | `agv::core` | Tool 持有的子 Process 列表，实现树形嵌套 |
| 连接描述 | `ProduceInfo` / `ConsumeInfo` / `ToolLinkage` | `agv::core` | Pin 连接关系的正向/反向引用与高层三元组 |
| 依赖图 | `RelationshipNetwork` | `agv::core` | 基于有向图的工具依赖关系，支持环检测 |
| 运行上下文 | `RunContext` | `agv::core` | 在各层间下传引擎、工作 ID、异步标志等运行时信息 |
| 全局变量 | `GlobalVariable` | `agv::core` | 基于 `std::any` 的类型安全全局变量容器 |

## 3.2 所有权与组合关系

```
Solution  ──unique_ptr──> Engine
         └──vector<unique_ptr>──> Procedure*
                                   └──shared_ptr──> Process(root)
                                                      ├──vector<shared_ptr>──> Tool*
                                                      │                         ├──map<PinKey, shared_ptr>──> ToolPin*
                                                      │                         └──unique_ptr──> ProcessManager
                                                      │                                            └──vector<shared_ptr>──> Process*（递归）
                                                      └──ProcessManager*（非拥有，指回父管理器）
```

关键所有权约定：

- `Solution` 独占 `Engine` 与所有 `Procedure`（`unique_ptr`），禁止拷贝。
- `Procedure` 以 `shared_ptr` 持有根 `Process`，根的 `iid` 与 Procedure 相同。
- `Process` 以 `shared_ptr` 持有 `Tool` 列表；`Tool` 以 `shared_ptr` 持有各 Pin 与懒创建的 `ProcessManager`。
- `ProcessManager` 的 `owner_` 与 `Process` 的 `manager_`、Tool 的 `joined_process_` 均为**非拥有反向指针**。

## 3.3 Solution（方案）

定义于 `core/solution.h/.cpp`。

| 项 | 内容 |
| --- | --- |
| 关键成员 | `unique_ptr<Engine> engine_`、`RunContext run_context_`、`vector<ProcedureUPtr> procedure_list_` |
| 构造 | `explicit Solution(size_t max_run_thread_number)`，构造时创建 Engine 并设定线程池大小 |
| 流程管理 | `createProcedure(iid)`（重复 iid 返回 `nullptr` 并告警）、`findProcedure`、`removeProcedure`、`removeAll`、`numberOfProcedures` |
| 执行 | `run()`：为每个 `activeRun()` 的 Procedure 注入 `engine` 与 `async_run`，然后调用其 `run()` |
| 持久化 | `serializeToBytes()` / `loadBytes()`，用 `VecProxy<Procedure*, true>` 引用流程列表 |

## 3.4 Procedure（流程）

定义于 `core/procedure.h/.cpp`。

| 项 | 内容 |
| --- | --- |
| 关键成员 | `shared_ptr<Process> root_`、`RunContext run_context_`、`string iid_`、`AgvString alias_`、`bool active_run_ = true` |
| 根过程 | 构造时创建，iid 与 Procedure 相同；`mutableRoot()` / `root()` |
| 执行 | `run()`：异步模式 `engine->commit(root_.get(), finish_cb)` 并记录 work_id；同步模式 `engine->syncRunProcess(root_.get())` |
| 持久化 | `serializeToBytes/loadBytes`：组合 iid、alias 与 root 的字节流 |

## 3.5 Process（过程）

定义于 `core/process.h/.cpp`。

| 项 | 内容 |
| --- | --- |
| 关键成员 | `vector<ToolSPtr> tools_`、`string iid_`、`AgvString alias_`、`RunContext run_context_`、`Timestamp timestamp_`、`bool interrupt_`、`mutex run_mutex_`、`ProcessManager* manager_` |
| 工具管理 | `appendTool` / `insertTool` / `removeTool` / `findTool` / `existsTool`；增删时同步 Tool 的 `joined_process_` |
| 执行 | `run()`：`run_mutex_` 串行化，置 `is_running=true`，按序调用 `Tool::run()`，以 `s = s && t->run()` 聚合结果；每轮检查 `interrupt_` 支持协作式中断 |
| 层级寻址 | `findChildProcess`（递归）、`getRootProcess` 沿 `manager_->owner()->joinedProcess()` 回溯根 |
| 持久化 | 组合 iid、alias 与各 Tool 字节流 |

## 3.6 Tool（工具）

定义于 `core/tool.h/.cpp`，抽象基类。

| 项 | 内容 |
| --- | --- |
| 内建引脚 | 构造时自动添加 `SingleInt()` 的 OutputPin，键为 `Tool::kStatusPinKey = "Status"`；取值 0=成功、1=Pin 兼容检查失败、2=`requestOutputData` 失败 |
| 纯虚接口 | `getClsGuid()`（类 GUID）、`requestOutputData()`（派生类实现算法并产出数据） |
| 虚函数 | `serializeToBytes/loadBytes`、`clone()`（基类默认返回 `nullptr`） |
| 公共接口 | `run()`、`setPinConnection(consume_key, ProduceInfo)`、`getOutputPin/getInputPin/getPropPin/getToolPin`、`setName/name/iid`、`joinedProcess` |
| 保护接口 | `addPin`、`autoCreateProcesManager`、`checkPinDataCompatible` |
| 关键成员 | `string iid_`、`AgvString name_`、`ToolPinDict tool_pin_dict_`、`unique_ptr<ProcessManager> child_process_manager_`、`ProcessPtr joined_process_` |

`run()` 流程：Status=0 → `checkPinDataCompatible()`（失败置 1）→ `requestOutputData()`（失败置 2）→ 返回成功。

## 3.7 Pin 体系

| 类型 | getPinType | 关键成员 | 角色 |
| --- | --- | --- | --- |
| `ToolPin`（抽象） | 纯虚 | `DataSpec ds_`、`AgvString alias_`、`bool deprecated_` | 引脚基类，提供规格与别名访问 |
| `InputPin` | `kInput` | `bool optional_`、`ProduceInfo produce_info_` | 数据输入，通过 ProduceInfo 引用上游 OutputPin |
| `OutputPin` | `kOutput` | `DataBuffer data_buffer_`、`vector<ConsumeInfo> consume_info_list_` | 数据输出，是数据的实际存储点 |
| `PropPin` | `kProp` | `DataBuffer data_buffer_`、`ProduceInfo produce_info_` | 属性引脚，兼具缓冲与上游引用；唯一实现序列化的 Pin |

## 3.8 数据规格与数据缓冲

**DataSpec**（`struct`）：`DataType major_type`（`kUnk/kInt/kFloat/kString/kBytes/kImage`）、`unsigned int subtype`、`unsigned int arry_size`（0 表示动态长度）。静态工厂 `Single* / StaticArray / DynamicArray`；`compatibleWith()` 要求三个字段**完全相等**。

**DataBuffer**：按 DataSpec 存储实际数据。
- `AgvBytes fundamental_bytes_`：连续存储 `kInt/kFloat/kImage`。
- `AgvMultiBytes bytes_table_`：逐元素存储 `kString/kBytes`。
- 类型化 API：`setIntValue/getIntValue/getIntPointer`、`setFloatValue/...`、`setImageValue/...`、`setStringValue/...`、`setBytesValue/...`、`resize`（仅动态数组）、`erase`、`valueSize`。
- 所有读写先校验类型与索引，失败记 `AGV_LOG_ERROR` 并返回空/`nullopt`。

注意区分：`agv::DataType`（Pin 数据主类型）与 `GlobalVariable::DataType`（`DT_BOOLEAN/DT_INT/DT_FLOAT/DT_STRING`）是两个不同枚举。

## 3.9 连接与上下文结构

| 结构 | 内容 |
| --- | --- |
| `ProduceInfo` | `OutputPin* pin` + `uint32_t data_location`（输出数组元素下标），提供 `operator bool`、`isNull`、`operator==` |
| `ConsumeInfo` | `ToolPin* pin`，提供 `bool`/`==` 语义 |
| `ToolLinkage` | 高层连接三元组：`producer`、`produce_pin_key`、`data_location`、`consumer`、`consume_pin_key`；`isValid()` 要求字段齐全 |
| `RunContext` | `is_running`、`run_result`、`occur_time`、`elapsed_time`、`thread_priority`、`cpu_id`、`Engine* engine`、`async_run`、`wkid_t work_id` |
| `VertexData` / `ArcData` | `RelationshipNetwork` 的有向图顶点/弧数据，弧携带 produce/consume pin key 与 data_location |

## 3.10 工厂与插件生命周期

`ToolFactory`（`core/tool_factory.h/.cpp`）通过 `DynamicLibraryLoader` 加载工具 DLL：

1. `loadToolFile(path)`：`load()` 后解析 `GetToolClsID`、`CreateToolInstance`、`DestroyToolInstance` 三个符号，齐备则以类 ID 登记到 `loader_dict_`。
2. `createTool(clsid, iid)`：按 clsid 找到 DllInfo，调用创建函数生成 `Tool*`，用 `shared_ptr<Tool>(instance, destroy_func)` 包装——**将 DLL 的销毁函数作为删除器**，保证跨模块正确释放。
3. `unloadAll()`（析构时自动调用）：卸载所有 DLL 并清空字典。

## 3.11 持久化的边界

当前持久化覆盖 `Solution → Procedure → Process → Tool → PropPin → DataBuffer → DataSpec` 链路。`Tool::loadBytes` 只恢复名称与 PropPin 数据；工具实例仍需由 DLL 工厂按 clsid 创建。**Pin 连接关系与 Input/Output Pin 不参与持久化**，全局变量序列化为空实现。详见 [序列化体系](serialization.md)。
