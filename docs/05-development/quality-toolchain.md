# 质量工具链

`tools/` 目录包含三个 Python 脚本，服务于编码规范的机械校验与批量修复。三者共享同一套判定逻辑（改写器 `import style_check`），确保检查门禁与批量改写的口径一致。

## 检查器：style_check.py

只读机械校验，返回非零表示有违规。

```bash
python tools/style_check.py [源码根目录，默认 source/agile_vision]
```

输出逐条 `相对路径:行号: [规则ID] 说明`，末尾按规则统计。退出码：0（无违规）/ 1（有违规）/ 2（目录不存在）。

### 规则 ID

| 规则 | 规范条款 | 检查内容 |
| --- | --- | --- |
| `TAB` | §5/§8.1 | 真实 tab 字符 |
| `LONG` | §8.6 | 行宽 > 160（全角按 2 列） |
| `PRAGMA_ONCE` | §3.1 | 使用 `#pragma once` |
| `HEADER_USING_NS` | §5 | 头文件顶层 `using namespace` |
| `NS_INDENT` | §5 | 命名空间体内未缩进 4 空格 |
| `SPACE_KW` | §8.1 | `if (` 等关键字后加空格 |
| `NO_BRACE` / `BRACE_NEXT_LINE` | §8.2 | 控制流缺大括号 / 大括号换行 |
| `MULTI_STMT` | §8.2 | 一行多语句 |
| `COMMA_FIRST` | §8.4 | 初始化列表逗号写在行首 |

### 关键机制

- `wide_len()`：按全角字符 2 列计算行宽。
- `strip_literals()`：把注释与字符串/字符字面量替换为等长空格，避免括号误判。
- `namespace_blocks()` / `needs_namespace_indent()`：用括号栈识别 namespace 块并判断缩进。

## 改写器：style_fix.py

批量修复 §5 命名空间整块缩进。

```bash
python tools/style_fix.py [源码根目录，默认 source/agile_vision]
```

机制：对需修复的 namespace 块，从最内层向外（`open_idx` 降序）给块内非空行加 4 空格，保证嵌套累加正确；仅增删空白。

## 改写器：style_fix_braces.py

批量修复 §8.2 控制流大括号缺失。

```bash
python tools/style_fix_braces.py [源码根目录，默认 source/agile_vision]
```

机制：找到 `if/for/while(...)` 后紧跟单条 `;` 结尾语句且无 `{`/非 `else` 的情况，**从后往前**处理避免行号错位，插入 `{`、`}` 并给 body 增加一级缩进；仅增删大括号与缩进，不改语义。

## 使用建议

1. 提交前先跑 `style_check.py`，确认零违规。
2. 批量整改时用改写器，整改后**再跑一次检查器**复核。
3. 改写器只处理机械项，语义类条目（命名、所有权、日志内容、序列化约定）需人工审查。
4. 检查器与改写器共用判定逻辑，若发现门禁通过但形态异常，应优先排查判定函数是否失配。

## 整改历史

项目曾对 `source/` 下 98 个 `.h/.hpp/.cpp` 执行全量整改：66 文件 68 个 namespace 块统一缩进、10 文件 41 处控制流补大括号、13 文件 67 处日志调用改为 `LOG_*`、头文件 `using namespace` 清理、`#pragma once` 改 guard 等。整改后检查器零违规，且 configure/build/install 全通过。整改前源码备份在 `C:/Users/cheng/WorkBuddy/spaces/2026_11_21_16-38-59/style_backup_20261121`。
