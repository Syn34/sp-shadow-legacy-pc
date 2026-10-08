#!/usr/bin/env python3
"""Build Spyro: Shadow Legacy (USA) from this repository.

Pipeline (all steps re-run each time; the whole build takes a few seconds):

  baserom.nds --dsd rom extract--> extract/            (assets, header, banner)
  config/     --dsd delink-------> build/delinks/*.o   (not-yet-decompiled code)
  src/*.c     --arm-none-eabi-gcc> build/src/*.o       (decompiled code)
  config/     --dsd lcf----------> build/arm9.lcf      (link order)
              --tools/lcf2ld.py--> build/arm9.ld       (GNU ld script)
  objects     --arm-none-eabi-ld-> build/arm9.elf
              --objcopy----------> build/build/{arm9,itcm,dtcm}.bin
              --dsd rom config/build + tools/fix_header.py--> build/spyro.nds

Two flavours are built from the same configuration:

  matching  every translation unit comes from the original machine code
            (decompiled ones included). Must reproduce baserom.nds exactly;
            this proves the split, symbols and relocations are right.
  decomp    translation units marked `complete` in config/arm9/delinks.txt are
            compiled from src/ with GCC. Not byte-identical, but must play
            the same (see tools/boot_test and tools/difftest.py).
  shifted   the matching build with 4 KiB of padding inserted in the middle of
            the code. Proves every pointer is a relocation: if the game still
            plays identically, code can grow or shrink freely (needed for any
            non-matching work, including a PC port).

Usage: python3 tools/build.py [matching|decomp|shifted|both] [--quiet]   (default: both)
"""
import hashlib
import json
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

BASEROM = "baserom.nds"
BASEROM_SHA1 = "6c36f7fc66999f5f31781163270a7b56efc88a41"
CONFIG = "config/arm9/config.yaml"
OUT_ROMS = {"matching": "build/spyro_matching.nds", "decomp": "build/spyro_decomp.nds",
            "shifted": "build/spyro_shifted.nds"}
SHIFT_BYTES = 0x1000  # padding inserted by the "shifted" build

DSD = os.environ.get("DSD") or (
    "tools/bin/dsd" if os.path.exists("tools/bin/dsd") else shutil.which("dsd") or "dsd")
CROSS = os.environ.get("CROSS", "arm-none-eabi-")
CC = CROSS + "gcc"
LD = CROSS + "ld"
OBJCOPY = CROSS + "objcopy"

CFLAGS = [
    # ARMv5T rather than the CPU's real ARMv5TE: v5T has no LDRD/STRD, which need
    # 8-byte alignment on the ARM946E-S while the game's (ATPCS) code only keeps
    # 4-byte alignment for stack and data. Long multiplies are still available.
    "-march=armv5t", "-mtune=arm946e-s", "-marm", "-mthumb-interwork", "-mfloat-abi=soft",
    "-O2", "-std=gnu11", "-ffreestanding", "-fno-builtin", "-nostdlib",
    "-fno-common", "-fno-strict-aliasing", "-fshort-wchar",
    # signed overflow wraps, as it does in the original code
    "-fwrapv",
    "-fno-unwind-tables", "-fno-asynchronous-unwind-tables",
    # never turn copy/fill loops into memcpy/memset calls: there is no libc to
    # link against, and the SDK copy routines must keep their 16/32-bit accesses
    "-fno-tree-loop-distribute-patterns",
    # no tail calls: a tail call that passes arguments on the stack reuses the
    # caller's outgoing argument area, which the original (MWCC) callers may
    # still rely on after the call
    "-fno-optimize-sibling-calls",
    # no switch-to-lookup-table conversion: only the C objects' .text is linked
    # (the game's own data stays where it is), so C code must not need .rodata
    "-fno-tree-switch-conversion",
    "-Wall", "-Wextra", "-Wno-unused-parameter", "-Werror=implicit-function-declaration",
    "-Iinclude",
]

QUIET = "--quiet" in sys.argv


def log(msg):
    if not QUIET:
        print(msg, flush=True)


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode != 0:
        sys.stderr.write(f"\n$ {' '.join(cmd)}\n{r.stdout}{r.stderr}")
        sys.exit(f"build failed at: {cmd[0]}")
    return r


