#!/usr/bin/env python3
"""Report decompilation progress: functions and code bytes covered by `complete` TUs."""
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

funcs = []
for line in open(os.path.join(ROOT, "config/arm9/symbols.txt")):
    m = re.match(r"(\S+) kind:function\(\w+,size=(0x[0-9a-f]+)[^)]*\) addr:(0x[0-9a-f]+)", line)
    if m:
        funcs.append((int(m.group(3), 16), int(m.group(2), 16), m.group(1)))

done_ranges, tus = [], []
cur, complete = None, False
for line in open(os.path.join(ROOT, "config/arm9/delinks.txt")):
    if re.match(r"^\S.*:\s*$", line):
        cur, complete = line.strip().rstrip(":"), False
        continue
    if cur and line.strip() == "complete":
        complete = True
        tus.append(cur)
    m = re.match(r"\s+\.text\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)", line)
    if cur and complete and m:
        done_ranges.append((int(m.group(1), 16), int(m.group(2), 16)))

done = [f for f in funcs if any(lo <= f[0] < hi for lo, hi in done_ranges)]
named = [f for f in funcs if not f[2].startswith("func_")]
total_bytes = sum(f[1] for f in funcs)
done_bytes = sum(f[1] for f in done)
print(f"translation units decompiled : {len(tus)}")
for t in tus:
    desc = ""
    src = os.path.join(ROOT, t)
    if os.path.exists(src):
        m = re.search(r"/\*\s*\n?\s*\*?\s*(.+?)\n", open(src).read())
        if m:
            desc = m.group(1).strip().rstrip(".")
    print(f"    {t:<28}{desc}")
print(f"functions decompiled         : {len(done)} / {len(funcs)} ({100 * len(done) / len(funcs):.2f}%)")
print(f"code bytes decompiled        : {done_bytes} / {total_bytes} ({100 * done_bytes / total_bytes:.2f}%)")
print(f"functions with real names    : {len(named)} / {len(funcs)}")
