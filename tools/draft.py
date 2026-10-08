#!/usr/bin/env python3
"""Show Ghidra draft decompilation (and optionally disassembly) for functions.

Usage: draft.py LO HI [--asm]        all functions in [LO, HI)
       draft.py NAME|ADDR ... [--asm]

Drafts come from build/ghidra/ (create with tools/ghidra_export.sh). They are a
reading aid only: types are guessed, and they are never committed.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
funcs = []
for line in open(os.path.join(ROOT, "config/arm9/symbols.txt")):
    m = re.match(r"(\S+) kind:function\((\w+),size=(0x[0-9a-f]+)[^)]*\) addr:(0x[0-9a-f]+)", line)
    if m:
        funcs.append((int(m.group(4), 16), int(m.group(3), 16), m.group(2), m.group(1)))
funcs.sort()
by_name = {f[3]: f for f in funcs}

args = [a for a in sys.argv[1:] if not a.startswith("--")]
show_asm = "--asm" in sys.argv
sel = []
if len(args) == 2 and all(re.fullmatch(r"(0x)?[0-9a-fA-F]{8}", a) for a in args):
    lo, hi = int(args[0], 16), int(args[1], 16)
    sel = [f for f in funcs if lo <= f[0] < hi]
else:
    for a in args:
        if a in by_name:
            sel.append(by_name[a])
        else:
            v = int(a, 16)
            sel += [f for f in funcs if f[0] == v]

# literal-pool words, so `_L_xxxxxxxx` in drafts can be shown with their value
literals = {}
import glob
for f in glob.glob(os.path.join(ROOT, "asm", "*.s")):
    for m in re.finditer(r"^\.L_([0-9a-f]{8}): \.word (\S+)", open(f).read(), re.M):
        literals[m.group(1)] = m.group(2)

calls = {}
cov = os.path.join(ROOT, "build", "cov", "calls.txt")
if os.path.exists(cov):
    for line in open(cov):
        a, n, c = line.split()
        calls[n] = int(c)


def sub_literal(m):
    v = literals.get(m.group(1))
    if v is None:
        return m.group(0)
    if re.fullmatch(r"-?0x[0-9a-fA-F]+|-?\d+", v):
        return f"({v})"
    return f"(&{v})"


for addr, size, mode, name in sel:
    print(f"//==== {name} @ {addr:#010x} size {size:#x} ({mode})"
          + (f"  [called {calls[name]}x in boot script]" if name in calls else ""))
    path = os.path.join(ROOT, "build", "ghidra", f"{addr:08x}.c")
    if os.path.exists(path):
        text = open(path).read().strip()
        text = re.sub(r"\n\s*\n", "\n", text)
        text = re.sub(r"\b_L_([0-9a-f]{8})\b", sub_literal, text)
        print(text)
    if show_asm:
        out = subprocess.run([sys.executable, "-I", os.path.join(ROOT, "tools", "show.py"), name],
                             capture_output=True, text=True).stdout
        print("/* asm\n" + out + "*/")
