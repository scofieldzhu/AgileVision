# 序列化体系

## 6.1 后端选择

后端由 CMake 选项 `AGV_SERIALIZER_BACKEND`（`proxy` / `protobuf`）通过 `agv_build_config` 定义对应宏，`core/serialize_policy.h` 据此选择：

- `AGV_SERIALIZER_PROXY`（默认）：引入 `agile_vision/geometry/geometry.h`，定义 `using SerializeDict = agv::DictProxy<string, string>`；无外部依赖。
- `AGV_SERIALIZER_PB`（预留未实现）：选择后 CMake 会告警并回退，当前无实际序列化实现。

`serialize_policy.h` 在两者都未定义时默认回退到 `AGV_SERIALIZER_PROXY`。旧宏名 `SERIALIZER_RATEL` 已统一改名，与外部 Ratel 库无关。

**拓扑帧例外**：`Solution/Procedure/Process/Tool` 的整链序列化已改用 `core/serialization_frame.h` 定义的小端版本化帧，不再包在后端宏里；`DataSpec/DataBuffer/PropPin` 等底层改用 `basic/portable_stream.h` 的小端版本化 Proxy 帧，作为不透明字段嵌入拓扑帧。这样后端宏未定义时也不会出现函数无返回值的隐患。

## 6.2 统一签名约定

| 函数 | 签名 | 语义 |
| --- | --- | --- |
| 序列化 | `ByteVec serializeToBytes() const` | 返回完整字节流 |
| 反序列化 | `size_t loadBytes(ConsAgvBytePtr buffer, size_t size)` | 返回**已消费字节数**，失败返回 0 |

`loadBytes` 返回消费字节数是整套协议可组合的关键：外层按顺序调用各成员的 `loadBytes` 并用返回值推进缓冲区指针。

## 6.3 自定义二进制协议（geometry Proxy 系列）

协议特点：每类自带可识别帧头与固定小端编码，按声明顺序拼接，可由 `loadBytes` 返回值组合。

统一帧头为 12 字节 `[uint32 magic][uint32 version=1][uint32 payload_size]`，字段均为小端；`payload_size` 只约束本帧，外层可继续拼接后续帧。魔数按 ASCII 小端排列，便于直接检查字节流前缀。

| 类型 | 魔数 | 载荷编码 |
| --- | --- | --- |
| `StringProxy` | `STRP` | `[uint32 字节数][UTF-8 字节]` |
| `ArrayX<T,N>` | `ARRX` | N 个标量，按 `sizeof(T)` 小端写出 |
| `ElementProxy<T>` | `ELMP` | 单个标量小端（T 为算术类型且 ≥4 字节） |
| `VecProxy<E>` | `VECP` | `[uint32 元素数][元素字节流依次拼接]`；算术元素用小端标量，复杂元素逐个 `serializeToBytes` |
| `DictProxy<K,V>` | `DCTP` | `[uint32 数量][key 段帧][value 段帧]`，key/value 各用 VecProxy 编码 |
| `Rect<T>` | `RECT` | 左上点帧 + 尺寸帧 + `float` 小端角度 |
| `ProxyCombine<A,B,Ref>` | 无 | A、B 字节流顺序拼接；`Ref=true` 时持成员引用，用于直接读写宿主字段 |

编码工具集中在 `basic/portable_stream.h`（`agv::detail::writeScalar/readScalar/writeFramedBytes/readFramedBytes`），所有 Proxy 复用同一套判定逻辑；`loadBytes` 只消费自身帧并返回消费字节数，因此可安全用于组合帧。

设计核心是**存储与序列化职责分离**：每个 Proxy 模板带 `bool Ref` 参数，在“拥有数据（false）”与“引用现有数据（true）”两种特化间切换。`Ref=true` 特化把实现委托给静态 `SerializeToBytes/LoadBytes`，让实体类在序列化时把成员“喂”给临时代理对象。约束由 C++20 concept（`is_serializable.hpp`）表达。

