#!/usr/bin/env python3
"""Find words in the ARM9 binary that look like pointers but have no relocation.

When code or data moves (any non-matching build), every pointer must be a
relocation, or it keeps pointing at the old address. This lists aligned 32-bit
values inside [lo, hi) that dsd did not record in relocs.txt.

Usage: find_unrelocated.py [lo] [hi]     (default: whole main module + BSS)
"""
import bisect
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE = 0x02000000
binary = open(os.path.join(ROOT, "extract/arm9/arm9.bin"), "rb").read()

lo = int(sys.argv[1], 16) if len(sys.argv) > 1 else BASE
hi = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x020FDF80

relocs = set()
for line in open(os.path.join(ROOT, "config/arm9/relocs.txt")):
    m = re.match(r"from:(0x[0-9a-f]+)", line)
    if m:
        relocs.add(int(m.group(1), 16))

# where are we (section + function)?
sections = []
for line in open(os.path.join(ROOT, "config/arm9/delinks.txt")):
    m = re.match(r"\s+(\.\w+)\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+) kind", line)
    if m:
        sections.append((int(m.group(2), 16), int(m.group(3), 16), m.group(1)))
funcs = []
for line in open(os.path.join(ROOT, "config/arm9/symbols.txt")):
    m = re.match(r"(\S+) kind:function\(\w+,size=(0x[0-9a-f]+)[^)]*\) addr:(0x[0-9a-f]+)", line)
    if m:
        funcs.append((int(m.group(3), 16), int(m.group(2), 16), m.group(1)))
funcs.sort()
starts = [f[0] for f in funcs]


def where(addr):
    sec = next((s[2] for s in sections if s[0] <= addr < s[1]), "?")
    i = bisect.bisect_right(starts, addr) - 1
    fn = ""
    if i >= 0 and funcs[i][0] <= addr < funcs[i][0] + funcs[i][1]:
        fn = funcs[i][2]
    return sec, fn


count = 0
for off in range(0, len(binary) - 3, 4):
    addr = BASE + off
    if addr in relocs:
        continue
    v = struct.unpack_from("<I", binary, off)[0]
    if lo <= v < hi:
        sec, fn = where(addr)
        print(f"{addr:#010x} {sec:9} value {v:#010x} {fn}")
        count += 1
print(f"{count} candidate unrelocated pointers", file=sys.stderr)
