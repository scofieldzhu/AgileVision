# basic 模块

基础设施层，命名空间 `agv`，编译为 `agile_vision.basic` SHARED 库。全部类型在 `agv` 根空间。

## 文件与职责

| 文件 | 核心类型 | 职责 |
| --- | --- | --- |
| `basic_typedef.h` | `Byte`、`ByteVec`、`AgvBytes`、`AgvString` 等 | 字节/字符串类型底座 |
| `image_data.h` | `ImageData`（`AGV_PACKED`） | 通用图像描述结构 |
| `cross_platform.h` | 平台宏、打包宏 | 平台检测与字节对齐统一收口 |
| `export_macro.h` | `AGV_WIN_DLL_*`、`AGV_EXTERN_C_*` | 基础导出/C 接口宏 |
| `basic_export.h` | `AGV_BASIC_API` | 基础模块 API 宏（含内嵌基础工具） |
| `log_service.h` | `AGV_LOG_*`、`GetLogger()`/`SetLogger()` | 全项目统一日志门面 |
| `string_proxy.h/.cpp` | `StringProxy` | UTF-8 字符串、序列化、UID、数值转换、编码转换 |
| `time_util.h/.cpp` | `TimeTracker`、`Timestamp` | 毫秒级计时器、线程安全原子计数器 |
| `dbg_tracker.h/.cpp` | `DbgTracker`、`_AUTO_FUNC_TRACK_` | RAII 函数进入/离开跟踪 |
| `thread_pool.h/.cpp` | `ThreadPool` | 固定 worker 数任务队列 |
| `id_seed.hpp` | `IdSeed<T>` | header-only 无符号整型 ID 生成器 |
| `win_header.h` | — | 统一 Windows 头入口 |

## 类型体系

`basic_typedef.h` 定义（均在 `namespace agv`）：

- 字节原语：`Byte = char`、`BytePtr`、`ConsBytePtr`、`ByteVec = std::vector<Byte>`。
- 尺寸常量：`kIntSize`、`kUIntSize`、`kFloatSize`。
- 业务别名：`agile_vision_byte`、`ConsAgvBytePtr`、`AgvBytes = ByteVec`、`AgvMultiBytes`、`AgvString = std::string`、`AgvMultiString`、`agile_vision_time_t = uint64_t`。

`ImageData`（`image_data.h`）：`DataType` 枚举（`DT_UNK/UCHAR/USHORT/INT/FLOAT`），成员 `type/channel/bits`（uint8）、`width/height/size`（uint32）、`data`（void*），结构体用 `AGV_PACKED` 修饰。

## 平台与打包宏

`cross_platform.h` 是平台判断的唯一收口：

- 平台：`PLATFORM_WIN`、`PLATFORM_LINUX`、`PLATFORM_UNKNOWN`。
- 打包：`AGV_PACK_PUSH`、`AGV_PACK_POP`、`AGV_PACKED`（Linux 为 `__attribute__((__packed__))`）。

业务代码不直接判断 `_MSC_VER` / `__GNUC__`，一律使用上述宏（编码规范 §10.2）。

## 日志

`log_service.h` 提供 `AGV_LOG_TRACE/DEBUG/INFO/WARN/ERROR/CRITICAL` 六个宏，统一经 `agv::GetLogger()` 记录；`details::SetLogger()` 可注入自定义 logger，未注入时使用 spdlog 默认 logger。规范 §12 要求：业务代码禁止直接调用 `spdlog::xxx`，日志消息用英文。

## StringProxy

`class AGV_BASIC_API StringProxy final`，内部持 `std::string stdstr_`（UTF-8）。四组能力：

| 组 | 接口 |
| --- | --- |
| 序列化 | `serializeToBytes()` → `[uint32 长度][UTF-8 字节]`；`loadBytes()` 反向解析 |
| UID | 静态 `NewUID()`：Windows 用 `CoCreateGuid`，Linux 用 `uuid_generate_random` |
| 数值转换 | `FromInt/FromUInt/FromLargeInt/FromLargeUInt/FromDouble` 与 `toXxx()` |
| 编码转换 | `toLower/toUpper`；`decodeToWString/encodeFromWString`；`decodeToLocale/encodeFromLocale`（Windows 在 UTF-8/ACP 间转换，其他平台记 AGV_LOG_ERROR 返回空） |

## 并发与 ID

- `ThreadPool`：构造启动固定数量 worker，`enqueue(task_func&&)` 投递 `std::function<void()>`，析构停止并 join。
- `IdSeed<T>`：模板参数限无符号整型，`next()` 递增、溢出回绕到初始值；**非线程安全**。
- `Timestamp`：`std::atomic_uint64_t value_`，`next()/force()/value()` 原子操作，**线程安全**。
- `DbgTracker`：RAII，构造 `AGV_LOG_TRACE` 记录 Enter，析构记录 Leave 与耗时；`_AUTO_FUNC_TRACK_` 宏一行启用。

## DirectedGraph

header-only 邻接表有向图 `template<class VDT, class ADT, class VKT=string>`：

- 顶点/弧数据需可默认构造且可拷贝赋值（concept 约束）。
- API：`addVertex`、`getVertexData`、`removeVertex`、`existVertex`、`addArc`、`removeArc`、`existArc`、`getVertexArcs`、`checkCycle`、`clear`。
- 算法：DFS 连通分量、`checkCycleImpl` 判环。
- 被 core 的 `RelationshipNetwork` 用作工具依赖图。

> 注意：`addVertex` 实现中 `if(!existVertex(key)) { return; }` 的条件疑似颠倒（详见 [已知问题](../06-reference/known-issues.md)）。
