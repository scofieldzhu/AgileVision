#!/usr/bin/env python3
"""§8.2 控制流语句块批量改写器。

判定逻辑全部来自 style_check.check_lines / control_headers，检查器与改写器共用同一套规则，
避免「门禁 0 违规、改写器却把形态改回去」的静默失配。

覆盖 §8.2 四条子规则：
1. 左大括号与 if/for/while/else/else if 等关键词同一行（BRACE_SAME_LINE）
2. if/while/for 语句体必加 {}（BRACE_NEXT_LINE、INLINE_NO_BRACE）
3. } 与 else / else if 同行（ELSE_SEPARATE、ELSE_IF_SEPARATE）
4. 一行一条语句（MULTI_STMT）

只增删空白与大括号，不改变语句顺序与语义；含复杂注释/原始字符串的形态跳过交人工处理。
"""
import os
import sys

import style_check


def violations(lines):
    """按规则分组返回 {rule: [0-based 行号]}。"""
    result = {}
    for line_no, rule, _ in style_check.check_lines(lines):
        result.setdefault(rule, []).append(line_no - 1)
    return result


def append_brace(line):
    """在语句头行尾追加 '{'，保留行尾注释。"""
    body = line.rstrip('\n\r')
    newline = '\n' if line.endswith('\n') else ''
    pos = body.find('//')
    if pos >= 0:
        return body[:pos].rstrip() + '{ ' + body[pos:] + newline
    return body.rstrip() + '{' + newline


def fix_brace_same_line(lines, targets):
    """把单独一行的 '{' 上提到控制流语句头末尾。"""
    count = 0
    for idx in reversed(targets):
        nxt = style_check.next_content_line(lines, idx)
        if nxt is None or lines[nxt].strip() != '{':
            continue
        lines[idx] = append_brace(lines[idx])
        del lines[nxt]
        count += 1
    return count


def fix_else_separate(lines, targets):
    """把 } 后另起一行的 else / else if（及其单独的 '{'）合并到 } 所在行。"""
    target_set = set()
    for idx in targets:
        if lines[idx].strip() == '}':
            target_set.add(idx)
            continue
        j = idx - 1
        while j >= 0 and not lines[j].strip():
            j -= 1
        if j >= 0 and lines[j].strip() == '}':
            target_set.add(j)
    out = []
    i = 0
    count = 0
    while i < len(lines):
        if i not in target_set or lines[i].strip() != '}':
            out.append(lines[i])
            i += 1
            continue
        j = i + 1
        while j < len(lines) and not lines[j].strip():
            j += 1
        if j >= len(lines) or not lines[j].strip().startswith('else'):
            out.append(lines[i])
            i += 1
            continue
        text = lines[j].strip()
        end = j
        if not text.endswith('{') and '//' not in text:
            k = j + 1
            while k < len(lines) and not lines[k].strip():
                k += 1
            if k < len(lines) and lines[k].strip() == '{':
                text += '{'
                end = k
        indent = ' ' * style_check.indent_of(lines[i])
        out.append(indent + '}' + text + '\n')
        i = end + 1
        count += 1
    lines[:] = out
    return count


def fix_inline_no_brace(lines, targets):
    """把与控制头同行的单条语句体改为带 {} 的三行形态。"""
    headers, clean_lines = style_check.control_headers(lines)
    count = 0
    for idx in reversed(targets):
        if idx not in headers:
            continue
        _, close_pos = headers[idx]
        body = clean_lines[idx][close_pos + 1:].strip()
        if not body or body.startswith('{') or '//' in body:
            continue
        raw = lines[idx].rstrip('\n\r')
        # 在原始行中定位同一右括号，避免破坏字符串内容
        depth = 0
        close = -1
        open_pos = raw.find('(')
        for p in range(open_pos, len(raw)):
            if raw[p] == '(':
                depth += 1
            elif raw[p] == ')':
                depth -= 1
                if depth == 0:
                    close = p
                    break
        if close < 0:
            continue
        header_part = raw[:close + 1].rstrip()
        body_part = raw[close + 1:].strip()
        if not body_part or body_part.startswith('{'):
            continue
        indent = ' ' * style_check.indent_of(raw)
        lines[idx:idx + 1] = [
            header_part + '{\n',
            indent + '    ' + body_part + '\n',
            indent + '}\n',
        ]
        count += 1
    return count


def fix_multi_stmt(lines, targets):
    """把分号拼接的多条语句拆成一行一条；复杂形态跳过。"""
    count = 0
    for idx in reversed(targets):
        clean = style_check.clean_code_lines(lines)[idx]
        depth = 0
        cuts = []
        for p, ch in enumerate(clean):
            if ch == '(':
                depth += 1
            elif ch == ')':
                depth = max(0, depth - 1)
            elif ch == ';' and depth == 0:
                cuts.append(p)
        if len(cuts) < 2:
            continue
        raw = lines[idx].rstrip('\n\r')
        parts = []
        start = 0
        for cut in cuts:
            parts.append(raw[start:cut + 1].strip())
            start = cut + 1
        if any('{' in part or '}' in part or not part for part in parts):
            continue
        indent = ' ' * style_check.indent_of(raw)
        lines[idx:idx + 1] = [indent + part + '\n' for part in parts]
        count += 1
    return count


def fix_file(lines):
    """反复应用修复直到门禁中 §8.2 规则全部通过。"""
    total = 0
    for _ in range(10):
        rules = violations(lines)
        n = 0
        n += fix_brace_same_line(lines, rules.get('BRACE_SAME_LINE', []))
        rules = violations(lines)
        n += fix_else_separate(lines, rules.get('ELSE_IF_SEPARATE', []) +
                               rules.get('ELSE_SEPARATE', []))
        rules = violations(lines)
        n += fix_inline_no_brace(lines, rules.get('INLINE_NO_BRACE', []))
        rules = violations(lines)
        n += fix_multi_stmt(lines, rules.get('MULTI_STMT', []))
        total += n
        if n == 0:
            break
    return total, lines


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else 'source/agile_vision'
    changed = 0
    total = 0
    for dp, _, fs in os.walk(root):
        for f in sorted(fs):
            if not f.endswith(style_check.EXT):
                continue
            p = os.path.join(dp, f)
            with open(p, 'r', encoding='utf-8', errors='surrogateescape') as fh:
                lines = fh.readlines()
            n, new = fix_file(lines)
            if n:
                with open(p, 'w', encoding='utf-8', errors='surrogateescape', newline='') as fh:
                    fh.writelines(new)
                changed += 1
                total += n
                print(f'{os.path.relpath(p, root).replace(chr(92), "/")}: {n} 处')
    print(f'\n改写文件 {changed} 个，§8.2 违规 {total} 处')
    return 0


if __name__ == '__main__':
    sys.exit(main())
