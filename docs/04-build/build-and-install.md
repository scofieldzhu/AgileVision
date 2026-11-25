# 构建与安装

## 8.1 环境要求

| 项 | 要求 |
| --- | --- |
| CMake | ≥ 3.21（preset version 6 的最低要求） |
| 编译器 | Visual Studio 17 2022（MSVC 19.44），x64 |
| vcpkg | 由环境变量 `VCPKG_ROOT` 指定，triplet `x64-windows`（见 `CMakeUserPresets.example.json`） |
| 第三方库 | spdlog（由 vcpkg 提供，必需） |

## 8.2 预设体系

`CMakePresets.json`（仓库通用，version 6）：

| 预设 | 类型 | 说明 |
| --- | --- | --- |
| `vs2022-base` | configure（hidden） | generator `Visual Studio 17 2022`，`binaryDir = build/<presetName>`，`architecture = x64 (strategy: set)` |
| `vs2022` | configure（hidden） | 继承自 `vs2022-base`，仅显示名 `VS2022 x64` |

`CMakeUserPresets.example.json`（随仓库分发的模板）复制到 `CMakeUserPresets.json` 后按本机环境调整：

| 预设 | 类型 | 说明 |
| --- | --- | --- |
| `local-vcpkg` | configure（hidden） | toolchain `$env{VCPKG_ROOT}/.../vcpkg.cmake`、triplet `x64-windows`、`X_VCPKG_APPLOCAL_DEPS_INSTALL=ON` |
| `vs2022-local` | configure（用户入口） | 继承 `vs2022 + local-vcpkg`，`binaryDir = build/vs2022-local`，`installDir = install` |
| `build-debug-local` | build | configurePreset `vs2022-local`，configuration `Debug` |
| `build-release-local` | build | configurePreset `vs2022-local`，configuration `Release` |

继承关系：`vs2022-local → { vs2022 → vs2022-base, local-vcpkg }`。

> `CMakeUserPresets.json` 含本机绝对路径，已加入 `.gitignore`，不入库；模板见 `CMakeUserPresets.example.json`。CMake 无 `installPresets` 字段，安装用 `cmake --install`。

## 8.3 常用命令

```bash
# 查看预设
cmake --list-presets=all

# 配置（VS 多配置生成器）
cmake --preset vs2022-local

# 构建
cmake --build --preset build-debug-local
cmake --build --preset build-release-local

# 安装
cmake --install build/vs2022-local --config Debug
cmake --install build/vs2022-local --config Release

# 运行冒烟测试（非交互，覆盖序列化往返与反序列化边界）
ctest --test-dir build/vs2022-local -C Debug --output-on-failure
```

Agent shell 注意事项：若环境同时存在 `HTTPS_PROXY/https_proxy` 大小写重复变量，MSBuild 会报 MSB6001。验证时可用 `env -u https_proxy -u http_proxy` 绕开；普通终端不受影响。

## 8.4 顶层 CMake 设计

| 机制 | 说明 |
| --- | --- |
| `agv_build_config`（INTERFACE） | 统一 C++20、include（build/install 双接口）、spdlog 链接与编译宏、MSVC 警告抑制 |
| `agv_add_module(name [NO_EXPORT] [extra_srcs...])` | 统一 GLOB 源文件、创建 SHARED 库、链接公共配置、`DEBUG_POSTFIX _d`、安装目标与头文件；`NO_EXPORT` 用于无导出符号的占位模块 |
| 输出目录 | `${CMAKE_BINARY_DIR}/bin/$<CONFIG>`，兼容单/多配置生成器，布局一致为 `bin/<Config>` |
| 安装前缀 | 未显式指定时回退 `${CMAKE_SOURCE_DIR}/install`；preset 的 `installDir` 优先 |
| 头文件安装 | `<prefix>/include/agile_vision/<module>`，与源码 include 路径一致 |
| 包导出 | `install(EXPORT AgileVisionTargets)` + `AgileVisionConfig.cmake(.in)`，消费者 `find_package(AgileVision CONFIG)` 后链接 `AgileVision::*` |
| CTest | `enable_testing()` + `agv_smoke_test`，`ctest -C Debug/Release` 运行 |

选项：

| 选项 | 默认值 | 用途 |
| --- | --- | --- |
| `AGV_BUILD_TESTS` | ON | 是否构建 `agv_demo` 演示程序与 `agv_smoke_test` |
| `AGV_REQUIRE_PROTOBUF` | OFF | Protobuf 缺失时是否使配置失败 |
| `AGV_SERIALIZER_BACKEND` | `proxy` | 序列化后端：`proxy`（内嵌几何代理，默认）/ `protobuf`（预留未实现，会告警并回退） |

**消费安装后的库**：

```cmake
find_package(AgileVision CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE AgileVision::agile_vision.core)
```

导出目标：`AgileVision::agile_vision.{basic,core,stdtool}`（package 为占位模块不导出）。运行时需保证 `install/bin` 在动态库搜索路径中。

## 8.5 模块构建差异

| 模块 | 差异 |
| --- | --- |
| basic | `agv_add_module(agile_vision.basic ../system/dynamic_library_loader.cpp)`；在 `if(WIN32)` 中定义 `AGV_{BASIC,GEOMETRY,SYSTEM}_EXPORT`；额外安装 geometry/system 头文件 |
| core | 标准模块，PUBLIC 链接 basic，定义 `AGV_CORE_EXPORT` |
| stdtool | 标准模块，PUBLIC 链接 basic + core，定义 `AGV_STDTOOL_EXPORT` |
| package | 标准模块 + Protobuf 探测，PUBLIC 链接 basic + core，定义 `AGV_PACKAGE_EXPORT` |
| agv_demo | `add_executable`，链接 basic + core + stdtool，`DEBUG_POSTFIX _d` |

## 8.6 验证结果

清理 build/install 后全新执行 configure → Debug/Release build → 两种 config install，全部通过，无 error/无警告。`install/bin` 含两套 DLL/EXE 与 spdlog/fmt 运行时；`install/include/agile_vision/` 含各模块头文件。package 为占位模块无导出符号，故无 `.lib`，属正常。
