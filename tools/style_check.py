#!/usr/bin/env python3
"""AgileVision C++ 编码规范机械校验器（CODING_STANDARD.md §3/§5/§8）。

本文件是检查器与批量改写器（style_fix.py）共用的唯一判定逻辑来源，
两者必须使用同一套函数，避免「门禁 0 违规、改写器却把形态改回去」的静默失配。
仅做只读检查，返回非零表示存在违规。

用法:
    python tools/style_check.py [源码根目录，默认 source/agile_vision]
"""
import os
import re
import sys

EXT = ('.h', '.hpp', '.cpp')
MAX_COL = 160
PLATFORM_HEADERS = {'cross_platform.h', 'win_header.h', 'export_macro.h'}


# ---------------------------------------------------------------------------
# 通用工具
# ---------------------------------------------------------------------------
def wide_len(line: str) -> int:
    """§8.6 行宽：中文/全角字符按 2 列计。"""
    width = 0
    for ch in line.rstrip('\n\r'):
        cp = ord(ch)
        wide = (
            0x1100 <= cp <= 0x115F or
            0x2E80 <= cp <= 0xA4CF or
            0xAC00 <= cp <= 0xD7A3 or
            0xF900 <= cp <= 0xFAFF or
            0xFE30 <= cp <= 0xFE6F or
            0xFF00 <= cp <= 0xFF60 or
            0xFFE0 <= cp <= 0xFFE6
        )
        width += 2 if wide else 1
    return width


def indent_of(line: str) -> int:
    """行首空白字符数。"""
    return len(line) - len(line.lstrip(' \t'))


def strip_literals(line: str, state):
    """把注释与字符串/字符字面量内容替换为等长空格，避免括号匹配误判。

    state: dict 携带跨行的 block_comment / string / raw_string 状态。
    """
    out = list(line)
    n = len(line)
    i = 0
    while i < n:
        if state['block']:
            if line[i:i + 2] == '*/':
                out[i] = out[i + 1] = ' '
                state['block'] = False
                i += 2
            else:
                out[i] = ' '
                i += 1
            continue
        if state['str']:
            if line[i] == '\\':
                out[i] = ' '
                if i + 1 < n:
                    out[i + 1] = ' '
                i += 2
            elif line[i] == '"':
                out[i] = ' '
                state['str'] = False
                i += 1
            else:
                out[i] = ' '
                i += 1
            continue
        # normal
        if line[i:i + 2] == '//':
            for j in range(i, n):
                out[j] = ' '
            break
        if line[i:i + 2] == '/*':
            out[i] = out[i + 1] = ' '
            state['block'] = True
            i += 2
            continue
        if line[i] == '"':
            out[i] = ' '
            state['str'] = True
            i += 1
            continue
        if line[i] == "'":
            # 字符字面量：处理简单转义
            out[i] = ' '
            i += 1
            while i < n:
                out[i] = ' '
                if line[i] == '\\':
                    if i + 1 < n:
                        out[i + 1] = ' '
                    i += 2
                elif line[i] == "'":
                    i += 1
                    break
                else:
                    i += 1
            continue
        i += 1
    return ''.join(out)


def namespace_blocks(lines):
    """返回所有 namespace 块 [(open_idx, close_idx)]（0-based 行号）。

    通过括号栈识别，能正确区分 namespace 的 '}' 与 class/函数/初始化列表的 '}'。
    """
    state = {'block': False, 'str': False}
    stack = []  # (is_ns, open_idx)
    blocks = []
    for i, line in enumerate(lines):
        clean = strip_literals(line, state)
        for pos, ch in enumerate(clean):
            if ch == '{':
                before = clean[:pos]
                is_ns = bool(re.search(r'\bnamespace\s*(?:[A-Za-z_]\w*(?:::\w+)*\s*)?$', before))
                stack.append((is_ns, i, pos))
            elif ch == '}':
                if stack:
                    is_ns, open_idx, _ = stack.pop()
                    if is_ns:
                        blocks.append((open_idx, i))
    return blocks


