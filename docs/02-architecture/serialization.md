# 序列化体系

## 6.1 后端选择

`core/serialize_policy.h` 通过编译期宏选择序列化后端：

- `SERIALIZER_RATEL`（当前生效）：引入 `agile_vision/geometry/geometry.h`，定义 `using SerializeDict = agv::DictProxy<string, string>`。
- `SERIALIZER_PB`（预留）：分支为空，尚未实现。

所有 core 类的序列化函数体都包在 `#ifdef SERIALIZER_RATEL` 中。注意：这里的 `RATEL` 只是历史宏名，源码依赖已完全内嵌到本仓库，与外部 Ratel 库无关。

## 6.2 统一签名约定

| 函数 | 签名 | 语义 |
| --- | --- | --- |
| 序列化 | `ByteVec serializeToBytes() const` | 返回完整字节流 |
| 反序列化 | `size_t loadBytes(ConsAgvBytePtr buffer, size_t size)` | 返回**已消费字节数**，失败返回 0 |

`loadBytes` 返回消费字节数是整套协议可组合的关键：外层按顺序调用各成员的 `loadBytes` 并用返回值推进缓冲区指针。

## 6.3 自定义二进制协议（geometry Proxy 系列）

协议特点：轻量、无自描述、按声明顺序拼接、原生内存布局。

| 类型 | 编码规则 |
| --- | --- |
| `StringProxy` | `[uint32 长度][UTF-8 字节]` |
| `ArrayX<T,N>` | 定长 N×sizeof(T) 的 `memcpy` 原始字节，无长度前缀、无字节序转换 |
| `ElementProxy<T>` | sizeof(T) 原始字节（要求 T 为算术类型且 ≥4 字节） |
| `VecProxy<E>` | `[uint32 元素数][元素字节流依次拼接]`；算术元素直接 memcpy，复杂元素逐个 `serializeToBytes` |
| `DictProxy<K,V>` | `[uint32 数量][uint32 key 段字节数][key 字节段][value 字节段]`，key/value 各用 VecProxy 编码 |
| `ProxyCombine<A,B,Ref>` | A、B 字节流顺序拼接；`Ref=true` 时持成员引用，用于直接读写宿主字段 |

设计核心是**存储与序列化职责分离**：每个 Proxy 模板带 `bool Ref` 参数，在“拥有数据（false）”与“引用现有数据（true）”两种特化间切换。`Ref=true` 特化把实现委托给静态 `SerializeToBytes/LoadBytes`，让实体类在序列化时把成员“喂”给临时代理对象。约束由 C++20 concept（`is_serializable.hpp`）表达。

多态反序列化：`VecProxy<E*>` 依赖 `DefaultCreator<E*>::Create()` 特化创建元素。`solution.cpp` 在 `namespace agv` 特化了 `DefaultCreator<Procedure*>`，使其 `new Procedure("xxx")` 后再调 `loadBytes` 回填。

## 6.4 各核心类的持久化内容

| 类 | 序列化内容 |
| --- | --- |
| `Solution` | `VecProxy<Procedure*, true>` 引用 `procedure_list_` 的裸指针数组 |
| `Procedure` | `ProxyCombine<StringProxy(iid), StringProxy(alias), false>` 再拼 `ProxyCombine<CombineA, Process, true>` 引用 root |
| `Process` | `StringProxy(iid)` + `StringProxy(alias)`，随后逐个追加 Tool 字节流 |
| `Tool` | `StringProxy(iid)` + `StringProxy(name)` + `DictProxy<string, VecProxy<byte>>`，**只序列化 kProp 类型 Pin**（name → PropPin 字节） |
| `PropPin` | 直接转发 `data_buffer_.serializeToBytes()` |
| `DataBuffer` | DataSpec 字节 + `VecProxy<byte>`(fundamental_bytes_) + `VecProxy<VecProxy<byte>>`(bytes_table_) + `uint32 value_size` |
| `DataSpec` | `ArrayX<unsigned int, 3>` 打包 `{major_type, subtype, arry_size}`（定长 12 字节） |
| `GlobalVariable` | 空实现（尚不能持久化） |

## 6.5 持久化边界与缺口

当前覆盖链路：`Solution → Procedure → Process → Tool → PropPin → DataBuffer → DataSpec`。

未覆盖 / 需注意：

- `ToolPin` 序列化实现整段被注释；`InputPin` / `OutputPin` 无独立序列化 → **Pin 连接关系未持久化**。
- `GlobalVariable::serializeToBytes/loadBytes` 为空实现。
- `Tool::loadBytes` 只恢复 name 与 PropPin 数据，Tool 实例仍需按 clsid 由 `ToolFactory` 创建。
- `DataSpec::loadBytes` 仅调用 `ArrayX` 的 loadBytes 并返回消耗字节数，未把结果写回自身字段（见 [已知问题](../06-reference/known-issues.md)）。
- 协议采用平台原生内存布局（无字节序转换、无版本号、无类型标签），跨字节序/ABI 需额外处理。

## 6.6 Protobuf 规划（package 模块）

`package/` 下已有 7 个 `proto3` 消息契约，包名 `agile_vision`，依赖链 `solution → procedure → process → tool → tool_pin → {common, data_buffer} → common`：

| 文件 | 定义 |
| --- | --- |
| `common.proto` | `pbDataType`、`pbDataSpec`、`pbIntList`、`pbImage` |
| `data_buffer.proto` | `pbDataBuffer`：ds + 五种 repeated 字段（fl/il/bl/sl/ml） |
| `procedure.proto` | `pbProcedure`：iid、active_run、alias、root |
| `process.proto` | `pbProcess`：iid、alias、repeated pbTool |
| `solution.proto` | `pbSolution`：repeated pbProcedure |
| `tool.proto` | `pbTool`：clsid、iid、name、三个 pin map、process_iid_list |
| `tool_pin.proto` | `pbToolPin`、`pbProduceInfo`、`pbPropPin`、`pbOutputPin`、`pbInputPin` |

状态：`.proto` 仅作为数据契约存在，CMake 未接入 `protobuf_generate()`，`AGV_REQUIRE_PROTOBUF` 默认 OFF 且探测为 QUIET。接入时需在 package 模块生成 C++ 代码并实现与领域对象的转换层。详见 [package 模块](../03-modules/package.md) 与 [扩展指南](../05-development/extension-guide.md)。
