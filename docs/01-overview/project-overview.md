# 项目总览

## 1.1 项目定位

AgileVision 是一个基于 C++20 的通用机器视觉流程框架，目标是把视觉算法封装为可动态加载的“工具（Tool）”，通过“引脚（Pin）”连接形成数据流图，由“方案（Solution）→ 流程（Procedure）→ 过程（Process）”三层容器组织，再由线程池引擎统一调度执行。

项目当前已从外部 Ratel 库依赖中完全脱离：原依赖的 basic/geometry/system 源码已按模块并入本仓库，命名空间统一为 `agv`，导出宏统一为 `AGV_*_API`。

## 1.2 技术栈

| 项 | 选型 / 约定 |
| --- | --- |
| 语言标准 | C++20（`target_compile_features(... cxx_std_20)`） |
| 构建系统 | CMake 3.21+，Visual Studio 17 2022 多配置生成器（x64） |
| 预设 | `CMakePresets.json` + `CMakeUserPresets.json`（version 6） |
| 第三方包管理 | vcpkg（`E:/Code/vcpkg`，triplet `x64-windows`） |
| 日志 | spdlog（vcpkg 提供，统一经 `basic/log.h` 的 `LOG_*` 宏使用） |
| Qt | 6.8.3（本机预设仅配置 `CMAKE_PREFIX_PATH`，当前源码未实际引用 Qt） |
| 序列化 | 自研二进制协议（geometry Proxy 系列）为主；Protobuf 契约已定义、代码未接入构建 |
| 产物形态 | 各模块 SHARED 库（DLL），Debug 版本加 `_d` 后缀；`test` 为可执行演示程序 |

## 1.3 顶层目录结构

| 路径 | 说明 |
| --- | --- |
| `source/agile_vision/basic/` | 基础设施：类型、日志、平台宏、时间、并发、有向图、字符串 |
| `source/agile_vision/geometry/` | 自定义二进制序列化代理框架 + 几何类型（纯头文件） |
| `source/agile_vision/system/` | 跨平台动态库加载（实现并入 basic 目标） |
| `source/agile_vision/core/` | 领域对象与执行引擎（框架核心） |
| `source/agile_vision/stdtool/` | 标准工具集（部分实现，部分占位） |
| `source/agile_vision/package/` | 打包模块（占位）+ 7 个 Protobuf 消息契约 |
| `source/agile_vision/test/` | 演示/冒烟测试可执行程序 |
| `tools/` | 编码规范机械检查与批量修复脚本 |
| `docs/` | 本知识库 |
| `CMakeLists.txt` | 根构建脚本，定义公共配置与 `agv_add_module()` |
| `CODING_STANDARD.md` | 编码规范权威来源 |

## 1.4 构建目标一览

| CMake 目标 | 类型 | 命名空间 | 导出宏 | 备注 |
| --- | --- | --- | --- | --- |
| `agile_vision.basic` | SHARED | `agv` | `AGV_BASIC_API` / `RATEL_BASIC_API` | 同时编译 system 的 cpp |
| `agile_vision.core` | SHARED | `agv::core` | `AGV_CORE_API` | 依赖 basic |
| `agile_vision.stdtool` | SHARED | `agv::stdtool` | `AGV_STDTOOL_API` | 依赖 basic + core |
| `agile_vision.package` | SHARED | `agv` | `AGV_PACKAGE_API` | 依赖 basic + core；实现占位 |
| `test` | EXECUTABLE | 全局（using `agv`、`agv::core`） | — | 由 `AGV_BUILD_TESTS` 控制 |

## 1.5 快速上手

1. 阅读 [系统架构](../02-architecture/system-architecture.md) 建立分层认知。
2. 按 [构建与安装](../04-build/build-and-install.md) 完成本地配置与构建。
3. 阅读 [领域模型](../02-architecture/domain-model.md) 与 [执行引擎与调度](../02-architecture/execution-and-scheduling.md) 理解运行主链路。
4. 参考 [test 模块](../03-modules/test.md) 与 [扩展指南](../05-development/extension-guide.md) 编写第一个工具。
