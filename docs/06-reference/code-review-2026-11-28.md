# AgileVision 全盘代码审查报告（2026-11-28）

## 1. 审查概况

| 项目 | 内容 |
|---|---|
| 审查依据 | 仓库根目录 `CODING_STANDARD.md`（项目编码规范权威来源） |
| 审查范围 | `source/agile_vision/` 下全部源码：105 个文件（`.h` 57、`.hpp` 12、`.cpp` 36），约 11,580 行；根 `CMakeLists.txt`、`CMakePresets.json`、`cmake/`、`tools/` |
| 审查方式 | ① 机械门禁 `python tools/style_check.py source/agile_vision`；② 补充脚本扫描 guard / MIT 头 / 参数列表 / 日志上下文 / 注释语言 / 访问域；③ 三路人工逐文件语义复核（basic/geometry/package/system、core 前段、core 后段/stdtool/test/smoke/CMake） |
| 机械门禁结果 | **0 处违规**（工具覆盖 §3.1 pragma、§5 命名空间缩进、§8.1/§8.2、§8.4、§8.6、§8.7 部分形态、§10、§12 日志中文） |
| 补充说明 | 工作树存在大量未提交改动，本报告基于**当前工作树快照**；审查全程只读，未修改任何源码 |

机械门禁通过不等于符合规范：`style_check.py` 不检查 §3.2 包含顺序、§3.1 guard 宏名正确性、§4 文件头、§6 命名、§8.3 类结构、§8.5 参数列表、§9 内存、§12 日志上下文等，本次问题主要集中在这些盲区。

## 2. 结论摘要

| 等级 | 类别 | 数量 | 说明 |
|---|---|---:|---|
| 高 | 违反硬约束且影响可维护性/正确性 | 约 20 | `.hpp` guard 后缀、控制流大括号、二元操作符跨行、裸 `new/delete`、异常做流程控制、共享状态无锁读取 |
| 中 | 命名、参数列表、日志上下文、包含顺序等 | 约 90 | 其中 §8.5 约 50 处、§12 日志上下文 40 处 |
| 系统性 | 全库统一但与规范文本冲突 | 2 类 | include guard 前缀（69 个头文件）、MIT 头缺 `Creator`（105 个文件） |
| 建议 | 规范表述与工程实践需对齐 | 约 15 | `.cpp` 中 `using namespace`、CMake 选项、QtConcurrent 适用性等 |

**总体判断**：机械排版质量较好（无 tab、无超 160 列、无真实关键字空格违规），但接口命名、参数列表形态、日志上下文、头文件 guard 与文件头模板存在成片偏差；`style_check.py` 的 0 违规不能作为“符合规范”的充分证据。

## 3. 详细发现

### 3.1 高优先级（确定违规）

| 条款 | 位置 | 问题 | 建议 |
|---|---|---|---|
| §3.1 | 全部 12 个 `.hpp`（`basic/id_seed.hpp:28`、`core/directed_graph.hpp:28`、`geometry/{array_x,circle,default_creator,dict_proxy,element_proxy,is_serializable,line,proxy_combine,rect,vec_proxy}.hpp`） | 模板头 guard 以 `_h__` 结尾，规范要求 `_hpp__` | guard 改为 `__<文件名>_hpp__`，同步 `#define` 与末尾注释 |
| §8.2 | `smoke_test/smoke_test.cpp:306,307,308` | 三个 `if` 无 `{}` 且语句体与条件同行 | 加大括号并换行 |
| §8.7 | `core/serialization_frame.h:74-77` | `|` 留在行尾、`<<` 出现在续行首，二元操作符跨行 | 提具名中间量或改写为单行表达式（当前工具未覆盖裸 `|`） |
| §9 | `core/solution.cpp:47` | `return new Procedure("")`，裸 `new` 且所有权不明 | 改 `std::make_unique`，返回 `UPtr`/`SPtr` |
| §9 | `core/directed_graph.hpp:156,176` | `new ArcNode` / `delete target_arc_node` 裸管理 | 由智能指针或容器持有，删除手工 `delete` |
| §9 | `geometry/default_creator.hpp:51` | `new object_type()` 返回裸指针 | 改 `std::make_unique`，接口返回 `*UPtr` |
| §12 | `system/dynamic_library_loader.cpp:34,86,122` | 定义并使用 `THROW_EXCEPTION_OF_METHOD_NOT_IMPL()`，以异常做流程控制 | 改返回 `false`/`nullptr` + `AGV_LOG_ERROR` 记录上下文 |
| §12 | `geometry/array_x.hpp:102` | `operator[]` 抛 `std::out_of_range` 做流程控制 | 按项目约定改返回码/断言 |
| §13 | `core/data_buffer.cpp:386,452,502,564,627` | `getIntPointer() const` / `getFloatPointer() const` / `getImagePointer() const` / `getStringValue() const` / `getAllBytesValues() const` 读共享状态未持锁，而写路径持 `unique_lock` | 加 `std::shared_lock`；`getAllBytesValues()` 还应返回值快照而非内部容器引用 |
| §13 | `basic/log_service.cpp:45-53` | 全局 logger 由 `SetLogger` / `GetLogger` 无锁读写，存在数据竞争 | 加互斥保护或以原子方式替换 |