def needs_namespace_indent(lines, block):
    """判定一个 namespace 块是否未按 §5 缩进。

    规范：体内每一行相对 namespace 声明行缩进 4 空格，内部层级再累加。
    判定口径：体内第一个非空行的缩进若与 namespace 声明行相同（顶格平齐），
    说明整个块尚未缩进，需要块内所有行统一 +4。
    """
    open_idx, close_idx = block
    ns_indent = indent_of(lines[open_idx])
    for i in range(open_idx + 1, close_idx):
        if lines[i].strip():
            return indent_of(lines[i]) == ns_indent
    return False


def namespace_violation_lines(lines):
    """返回未按 §5 缩进的 namespace 块的体内行号集合（0-based）。"""
    violating = set()
    for block in namespace_blocks(lines):
        if needs_namespace_indent(lines, block):
            o, c = block
            for i in range(o + 1, c):
                if lines[i].strip():
                    violating.add(i)
    return violating


# ---------------------------------------------------------------------------
# 检查规则（规则 ID 保持稳定，供改写器复用）
# ---------------------------------------------------------------------------
def check_file(path: str):
    """返回 [(line_no, rule, detail)]。line_no 为 1-based。"""
    issues = []
    with open(path, 'r', encoding='utf-8', errors='surrogateescape') as fh:
        lines = fh.readlines()

    ns_violations = namespace_violation_lines(lines)
    is_header = path.endswith(('.h', '.hpp'))

    in_block_comment = False
    for n, raw in enumerate(lines, 1):
        line = raw.rstrip('\n\r')
        idx = n - 1
        stripped = line.strip()

        # §5/§8.1 禁止真实 tab
        if '\t' in line:
            issues.append((n, 'TAB', '出现真实 tab，应使用 4 空格'))

        # §8.6 行宽
        if wide_len(line) > MAX_COL:
            issues.append((n, 'LONG', f'行宽 {wide_len(line)} > {MAX_COL}'))

        # §3.1 禁止 #pragma once
        if stripped == '#pragma once':
            issues.append((n, 'PRAGMA_ONCE', '§3.1 应使用 #ifndef include guard'))

        # §5 头文件禁止 using namespace
        if is_header and re.match(r'\s*using\s+namespace\b', line):
            issues.append((n, 'HEADER_USING_NS', '§5 头文件禁止 using namespace'))

        # §5 命名空间体内缩进：整块相对 namespace 声明行 +4
        if idx in ns_violations:
            issues.append((n, 'NS_INDENT', '命名空间体内未统一缩进 4 空格'))

        # 块注释状态跟踪（授权模板等，避免误报代码规则）
        if in_block_comment:
            if '*/' in stripped:
                in_block_comment = False
            continue
        if stripped.startswith('/*') and '*/' not in stripped:
            in_block_comment = True
            continue
        if stripped.startswith('/*') or stripped.startswith('*') or stripped.startswith('//'):
            continue

        # §8.1 关键字后加空格：if ( / for ( / while ( / switch (
        for kw in ('if', 'for', 'while', 'switch'):
            if re.search(r'\b' + kw + r'\s+\(', line):
                issues.append((n, 'SPACE_KW', f'{kw} 后不应有空格（§8.1）'))
                break

        # §8.2 控制流缺大括号
        m = re.search(r'\b(if|for|while)\s*\(.*\)\s*(return|break|continue|throw)\b', line)
        if m:
            issues.append((n, 'NO_BRACE', f'{m.group(1)} 语句体缺少 {{}}（§8.2）'))

        # §8.2 跨行单语句体无大括号：if/for/while 行以 ')' 结尾，下一非空行非 '{'
        # （else/else if 由前置 if 的大括号保证，这里只处理控制头本身）
        if re.match(r'\s*(if|for|while)\s*\(.*\)\s*$', line):
            for j in range(idx + 1, min(idx + 4, len(lines))):
                nxt = lines[j].strip()
                if not nxt or nxt.startswith('//'):
                    continue
                if not nxt.startswith('{'):
                    issues.append((n, 'BRACE_NEXT_LINE', '控制流语句体应加大括号（§8.2）'))
                break

        # §8.2 一行多语句
        if re.search(r';\s*(?:int|auto|const|std::|[A-Za-z_]\w*\s*=)', line) and not stripped.startswith('#'):
            if not re.match(r'\s*(?:for|if|while)\s*\(', stripped):
                issues.append((n, 'MULTI_STMT', '一行多条语句（§8.2）'))

        # §8.4 初始化列表逗号行首
        if re.match(r'^\s*,\s*\S', line):
            issues.append((n, 'COMMA_FIRST', '初始化列表逗号应置于上一项行尾（§8.4）'))

        # §8.7 二元操作符断行：行尾留操作符（排除预处理续行）
        if (not stripped.startswith('#') and
                re.search(r'(\&\&|\|\||==|!=|<=|>=|<<|>>|\+=|-=|\*=|/=|[\+\*/%])\s*$', line)):
            issues.append((n, 'OP_EOL', '二元操作符不得留在行尾（§8.7）'))

        # §8.7 三元断行：'?' 独占行尾，或 '?/:' 独占续行
        # 排除构造初始化列表 ': member_(' 形态（§8.4 逗号后置场景）
        if re.search(r'\?\s*$', line):
            issues.append((n, 'TERNARY_BREAK', '三元 ? 不得断行（§8.7）'))
        if re.match(r'^\s*\?', line):
            issues.append((n, 'TERNARY_BREAK', '三元 ? 不得独占续行（§8.7）'))
        if (re.match(r'^\s*:\s', line) and
                not re.match(r'^\s*:\s*\w+\s*[(\{]', line) and
                idx > 0 and re.search(r'\?\s*$', lines[idx - 1])):
            issues.append((n, 'TERNARY_BREAK', '三元 : 不得独占续行（§8.7）'))

        # §1/§12 日志消息中文
        ml = re.search(r'AGV_LOG_\w+\s*\(\s*"([^"]*)"', line)
        if ml and re.search(r'[\u4e00-\u9fff]', ml.group(1)):
            issues.append((n, 'LOG_CHINESE', '日志消息必须使用英文（§12）'))

        # §1/§10.2 业务代码散落原生平台宏（平台头豁免）
        if os.path.basename(path) not in PLATFORM_HEADERS and re.search(
                r'\b(_WIN32|_WIN64|_MSC_VER|__GNUC__|__linux__|__APPLE__)\b', line):
            issues.append((n, 'PLATFORM_MACRO', '平台宏应复用平台头（§10.2）'))

        # §6.2 typedef
        if re.search(r'\btypedef\b', line):
            issues.append((n, 'TYPEDEF', '类型别名应使用 using（§6.2）'))

        # §7 enum class 成员 k 前缀
        me = re.search(r'enum\s+class\s+\w+\s*\{([^}]*)\}', line)
        if me:
            bad_members = [m.strip().split('=')[0].strip() for m in me.group(1).split(',')
                           if m.strip() and not m.strip().startswith('k')]
            if bad_members:
                issues.append((n, 'ENUM_MEMBER', 'enum class 成员应加 k 前缀（§7）'))

        # §6.1 常量 k 前缀
        mc = re.search(r'\bconstexpr\s+(?:auto|int|size_t|std::size_t|unsigned|long|float|double)\s+(\w+)', line)
        if mc and not mc.group(1).startswith('k'):
            issues.append((n, 'CONSTEXPR_NAME', '编译期常量应以 k 开头（§6.1）'))

        # §8.1 else 独占一行
        if re.match(r'^\s*else\s*$', line):
            issues.append((n, 'ELSE_SEPARATE', '} 与 else 应写在同一行（§8.2）'))
    return issues


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else 'source/agile_vision'
    if not os.path.isdir(root):
        print(f'目录不存在: {root}')
        return 2
    total = 0
    by_rule = {}
    for dp, _, fs in os.walk(root):
        for f in sorted(fs):
            if not f.endswith(EXT):
                continue
            p = os.path.join(dp, f)
            issues = check_file(p)
            if issues:
                rel = os.path.relpath(p, root).replace('\\', '/')
                for n, rule, detail in issues:
                    print(f'{rel}:{n}: [{rule}] {detail}')
                    by_rule[rule] = by_rule.get(rule, 0) + 1
                total += len(issues)
    print(f'\n总计 {total} 处违规')
    for rule in sorted(by_rule):
        print(f'  {rule}: {by_rule[rule]}')
    return 1 if total else 0


if __name__ == '__main__':
    sys.exit(main())
