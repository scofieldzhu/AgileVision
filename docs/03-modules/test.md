# test 模块

演示/冒烟测试可执行程序，由顶层选项 `AGV_BUILD_TESTS`（默认 ON）控制是否构建。链接 basic + core + stdtool，Debug 产物为 `test_d.exe`。**未使用任何第三方测试框架**（无 gtest/catch2），本质是手工 demo。

## 文件

| 文件 | 职责 |
| --- | --- |
| `test.cpp` | main 入口 |
| `test_solution.h/.cpp` | `Test_Solution()` 演示场景 |

## test.cpp

- 设置 `spdlog::set_level(trace)`（日志级别配置，符合规范例外）。
- 使用 `_AUTO_FUNC_TRACK_` 启用函数跟踪。
- 调用 `Test_Solution()`。
- `while(true)` 每秒 sleep 1000ms 保持进程存活，便于观察异步流程。

## Test_Solution() 演示流程

全局静态：`ToolFactory sToolFactory`、本地 2D 图像工具 CLSID `dbd43f17-8e6d-4776-b383-a73b75b20601`、引脚键 `ImagePath`/`DirType`/`ImageData`、`sExecDir`。

1. `GetModuleFileName` 获取可执行文件目录 `sExecDir`。
2. `InitToolFactory()`：遍历目录下的 `.agvt` 工具文件，调用 `sToolFactory.loadToolFile()` 动态加载。
3. `InitProcedure()`：`sln->createProcedure(uid)` 创建流程。
4. `InitProcess()`：`sToolFactory.createTool(clsId, uid)` 创建工具，设置属性 Pin `ImagePath`（`ImageCalib.bmp`）、`DirType`（0），`p->appendTool(local_image)`。
5. `new Solution(3)` 创建解决方案（线程池 3），`InitProcedure(sln)`，`sln->run()` 运行。

## 演示的 API 用法

- `ToolFactory::loadToolFile` / `createTool`
- `Solution::createProcedure` / `run`
- `Procedure` / `Process` / `Tool` 的层级操作
- `PropPin::mutableDataBuffer().setStringValue` / `setIntValue`
- `StringProxy::NewUID()` 生成 UID（别名 `UIDGen`）
- `_AUTO_FUNC_TRACK_` 与 `LOG_TRACE`

## 运行前提

程序依赖外部 `.agvt` 工具插件文件与 `ImageCalib.bmp` 图像，需放置在可执行文件目录才能完整运行。当前测试不依赖 OpenCV（注释中保留 `cv::waitKey`），保留为后续视觉测试入口。

`CMakeLists.txt` 使用 `add_executable(test)`（非 `agv_add_module`），安装到 `<prefix>/bin/`。
