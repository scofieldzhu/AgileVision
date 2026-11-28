# 数据流与引脚连接

## 5.1 数据规格 DataSpec

定义于 `core/data_spec.h/.cpp`（`struct`）。

| 字段 | 类型 | 含义 |
| --- | --- | --- |
| `major_type` | `DataType` | `kUnk / kInt / kFloat / kString / kBytes / kImage` |
| `subtype` | `unsigned int` | 子类型，默认 0 |
| `arry_size` | `unsigned int` | 元素个数，默认 1；**0 表示动态长度** |

静态工厂：`Single(type)`、`SingleInt/SingleFloat/SingleString/SingleBytes/SingleImage`、`StaticArray(type, fixed_size=1)`、`DynamicArray(type)`；判定 `isStaticArray()` / `isDynamicArray()`。

`compatibleWith(other)` 要求 `major_type`、`subtype`、`arry_size` **三者完全相等**才兼容。这是 Pin 连接时的类型校验依据——注意定长数组长度不同也会判为不兼容。

基础类型判定：`isFundamentalType(kInt/kFloat/kImage) == true`，它们用连续字节存储；`kString/kBytes` 走逐元素字节表。`getFundamentalTypeSize()` 分别返回 4/4/`sizeof(ImageData)`。

## 5.2 数据缓冲 DataBuffer

定义于 `core/data_buffer.h/.cpp`，是 Pin 间共享的数据载体。

| 成员 | 用途 |
| --- | --- |
| `DataSpec ds_` | 规格 |
| `AgvBytes fundamental_bytes_` | `kInt/kFloat/kImage` 的连续原始字节 |
| `AgvMultiBytes bytes_table_` | `kString/kBytes` 的逐元素字节 |
| `size_t value_size_` | 有效元素数 |
| `agile_vision_time_t last_modify_time_` | 最后修改时间 |
| `atomic_uint64_t version_` | 数据版本，每次成功写入递增 |
| `mutable shared_mutex io_mutex_` | 保护值语义读写；写独占、读共享 |

构造时若为定长数组，按 `arry_size * getFundamentalTypeSize` 预分配 `fundamental_bytes_`，或 `resize` `bytes_table_`。

**并发契约**：所有返回值的读接口（`getXxxValue`、`getAllXxxValues`、`valueSize`、`dataSpec`、`lastModifyTime`、`serializeToBytes`）持共享锁；所有写入接口持独占锁并在成功后递增版本。`DataBufferSnapshot` 在锁内一次性拷贝底层字节与版本，之后与源解耦，其取值接口全部返回值而非裸指针，适合跨阶段持有。`getXxxPointer`、`getStringValue`、`getAllBytesValues` 返回裸指针/引用，锁无法覆盖其生命周期，**仅可在阶段屏障内使用**（同一时刻只有一个生产者写该缓冲）。

类型化 API（均先校验类型与索引，失败记 `AGV_LOG_ERROR` 并返回空/`nullopt`）：

- 整型：`setIntValue / getIntValue / getIntPointer`
- 浮点：`setFloatValue / getFloatValue / getFloatPointer`
- 图像：`setImageValue / getImageValue / getImagePointer`
- 字符串：`setStringValue / getStringValue / getAllStringValues`
- 字节：`setBytesValue / getBytesValue / getAllBytesValues`
- 其他：`resize`（仅动态数组）、`erase`、`valueSize`、`updateModifyTime / lastModifyTime`
- 版本与快照：`version()`、`snapshot()` → `DataBufferSnapshot`（`version/valueSize/dataSpec/getXxxValue`，全部返回值语义）

## 5.3 三类引脚

| Pin | PinType | 数据缓冲 | 上游引用 | 下游登记 | 序列化 |
| --- | --- | --- | --- | --- | --- |
| `InputPin` | `kInput` | 无 | `ProduceInfo produce_info_` | — | 无 |
| `OutputPin` | `kOutput` | `DataBuffer data_buffer_` | — | `vector<ConsumeInfo>` | 无 |
| `PropPin` | `kProp` | `DataBuffer data_buffer_` | `ProduceInfo produce_info_` | — | 有（转发 DataBuffer） |

- InputPin：`optional_` 标记可选；`bindProduceInfo/unbind` 管理上游绑定。
- OutputPin：`addConsumeInfo`（去重追加）、`removeConsumePin`、`clear`、`mutableDataBuffer/dataBuffer`。
- PropPin：属性端口，既可本地存值也可绑定上游，是唯一参与持久化的 Pin。

## 5.4 连接建立流程

连接入口：`Tool::setPinConnection(const PinKey& consume_key, const ProduceInfo& pi)`。

1. 通过 `getInputPin(consume_key)` 找到目标 InputPin（不存在则报错返回 false）。
2. 若已绑定旧 producer，先从旧 OutputPin 的 `consume_info_list_` 摘除本 Pin。
3. `consume_pin->unbind()`；若 `pi.isNull()` 则完成断开并返回 true。
4. 否则 `bindProduceInfo(pi)` 建立 InputPin → OutputPin 引用。
5. 在 `pi.pin` 侧 `addConsumeInfo` 登记消费者。

