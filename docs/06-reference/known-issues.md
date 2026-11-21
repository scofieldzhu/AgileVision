# 已知问题与技术债

本文汇总源码阅读中发现的实现缺口、疑似缺陷与演进方向。**“疑似”项为基于源码的推断，需在实际修改前复核**。

## 11.1 占位与未实现

| 项 | 位置 | 状态 |
| --- | --- | --- |
| `bytes_writer` / `compressor` / `package` | `package/` 下三对 .h/.cpp | 仅 MIT 许可证头，无类定义 |
| `AsyncProcess` / `BreakTool` / `ConditionProcessTool` / `LoopProcessTool` / `SyncProcedureTool` | `stdtool/` 下五对 .h/.cpp | 仅许可证头，无类定义 |
| `GlobalVariable::serializeToBytes/loadBytes` | `core/global_variable.cpp` | 空实现，全局变量不能持久化 |
| `SERIALIZER_PB` 分支 | `core/serialize_policy.h` | 空，Protobuf 后端未接入 |
| `protobuf_generate()` | `package/CMakeLists.txt` | 未接入，7 个 .proto 仅作契约 |

## 11.2 序列化缺口

| 项 | 说明 |
| --- | --- |
| Pin 连接未持久化 | `ToolPin` 序列化被注释；`InputPin`/`OutputPin` 无独立序列化；`Tool::loadBytes` 只恢复 name 与 PropPin |
| `DataSpec::loadBytes` 未回填 | 仅调用 `ArrayX::loadBytes` 并返回消耗字节数，未把三元组写回自身成员，反序列化后规格可能为默认值 |
| `Line::loadBytes` / `Circle::loadBytes` 未回填 | 用值模式（Ref=false）的临时 `ProxyCombine` 反序列化，结果留在临时对象，成员未更新 |
| `Circle::serializeToBytes` 参数存疑 | `ProxyCombine<point_type, float_proxy, true> pc(center_, radius_)` 中 `radius_` 为 float，而引用模式构造签名要求 `ElementProxy<float>&` |
| `DictProxy::LoadBytes` 边界偏松 | 读 `key_byte_size` 后重复执行 `left_size = size - kUIntSize`（未累计扣减头部），values 可用长度上限多算 4 字节 |
| 协议平台相关 | 原生内存 memcpy（无字节序转换、无版本号、无类型标签），跨字节序/ABI 需额外处理；与规范 §11“不直接序列化结构体内存”存在张力（ArrayX/ElementProxy/ImageData 实际用 memcpy） |

## 11.3 疑似逻辑缺陷（待核实）

| 项 | 说明 |
| --- | --- |
| `Tool::setPinConnection` 消费者登记 | `tool.cpp` 中写 `pi.pin->addConsumeInfo({pi.pin})`，按对称语义应为 `{consume_pin}`；当前 OutputPin 记录的可能是自己而非下游 Pin |
| `DirectedGraph::addVertex` 条件 | 实现为 `if(!existVertex(key)) { return; }` 后才写入，即顶点不存在时返回、仅存在时更新；新增顶点目前依赖 `addArc` 中 `operator[]` 副作用 |
| `DirectedGraph::existArc` 依赖 `VKT::empty()` | 更换 key 类型时需保证有 `empty()` 成员（默认 string 满足） |
| `ArrayX::operator[]` 仅查上界 | 条件为 `idx >= kN`，`int` 负索引未被拦截 |
| `OutputPin` 声明未实现 | `removeConsumeTool(ToolPtr)`、`clear()` 在头中声明，cpp 中未见实现 |

## 11.4 遗留与待收敛

| 项 | 说明 |
| --- | --- |
| 图网络未联动 | `Tool::setPinConnection` 的启用版本直接在 Pin 层建引用，基于 `RelationshipNetwork` 的旧校验代码被注释；框架正从“图驱动”向“Pin 直接引用驱动”迁移 |
| Engine 未使用成员 | `rem_work_cv_`、`rem_work_mutex_`、`rem_work_id_`、`exit_rem_task_` 在 `engine.cpp` 中未使用，疑似遗留/待实现 |
| `_AUTO_FUNC_TRACK_` 宏 | 调试跟踪宏仍在使用，按规范逐步收敛（注释/命名/日志） |
| `RATEL_*` 命名遗留 | 导出宏与注释仍有 RATEL 前缀，与外部依赖无关，可择机统一为 AGV 前缀 |
| 非 Windows 导出宏缺口 | 所有 `AGV_*_API` / `RATEL_*_API` 只在 `#ifdef PLATFORM_WIN` 分支定义；非 Windows 编译非模板声明会因宏未定义失败，需构建脚本补空定义 |
| `Tool::clone()` 基类实现 | 返回 `nullptr`，依赖派生类 override；未 override 的工具调用 clone 会得到空指针 |
| 全角文档路径 | 内嵌源码授权条款需对外分发前确认（Ratel 源码副本） |

## 11.5 已修复的问题（记录备查）

- 外部 Ratel 依赖已清零，源码按模块并入主源码树。
- `dynamic_library_loader.h` Linux 分支 `dlsym` 缺右括号（编译错误）已修复。
- `relationship_network.cpp` 的 `consume_linkages` 分支误写 `produce_linkages->push_back` 已修正。
- vcpkg spdlog 与手写 `SPDLOG_USE_STD_FORMAT` 冲突导致 LNK2001，已移除相关定义。
- 头文件 `#pragma once`、命名空间缩进、控制流大括号、日志直调等规范问题已全量整改，检查器零违规。

## 11.6 演进优先级建议

1. **高**：核实并修复 `setPinConnection` 消费者登记、补 OutputPin 缺失实现、修复 `DataSpec::loadBytes` 回填。
2. **中**：接入 Protobuf 后端并补全 package 模块；实现 Pin 连接持久化；处理非 Windows 导出宏。
3. **低**：清理 Engine 未使用成员、统一 RATEL 遗留命名、补齐 stdtool 占位工具、为几何类型写单元测试。
