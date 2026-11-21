#!/usr/bin/env python3
"""§8.2 控制流大括号批量改写器，判定逻辑与 style_check.py 的 BRACE_NEXT_LINE 一致。

对「if/for/while(...) 后紧跟单条语句且无大括号」的情况补 {}。
从后往前处理，避免行号错位。仅增删大括号与缩进，不改变语句语义。
"""
import os
import re
import sys

import style_check


def fix_file_v2(lines):
    targets = []
    for i in range(len(lines) - 1):
        s = lines[i].strip()
        if not re.match(r'(if|for|while)\s*\(.*\)\s*$', s):
            continue
        j = i + 1
        while j < len(lines) and (not lines[j].strip() or lines[j].strip().startswith('//')):
            j += 1
        if j >= len(lines):
            continue
        nxt = lines[j].strip()
        if nxt.startswith('{') or nxt.startswith('else') or not nxt.endswith(';'):
            continue
        targets.append((i, j))

    # 从后往前，逐个在 j 前插 '{'，j 后插 '}'，并给 body 加一级缩进
    for i, j in reversed(targets):
        head = lines[i]
        indent = style_check.indent_of(head)
        body_line = lines[j]
        new_body = (' ' * (indent + 4)) + body_line.lstrip(' ')
        lines[j:j + 1] = [
            ' ' * indent + '{\n',
            new_body if new_body.endswith('\n') else new_body + '\n',
            ' ' * indent + '}\n',
        ]
    return len(targets), lines


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
            n, new = fix_file_v2(lines)
            if n:
                with open(p, 'w', encoding='utf-8', errors='surrogateescape', newline='') as fh:
                    fh.writelines(new)
                changed += 1
                total += n
                print(f'{os.path.relpath(p, root).replace(chr(92), "/")}: {n} 处')
    print(f'\n改写文件 {changed} 个，控制流 {total} 处')
    return 0


if __name__ == '__main__':
    sys.exit(main())