数据交换方式：下游工具在 `requestOutputData()` 中经 `getInputPin(key)->produceInfo().pin` **直接读取上游 OutputPin 的 DataBuffer**（共享引用、无拷贝），`ProduceInfo::data_location` 指明取输出数组第几个元素。写入侧由 `OutputPin::mutableDataBuffer()` 提供。

> 待核实：`tool.cpp` 中登记消费者时写的是 `pi.pin->addConsumeInfo({pi.pin})`，按对称语义应为 `{consume_pin}`。详见 [已知问题](../06-reference/known-issues.md)。

## 5.5 类型兼容校验

`Tool::checkPinDataCompatible()` 遍历 `tool_pin_dict_`：

- 跳过非 Input 类型与 `optional()` 的 Pin。
- 必选 InputPin 必须 `produceInfo()` 非空，否则 `AGV_LOG_WARN` 并返回 false。
- 用 `inputSpec.compatibleWith(outputSpec)` 校验规格一致。

## 5.6 关系网络（RelationshipNetwork）

定义于 `core/relationship_network.h/.cpp`，基于 `agv::DirectedGraph<VertexData, ArcData, string>`（basic 模块的 header-only 邻接表有向图）。

| 接口 | 说明 |
| --- | --- |
| `addTool(tool)` / `existTool(tool)` | 增删查顶点 |
| `makeRelationship(linkage)` | 按 ToolLinkage 添加 producer → consumer 的弧，弧上携带 pin key 与 data_location |
| `breakRelationship(linkage)` | 删除弧 |
| `getToolRelationships(tool, produce_links, consume_links)` | 分别取出该工具的全部出弧（产出）与入弧（消费） |
| `checkCycle()` | 转发 `DirectedGraph::checkCycle()`，DFS 判环，用于检测数据流回路 |

当前架构已确立 **Pin 直连为唯一运行时真相源**，`RelationshipNetwork` 降级为从 Pin 连接派生的只读校验视图：

- `Tool::collectPinLinkages()` 从本工具的 InputPin 绑定关系派生 `ToolLinkage` 列表（生产引脚键通过反查生产者字典得到）。
- `RelationshipNetwork::buildFrom(ConstProcessPtr root)` 递归遍历进程树（含子过程的 `ProcessManager`），先登记所有工具顶点，再由各工具的 `collectPinLinkages()` 建立弧。
- `Procedure::checkDataflowCycle()` 基于派生图判环；`Procedure::run()` 在执行前调用它，检测到回路时记录错误并拒绝运行，避免数据流自引用导致的死锁或未定义行为。

图不再单独维护连接状态，仅在执行前即时派生，因此不存在“图与 Pin 两份状态”不一致的风险。旧的基于图网络做兼容校验的代码保持注释。

### 5.8 拓扑调度（派生图的运行时复用）

`RelationshipNetwork::topologicalSort()` 基于 Kahn 算法输出工具依赖顺序（producer 先于 consumer）；有环时返回 false。`buildFrom(root, recursive)` 支持递归开关，`recursive=false` 时只收集单个 Process 的直接工具，用于 Process 级调度。

`Process::runInTopologicalOrder()` 是按依赖顺序执行的新入口：先判环，再按拓扑序调用工具，复用与 `run()` 相同的执行循环（同样具备失败不短路、原子中断语义）。默认 `run()` 仍按工具在 vector 中的配置顺序执行，保持向后兼容。调用方可在需要自动满足依赖顺序时改用拓扑入口。

### 5.8.1 阶段屏障并行执行

`RelationshipNetwork::topologicalLevels()` 基于 Kahn 算法逐层推进，把工具分成若干"互不依赖"的层。`Process::runInParallelLayers()` 对每层用 `std::async` 并发执行全部工具，并等待该层所有 future 完成后才进入下一层：

- **同层无写冲突**：每个 OutputPin 只属于一个工具，同层工具互不依赖，因此不会出现两个工具同时写同一 DataBuffer；多个消费者并发读同一上游缓冲由 DataBuffer 的共享锁保护。
- **层间 happens-before**：future 的完成建立跨阶段同步，下一层读取的一定是上一层写入完成后的数据。
- **失败不短路**：同层所有工具都会执行，异常被隔离并记录，最终聚合失败状态；每层开始前检查原子 `interrupt_`，取消后跳出并复位。
- 有环时先判环再拒绝执行；默认 `run()` 与 `runInTopologicalOrder()` 语义不变，并行是显式可选能力。

限制：工具若通过 Pin 之外的共享状态（全局变量、外部设备）互相通信，框架无法从派生图推断依赖，需由调用方保证同层无此类冲突。裸指针/引用式 DataBuffer API 的生命周期也仅在该阶段屏障模型下得到保证。

## 5.7 数据流示意

```
Tool A                            Tool B
  OutputPin("Result")              InputPin("Source")
   ├─ DataBuffer[0..n]  <──produceInfo.pin──┐
   └─ ConsumeInfo list ─────addConsumeInfo──┘
                          data_location = 元素下标
```

- 上游写：`A.getOutputPin("Result")->mutableDataBuffer().setXxxValue(...)`。
- 下游读：`B.getInputPin("Source")->produceInfo().pin->dataBuffer().getXxxValue(loc)`。
- 断开：`setPinConnection(key, ProduceInfo{})`，会同时清理双向引用。