`data_buffer.cpp:386` 已抽样复核：同文件 `getFloatValue()` 等读路径使用 `std::shared_lock`，而 `getIntPointer() const` 直接读取 `ds_` 与 `fundamental_bytes_`，确属读写竞争。

### 3.2 中优先级（确定违规）

#### §3.2 包含顺序（9 处）

| 位置 | 问题 |
|---|---|
| `core/data_buffer.cpp:29-34` | 标准库头在自身头之前；跨模块头 `geometry/` 排在 `basic/` 之前 |
| `core/engine.cpp:30-33` | `<algorithm>` 在跨模块头之前 |
| `core/global_variable.cpp:30-33` | `geometry/` 排在 `basic/` 之前 |
| `core/solution.cpp:29-36` | `<ranges>` 夹在自身头与跨模块头之间 |
| `core/process.cpp:28-30` | 标准库头在自身头 `process.h` 之前 |
| `core/relationship_network.cpp:29-31` | 标准库头在自身头之前 |
| `basic/dbg_tracker.cpp:32` | `log_service.h` 置于 `<filesystem>` 之后 |
| `basic/string_proxy.cpp:33-34` | 跨模块头置于标准库之后 |
| `smoke_test/smoke_test.cpp:27-55` | 标准库头置于 `agile_vision/...` 之前 |

统一顺序：自身头文件 → 跨模块完整路径头（按模块字母序）→ 标准库 → 第三方。

#### §6 命名（约 25 处）

| 位置 | 问题 | 建议 |
|---|---|---|
| `basic/log_service.h:21,23`、`basic/log_service.cpp:45-52` | `SetLogger` / `GetLogger` 非静态工厂却用 PascalCase；全局 `gLogger_` 用 PascalCase + 尾下划线 | 改 `setLogger` / `getLogger`、`g_logger`（同步全部调用点） |
| `core/core_typedef.h:84` | `using wkid_list`（snake_case，非 `List` 后缀） | `WkidList` |
| `core/engine.h:88,89` | `WorkTableType` / `ResultTableType` 为字典别名，缺 `Dict` 后缀 | `WorkDict` / `ResultDict` |
| `core/directed_graph.hpp:75-78` | `graph_list_type`、`visit_map`、`vertex_key_list_type`、`vertex_key_list2_type` | `GraphList`、`VisitDict`、`VertexKeyList`、`VertexKeyList2` |
| `core/tool_factory.h:65` | `DllLoaderPtr` 实际是 `shared_ptr`，应带 `SPtr` | `DllLoaderSPtr` |
| `stdtool/string_process_tool.h:40-42`、`stdtool/wait_process_tool.h:41-42` | `PK_P_*` 常量非 `k` + PascalCase | `kProcessIidListPinKey` 等 |
| `test/test_solution.{h:32,cpp:52,65,77,85}` | `Test_Solution` / `InitToolFactory` / `InitProcess` / `InitProcedure` 非 camelCase | `testSolution` / `initToolFactory` / ... |
| `test/test_solution.cpp:44-48` | `sToolFactory`、`sLocalImage2dClsId` 匈牙利命名；`PK_P_*` / `PK_O_*` 常量命名 | snake_case 与 `kXxx` |
| `basic/basic_typedef.h:47,58-61` | 容器别名 `ByteVec`、`AgvBytes`、`AgvMultiBytes`、`AgvMultiString` 无 `List` 后缀 | 统一 `*List`（历史兼容别名可保留并标注） |
| `system/system_typedef.h:36` | 类型别名 `DLL_HANDLE` 全大写 | `DllHandle` |
| `geometry/{proxy_combine,vec_proxy,dict_proxy}.hpp` 中 `SerializeToBytes` / `LoadBytes` | 静态工具函数非静态工厂却用 PascalCase | `serializeToBytes` / `loadBytes`（注意与同名成员区分） |

#### §8.5 参数列表（约 50 处，均为确定违规）