def sha1(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def prepare():
    if not os.path.exists(BASEROM):
        sys.exit(f"Put your own copy of the game at ./{BASEROM} (see README).")

    # 1. extract assets from the base ROM (once)
    if not os.path.exists("extract/config.yaml"):
        got = sha1(BASEROM)
        if got != BASEROM_SHA1:
            sys.exit(f"{BASEROM} has SHA1 {got}, expected {BASEROM_SHA1} (Spyro: Shadow Legacy USA)")
        log("[1/7] extracting base ROM")
        run([DSD, "rom", "extract", "--rom", BASEROM, "--output-path", "extract"])
    else:
        log("[1/7] base ROM already extracted")

    # 2. delink everything (complete TUs too, for the matching build)
    log("[2/7] delinking")
    if os.path.isdir("build/delinks"):
        shutil.rmtree("build/delinks")
    run([DSD, "delink", "--config-path", CONFIG])

    # 3. link order
    log("[3/7] generating link order")
    run([DSD, "lcf", "--config-path", CONFIG])
    return [l.strip().strip('"') for l in open("build/objects.txt") if l.strip()]


def aeabi_map():
    """GCC runtime helper -> game routine (tools/aeabi.txt)."""
    out = {}
    for line in open("tools/aeabi.txt"):
        line = line.split("#")[0].split()
        if len(line) == 2:
            out[line[0]] = line[1]
    return out


def aeabi_defsyms():
    return [f"--defsym={k}={v}" for k, v in aeabi_map().items()]


def insert_padding(ld_path, nbytes):
    """Insert padding before the second .text input of the ARM9 module."""
    lines = open(ld_path).read().split("\n")
    seen = 0
    for i, l in enumerate(lines):
        if l.strip().endswith("(.text)") and '"' in l:
            seen += 1
            if seen == 2:
                indent = l[:len(l) - len(l.lstrip())]
                lines.insert(i, f"{indent}. += {nbytes:#x};  /* shift test */")
                break
    else:
        sys.exit("shift test needs at least two translation units in .text")
    open(ld_path, "w").write("\n".join(lines))


def build(mode, objects):
    log(f"--- {mode} build ---")
    # 4. prepare objects: normalize delinked ones, compile decompiled sources
    log("[4/7] compiling sources / normalizing delinked objects")
    link_objs = []
    n_src = 0
    for obj in objects:
        rel = os.path.relpath(obj, "build")
        if not rel.startswith("delinks" + os.sep) and mode in ("matching", "shifted"):
            obj = os.path.join("build", "delinks", rel)   # use the original code
            rel = os.path.relpath(obj, "build")
        if rel.startswith("delinks" + os.sep):
            out = os.path.join("build/norm", os.path.relpath(obj, "build/delinks"))
            os.makedirs(os.path.dirname(out), exist_ok=True)
            run([sys.executable, "-I", "tools/normalize_obj.py", obj, out])
            link_objs.append(out)
        else:
            base = os.path.splitext(rel)[0]
            srcs = [base + ext for ext in (".c", ".s") if os.path.exists(base + ext)]
            if not srcs:
                sys.exit(f"complete TU {rel} has no source file ({base}.c)")
            os.makedirs(os.path.dirname(obj), exist_ok=True)
            run([CC, *CFLAGS, "-c", srcs[0], "-o", obj])
            link_objs.append(obj)
            n_src += 1
    log(f"      {n_src} translation unit(s) from C, {len(link_objs) - n_src} from original code")

    # 5. link with GNU ld
    log("[5/7] linking")
    with open("build/objects_link.txt", "w") as f:
        f.write("\n".join(f'"{o}"' for o in link_objs) + "\n")
    run([sys.executable, "-I", "tools/lcf2ld.py", "build/arm9.lcf", "build/objects_link.txt", "build/arm9.ld"])
    if mode == "shifted":
        insert_padding("build/arm9.ld", SHIFT_BYTES)
    run([LD, "--use-blx", "--no-warn-rwx-segments", "-T", "build/arm9.ld", *aeabi_defsyms(),
         "-Map", "build/arm9.map", "-o", "build/arm9.elf", *link_objs])

    # 6. binaries + ROM config
    log("[6/7] extracting module binaries")
    os.makedirs("build/build", exist_ok=True)
    for region, name in (("ARM9", "arm9"), ("ITCM", "itcm"), ("DTCM", "dtcm")):
        run([OBJCOPY, "-O", "binary", "-j", region, "build/arm9.elf", f"build/build/{name}.bin"])
    run([DSD, "rom", "config", "--elf", "build/arm9.elf", "--config", CONFIG])

    # 7. ROM
    log("[7/7] building ROM")
    out_rom = OUT_ROMS[mode]
    run([DSD, "rom", "build", "--config", "build/build/rom_config.yaml", "--rom", out_rom])
    r = run([sys.executable, "-I", "tools/fix_header.py", out_rom])
    log("      " + r.stdout.strip())
    shutil.copy("build/arm9.elf", f"build/arm9_{mode}.elf")
    shutil.copy("build/arm9.map", f"build/arm9_{mode}.map")

    got = sha1(out_rom)
    if mode == "shifted":
        print(f"OK   {out_rom}: original code with {SHIFT_BYTES:#x} bytes inserted; "
              "run the boot test to confirm it still plays identically")
    elif mode == "matching":
        if got == BASEROM_SHA1:
            print(f"OK   {out_rom}: byte-identical to the original game (sha1 {got})")
        else:
            print(f"FAIL {out_rom}: sha1 {got} does not match the original {BASEROM_SHA1}")
            sys.exit(1)
    else:
        same = "identical to the original" if got == BASEROM_SHA1 else "non-matching (expected: C compiled with GCC)"
        print(f"OK   {out_rom}: built from {n_src} decompiled translation unit(s), {same}")


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    modes = ["matching", "decomp"] if not args or args[0] == "both" else [args[0]]
    objects = prepare()
    for m in modes:
        build(m, objects)


if __name__ == "__main__":
    main()
