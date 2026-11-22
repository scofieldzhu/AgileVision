# 系统架构

## 2.1 分层结构

项目采用自底向上的四层依赖结构，低层不依赖高层：

```
┌─────────────────────────────────────────────┐
│  应用层   test（演示程序）                    │
├─────────────────────────────────────────────┤
│  工具层   stdtool（标准工具） package（打包） │
├─────────────────────────────────────────────┤
│  框架层   core（领域对象 + 执行引擎）          │
├─────────────────────────────────────────────┤
│  基础层   basic（类型/日志/并发/图）           │
│          geometry（序列化代理，纯头文件）      │
│          system（动态库加载，编入 basic）      │
└─────────────────────────────────────────────┘
```

依赖方向严格向下：`test → {stdtool, package} → core → basic`。`geometry` 与 `system` 没有独立构建目标，其头文件随 basic 安装，`system/dynamic_library_loader.cpp` 由 basic 目标编译。

## 2.2 模块依赖关系

| 模块 \ 依赖 | basic | geometry | system | core | stdtool | package |
| --- | --- | --- | --- | --- | --- | --- |
| basic | — | 头文件包含 | 编入其 cpp | — | — | — |
| core | 是 | 序列化头 | 间接（经 basic） | — | — | — |
| stdtool | 是 | — | — | 是 | — | — |
| package | 是 | — | — | 是 | — | — |
| test | 是 | — | — | 是 | 是 | — |

CMake 层面：`agv_build_config`（INTERFACE）统一提供 C++20、include 根、spdlog 与 MSVC 警告开关；各模块通过 `agv_add_module()` 创建 SHARED 库并链接该接口目标。

## 2.3 命名空间规划

| 命名空间 | 归属 | 主要内容 |
| --- | --- | --- |
| `agv` | 公共根空间 | 基础类型（`DataSpec` 之外的 `DataType`、`PinType`、`PinKey`、`wkid_t`、`kNullId`）、basic/geometry/system 全部类型、core 类的前向声明 |
| `agv::core` | 框架层 | `Solution`、`Procedure`、`Process`、`Tool`、`ToolPin` 及派生 Pin、`DataSpec`、`DataBuffer`、`GlobalVariable`、`Engine`、`ProcessManager`、`RelationshipNetwork`、`RunContext` 等 |
| `agv::stdtool` | 工具层 | `StringProcessTool`、`WaitProcessTool` 等标准工具 |
| 全局 | 特殊 | `module_typedef.h` 的 `DLL_HANDLE`；`test.cpp` 的 `main` |

约定：业务类归入模块子空间，公共数据类型留在 `agv` 根。`solution.cpp` 中 `DefaultCreator<Procedure*>` 的特化保留在 `agv` 根，与序列化模板所在空间一致。

## 2.4 导出宏体系

宏定义层级：`cross_platform.h`（平台宏）→ `export_macro.h`（基础宏 `AGV_WIN_DLL_*`、`AGV_EXTERN_C_*`）→ 各 `*_export.h`（模块 API 宏）。全部导出宏统一使用 `AGV_` 前缀。

| 宏 | 定义文件 | 判定开关 | 使用者 |
| --- | --- | --- | --- |
| `AGV_BASIC_API` | `basic/module_export.h` | `AGV_BASIC_EXPORT` | basic 模块公共类与内嵌基础工具 |
| `AGV_GEOMETRY_API` | `geometry/module_export.h` | `AGV_GEOMETRY_EXPORT` | geometry 导出声明 |
| `AGV_SYSTEM_API` | `system/module_export.h` | `AGV_SYSTEM_EXPORT` | 动态库加载器 |
| `AGV_CORE_API` | `core/module_export.h` | `AGV_CORE_EXPORT` | core 全部公共类 |
| `AGV_STDTOOL_API` | `stdtool/module_export.h` | `AGV_STDTOOL_EXPORT` | 标准工具类 |
| `AGV_PACKAGE_API` | `package/module_export.h` | `AGV_PACKAGE_EXPORT` | 打包模块 |

构建侧：basic 目标在 `if(WIN32)` 中 `PRIVATE` 定义 `AGV_BASIC_EXPORT`、`AGV_GEOMETRY_EXPORT`、`AGV_SYSTEM_EXPORT`，使 system 实现与 geometry 内联代码随 basic 库导出；core/stdtool/package 在消费侧走 `dllimport`。其余模块同样用 `if(WIN32)` 定义各自的 `AGV_*_EXPORT` 开关。

> 注意：上述 API 宏目前只在 `#ifdef PLATFORM_WIN` 分支内定义。非 Windows 平台直接编译含这些宏的非模板声明会因宏未定义报错，需构建脚本补充空定义。详见 [已知问题](../06-reference/known-issues.md)。

## 2.5 运行时架构视图

```
Solution（方案）
 ├─ Engine（线程池调度器）
 ├─ RunContext（运行上下文）
 └─ Procedure*（流程，多个）
      └─ Process root（根过程）
           ├─ Tool*（工具，多个）
           │    ├─ Pin 字典（Input/Output/Prop）
           │    └─ ProcessManager（子过程容器）
           │         └─ Process*（子过程）→ 递归
           └─ RunContext
```

工具之间不直接互调，而是通过 Pin 引用上游 OutputPin 的 DataBuffer 交换数据；`ToolFactory` 负责从 `.agvt` 动态库创建工具实例。详见 [领域模型](domain-model.md) 与 [数据流与引脚连接](dataflow-and-pins.md)。

## 2.6 安装布局

| 目标路径 | 内容 |
| --- | --- |
| `<prefix>/bin/` | 各模块 DLL / `test.exe`（Debug 为 `*_d`），vcpkg 自动拷贝 spdlog/fmt 运行时 |
| `<prefix>/lib/` | 各模块导入库 `.lib` |
| `<prefix>/include/agile_vision/{basic,core,geometry,system,stdtool,package,test}/` | 对应模块头文件 |

头文件安装路径与源码 `#include "agile_vision/..."` 一致，外部消费者只需将 `<prefix>/include` 加入 include 路径。
