#!/usr/bin/env bash
# One-time setup: toolchain, ds-decomp (dsd) and the boot-test harness.
# Tested on Ubuntu 24.04 (also works in WSL2 on Windows).
set -euo pipefail
cd "$(dirname "$0")/.."

DSD_COMMIT=c408063            # AetiasHax/ds-decomp, Sep 2026
MELONDS_COMMIT=906e9eb        # melonDS-emu/melonDS, Aug 2026

if [[ "${SKIP_APT:-0}" != 1 ]] && command -v apt-get >/dev/null; then
    echo "== installing packages (sudo)"
    sudo apt-get update -q
    sudo apt-get install -y -q binutils-arm-none-eabi gcc-arm-none-eabi cmake ninja-build \
        g++ git python3 python3-pip python3-pil curl zlib1g-dev
fi

echo "== python packages"
python3 -m pip install --user -q capstone unicorn pillow 2>/dev/null \
    || python3 -m pip install --user --break-system-packages -q capstone unicorn pillow

if ! command -v cargo >/dev/null; then
    echo "== installing Rust (rustup)"
    curl -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal
    source "$HOME/.cargo/env"
fi

mkdir -p tools/bin tools/_deps
if [[ ! -x tools/bin/dsd ]]; then
    echo "== building dsd ($DSD_COMMIT + local patch)"
    rm -rf tools/_deps/ds-decomp
    git clone -q https://github.com/AetiasHax/ds-decomp.git tools/_deps/ds-decomp
    git -C tools/_deps/ds-decomp checkout -q "$DSD_COMMIT"
    git -C tools/_deps/ds-decomp apply "$PWD/tools/dsd-gap-names.patch"
    (cd tools/_deps/ds-decomp && CARGO_BUILD_JOBS=${JOBS:-2} cargo build -q --profile release-fast)
    cp tools/_deps/ds-decomp/target/release-fast/dsd tools/bin/dsd
fi

if [[ ! -x tools/bin/boot_test ]]; then
    echo "== building boot_test (melonDS $MELONDS_COMMIT core)"
    if [[ ! -d tools/_deps/melonDS ]]; then
        git clone -q https://github.com/melonDS-emu/melonDS.git tools/_deps/melonDS
        git -C tools/_deps/melonDS checkout -q "$MELONDS_COMMIT"
        git -C tools/_deps/melonDS apply "$PWD/tools/boot_test/melonds-capture.patch"
    fi
    cmake -S tools/boot_test -B tools/_deps/boot_test-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DMELONDS_DIR="$PWD/tools/_deps/melonDS" >/dev/null
    ninja -C tools/_deps/boot_test-build boot_test >/dev/null
    cp tools/_deps/boot_test-build/boot_test tools/bin/boot_test
fi

echo "== done. Put your copy of the game at ./baserom.nds and run: make"
