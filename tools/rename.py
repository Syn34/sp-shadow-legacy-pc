#!/usr/bin/env python3
"""Give a symbol a real name in config/*/symbols.txt.

Usage: rename.py <old-name-or-address> <new-name> [<old> <new> ...]

Every delinked object and the disassembly pick the new name up on the next
build (`dsd delink` / `dsd dis` read symbols.txt).
"""
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
files = glob.glob(os.path.join(ROOT, "config", "**", "symbols.txt"), recursive=True)
args = sys.argv[1:]
if not args or len(args) % 2:
    sys.exit(__doc__)

contents = {f: open(f).read() for f in files}
for old, new in zip(args[::2], args[1::2]):
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_$.@]*", new):
        sys.exit(f"bad name {new!r}")
    if re.fullmatch(r"(0x)?[0-9a-fA-F]{8}", old):
        pat = re.compile(r"^(\S+)( kind:\S+ addr:0x" + old[-8:].lower() + r"\b)", re.M)
    else:
        pat = re.compile(r"^(" + re.escape(old) + r")( kind:)", re.M)
    hits = 0
    for f in files:
        if re.search(r"^" + re.escape(new) + r" kind:", contents[f], re.M):
            sys.exit(f"{new} already exists in {os.path.relpath(f, ROOT)}")
        contents[f], n = pat.subn(lambda m: new + m.group(2), contents[f])
        hits += n
    if hits != 1:
        sys.exit(f"{old}: expected exactly one match, found {hits}")
    print(f"{old} -> {new}")
for f, c in contents.items():
    open(f, "w").write(c)
