# Spyro: Shadow Legacy (DS) decompilation

A decompilation project for **Spyro: Shadow Legacy** (Nintendo DS, USA, game code `ASSE`), aimed
at an eventual native PC port.

This repository holds **no game code or data**. It holds the analysis (function boundaries,
symbols, relocations), the decompiled C, and the tools. You supply your own copy of the game, and
everything else is built from it.

## Status

| Milestone | State |
|---|---|
| ROM extracted, rebuilt **byte-identical** | done (`sha1 6c36f7fc66999f5f31781163270a7b56efc88a41`) |
| ARM9 code split into functions | done: **3,682 functions**, ~695 KB of code, with 11,833 data pointers and 13,783 calls/branches recorded as relocations |
| Matching build from relocatable objects with the free GNU toolchain | done: byte-identical |
| Code can move ("shiftable") | done: 4 KiB inserted near the start of the code still plays the test route with an identical save |
| Decompiled to C | in progress, batch by batch: see **[PROGRESS.md](PROGRESS.md)**. Every decompiled function is verified against the original machine code |
| Native PC port | not started; see [Road to a PC port](#road-to-a-pc-port) |

### What you have and what you don't

This is the foundation, and the long part is still ahead. A decompilation means rewriting every
function as source code; this game has about 3,700 functions. Teams working on comparable DS games
have spent years on it. Even a finished decomp still builds a **DS ROM**. Before it runs natively
on PC, everything that talks to DS hardware (2D/3D graphics, sound, touch screen, cartridge file
system) has to be reimplemented for PC. That is how the Super Mario 64 PC port happened: the
decomp was finished first, and the port was built on top of it.

What's here already removes the hardest parts of getting started:

* the game splits into relocatable pieces, so you can replace any function with C and rebuild
* every pointer is a relocation, so replaced code may be any size
* you can check every decompiled function automatically against the original

## Requirements

Linux, or Windows through WSL2 (Ubuntu 24.04 tested). `tools/setup.sh` installs everything:

* `arm-none-eabi` GCC/binutils (compiler and linker for the DS's ARM9 CPU)
* [ds-decomp (`dsd`)](https://github.com/AetiasHax/ds-decomp) at commit `c408063`, plus a one-line
  patch (`tools/dsd-gap-names.patch`) that lets its linker-script generator handle more than one
  undecompiled gap per module
* the [melonDS](https://github.com/melonDS-emu/melonDS) emulation core at `906e9eb`, built into
  `tools/bin/boot_test`, a headless test runner (GPLv3)
* Python 3 with `capstone`, `unicorn` and `pillow`

```sh
git clone https://github.com/Syn34/sp-shadow-legacy-pc.git
cd sp-shadow-legacy-pc
tools/setup.sh                      # once, about 5 minutes
cp /path/to/your/spyro.nds baserom.nds
make                                # matching + decomp ROMs
make check                          # full verification, about 2 minutes
```

## Builds

`python3 tools/build.py [matching|decomp|shifted|both]`

| Build | Output | What it is |
|---|---|---|
| `matching` | `build/spyro_matching.nds` | Every function from the original machine code, delinked into ELF objects and relinked with GNU `ld`. **Must equal `baserom.nds` byte for byte**; this proves the split, symbols and relocations are right. |
| `decomp` | `build/spyro_decomp.nds` | Translation units marked `complete` are compiled from `src/` with GCC. Not byte-identical. It must play the same. |
| `shifted` | `build/spyro_shifted.nds` | The matching build with 4 KiB of padding inserted mid-code. If it still plays the same, nothing depends on code staying at its original address. |

The pipeline:
`dsd delink` → `tools/normalize_obj.py` → (GCC for `src/`) → `dsd lcf` → `tools/lcf2ld.py` →
`arm-none-eabi-ld` → `objcopy` → `dsd rom config/build` → `tools/fix_header.py`

## Layout

```
config/arm9/            the analysis, which is the heart of the project
  symbols.txt           every function/data symbol: name, address, ARM/Thumb, size
  relocs.txt            every pointer and branch (what must be fixed up when code moves)
  delinks.txt           sections + translation units (which address ranges are decompiled)
  itcm/ dtcm/           same for the two autoload modules (fast on-chip memory)
src/                    decompiled C, one file per translation unit
include/                shared headers
tools/
  build.py              the build
  difftest.py           CPU-level differential tester (original code vs. C)
  boot_test/            headless melonDS runner with scripted input (frame hashes, screenshots, RAM dumps)
  boot_script.txt       5,200-frame input script: boot, title, save slot, intro, dialogue, walking
  compare_boot.py       compares two boot_test runs
  normalize_obj.py      adapts dsd's objects for GNU ld (see technical notes)
  lcf2ld.py             converts dsd's Metrowerks linker script to a GNU ld script
  find_unrelocated.py   lists pointer-like words that have no relocation
  funcinfo.py, show.py  find and read functions
  rename.py             give a symbol a real name
  progress.py           progress report
```

`make asm` writes a disassembly of the whole game to `asm/` for reading. It is generated from your
ROM and is not part of the repository.

## Decompiling a function

1. **Pick a range.** A translation unit (TU) is a contiguous address range of whole functions.
   The work proceeds through the code in address order:
   `python3 tools/funcinfo.py --range 0x02040000 0x02041000`
2. **Read it.** `python3 tools/draft.py 0x02040abc 0x02040b20 [--asm]` prints, per function, a
   Ghidra draft (literal-pool values filled in, how often the boot script calls it) and
   optionally the disassembly. Drafts come from `tools/ghidra_export.sh` (optional, needs
   Ghidra 11 + JDK 21). Treat them as hints: Ghidra often misses pass-through arguments and
   return values, so check the assembly (`make asm`, `tools/show.py`).
3. **Declare the TU**: `python3 tools/new_tu.py src/game/MyFile.c 0x02040abc 0x02040b20`
4. **Write the C.** Every function in the range must be defined. Include `game.h`; declare
   callees in `include/functions.h` and data in `include/data.h` (one declaration each, so
   all callers agree). Functions the boot script reaches are verified automatically from
   recordings; for the others add `@difftest` lines (see the top of `tools/difftest.py`):
   ```c
   /* @difftest ptr:64:4 s32 */
   fx32 func_02040abc(const fx32 *vec, s32 index) { ... }
   ```
5. **Name things** when you know what they are: `python3 tools/rename.py func_02040abc Vec_Get`.
6. **Verify**: `python3 tools/capture.py --tu src/game/MyFile.c` (records real calls), then
   `python3 tools/difftest.py src/game/MyFile.c`, and finally `make check`.

## Verifying non-matching code

The C is compiled with GCC, not the original Metrowerks compiler, so the output is not
byte-identical. Three checks stand in for byte-matching:

**1. Differential CPU testing** (`tools/difftest.py`). Each decompiled function and its original
machine code run side by side in Unicorn (ARMv5 CPU emulator) from the same starting state, and
afterwards the return value, the callee-saved registers and **every byte of memory** must be
identical. Calls into not-yet-decompiled code execute the original code in both runs. The tester
also counts 8-bit writes to VRAM, because the DS hardware drops them. Starting states come from
two sources:

* **Recorded game calls.** `tools/capture.py` plays the boot script on the original game with a
  hook in the emulator's CPU loop and saves the complete CPU and memory state at entry to the
  1st, 2nd, 4th, 8th... call of each function. Replaying those gives each function the exact
  inputs the game really uses. The boot script reaches about half of all functions.
* **Generated inputs** for everything else, described by `@difftest` annotations, starting
  from memory snapshots of the running game (title screen, intro, gameplay).

The test run also lists any decompiled function that neither source covers.

This catches subtle bugs. In the first version of `MI_CpuFill8`, GCC merged a deliberate 16-bit
read-modify-write into a single byte store. That is legal C, but on a DS it silently does nothing
when the target is VRAM. The VRAM check failed, and the fix (`volatile` halfword access) is
documented in the source.

**2. Shift test** (`make shifted` + boot test). The original code with 4 KiB inserted before the
second translation unit, which moves nearly the whole program, all data and the heap. The scripted run differs
from the original in 11 of 5,200 frames (a sparkle drawn at a slightly different moment), then
reconverges, and the save data is byte-identical. Getting there required adding relocations that
automatic analysis missed (see technical notes). Without them, the shifted game crashed at the
title screen.

**3. Boot test of the decomp build.** The decomp ROM plays the full scripted route (boot, menus,
new game, intro cutscene, dialogue, walking around Spyro's home). It writes a **byte-identical save
file** and ends at the same spot (the final frame differs by a 2×3-pixel animation detail).
Individual frames diverge, because the C runs at a different speed than the hand-written original
assembly, and this game seeds its `rand()` from a timing-dependent counter. For scale: the
*unmodified* original ROM, run under melonDS's JIT versus its interpreter (two timing models),
diverges in 41% of frames. The decomp build diverges in roughly half of the frames (55% as of batch 2).

## Technical notes

* **Toolchain.** The game was built with Metrowerks CodeWarrior for NITRO (C++, with exception
  tables), on NitroSDK 2.0-era libraries (SDK version word `0x02004FB1`). This project links with
  GNU `ld` instead of the proprietary `mwldarm`. `tools/normalize_obj.py` makes dsd's objects
  link byte-identically:
  * re-sorts the symbol table
  * marks Thumb functions so BL/BLX are chosen correctly
  * moves `R_ARM_ABS32` addends in place, because GNU ld's ARM backend reads them from the
    relocation site and ignores the RELA field
  * converts `R_ARM_PC24` to `R_ARM_CALL`/`R_ARM_JUMP24`
  * sets the EABI version
* **Relocations added by hand.** Automatic analysis missed these, and `tools/find_unrelocated.py`
  found them:
  * `0x02093af4`, `0x02000b10`: the OS arena start, the end of static memory (`__CODE_HI`). The
    heap begins there. Unrelocated, the heap overlapped moved BSS and the game crashed.
  * `0x02093afc`: the ITCM arena start (`__ITCM_HI`).
  * `0x0202285c..64` and ITCM `0x01ff8174..7c`: pointers into a data table embedded in the code
    of the decompressor at `0x02000f08`.
* **Secure-area CRC.** The header's secure-area CRC needs the DS BIOS key table to compute. It is
  restored from the original when the first 16 KiB of code are unchanged. Otherwise it is left
  at 0, which emulators and flashcarts ignore.
* **Hand-written assembly** at `0x02000df4`-`0x02001930` (a decompressor and audio filter
  routines that pass values in non-standard registers) stays as original code.
* **No LDRD/STRD.** C is compiled for ARMv5T instead of the CPU's ARMv5TE: the game's code only
  keeps 4-byte alignment, and doubleword loads/stores need 8. 64-bit fields are accessed as two
  words (`Get64`/`Put64` in `include/game.h`).
* **Debug strings** left in the game name some original source files (`TextActor.cpp`,
  `Actor.cpp`, ... about 35 in total); TUs are named after them where known.
* **ABI.** GCC uses AAPCS and the original uses ATPCS. They agree for 32-bit integers and
  pointers. Watch out for 64-bit arguments (register-pair alignment) and structs passed or
  returned by value.

## Road to a PC port

1. **Decompile the game logic**, translation unit by translation unit. Most of it is C++, so
   classes and virtual calls need modelling. `game_root.c` shows the pattern.
2. **Isolate the hardware.** Game code reaches the DS hardware almost entirely through NitroSDK
   and NitroSystem calls: GX/G3 (3D), G2/OAM (2D), DMA, FS (cartridge files), PAD/TP (input),
   SND (sound commands to the ARM7), OS (threads, interrupts, ticks). Name and catalogue those
   calls as you go.
3. **Write a PC platform layer** that implements those APIs: draw the 2D layers and sprites and
   translate the 3D command stream to OpenGL/Vulkan, read files from the user's ROM, play the
   SDAT sound banks, and map keyboard, mouse and controller input.
4. **Compile the decompiled game natively** against that layer. The DS build stays useful
   throughout as a reference to test against.

## Legal

The game, its code and its assets are © their respective owners. This repository contains no
part of the ROM. Generated files (`extract/`, `asm/`, `build/`) come from your own copy and must
not be redistributed. Decompiled code is an independent reimplementation for interoperability and
preservation. Tool licences: ds-decomp (MIT), melonDS (GPLv3, used only for the test runner).
