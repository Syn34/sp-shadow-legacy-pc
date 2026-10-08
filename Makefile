PYTHON ?= python3

.PHONY: all matching decomp shifted check difftest asm progress setup clean

all:
	$(PYTHON) tools/build.py both

matching decomp shifted:
	$(PYTHON) tools/build.py $@

check:
	tools/check.sh

difftest:
	@[ -f build/ram_snapshot_4900_gameplay.bin ] || tools/make_snapshots.sh
	$(PYTHON) tools/difftest.py

asm:
	tools/bin/dsd dis --config-path config/arm9/config.yaml --asm-path asm

progress:
	@$(PYTHON) tools/progress.py

setup:
	tools/setup.sh

clean:
	rm -rf build asm
