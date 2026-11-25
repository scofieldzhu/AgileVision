# 已知问题与技术债

本文汇总源码阅读中发现的实现缺口、疑似缺陷与演进方向。**“疑似”项为基于源码的推断，需在实际修改前复核**。

## 11.1 占位与未实现

| 项 | 位置 | 状态 |
| --- | --- | --- |
| `bytes_writer` / `compressor` / `package` | `package/` 下三对 .h/.cpp | 仅 MIT 许可证头，无类定义 |
| `AsyncProcess` / `BreakTool` / `ConditionProcessTool` / `LoopProcessTool` / `SyncProcedureTool` | `stdtool/` 下五对 .h/.cpp | 仅许可证头，无类定义 |
| `GlobalVariable::serializeToBytes/loadBytes` | `core/global_variable.cpp` | 空实现，全局变量不能持久化 |
| `AGV_SERIALIZER_PB` 后端 | `core/serialize_policy.h` | 预留未实现；由 CMake `AGV_SERIALIZER_BACKEND=protobuf` 选择，配置时告警并回退 |
| `protobuf_generate()` | `package/CMakeLists.txt` | 未接入，7 个 .proto 仅作契约 |

## 11.2 序列化缺口

| 项 | 说明 |
| --- | --- |
| Pin 连接未持久化 | `ToolPin` 序列化被注释；`InputPin`/`OutputPin` 无独立序列化；`Tool::loadBytes` 只恢复 name 与 PropPin |
| 协议平台相关 | 原生内存 memcpy（无字节序转换、无版本号、无类型标签），跨字节序/ABI 需额外处理；与规范 §11“不直接序列化结构体内存”存在张力（ArrayX/ElementProxy/ImageData 实际用 memcpy） |

## 11.3 疑似逻辑缺陷（待核实）

| 项 | 说明 |
| --- | --- |
| `DirectedGraph::existArc` 依赖 `VKT::empty()` | 更换 key 类型时需保证有 `empty()` 成员（默认 string 满足） |
| `ArrayX::operator[]` 仅查上界 | 条件为 `idx >= kN`，`int` 负索引未被拦截 |

## 11.4 遗留与待收敛

| 项 | 说明 |
| --- | --- |
| 图网络未联动 | `Tool::setPinConnection` 直接在 Pin 层建引用，基于 `RelationshipNetwork` 的旧校验代码被注释；框架已从“图驱动”迁移到“Pin 直接引用驱动”，图模块保留为预留调度设施 |
| `_AUTO_FUNC_TRACK_` 宏 | 调试跟踪宏仍在使用，按规范逐步收敛（注释/命名/日志） |
| 授权头 Ratel 字样 | 部分文件头的 MIT 授权注释仍含 Ratel 字样（历史文本，与外部依赖无关） |
| `Tool::clone()` 基类实现 | 默认返回 `nullptr`，由需要的派生类重写；基类无法通用复制派生行为与引脚连接 |
| 全角文档路径 | 内嵌源码授权条款需对外分发前确认（Ratel 源码副本） |

## 11.5 已修复的问题（记录备查）

- 外部 Ratel 依赖已清零，源码按模块并入主源码树。
- `dynamic_library_loader.h` Linux 分支 `dlsym` 缺右括号（编译错误）已修复。
- `relationship_network.cpp` 的 `consume_linkages` 分支误写 `produce_linkages->push_back` 已修正。
- vcpkg spdlog 与手写 `SPDLOG_USE_STD_FORMAT` 冲突导致 LNK2001，已移除相关定义。
- 头文件 `#pragma once`、命名空间缩进、控制流大括号、日志直调等规范问题已全量整改，检查器零违规。
- 导出宏命名已统一为 `AGV_` 前缀：`RATEL_BASIC_API`/`RATEL_GEOMETRY_API`/`RATEL_SYSTEM_API` 及底层 `RATEL_WIN_DLL_*`、`RATEL_EXTERN_C_*`、`RATEL_*_EXPORTS` 全部改为对应 `AGV_` 宏，`RATEL_BASIC_API` 与既有 `AGV_BASIC_API` 合并。
- 内存与并发（高危）：`DataBuffer::setStringValue` 的 `safe_size` 现真正用于截断写入并按需扩容；`syncRunProcess` 空指针解引用已修正；`Engine` 工作表全部访问改为加锁并消除 `exist()`/`operator[]` 竞态，回调在锁外执行且 promise 先于回调满足；线程池成员改为最后声明，保证析构时先 join 在途任务，并捕获任务异常避免 `std::terminate` 与等待方挂起；`ProcessManager::removeProcess(iterator)` 在失效前复位 manager。
- 反序列化边界：`DataSpec::loadBytes` 现回填三元组；`DataBuffer::loadBytes` 补齐游标推进、尾部长度字段校验并将 `value_size` 限制为容器实际容量；`VecProxy`/`DictProxy`/`StringProxy` 增加长度上界、游标推进与无符号回绕防护；`DirectedGraph::addVertex` 条件反转已修正。
- 第 2 组：`Tool::setPinConnection` 现正确登记消费者引脚（`{consume_pin}`）且连接成功返回 `true`；`ToolPin` 增加 `owner_` 反查，补齐 `OutputPin::clear`/`removeConsumeTool` 与 `RelationshipNetwork::breakRelationship` 实现；`Line`/`Circle` 改用静态 `SerializeToBytes`/`LoadBytes` 并回填成员，几何模板可正常实例化；`ProxyCombine::LoadBytes` 返回值补齐第二段字节；`ToolFactory` 删除器捕获 loader 的 `shared_ptr`，DLL 生命周期不短于其创建的 Tool。
- 第 3 组：六个 `*_export.h` 补齐非 Windows 空定义；线程池 `stop_` 改原子、任务加异常边界、析构安全 join；序列化宏统一为 `AGV_SERIALIZER_PROXY`/`AGV_SERIALIZER_PB` 并接入 CMake 选项 `AGV_SERIALIZER_BACKEND`；`solution.cpp` 的占位 iid `"xxx"` 改为空串；`CMakeUserPresets.json` 移出版本控制、提供无硬编码模板；接入 CTest 与非交互冒烟测试 `agv_smoke_test`；`test` 目标改名 `agv_demo` 以避开 CTest 保留字。
- 第 4 组：CMake 新增 `install(EXPORT AgileVisionTargets)` 与 `AgileVisionConfig(.Version).cmake`，消费者可 `find_package(AgileVision CONFIG)` 链接 `AgileVision::*`；占位模块 package 以 `NO_EXPORT` 排除导出（无 import lib）。

## 11.6 演进优先级建议

1. **高**：接入 Protobuf 后端并补全 package 模块；为协议补充魔数/版本号/端序转换与全局长度上限。
2. **中**：实现 Pin 连接持久化（当前仅 PropPin 可序列化）；补齐 stdtool 占位工具。
3. **低**：收敛 `_AUTO_FUNC_TRACK_` 调试宏；为几何类型与并发路径补充更多单元测试；对外分发前确认内嵌源码授权条款。
