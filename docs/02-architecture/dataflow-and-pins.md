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

构造时若为定长数组，按 `arry_size * getFundamentalTypeSize` 预分配 `fundamental_bytes_`，或 `resize` `bytes_table_`。

类型化 API（均先校验类型与索引，失败记 `LOG_ERROR` 并返回空/`nullopt`）：

- 整型：`setIntValue / getIntValue / getIntPointer`
- 浮点：`setFloatValue / getFloatValue / getFloatPointer`
- 图像：`setImageValue / getImageValue / getImagePointer`
- 字符串：`setStringValue / getStringValue / getAllStringValues`
- 字节：`setBytesValue / getBytesValue / getAllBytesValues`
- 其他：`resize`（仅动态数组）、`erase`、`valueSize`、`updateModifyTime / lastModifyTime`

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
- 必选 InputPin 必须 `produceInfo()` 非空，否则 `LOG_WARN` 并返回 false。
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

注意：当前 `Tool::setPinConnection` 的启用版本直接在 Pin 层建立引用，`tool.cpp` 中基于 `RelationshipNetwork` 的旧校验代码已被注释，图网络与 Pin 连接尚未在运行时联动（框架正从“图驱动”向“Pin 直接引用驱动”迁移）。

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
