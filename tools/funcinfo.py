#!/usr/bin/env python3
"""Per-function summary of the ARM9 main module, for picking decompilation targets.

Reads config/arm9/symbols.txt + relocs.txt and the extracted arm9.bin, and prints
one line per function: address, size, mode, number of calls, callees, whether
it touches hardware I/O (0x04xxxxxx literals), and a name.

Usage: funcinfo.py [--leaf] [--max-size N] [--min-size N] [--range LO HI] [--name REGEX]
"""
import argparse
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load_symbols(path):
    funcs, names = [], {}
    for line in open(path):
        m = re.match(r"(\S+) kind:function\((\w+),size=(0x[0-9a-f]+)[^)]*\) addr:(0x[0-9a-f]+)", line)
        if m:
            name, mode, size, addr = m.group(1), m.group(2), int(m.group(3), 16), int(m.group(4), 16)
            funcs.append((addr, size, mode, name))
            names[addr] = name
    funcs.sort()
    return funcs, names


def load_relocs(path):
    rel = {}
    for line in open(path):
        m = re.match(r"from:(0x[0-9a-f]+) kind:(\w+) to:(0x[0-9a-f]+)", line)
        if m:
            rel[int(m.group(1), 16)] = (m.group(2), int(m.group(3), 16))
    return rel


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--leaf", action="store_true")
    ap.add_argument("--max-size", type=lambda x: int(x, 0), default=1 << 30)
    ap.add_argument("--min-size", type=lambda x: int(x, 0), default=0)
    ap.add_argument("--range", nargs=2, type=lambda x: int(x, 0))
    ap.add_argument("--name")
    a = ap.parse_args()

    funcs, names = load_symbols(os.path.join(ROOT, "config/arm9/symbols.txt"))
    rel = load_relocs(os.path.join(ROOT, "config/arm9/relocs.txt"))
    binary = open(os.path.join(ROOT, "extract/arm9/arm9.bin"), "rb").read()
    base = 0x02000000

    for addr, size, mode, name in funcs:
        if not (a.min_size <= size <= a.max_size):
            continue
        if a.range and not (a.range[0] <= addr < a.range[1]):
            continue
        if a.name and not re.search(a.name, name):
            continue
        calls, io = [], False
        for off in range(addr, addr + size, 4 if mode == "arm" else 2):
            r = rel.get(off)
            if r and r[0] in ("arm_call", "thumb_call", "arm_call_thumb", "thumb_call_arm"):
                calls.append(names.get(r[1], hex(r[1])))
        if base <= addr < base + len(binary):
            for off in range(addr - base, addr - base + size - 3, 4):
                w = struct.unpack_from("<I", binary, off)[0]
                if 0x04000000 <= w < 0x04800000:
                    io = True
        if a.leaf and calls:
            continue
        print(f"{addr:#010x} {size:#06x} {mode:5} calls={len(calls):<3} io={'Y' if io else '-'} {name}"
              + (f"  -> {', '.join(sorted(set(calls))[:6])}" if calls else ""))


if __name__ == "__main__":
    main()