- **短参数折行 30 处**：`smoke_test/smoke_test.cpp` 的 `check(...)` 等两参数调用折成两行；已逐条计算，合并后全部 ≤160 列，按规范必须写在同一行。代表行：`238, 310, 353, 380, 383, 387, 389, 403, 415, 417, 456, 463, 475, 504, 528, 532, 549, 554, 584, 588, 611, 650, 655, 659, 668, 696, 725, 740, 765, 779`。
- **含 lambda 未按一参一行展开 15 处（脚本确认的典型形态）**：`basic/thread_pool.cpp:44`、`core/data_buffer.cpp:555,585`、`core/engine.cpp:144`、`core/output_pin.cpp:52,62`、`core/process.cpp:339,408,421,428`、`core/process_manager.cpp:78,123`、`core/solution.cpp:215`、`core/tool_factory.cpp:244`、`stdtool/string_process_tool.cpp` 等；另有 `smoke_test.cpp:376,545,580,854,499` 等嵌套/多行形态。
- **其他折行形态**：`basic/string_proxy.cpp:172`（`std::sprintf` 多参数挤行）、`system/dynamic_library_loader.cpp:47`（`FormatMessageA` 首参数未另起行、续行对齐左括号）、`core/tool.cpp:241-242`（5 参数无 lambda，合并约 149 列仍折行）、`stdtool/string_process_tool.cpp:71-72`（4 参数日志折行）。

#### §12 日志上下文（40 处）

`AGV_LOG_ERROR/WARN` 消息无上下文占位符与实参。代表位置：

- `core/data_buffer.cpp:252,314,337,353,357,369,389,403,419,423,435,455,469,485,505,542,567,581,595,617`（20 处，多为 `"Data buffer is not integer type!"`、`"Invalid param 'vp' or 'size'!"`）
- `core/engine.cpp:150,167,258`（`"Null process pointer!"` 等）
- `basic/string_proxy.cpp:253,280`、`basic/thread_pool.cpp:65,89`
- 其余见补充扫描结果，按模块分布总计 40 处

建议：日志中至少带上类型/索引/参数值/对象标识，例如 `"Data buffer type mismatch: expect={}, actual={}, buffer={}"`。

#### §1 注释语言与其他

| 位置 | 问题 |
|---|---|
| `core/procedure.cpp:43` | 行尾注释 `// same iid with procedure's` 为英文 |
| `core/tool.cpp:470,627` | `//disconnect pin relationship.`、`//other stuffs` 为英文 |
| `stdtool/string_process_tool.cpp:87,92` | `// sync mode`、`// async mode` 为英文 |
| `stdtool/wait_process_tool.h:37` | `@brief` 说明为英文 |
| `core/engine.cpp:172` | `//push to thread pool!` 为英文 |
| `smoke_test/smoke_test.cpp:1-6` | 文件头 MIT 注释缺 `File:` / `Creator:` 字段 |
| `core/directed_graph.hpp:2` | 项目简介写为 `Ratel is a application framework`，应为 AgileVision |

### 3.3 系统性约定冲突（需决策后统一整改）

| 条款 | 范围 | 现状 | 说明 |
|---|---|---|---|
| §3.1 | **69 个头文件**（全部 `.h`/`.hpp`） | guard 统一为 `__agile_vision_<模块>_<文件>_h__`，规范要求 `__<文件名>_h__`（`.hpp` 用 `_hpp__`） | 全库一致且唯一，属于规范文本与项目既有约定的冲突：要么批量改名（含 12 个 `.hpp` 后缀），要么在规范中显式允许路径前缀 |
| §4 | **105 个源文件** | 文件头有 `File:` 与 `Copyright (c) YYYY-YYYY ...`，但**全部缺 `Creator:`**，年份分隔符和 `All rights reserved.` 也与模板不完全一致 | 历史模板遗留；建议批量补齐模板并记录创建者，或明确“历史文件豁免、新文件严格执行” |
| §5 | 约 11 个 `.cpp`（`core/global_variable.cpp:35`、`core/tool_pin.cpp:31`、`core/process.cpp:39`、`core/procedure.cpp:37`、`core/tool.cpp:37`、`core/tool_factory.cpp:34`、`stdtool/string_process_tool.cpp:36`、`stdtool/wait_process_tool.cpp:36`、`test/test_solution.cpp:37-39` 等） | `.cpp` 中 `using namespace agv` / `agv::core` / `std::filesystem` | 规范仅禁止头文件使用；`.cpp` 为“优先显式限定”，属可整改项，非硬性违规 |
| §5 | `core/engine.h:77`、`core/engine.cpp:47` | 文件内辅助类型 `RunThreadTask` 通过 `friend` 暴露到头文件，未放匿名命名空间 | 建议改为匿名命名空间内类型 + 公开回调接口 |
| §14 | `CMakeLists.txt:66-68` | MSVC 仅 `/wd4996 /wd4275 /wd4251`，未显式 `/W4 /utf-8` | §1 要求根脚本统一配置编译器警告与编码；建议补齐或说明由预设注入 |
| §14 | `test/CMakeLists.txt`、`smoke_test/CMakeLists.txt` | 子目录自行 `GLOB` 源文件并维护依赖 | §14 要求源文件与依赖统一在根脚本；建议收敛 |
| §13 | `core/process.cpp:305-387` | 并行执行用 `std::async` / `std::future` | 规范建议优先 `QtConcurrent::run` / `QFutureWatcher`；本项目未使用 Qt，建议确认规范适用性或在规范中豁免 |
| §7 | `stdtool/wait_process_tool.cpp:69` | `timeout * 1000` 秒转毫秒裸字面量 | 提取具名常量或改用 `std::chrono::duration_cast` |

