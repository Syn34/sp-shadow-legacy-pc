#!/usr/bin/env bash
# Optional: Ghidra draft decompilation of every function into build/ghidra/
# (reading aid for tools/draft.py; never committed). Needs Ghidra 11.x + JDK 21:
#   GHIDRA=/path/to/ghidra_11.3.2_PUBLIC tools/ghidra_export.sh
set -euo pipefail
cd "$(dirname "$0")/.."
: "${GHIDRA:?set GHIDRA to the Ghidra install directory}"
[[ -f build/arm9_matching.elf ]] || python3 tools/build.py matching
rm -rf build/ghidra_proj && mkdir -p build/ghidra_proj
"$GHIDRA/support/analyzeHeadless" build/ghidra_proj spyro -import build/arm9_matching.elf \
    -scriptPath tools/ghidra -postScript ExportDecomp.java build/ghidra > build/ghidra_log.txt 2>&1
grep "ExportDecomp:" build/ghidra_log.txt
