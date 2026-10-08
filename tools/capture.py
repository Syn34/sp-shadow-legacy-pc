#!/usr/bin/env python3
"""Record real calls of functions while the original game plays the boot script.

Usage:
  capture.py [NAME ...] [--tu src/game/foo.c ...] [--range LO HI] [--decompiled] [--max K]

Runs tools/bin/boot_test on baserom.nds (interpreter, with the capture hook) and
stores the CPU + memory state at entry of the 1st, 2nd, 4th, 8th... call of each
selected function in build/captures/NAME/N.bin.z. tools/difftest.py replays those
states against the original code and the decompiled C ("replay" tests).

Functions that the boot script never reaches get no captures; they are still
covered by the generated-input tests.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)


def functions():
    out = {}
    for line in open("config/arm9/symbols.txt"):
        m = re.match(r"(\S+) kind:function\(\w+,size=(0x[0-9a-f]+)[^)]*\) addr:(0x[0-9a-f]+)", line)
        if m:
            out[m.group(1)] = (int(m.group(3), 16), int(m.group(2), 16))
    return out


def tu_range(path):
    cur = None
    for line in open("config/arm9/delinks.txt"):
        if re.match(r"^\S.*:\s*$", line):
            cur = line.strip().rstrip(":")
            continue
        m = re.match(r"\s+\.text\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)", line)
        if m and cur == path:
            return int(m.group(1), 16), int(m.group(2), 16)
    sys.exit(f"{path}: no .text range in delinks.txt")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("names", nargs="*")
    ap.add_argument("--tu", action="append", default=[])
    ap.add_argument("--range", nargs=2, action="append", default=[])
    ap.add_argument("--max", type=int, default=6)
    ap.add_argument("--frames", type=int, default=5200)
    ap.add_argument("--decompiled", action="store_true", help="every function in a complete TU")
    a = ap.parse_args()
    if a.decompiled:
        cur, complete = None, False
        for line in open("config/arm9/delinks.txt"):
            if re.match(r"^\S.*:\s*$", line):
                cur, complete = line.strip().rstrip(":"), False
            elif cur and line.strip() == "complete":
                a.tu.append(cur)

    funcs = functions()
    ranges = [tu_range(t) for t in a.tu] + [(int(lo, 16), int(hi, 16)) for lo, hi in a.range]
    chosen = {n for n in a.names if n in funcs}
    for missing in set(a.names) - chosen:
        print(f"warning: unknown function {missing}")
    for name, (addr, size) in funcs.items():
        if any(lo <= addr < hi for lo, hi in ranges):
            chosen.add(name)
    if not chosen:
        sys.exit("nothing selected")

    os.makedirs("build/captures", exist_ok=True)
    for name in chosen:
        shutil.rmtree(os.path.join("build/captures", name), ignore_errors=True)
    lst = "build/capture_list.txt"
    with open(lst, "w") as f:
        for name in sorted(chosen, key=lambda n: funcs[n][0]):
            f.write(f"{funcs[name][0]:08x} {name}\n")
    tmp = "build/capture_run"
    os.makedirs(tmp, exist_ok=True)
    r = subprocess.run(["tools/bin/boot_test", "baserom.nds", tmp, "--frames", str(a.frames),
                        "--script", "tools/boot_script.txt", "--capture", lst,
                        "--capture-max", str(a.max)], capture_output=True, text=True)
    if r.returncode not in (0,):
        sys.exit(f"boot_test failed: {r.stderr}")
    got = 0
    src_root = os.path.join(tmp, "captures")
    for name in sorted(chosen, key=lambda n: funcs[n][0]):
        d = os.path.join(src_root, name)
        if os.path.isdir(d):
            shutil.move(d, os.path.join("build/captures", name))
            got += 1
    print(f"{got}/{len(chosen)} functions were called during the boot script; captures in build/captures/")


if __name__ == "__main__":
    main()
