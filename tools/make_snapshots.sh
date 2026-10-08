#!/usr/bin/env bash
# Record memory snapshots of the original game (title screen, intro, gameplay)
# for tools/difftest.py. Deterministic: the same ROM always gives the same files.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/snap
tools/bin/boot_test baserom.nds build/snap --frames 4901 --script tools/boot_script.txt \
    --dump-ram 1600,3300,4900
mv build/snap/ram_1600.bin build/ram_snapshot_1600_title.bin
mv build/snap/ram_3300.bin build/ram_snapshot_3300_intro.bin
mv build/snap/ram_4900.bin build/ram_snapshot_4900_gameplay.bin
echo "snapshots written to build/ram_snapshot_*.bin"
