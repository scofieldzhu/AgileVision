# 依赖与环境

## 9.1 第三方依赖

| 依赖 | 用途 | 来源 | 必需性 |
| --- | --- | --- | --- |
| spdlog | 日志（经 `basic/log_service.h` 封装） | vcpkg `x64-windows` | 必需（根 CMake `find_package(... REQUIRED)`） |
| fmt | spdlog 后端依赖 | vcpkg 自动带入 | 必需（运行时 DLL 随安装拷贝） |
| Qt 6.8.3 | 本机预设配置了 `CMAKE_PREFIX_PATH` | `D:/Qt68/6.8.3/msvc2022_64` | 当前源码未实际引用，仅路径预留 |
| Protobuf | 第二序列化后端（规划中） | 系统/vcpkg | 可选，`AGV_REQUIRE_PROTOBUF` 控制 |
| OpenCV | 早期视觉测试（已移除） | — | 无需；test 已去掉 OpenCV 依赖 |

## 9.2 vcpkg 集成

`CMakeUserPresets.json` 的 `local-vcpkg` 预设设置：

- `CMAKE_TOOLCHAIN_FILE = E:/Code/vcpkg/scripts/buildsystems/vcpkg.cmake`
- `VCPKG_TARGET_TRIPLET = x64-windows`
- `X_VCPKG_APPLOCAL_DEPS_INSTALL = ON`：安装时自动把 spdlog/fmt 等运行时 DLL 拷贝到目标目录。

依赖解析由 vcpkg manifest 或全局安装决定；本机需确保 spdlog 已安装（`vcpkg install spdlog:x64-windows`）。

## 9.3 已移除的外部依赖：Ratel

项目曾依赖外部 Ratel 库（`basic`、`geometry`、`system` 三部分），现已完全移除：

1. 按需复制所需源码到 `source/agile_vision/vendor/ratel/`，删除外部链接。
2. 取消 vendor 中间层与独立 `agv_ratel` 目标，源码按模块并入主源码树（basic/geometry/system）。
3. include 路径由 `"ratel/xxx.h"` 统一改为 `"agile_vision/xxx.h"`。
4. 命名统一：导出宏 `AGV_*_API`、命名空间 `agv` / `agv::core` / `agv::stdtool`。

当前全工程无外部 Ratel 路径或 `ratel.basic/ratel.system` 链接残留，所有导出宏已统一为 `AGV_` 前缀，无 `RATEL_*` 宏残留。部分文件头的 MIT 授权注释仍保留 Ratel 字样（历史文本，与外部库无关）。

> 授权提示：内嵌代码源自 Ratel 源码副本，若对外分发需自行确认其授权条款。

## 9.4 本机路径汇总

| 用途 | 路径 |
| --- | --- |
| vcpkg 根目录 | `E:/Code/vcpkg` |
| Qt 6 CMake 配置 | `D:/Qt68/6.8.3/msvc2022_64/lib/cmake/Qt6` |
| 构建目录 | `E:/Code/AgileVision/build/vs2022-local` |
| 安装目录 | `E:/Code/AgileVision/install` |
| 工作区备份 | `C:/Users/cheng/WorkBuddy/spaces/.../style_backup_20261121`（样式整改前的源码备份） |

## 9.5 环境注意事项

- VS 多配置生成器不使用 `CMAKE_BUILD_TYPE` 判断部署；产物布局由 `$<CONFIG>` 生成器表达式统一。
- Debug 库带 `_d` 后缀（如 `agile_vision.core_d.dll`），由各目标 `DEBUG_POSTFIX` 自动附加。
- MSVC 警告 `/wd4996 /wd4275 /wd4251` 由 `agv_build_config` 统一抑制（DLL 接口 STL 类型的常见警告）。
