# 已知问题与技术债

本文汇总源码阅读中发现的实现缺口、疑似缺陷与演进方向。**“疑似”项为基于源码的推断，需在实际修改前复核**。

## 11.1 占位与未实现

| 项 | 位置 | 状态 |
| --- | --- | --- |
| `bytes_writer` / `compressor` / `package` | `package/` 下三对 .h/.cpp | 仅 MIT 许可证头，无类定义 |
| `AsyncProcess` / `BreakTool` / `ConditionProcessTool` / `LoopProcessTool` / `SyncProcedureTool` | `stdtool/` 下五对 .h/.cpp | 仅许可证头，无类定义 |
| `AGV_SERIALIZER_PB` 后端 | `core/serialize_policy.h` | 预留未实现；由 CMake `AGV_SERIALIZER_BACKEND=protobuf` 选择，配置时告警并回退 |
| `protobuf_generate()` | `package/CMakeLists.txt` | 未接入，7 个 .proto 仅作契约 |

## 11.2 序列化缺口

| 项 | 说明 |
| --- | --- |
| Pin 连接未持久化（已修复） | 拓扑帧已记录全部 Pin 规格与输入引脚绑定表，加载时经 ToolFactory 重建工具后统一重连，详见 11.5 第 10 组 |
| 协议平台相关（已修复） | 拓扑帧与底层 Proxy 帧均统一小端并带魔数/版本/长度上限；`ImageData` 仅序列化元数据，不写入进程内指针 |

## 11.3 疑似逻辑缺陷（待核实）

| 项 | 说明 |
| --- | --- |
| `DirectedGraph::existArc` 依赖 `VKT::empty()` | 更换 key 类型时需保证有 `empty()` 成员（默认 string 满足） |

## 11.4 遗留与待收敛

