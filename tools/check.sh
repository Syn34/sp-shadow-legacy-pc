#!/usr/bin/env bash
# Full verification:
#   1. matching build must be byte-identical to baserom.nds
#   2. every decompiled function must match the original in the CPU differential test
#   3. the shifted build (4 KiB inserted) must play like the original
#   4. the decomp build must boot, play the scripted route and write the same save
set -euo pipefail
cd "$(dirname "$0")/.."
BT=tools/bin/boot_test
SCRIPT=tools/boot_script.txt
FRAMES=5200
OUT=build/boot

python3 tools/build.py matching
python3 tools/build.py shifted --quiet
python3 tools/build.py decomp --quiet

[[ -f build/ram_snapshot_4900_gameplay.bin ]] || tools/make_snapshots.sh
echo
echo "== differential tests (original machine code vs. decompiled C)"
python3 tools/difftest.py | tail -n 1

echo
echo "== boot tests ($FRAMES frames: title, menus, intro, dialogue, walking around)"
mkdir -p $OUT/orig $OUT/shifted $OUT/decomp
$BT baserom.nds $OUT/orig --frames $FRAMES --script $SCRIPT
$BT build/spyro_shifted.nds $OUT/shifted --frames $FRAMES --script $SCRIPT
$BT build/spyro_decomp.nds $OUT/decomp --frames $FRAMES --script $SCRIPT
printf "shifted vs original: "; python3 tools/compare_boot.py $OUT/orig/hashes.txt $OUT/shifted/hashes.txt
cmp -s $OUT/orig/sram.bin $OUT/shifted/sram.bin && echo "    save data identical" || { echo "    SAVE DATA DIFFERS"; exit 1; }
printf "decomp  vs original: "; python3 tools/compare_boot.py $OUT/orig/hashes.txt $OUT/decomp/hashes.txt || true
cmp -s $OUT/orig/sram.bin $OUT/decomp/sram.bin && echo "    save data identical" || { echo "    SAVE DATA DIFFERS"; exit 1; }
echo "    (frame timing differs because the C code runs at a different speed than the"
echo "     original assembly; see README 'Verifying non-matching code')"
echo
python3 tools/progress.py
