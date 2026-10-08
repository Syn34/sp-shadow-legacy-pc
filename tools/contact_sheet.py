#!/usr/bin/env python3
"""Tile boot_test screenshots (shot_<frame>.ppm) into one PNG for quick review.

Usage: contact_sheet.py <dir> <out.png> [first_frame] [last_frame] [columns]
"""
import glob
import os
import re
import sys

from PIL import Image

d, out = sys.argv[1], sys.argv[2]
lo = int(sys.argv[3]) if len(sys.argv) > 3 else 0
hi = int(sys.argv[4]) if len(sys.argv) > 4 else 1 << 30
cols = int(sys.argv[5]) if len(sys.argv) > 5 else 6
shots = []
for f in glob.glob(os.path.join(d, "shot_*.ppm")):
    n = int(re.search(r"shot_(\d+)\.ppm", f).group(1))
    if lo <= n <= hi:
        shots.append((n, f))
shots.sort()
rows = (len(shots) + cols - 1) // cols
sheet = Image.new("RGB", (256 * cols, 400 * rows), (40, 40, 40))
from PIL import ImageDraw
draw = ImageDraw.Draw(sheet)
for i, (n, f) in enumerate(shots):
    x, y = (i % cols) * 256, (i // cols) * 400
    sheet.paste(Image.open(f), (x, y + 16))
    draw.text((x + 4, y + 2), f"frame {n}", fill=(255, 255, 0))
sheet.save(out)
print(f"{len(shots)} shots -> {out}")