| 项 | 说明 |
| --- | --- |
| 图网络未联动（已收敛） | 已确立 Pin 直连为唯一运行时真相源；`RelationshipNetwork::buildFrom()` 在每次 `Procedure::run()` 前从 Pin 连接即时派生依赖图并判环，检测到回路时拒绝执行。旧的基于图网络的兼容校验代码仍保留注释，供后续拓扑调度参考 |
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
- 第 5 组：`GlobalVariable::serializeToBytes/loadBytes` 补全四种类型的二进制持久化（类型头 + 定长标量/字符串长度前缀），`loadBytes` 校验类型枚举、逐字段边界与畸形输入并返回消费字节数；四个 `getXxxValue` 改用 `std::any_cast<T>(&value_)` 指针式检测，类型状态不符时安全返回 `nullopt` 而非抛 `bad_any_cast`；`ArrayX::operator[]` 补齐负索引下界检查。冒烟测试新增 4 类用例覆盖往返、畸形输入与负索引。
- 第 6 组（P0 架构优化）：`Process::run()` 由 `s = s && t->run()` 改为显式循环聚合，消除首个工具失败后短路跳过后续工具的问题；`Process::interrupt_` 改为 `std::atomic_bool`，消除 `cancel()` 与 `run()` 跨线程数据竞争；新增 `Tool::collectPinLinkages()` 与 `RelationshipNetwork::buildFrom()`，`Procedure::run()` 执行前从 Pin 连接派生依赖图并判环，发现回路即拒绝运行。冒烟测试新增 3 个用例（失败不短路、中断复位、环检测）覆盖上述改动。
- 第 7 组（P1 第一阶段）：`RelationshipNetwork::topologicalSort()` 以 Kahn 算法输出依赖顺序，`buildFrom` 增加递归开关；`Process::runInTopologicalOrder()` 提供按依赖顺序执行的新入口（默认 `run()` 保持配置顺序不变）。新增 `ProcessRunner` 统一子过程同步/异步执行、等待与查找，`StringProcessTool`/`WaitProcessTool` 改用该入口，修复前者 `runContext()` 按值拷贝导致 work_id 写入副本的问题。冒烟测试新增 3 个用例（拓扑顺序、有环拒绝、ProcessRunner）。
- 第 8 组（P1 结果契约）：新增 `WorkResult{work_id, success, cancelled, error, elapsed}`，由 `promise<WorkResult>`、`ResultCallback` 与 `getResult()` 共同承载；旧 `FinishCallback` 重载保留兼容。`syncRunProcess()` 现返回 `Process::run()` 的真实结果而非恒为 true；`WaitResult` 新增 `kNotFound`，区分“从未提交”与“已完成但工作表已清理”，完成结果登记到有上限的 `completed_results_`；`cancel()` 额外标记 `cancelled`。`Solution` 新增 `run(bool)`、`waitAll()`、`cancelAll()`、`getWorkResult()` 聚合接口；`Procedure` 异步回调按值捕获别名，消除悬垂 this。冒烟测试新增 4 个用例（同步失败传播、结果回调与查询、未提交判定、Solution 聚合），现有 40 个用例全通过。
- 第 9 组（P1 数据流并发）：`DataBuffer` 增加 `shared_mutex` 读写保护、`atomic_uint64_t version_` 与 `snapshot()`；新增值语义 `DataBufferSnapshot`（构造时锁内拷贝，取值返回值而非裸指针），裸指针/引用 API 的生命周期约束为"仅在阶段屏障内使用"。`RelationshipNetwork::topologicalLevels()` 按 Kahn 分层，`Process::runInParallelLayers()` 同层 `std::async` 并行、层间 future 屏障，保留失败不短路与原子中断。冒烟测试新增 4 个用例（快照隔离、版本递增、分层并行依赖顺序、并行失败聚合），现有 48 个用例全通过。
- 第 10 组（P2 拓扑序列化）：新增 `serialization_frame.h` 小端版本化帧（12 字节头 magic/version/payload_size，字段长度前缀 + 上限校验）。`Solution/Procedure/Process/Tool` 整链改为帧格式：工具帧含 clsid、全部 Pin 规格与输入引脚绑定表；`Process::loadBytes(buffer,size,factory)` 经 `ToolFactory` 重建工具、恢复 PropPin 值与 Pin 连接。新增 `ToolFactory::registerCreator()` 支持进程内类型注册；补齐 `Process::setAlias()` 缺失定义。旧双参数 loadBytes 保留（factory 为空时复用已有工具）；旧无魔数方案文件不做自动迁移。冒烟测试新增 3 个用例（拓扑往返、Solution 往返、畸形帧/版本/魔数拒绝），共 64 项断言全通过。
- 第 11 组（P2 插件 ABI 元数据）：新增 `core/tool_metadata.h/.cpp`，定义 POD 结构 `AgvPinMetaC`/`AgvToolMetaC` 与值语义 `PinMetadata`/`ToolMetadata`。`ToolFactory::loadToolFile()` 可选解析 `GetToolABIVersion`（缺失按 v1 兼容，未来版本拒绝加载）与 v2 必需的 `GetToolMeta`（clsid 一致性、版本一致性、pin_count 上限 256 校验），元数据立即拷贝为值语义结构，插件缓冲可安全复用。`Tool::pinMetadata()` 反射全部引脚（含基类 Status），`ToolFactory` 新增 `registerMetadata/getToolMeta/getAllToolMetas/getToolAbiVersion`，查询优先级为显式注册 > DLL v2 > 临时实例反射。冒烟测试新增 3 个用例（版本协商、C ABI 拷贝与畸形拒绝、元数据发现），共 86 项断言全通过。
- 第 12 组（P2 子过程树拓扑帧）：工具帧尾部追加带 magic（PCGM）的可选子过程段，递归嵌入 Process 帧，任意层级 `Tool → ProcessManager → Process` 树均可完整往返；加载时按 clsid 工厂重建子过程工具并恢复 active_iid 与 manager 回指。新增 `TopologyContext` 限制深度 16 层、节点 256，序列化/反序列化双向拒绝超限。修复 `ProcessManager::removeProcess(iterator)` 先 erase 后解引用失效迭代器的 UB，新增 `clear()` 供重建使用。冒烟测试新增 2 个用例（两级嵌套往返、超深拒绝），共 105 项断言全通过。

- 第 13 组（P2 底层字节流收尾）：新增 `basic/portable_stream.h`，为 Proxy 字节流统一 12 字节帧头 `[magic][version][payload_size]` 与小端标量编码（浮点经 `bit_cast`）。`StringProxy/ArrayX/ElementProxy/VecProxy/DictProxy/Rect` 全部改为帧格式，校验魔数、版本、载荷长度与帧内尾部剩余；`DataSpec` 随之升级。`DataBuffer` 增加 `DATB` 帧，int/float 按小端逐元素编码，`ImageData` 只保存元数据（type/channel/bits/width/height/size），加载后 `data` 置空；`GlobalVariable` 增加 `GLBV` 帧，类型头与标量均小端编码。冒烟测试新增 7 个用例，共 116 项断言全通过。

## 11.6 演进优先级建议

1. **高**：接入 Protobuf 后端并补全 package 模块。
2. **中**：补齐 stdtool 占位工具。P1 三项与 P2 的拓扑序列化、插件 ABI 元数据、子过程树帧、底层字节流端序/魔数约束均已落地。
3. **低**：收敛 `_AUTO_FUNC_TRACK_` 调试宏；`DirectedGraph::existArc` 对 key 类型 `empty()` 的依赖改为模板约束；为几何类型与并发路径补充更多单元测试；对外分发前确认内嵌源码授权条款。