多态反序列化：`VecProxy<E*>` 依赖 `DefaultCreator<E*>::Create()` 特化创建元素。`solution.cpp` 在 `namespace agv` 特化了 `DefaultCreator<Procedure*>`，使其 `new Procedure("xxx")` 后再调 `loadBytes` 回填。

## 6.4 各核心类的持久化内容

| 类 | 序列化内容 |
| --- | --- |
| `Solution` | 拓扑帧：流程数量 + 每个流程帧（带长度前缀） |
| `Procedure` | 拓扑帧：iid、alias、`active_run` + 根 Process 帧 |
| `Process` | 拓扑帧：iid、alias + 每个工具帧（带长度前缀） |
| `Tool` | 拓扑帧：clsid、iid、name + 全部 Pin 规格（类型/DataSpec/alias/deprecated/optional/PropPin 值）+ 输入引脚绑定表 + 可选子过程段（递归 Process 帧） |
| `PropPin` | 直接转发 `data_buffer_.serializeToBytes()` |
| `DataBuffer` | `DATB` 帧：`[DataSpec 帧][uint32 基础段字节数][基础段][字节表帧][uint32 value_size]`；int/float 按小端逐元素编码，ImageData 仅存元数据（不写入进程内 `data` 指针） |
| `DataSpec` | `ARRX` 帧打包 `{major_type, subtype, arry_size}` 三个 `uint32` 小端值 |
| `GlobalVariable` | `GLBV` 帧：`[uint32 类型][值]`；bool 1 字节、int/float 小端标量、string 内嵌 `STRP` 帧 |

### 6.4.1 拓扑帧格式

所有拓扑帧共享 12 字节头：`[uint32 magic][uint32 version][uint32 payload_size]`，统一小端编码，`version=1`。各层魔数不同（AGVS/AGVR/AGVP/AGVT），避免误判。

- 字符串与字节段：`[uint32 长度][字节]`，长度超过 64 MiB 或超出剩余缓冲区即判为畸形。
- 工具帧的 Pin 表：`[key][type:uint8][spec 字节段][alias][deprecated:uint8][optional:uint8][prop 字节段]`；Prop 字节段只有 `kProp` 才有内容。
- 工具帧的绑定表：`[consume_key][producer_iid][produce_pin_key][data_location:uint32]`，由所属 Process 在全部工具重建后统一调用 `setPinConnection` 重连。
- 工具帧尾部的可选子过程段：`[uint32 magic=PCGM][uint8 has_children]`，有子节点时续写 `[active_iid][uint32 child_count][Process 帧...]`。旧帧读完绑定表后无剩余字节即跳过此段，保持向前兼容。
- 递归防护：`TopologyContext` 限制子过程树深度 16 层、节点总数 256；超限在序列化与反序列化两端均拒绝，避免畸形帧导致栈溢出或超大分配。
- 帧总大小上限 256 MiB；版本/魔数不匹配返回 0。

反序列化入口为 `loadBytes(buffer, size, ToolFactory*)`，旧双参数重载保留（factory 为空时只能复用已存在的同 iid 工具）。`ToolFactory::registerCreator()` 支持注册进程内工具类型，DLL 加载方式不变。

## 6.5 持久化边界与缺口

当前覆盖链路：`Solution → Procedure → Process → Tool → PropPin → DataBuffer → DataSpec`，并递归覆盖 `Tool → ProcessManager → Process` 子过程树（任意层级）。

未覆盖 / 需注意：

- 拓扑帧与底层 Proxy 帧均不支持旧版无魔数字节流的自动迁移，格式切换需重新保存方案。
- 底层 Proxy 帧与拓扑帧各自带魔数/版本；嵌套时由外层不透明携带，解析按各自帧头独立完成。
- `ImageData` 的 `data` 指针是进程内地址，序列化只保留 type/channel/bits/width/height/size，加载后指针置空。
- `ToolFactory` 已支持 v2 元数据协商与 Pin 清单反射（见 [扩展指南](../05-development/extension-guide.md)）。

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
