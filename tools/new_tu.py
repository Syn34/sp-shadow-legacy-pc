#!/usr/bin/env python3
"""Declare a decompiled translation unit in config/arm9/delinks.txt.

Usage: new_tu.py src/path/file.c LO HI      (.text range [LO, HI), hex)

Checks that LO is a function start, that HI is a function start (or the end of
.text), and that the range does not overlap an existing TU. Prints the
functions the C file must define.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
path, lo, hi = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3], 16)

funcs = []
for line in open(os.path.join(ROOT, "config/arm9/symbols.txt")):
    m = re.match(r"(\S+) kind:function\((\w+),size=(0x[0-9a-f]+)[^)]*\) addr:(0x[0-9a-f]+)", line)
    if m:
        funcs.append((int(m.group(4), 16), int(m.group(3), 16), m.group(1)))
funcs.sort()
starts = {f[0] for f in funcs}
text_end = 0x020A7480
if lo not in starts:
    sys.exit(f"{lo:#x} is not a function start")
if hi not in starts and hi != text_end:
    sys.exit(f"{hi:#x} is not a function start")

delinks = open(os.path.join(ROOT, "config/arm9/delinks.txt")).read()
for m in re.finditer(r"^(\S.*):\n(?:\s+.*\n)*?\s+\.text\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)", delinks, re.M):
    a, b = int(m.group(2), 16), int(m.group(3), 16)
    if a < hi and lo < b:
        sys.exit(f"overlaps existing TU {m.group(1)} [{a:#x}, {b:#x})")
    if m.group(1) == path:
        sys.exit(f"{path} already declared")

with open(os.path.join(ROOT, "config/arm9/delinks.txt"), "a") as f:
    f.write(f"\n{path}:\n    complete\n    .text       start:{lo:#010x} end:{hi:#010x}\n")
inside = [x for x in funcs if lo <= x[0] < hi]
print(f"declared {path}: {len(inside)} functions")
for a, s, n in inside:
    print(f"  {a:#010x} {s:#06x} {n}")
