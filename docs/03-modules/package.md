# package 模块

打包模块，命名空间 `agv`，编译为 `agile_vision.package` SHARED 库，PUBLIC 依赖 basic + core。导出宏 `AGV_PACKAGE_API`。

## 实现状态

> `bytes_writer.h/.cpp`、`compressor.h/.cpp`、`package.h/.cpp` 六个文件**均只有 MIT 许可证头，无任何类定义**，属于设计占位。按命名推断预期职责分别为字节写入器、压缩器、打包器，但不可视为可用接口。

唯一有效的基础设施是 `package_export.h`（定义 `AGV_PACKAGE_API`）。

## Protobuf 消息契约

7 个 `proto3` 文件，包名 `agile_vision`，依赖链：`solution → procedure → process → tool → tool_pin → {common, data_buffer} → common`。这组契约完整描述了领域模型的序列化结构，与 core 的 `Solution/Procedure/Process/Tool/Pin` 一一对应。

| 文件 | 定义的消息 / 枚举 |
| --- | --- |
| `common.proto` | 枚举 `pbDataType`（PB_DT_UNK/INT/FLOAT/STRING/BYTES/IMAGE）；`pbDataSpec`、`pbIntList`、`pbImage` |
| `data_buffer.proto` | `pbDataBuffer`：ds + 五种 repeated 字段（fl:float、il:int32、bl:bytes、sl:string、ml:pbImage） |
| `procedure.proto` | `pbProcedure`：iid、active_run、alias、`pbProcess root` |
| `process.proto` | `pbProcess`：iid、alias、repeated `pbTool tools` |
| `solution.proto` | `pbSolution`：repeated `pbProcedure procedure_list` |
| `tool.proto` | `pbTool`：clsid、iid、name，三个 `map<string, pbPin>`（input/output/prop），`repeated string process_iid_list` |
| `tool_pin.proto` | `pbToolPin`（基类）、`pbProduceInfo`（tool_clsid/tool_iid/pin_key/data_location）、`pbPropPin`、`pbOutputPin`、`pbInputPin` |

## 构建状态

`package/CMakeLists.txt` 是唯一含 Protobuf 探测逻辑的模块：

```cmake
if(AGV_REQUIRE_PROTOBUF)
    find_package(Protobuf REQUIRED)
else()
    find_package(Protobuf QUIET)
endif()
```

- 顶层选项 `AGV_REQUIRE_PROTOBUF` 默认 OFF。
- **尚未接入代码生成**：没有 `protobuf_generate()`、未链接 protobuf 库、`.proto` 未纳入目标。
- `agv_add_module(agile_vision.package)` 无条件调用，`WIN32` 下定义 `AGV_PACKAGE_EXPORT`。

## 演进方向

将 Protobuf 作为第二序列化后端接入 `SerializePolicy` 的 `AGV_SERIALIZER_PB` 分支（由 CMake 选项 `AGV_SERIALIZER_BACKEND=protobuf` 启用），需完成：

1. CMake 接入 `protobuf_generate()` 并链接 protobuf 运行时。
2. 实现领域对象 ↔ pb 消息的双向转换层（注意 `Tool` 实例仍需由 `ToolFactory` 按 clsid 创建）。
3. 处理 Pin 连接关系（`pbProduceInfo`/`pbInputPin`/`pbOutputPin` 已预留字段，当前二进制协议未持久化连接）。
4. 补全 bytes_writer/compressor/package 三个占位类的实现（如需）。

详见 [序列化体系](../02-architecture/serialization.md) 与 [扩展指南](../05-development/extension-guide.md)。
