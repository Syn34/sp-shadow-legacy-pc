#!/usr/bin/env python3
"""Call an original game function in the CPU emulator and print r0/r1.

Handy for identifying library routines (e.g. soft-float helpers):
    callfn.py func_020a324c 0x3f800000 0x40000000      -> r0=0x40400000 (1.0 + 2.0)

Arguments are integers (hex or decimal) or "f:1.5" for an IEEE single.
Runs from the gameplay memory snapshot.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import difftest as d  # noqa: E402

name, raw = sys.argv[1], sys.argv[2:]
syms, modes = d.original_symbols()
addr = syms.get(name)
if addr is None:
    addr = int(name, 16)
args = []
for a in raw:
    if a.startswith("f:"):
        args.append(struct.unpack("<I", struct.pack("<f", float(a[2:])))[0])
    else:
        args.append(int(a, 0) & 0xFFFFFFFF)
snap = sorted(p for p in os.listdir(os.path.join(d.ROOT, "build")) if p.startswith("ram_snapshot"))[-1]
m = d.Machine(open(os.path.join(d.ROOT, "build", snap), "rb").read(), b"")
fault, regs, _ = m.call(addr & ~1, modes.get(name) == "thumb", args, bytes(d.SCRATCH_SIZE))
r0, r1 = regs[0], regs[1]
asf = struct.unpack("<f", struct.pack("<I", r0))[0]
print(f"r0={r0:#010x} ({r0 if r0 < 1 << 31 else r0 - (1 << 32)}, as float {asf:g})  r1={r1:#010x}"
      + (f"  FAULT: {fault}" if fault else ""))
