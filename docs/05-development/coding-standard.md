# 编码规范

权威来源为仓库根目录 [`CODING_STANDARD.md`](../../CODING_STANDARD.md)（通用 C++20 项目编码规范，以“规则 + 正例/反例”形式组织，16 章）。本文只做摘要与落地说明，冲突时以原文为准。

## 章节概览

| 章 | 标题 | 核心要求 |
| --- | --- | --- |
| 1 | 语言与平台 | C++20；平台宏统一隔离；注释中文、日志英文、UI 走 `tr()` |
| 2 | 文件与目录组织 | 模块目录低层不依赖高层；`.h/.hpp/.proto` 后缀语义 |
| 3 | 头文件规范 | `#ifndef __xxx_h__` guard，禁 `#pragma once`；规定 include 顺序；优先前向声明 |
| 4 | 文件头注释模板 | 新文件带 MIT 授权头，占位符写年份与创建者 |
| 5 | 命名空间 | 头文件禁 `using namespace`；辅助类型放匿名命名空间；命名空间体内每行缩进 4 空格、全库禁 tab |
| 6 | 命名规范 | 类 PascalCase、函数 camelCase、类成员 snake_case + 尾下划线、结构体成员无尾下划线、常量 `k` 前缀、类型别名后缀 `SPtr/UPtr/List/Dict` |
| 7 | 枚举与常量 | `enum class` 成员加 `k` 前缀；跨 C 边界枚举用 `MODULE_` 前缀；常量 `constexpr` |
| 8 | 空格缩进与代码结构 | 关键字后不加空格、4 空格缩进；控制流必加 `{}`；访问域唯一；初始化列表逗号后置；参数换行规则；行宽 ≤160；二元与 `?:` 不换行 |
| 9 | 内存与指针管理 | 拥有用 `shared_ptr/unique_ptr`，非拥有用裸指针，禁裸 new/delete，成员声明处初始化 |
| 10 | 导出宏与平台宏 | 平台头统一定义导出宏；`PLATFORM_WIN/LINUX/UNKNOWN`；C 接口用 `PROJECT_EXTERN_C_BEGIN/END` |
| 11 | 序列化约定 | `loadBytes` 返回已消费字节数（失败 0）；按协议顺序编码；不直接序列化结构体内存 |
| 12 | 错误处理与日志 | bool/指针/0 返回码约定；不用异常做流程控制；日志用 `LOG_*` 且英文带上下文 |
| 13 | 并发 | mutex + lock_guard 保护共享状态；跨线程 ID 用原子 |
| 14 | 构建与模块 | 根 CMake 统一管理；Debug 带 `_d` 后缀；多配置生成器不靠 `CMAKE_BUILD_TYPE` |
| 15 | 代码示例 | 头文件声明与实现文件示例 |
| 16 | 评审要点清单 | 新增/修改代码逐条自查的 checkbox |

## 强制条目摘要（审查必查）

- **语言**：C++20；平台代码用平台头条件编译隔离，业务代码不散落 `_WIN32` 等原生宏。
- **注释/字符串**：注释与解释性文字中文；日志消息英文；UI 文本用英文源文 + `tr()`。
- **文件后缀**：`.h` 普通声明；`.hpp` 模板/header-only；模板实现不进 `.cpp`。
- **Include Guard**：`#ifndef` 风格，宏名 `__<文件名去后缀>_h__` / `_hpp__`；禁与 `#pragma once` 混用。
- **包含顺序**：自身头 → 跨模块 `agile_vision/...` → 标准库 → 第三方。
- **命名空间**：头文件禁 `using namespace`；文件内辅助类型放匿名命名空间；命名空间体内 4 空格缩进、全库禁 tab。
- **命名**：类 PascalCase；函数 camelCase 动词开头；类成员 snake_case + 尾 `_`；结构体成员 snake_case 无下划线；常量 `k` + PascalCase；宏全大写下划线；类型别名 `using`（指针后缀 `Ptr/SPtr/UPtr`、容器 `List/Dict`）。
- **枚举**：`enum class` 成员加 `k` 前缀；跨 C 边界朴素枚举用 `MODULE_` 前缀。
- **空格缩进**：关键字后不加空格（`if(1)`）；缩进 4 空格禁 tab；`case` 相对 `switch` 缩进 4 空格；三元表达式一行写完。
- **语句结构**：一行一语句；`if/while/for` 体必加 `{}`；`}` 与 `else if` 同行；类访问域各只出现一次；构造/析构置于所在访问域末尾；初始化列表逗号后置；参数 ≤5 且无 lambda 可单行。
- **行宽**：≤160 列（全角按 2 列）；二元操作符与 `?:` 不跨行，超长靠具名中间量/辅助函数改写。
- **内存**：拥有关系 `shared_ptr/unique_ptr`，非拥有裸指针；禁裸 new/delete；成员声明处初始化。
- **日志**：统一 `LOG_TRACE/LOG_WARN/LOG_ERROR` 等（定义于 `basic/log.h`），业务代码禁直调 `spdlog::xxx`（级别配置例外）。

## 落地方式

规范中多处要求“由项目约定的代码风格检查脚本执行机械校验”。项目已落地配套工具（`tools/`）：

- `style_check.py`：机械门禁，覆盖禁 pragma once、头文件 using namespace、命名空间缩进、tab/关键字空格、控制流大括号、逗号后置、行宽。
- `style_fix.py` / `style_fix_braces.py`：批量改写器，**import style_check 复用同一判定逻辑**，避免门禁 0 违规但改写把形态改回去的静默失配。

当前状态：`python tools/style_check.py source/agile_vision` 零违规。详见 [质量工具链](quality-toolchain.md)。
