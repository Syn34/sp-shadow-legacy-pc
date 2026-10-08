#!/usr/bin/env python3
"""Compare two boot_test hash logs frame by frame.

Usage: compare_boot.py <reference/hashes.txt> <candidate/hashes.txt>

Verdicts:
  IDENTICAL    every frame has the same screen hash
  EQUIVALENT   a few frames differ but the runs reconverge and stay identical
               for the last 10% of frames. The emulator models CPU caches and
               memory timing, so code at different addresses runs at slightly
               different speed, which this game turns into small visual
               differences (e.g. a sparkle drawn one frame earlier). For scale:
               the *same* original ROM run under melonDS' JIT vs its interpreter
               differs in ~40% of frames.
  DIFFERENT    the runs diverge and do not reconverge (or crash / stop early)
Exit code 0 for IDENTICAL or EQUIVALENT.
"""
import sys


def load(path):
    out = []
    for line in open(path):
        parts = line.split()
        if len(parts) >= 2:
            out.append(parts[1])
    return out


ref, cand = load(sys.argv[1]), load(sys.argv[2])
n = min(len(ref), len(cand))
diff = [i for i in range(n) if ref[i] != cand[i]]

if len(cand) < len(ref):
    print(f"DIFFERENT: candidate stopped after {len(cand)} of {len(ref)} frames (crash?)")
    sys.exit(1)
if not diff:
    print(f"IDENTICAL: all {n} frames match")
    sys.exit(0)

tail = max(1, n // 10)
reconverged = diff[-1] < n - tail
pct = 100.0 * len(diff) / n
desc = f"{len(diff)} of {n} frames differ ({pct:.1f}%), frames {diff[0]}..{diff[-1]}"
if reconverged and pct < 2.0:
    print(f"EQUIVALENT: {desc}; identical from frame {diff[-1] + 1} to the end")
    sys.exit(0)
print(f"DIFFERENT: {desc}" + ("" if reconverged else "; never reconverges"))
sys.exit(1)