## 4. 整改建议（按优先级）

1. **先补工具门禁**：`style_check.py` 增加 §8.5 参数列表形态（含 lambda 展开、短参数合并 ≤160 判定）、§8.7 裸 `|` / `&` 行尾、§3.2 包含顺序、§3.1 guard 宏名与后缀、§4 MIT 模板字段、§6 命名/别名后缀、§12 日志上下文。检查器与改写器继续共用同一判定逻辑。
2. **一轮批量整改**（可安全自动化）：30 处 `check(...)` 折行合并、12 个 `.hpp` guard 后缀、69 个 guard 前缀（需先决策）、105 个文件头 `Creator`（需先决策）、9 处包含顺序、§6 别名与常量改名。
3. **人工改写**：`serialization_frame.h:74-77` 表达式、`data_buffer.cpp` 无锁读、4 处裸 `new/delete`、异常流程控制、含 lambda 的参数列表、日志上下文补全。
4. **规范对齐**：明确 guard 前缀是否豁免、历史文件 MIT 头是否豁免、非 Qt 项目是否适用 `QtConcurrent` 条款，避免“全库一致但全库违规”的长期状态。

## 5. 审查覆盖

- 已逐一审查：`basic/`、`core/`、`geometry/`、`package/`、`system/`、`stdtool/`、`test/`、`smoke_test/` 全部 105 个源码文件，根构建脚本与检查工具。
- 未发现问题的条款：§2 目录分层与 Qt 布局（本项目无 UI 代码）、§10 导出宏/平台宏（业务代码未散落原生宏）、§11 序列化（`loadBytes` 均返回消费字节数，未直接序列化结构体内存）、§8.3 访问域唯一性（扫描 0 重复；仅 `TestTool` 构造位置 1 处），§8.6 行宽、§8.1 tab。
- 本报告为静态审查；§8.2 专项整改后已补做构建与冒烟测试，其余并发/日志问题的运行期影响仍需结合测试进一步确认。

## 6. §8.2 专项整改（同日补录）

### 6.1 评审清单与工具更新

- `CODING_STANDARD.md` §16 评审清单原只有「一行一条语句 + 控制流必加 `{}`」一条，已拆为四条，完整覆盖 §8.2：① 一行一条语句且语句体不与条件同行；② 控制流左大括号与关键词同一行；③ `if`/`while`/`for` 必加 `{}`；④ `}` 与 `else`/`else if` 同行。
- `tools/style_check.py` 新增规则：`BRACE_SAME_LINE`、`INLINE_NO_BRACE`、`ELSE_IF_SEPARATE`，并重写 `MULTI_STMT` 判定（按括号深度统计分号，忽略 `for` 头）；同时将逐行检查抽取为 `check_lines()`。
- `tools/style_fix_braces.py` 重写为调用 `style_check.check_lines()` / `control_headers()` 的共享判定，覆盖上述四条子规则；检查器与修复器不再各写一套逻辑。

### 6.2 整改结果

| 项 | 整改前 | 整改后 |
|---|---:|---:|
| `BRACE_SAME_LINE`（左大括号另起一行） | 409 | 0 |
| `INLINE_NO_BRACE`（语句体无 `{}`） | 3 | 0 |
| `ELSE_IF_SEPARATE`（`}` 与 `else if` 分行） | 2 | 0 |
| 合计 | **414**（涉及 30 个源码文件） | **0** |

- 批量修复只增删空白与 `{}`；已比对修复前后 token：除 `core/tool.cpp:470` 一处行尾注释与新增 `{` 的相对位置调整外，其余文件仅空白差异，注释内容未丢失。
- 修复后验证：`cmake --build --preset build-debug-local` 成功（basic/core/package/stdtool/demo/smoke_test 全部目标）；`agv_smoke_test_d.exe` 通过，0 个 `[FAIL]` 用例。
- 第 3 节的 §8.2 条目（含 `smoke_test.cpp:306-308`）已随本轮整改全部解决。
