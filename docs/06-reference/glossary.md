# 术语表

## 领域概念

| 术语 | 英文 / 类 | 含义 |
| --- | --- | --- |
| 方案 | `Solution` | 顶层容器与运行入口，持有 Engine 与多个 Procedure |
| 流程 | `Procedure` | 一个可执行流程单元，持有一个根 Process |
| 过程 | `Process` | 基本执行单元，持有 Tool 列表，可嵌套子 Process |
| 工具 | `Tool` | 抽象算法单元，由插件 DLL 实现，持有 Pin 字典 |
| 引脚 / Pin | `ToolPin` | 工具的数据端口，分 Input/Output/Prop 三类 |
| 子过程管理器 | `ProcessManager` | Tool 持有的子 Process 容器，实现递归树 |
| 工作 ID | `wkid_t` | Engine 分配的异步任务标识，`kNullId=0` 为空值 |
| 运行上下文 | `RunContext` | 在各层间传递引擎、异步标志、工作 ID 等运行时信息 |
| 关系网络 | `RelationshipNetwork` | 基于有向图的工具依赖关系，支持判环 |
| 全局变量 | `GlobalVariable` | 基于 `std::any` 的类型安全全局变量容器 |

## 数据与序列化

| 术语 | 含义 |
| --- | --- |
| `DataSpec` | 引脚数据规格：主类型 + 子类型 + 数组长度（0=动态） |
| `DataBuffer` | 按 DataSpec 存储实际数据的缓冲，Pin 间共享 |
| `ProduceInfo` | 上游引用：`OutputPin* pin` + `data_location`（数组元素下标） |
| `ConsumeInfo` | 下游登记：`ToolPin* pin` |
| `ToolLinkage` | 高层连接三元组：producer → consumer + 各自 pin key + data_location |
| Proxy | geometry 模块的序列化代理（Vec/Dict/Combine/Element/ArrayX） |
| `Ref` 参数 | Proxy 模板的 bool 参数：false=拥有数据，true=引用宿主成员 |
| `loadBytes` | 反序列化接口，返回已消费字节数，失败返回 0 |
| `SerializePolicy` | 编译期序列化后端选择头（RATEL/PB） |
| pb 前缀 | Protobuf 消息命名前缀，如 `pbSolution`、`pbDataSpec` |

## 命名与缩写

| 术语 | 含义 |
| --- | --- |
| `agv` | AgileVision 根命名空间 |
| `agv::core` | 框架层命名空间 |
| `agv::stdtool` | 标准工具层命名空间 |
| `AGV_*_API` | 模块导出宏（如 `AGV_CORE_API`） |
| `AGV_*_EXPORT` | 导出侧编译开关，构建库时定义 |
| `RATEL_*` | 历史命名遗留（宏/注释），与外部 Ratel 库无关 |
| `PLATFORM_WIN/LINUX/UNKNOWN` | 平台宏，定义于 `cross_platform.h` |
| 尾下划线 `_` | 类成员命名约定（`iid_`、`name_`） |
| `k` 前缀 | 常量与 `enum class` 成员命名约定（`kNullId`、`kInt`） |
| `_d` 后缀 | Debug 产物后缀（`agile_vision.core_d.dll`） |
| `.agvt` | 工具插件 DLL 文件的约定扩展名 |
| CLSID / GUID | 工具类标识，供 `ToolFactory` 按 ID 创建实例 |

## 构建术语

| 术语 | 含义 |
| --- | --- |
| preset | CMake 预设，`CMakePresets.json`（通用）/ `CMakeUserPresets.json`（本机） |
| 多配置生成器 | Visual Studio / Xcode，一次 configure 支持 Debug/Release 等多配置 |
| `$<CONFIG>` | CMake 生成器表达式，求值为当前配置名 |
| INTERFACE 目标 | `agv_build_config`，只传播编译要求不产出二进制 |
| vcpkg triplet | `x64-windows`，本机依赖目标平台 |
| `X_VCPKG_APPLOCAL_DEPS_INSTALL` | vcpkg 选项，安装时自动拷贝运行时 DLL |
| `AGV_BUILD_TESTS` | 顶层选项，控制是否构建 test 演示程序 |
| `AGV_REQUIRE_PROTOBUF` | 顶层选项，控制 Protobuf 缺失时是否配置失败 |
