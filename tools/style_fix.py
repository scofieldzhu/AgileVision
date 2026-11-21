#!/usr/bin/env python3
"""AgileVision 编码规范批量改写器。

与 style_check.py 共用同一套判定逻辑（namespace_blocks / needs_namespace_indent），
确保「检查器报违规」与「改写器修正」口径完全一致。

当前处理：
  - §5 命名空间体内整块缩进 4 空格（使用 style_check 的块判定）

用法:
    python tools/style_fix.py [源码根目录，默认 source/agile_vision]
"""
import os
import sys

import style_check


def fix_namespaces(lines):
    """对未按 §5 缩进的 namespace 块，块内所有非空行 +4 空格。

    从最内层块向外处理（open_idx 降序），保证嵌套块层层累加正确。
    """
    blocks = style_check.namespace_blocks(lines)
    # 只取需要修复的块，从内到外（open 越大越靠内）
    todo = [b for b in blocks if style_check.needs_namespace_indent(lines, b)]
    todo.sort(key=lambda b: b[0], reverse=True)
    for o, c in todo:
        for i in range(o + 1, c):
            if lines[i].strip():
                lines[i] = '    ' + lines[i]
    return len(todo), lines


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else 'source/agile_vision'
    if not os.path.isdir(root):
        print(f'目录不存在: {root}')
        return 2
    changed_files = 0
    fixed_blocks = 0
    for dp, _, fs in os.walk(root):
        for f in sorted(fs):
            if not f.endswith(style_check.EXT):
                continue
            p = os.path.join(dp, f)
            with open(p, 'r', encoding='utf-8', errors='surrogateescape') as fh:
                lines = fh.readlines()
            n, new_lines = fix_namespaces(lines)
            if n:
                with open(p, 'w', encoding='utf-8', errors='surrogateescape', newline='') as fh:
                    fh.writelines(new_lines)
                changed_files += 1
                fixed_blocks += n
                print(f'缩进 {os.path.relpath(p, root).replace(chr(92), "/")}: {n} 个 namespace 块')
    print(f'\n改写文件 {changed_files} 个，namespace 块 {fixed_blocks} 个')
    return 0


if __name__ == '__main__':
    sys.exit(main())
