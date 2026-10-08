#!/usr/bin/env python3
"""Differential test: decompiled C function vs. the original machine code.

For every function in src/ that carries a `@difftest` annotation, this runs the
original ARM code and the GCC-compiled C side by side in a CPU emulator
(Unicorn), from the same starting state, with many generated inputs, and
compares everything observable afterwards:

  * return value (r0, r1) and the callee-saved registers r4-r11 and sp
  * every byte of memory (main RAM, TCMs, VRAM/IO stand-ins, scratch buffers)

The starting state is a real snapshot of the game's memory taken by
tools/boot_test (--dump-ram), so pointers into live game data are meaningful.
Calls from either version into other (not yet decompiled) functions run the
original code for those functions in both cases.

Annotation syntax (in a comment right above the function):

    /* @difftest ptr:300 u8 int:0:256 */
    void MI_CpuFill8(void *dest, u8 data, u32 size)

Argument generators, one per parameter (r0-r3, then the stack):
    u8 u16 u32 s8 s16 s32     random value of that type
    int:LO:HI                 random integer in [LO, HI)
    pick:A,B,C                one of the listed values
    ptr:SIZE[:ALIGN]          pointer to a fresh SIZE-byte buffer of random data
                              (ALIGN 1, 2 or 4; default 1 = any alignment)
    vram:SIZE[:ALIGN]         same, but the buffer lives in VRAM; 8-bit writes to
                              VRAM are counted and must match (the DS drops them)
    zero:SIZE                 pointer to a SIZE-byte buffer of zeros (e.g. an empty container)
    bytes:N:Z                 pointer to N random bytes followed by Z zero bytes (bounded streams)
    fifo                      pointer to a single word (a hardware FIFO register)
    ram:ADDR                  fixed address (e.g. a game object)
Memory setup (applied to the starting state of both runs):
    $NAME=GEN                 bind NAME to a generated value or buffer (ptr:, zero:, u32 ...)
    @TARGET:W=VALUE           write VALUE (literal, $NAME[+OFFSET] or generator) as W-bit (8/16/32)
                              to TARGET (hex address or $NAME+OFFSET)
  e.g. `$R=ptr:16:4 $O=ptr:0x400:4 @020e36a4:8=1 @020e36c8:32=$R @$R+8:32=$O`
Options after the arguments: `cases=N` per snapshot (default 300).
    `stub=ADDR[,ADDR...]` makes those functions return at once (r0 unchanged) in
    both runs, the C copy too if one exists. Only for callees that cannot run
    here (card/file I/O, IPC or save waits, halt; operator delete on a scratch
    buffer), so that the code around them can still be compared.
    `faultmem=1`: when both versions stop with the same fault, compare memory
    anyway (for code that runs a long way before an unavoidable fault).
A function may carry several @difftest lines (e.g. RAM and VRAM pointers).

Usage: difftest.py [function-name | src/file.c ...] [--snapshot ram.bin ...] [--cases N] [--max-cases N]
                   [--replay-only | --no-replay]

Every function defined in src/ that was called during the boot script (and has
recordings in build/captures/, see tools/capture.py) is also replayed with the
exact CPU and memory state of those real calls.
"""
import argparse
import glob
import os
import random
import re
import struct
import subprocess
import sys
import tempfile
import zlib

from unicorn import Uc, UcError, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_MEM_UNMAPPED, UC_HOOK_INTR, UC_HOOK_MEM_WRITE, UC_HOOK_CODE
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4,
                               UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8, UC_ARM_REG_R9,
                               UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_R12, UC_ARM_REG_SP, UC_ARM_REG_LR,
                               UC_ARM_REG_PC, UC_ARM_REG_CPSR, UC_CPU_ARM_926)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CROSS = os.environ.get("CROSS", "arm-none-eabi-")
sys.path.insert(0, os.path.join(ROOT, "tools"))
from build import CFLAGS  # noqa: E402  (same flags as the real build)

MAIN, MAIN_SIZE = 0x02000000, 0x400000
ITCM, ITCM_SIZE = 0x01FF8000, 0x8000
DTCM, DTCM_SIZE = 0x027C0000, 0x4000
# the last 4 KiB of main RAM (system area: 0x027ffc00...) as the SDK reads it,
# through the 0x027xxxxx mirror; a copy, so writes do not reach the original
SYSAREA, SYSAREA_SIZE = 0x027FF000, 0x1000
EXTRA = [(0x04000000, 0x200000), (0x05000000, 0x1000), (0x06000000, 0x1000000), (0x07000000, 0x1000)]
SCRATCH, SCRATCH_SIZE = 0x0B000000, 0x100000      # argument buffers
VRAM_BUF = 0x06000000                              # `vram:` argument buffers (64 KiB)
CAND, CAND_SIZE = 0x0C000000, 0x100000            # candidate code
STACK, STACK_SIZE = 0x0D000000, 0x10000
MAGIC_LR = 0x0E000000                              # return address sentinel
MAX_INSNS = 5_000_000

REGS = [UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3]
SAVED = [UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8,
         UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_SP]


def sh(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"$ {' '.join(cmd)}\n{r.stdout}{r.stderr}")
    return r.stdout


# ---------------------------------------------------------------- annotations
def find_tests(names):
    tests = []
    sig = re.compile(r"/\*((?:[^*]|\*(?!/))*@difftest(?:[^*]|\*(?!/))*)\*/\s*\n(?:static\s+)?([\w\s\*]+?)\b(\w+)\s*\(")
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.c"), recursive=True)):
        text = open(path).read()
        for m in sig.finditer(text):
            comment, rtype, func = m.group(1), m.group(2).strip(), m.group(3)
            if names and func not in names:
                continue
            for line in re.findall(r"@difftest\b(.*)", comment):
                spec = line.strip().rstrip("*/").split()
                args = [x for x in spec if "=" not in x]
                opts = dict(x.split("=", 1) for x in spec
                            if "=" in x and not x.startswith(("$", "@")))
                opts["_setup"] = [x for x in spec if x.startswith("@") or (x.startswith("$") and "=" in x)]
                # which return registers carry a value: void -> none, 64-bit -> r0:r1
                opts["_ret"] = 0 if rtype == "void" else 2 if re.search(r"\b(u64|s64|long long)\b", rtype) else 1
                opts["_label"] = " ".join(x for x in spec if "=" not in x or x.startswith(("$", "@")))
                tests.append((func, path, args, opts))
    return tests


DEF_RE = re.compile(r"^(?!static\b)(?:[A-Za-z_][\w \*]*?[\s\*])(\w+)\s*\(([^;{}]*)\)\s*\{", re.M)


def find_definitions(path):
    """All non-static function definitions in a source file -> number of return regs."""
    out = {}
    for m in DEF_RE.finditer(open(path).read()):
        head = m.group(0)
        rtype = head[:head.index(m.group(1))].strip()
        if rtype in ("return", "else", "if", "while", "for", "switch"):
            continue
        out[m.group(1)] = 0 if rtype == "void" else 2 if re.search(r"\b(u64|s64|long long)\b", rtype) else 1
    return out


# ---------------------------------------------------------------- symbols
def original_symbols():
    syms = {}
    for line in sh([CROSS + "nm", os.path.join(ROOT, "build/arm9_matching.elf")]).splitlines():
        parts = line.split()
        if len(parts) == 3:
            syms[parts[2]] = int(parts[0], 16)
    modes = {}
    for line in open(os.path.join(ROOT, "config/arm9/symbols.txt")):
        m = re.match(r"(\S+) kind:function\((\w+)", line)
        if m:
            modes[m.group(1)] = m.group(2)
    return syms, modes


def build_candidate(path, syms, workdir):
    """Compile one source file and link it at CAND against original symbol addresses."""
    obj = os.path.join(workdir, "cand.o")
    sh([CROSS + "gcc", *CFLAGS, "-I" + os.path.join(ROOT, "include"), "-c", path, "-o", obj])
    script = os.path.join(workdir, "cand.ld")
    with open(script, "w") as f:
        f.write(f"SECTIONS {{ . = {CAND:#x}; .text : {{ *(.text*) *(.rodata*) *(.data*) *(.bss*) *(COMMON) }} }}\n")
        for name, addr in syms.items():
            if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
                f.write(f"PROVIDE({name} = {addr:#x});\n")
        from build import aeabi_map
        for helper, target in aeabi_map().items():
            if target in syms:
                f.write(f"PROVIDE({helper} = {syms[target]:#x});\n")
    elf = os.path.join(workdir, "cand.elf")
    sh([CROSS + "ld", "--use-blx", "-T", script, "-o", elf, obj])
    binary = os.path.join(workdir, "cand.bin")
    sh([CROSS + "objcopy", "-O", "binary", "-j", ".text", elf, binary])
    cand_syms = {}
    for line in sh([CROSS + "nm", elf]).splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in "Tt":
            cand_syms[parts[2]] = int(parts[0], 16)
    return open(binary, "rb").read(), cand_syms


# ---------------------------------------------------------------- emulator
class Machine:
    """One emulated ARM9 address space. Only pages written during a call are
    restored afterwards, so thousands of calls stay fast."""
    PAGE = 0x1000

    def __init__(self, snapshot, cand_code):
        self.uc = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        self.uc.ctl_set_cpu_model(UC_CPU_ARM_926)  # ARMv5TE, like the DS's ARM946E-S
        self.regions = [(MAIN, MAIN_SIZE), (ITCM, ITCM_SIZE), (DTCM, DTCM_SIZE), (SYSAREA, SYSAREA_SIZE), *EXTRA,
                        (SCRATCH, SCRATCH_SIZE), (CAND, CAND_SIZE), (STACK, STACK_SIZE), (MAGIC_LR, 0x1000)]
        for base, size in self.regions:
            self.uc.mem_map(base, size)
        self.uc.mem_write(MAIN, snapshot[0:MAIN_SIZE])
        self.uc.mem_write(ITCM, snapshot[MAIN_SIZE:MAIN_SIZE + ITCM_SIZE])
        self.uc.mem_write(DTCM, snapshot[MAIN_SIZE + ITCM_SIZE:MAIN_SIZE + ITCM_SIZE + DTCM_SIZE])
        self.uc.mem_write(SYSAREA, snapshot[MAIN_SIZE - SYSAREA_SIZE:MAIN_SIZE])
        self.uc.mem_write(CAND, cand_code)
        self.snapshot = snapshot
        self.cand_code = cand_code
        self.fnmap = {}   # C build function address -> original address
        self.dirty = set()
        self.buffers = bytes(SCRATCH_SIZE)
        self.vbuffers = bytes(0x10000)
        self.vram_byte_writes = 0
        self.fault = None
        self.uc.hook_add(UC_HOOK_MEM_UNMAPPED, self._unmapped)
        self.uc.hook_add(UC_HOOK_INTR, self._intr)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self._write)
        # Unicorn faults on the very first emu_start after a CPSR write; run a
        # throw-away `bx lr` once so every real call starts from a settled CPU.
        self.uc.mem_write(MAGIC_LR + 0x100, b"\x1e\xff\x2f\xe1")
        self.uc.reg_write(UC_ARM_REG_CPSR, 0x1F)
        self.uc.reg_write(UC_ARM_REG_LR, MAGIC_LR)
        try:
            self.uc.emu_start(MAGIC_LR + 0x100, MAGIC_LR, count=10)
        except UcError:
            pass
        self.fault = None

    def pristine(self, page_addr):
        """Initial content of a page (snapshot, candidate code, buffers or zero)."""
        n = self.PAGE
        if MAIN <= page_addr < MAIN + MAIN_SIZE:
            o = page_addr - MAIN
            return self.snapshot[o:o + n]
        if ITCM <= page_addr < ITCM + ITCM_SIZE:
            o = MAIN_SIZE + page_addr - ITCM
            return self.snapshot[o:o + n]
        if DTCM <= page_addr < DTCM + DTCM_SIZE:
            o = MAIN_SIZE + ITCM_SIZE + page_addr - DTCM
            return self.snapshot[o:o + n]
        if SYSAREA <= page_addr < SYSAREA + SYSAREA_SIZE:
            return self.snapshot[MAIN_SIZE - SYSAREA_SIZE:MAIN_SIZE]
        if CAND <= page_addr < CAND + CAND_SIZE:
            o = page_addr - CAND
            return (self.cand_code[o:o + n] + bytes(n))[:n]
        if SCRATCH <= page_addr < SCRATCH + SCRATCH_SIZE:
            o = page_addr - SCRATCH
            return self.buffers[o:o + n]
        if VRAM_BUF <= page_addr < VRAM_BUF + 0x10000:
            o = page_addr - VRAM_BUF
            return self.vbuffers[o:o + n]
        return bytes(n)

    def mapped(self, addr):
        return any(base <= addr < base + size for base, size in self.regions)

    def masked_pristine(self, page, ign):
        data = self.pristine(page)
        if ign and page < ign[1] and page + self.PAGE > ign[0]:
            b = bytearray(data)
            a, z = max(ign[0], page) - page, min(ign[1], page + self.PAGE) - page
            b[a:z] = bytes(z - a)
            data = bytes(b)
        return data

    def _write(self, uc, access, addr, size, value, data):
        if size == 1 and 0x05000000 <= addr < 0x08000000:
            # the DS ignores 8-bit writes to palette RAM, VRAM and OAM
            self.vram_byte_writes += 1
        self.dirty.add(addr & ~(self.PAGE - 1))
        if (addr + size - 1) & ~(self.PAGE - 1) != addr & ~(self.PAGE - 1):
            self.dirty.add((addr + size - 1) & ~(self.PAGE - 1))

    def _unmapped(self, uc, access, addr, size, value, data):
        self.fault = f"unmapped access at {addr:#x}"
        return False

    def set_stubs(self, addrs):
        for h in getattr(self, "stub_hooks", []):
            self.uc.hook_del(h)
        self.stub_hooks = [self.uc.hook_add(UC_HOOK_CODE, self._stub, begin=a, end=a) for a in addrs]

    def _stub(self, uc, addr, size, data):
        uc.reg_write(UC_ARM_REG_PC, uc.reg_read(UC_ARM_REG_LR) & ~1)

    def _intr(self, uc, intno, data):
        self.fault = f"exception/swi {intno}"
        uc.emu_stop()

    def call(self, addr, thumb, args, buffers, patches=(), regs=None):
        """Run one function. `regs` (r0-r15 from a recorded call) replaces the
        generated arguments and the scratch stack with the recorded state."""
        uc = self.uc
        # restore whatever the previous call touched, then install this case's buffers
        for page in self.dirty:
            if self.mapped(page):
                uc.mem_write(page, self.pristine(page))
        self.dirty = set()
        self.buffers = buffers
        self.vbuffers = buffers[0x10000:0x20000]
        uc.mem_write(SCRATCH, buffers[:0x10000])
        uc.mem_write(VRAM_BUF, self.vbuffers)
        self.dirty.update(range(VRAM_BUF, VRAM_BUF + 0x10000, self.PAGE))
        for paddr, data in patches:
            uc.mem_write(paddr, data)
            self.dirty.add(paddr & ~(self.PAGE - 1))
        self.vram_byte_writes = 0
        self.fault = None

        sp = STACK + STACK_SIZE - 0x100
        stack_args = args[4:]
        sp -= 4 * len(stack_args)
        for i, a in enumerate(stack_args):
            uc.mem_write(sp + 4 * i, struct.pack("<I", a & 0xFFFFFFFF))
        self.dirty.add(sp & ~(self.PAGE - 1))
        for i, r in enumerate(REGS):
            uc.reg_write(r, args[i] & 0xFFFFFFFF if i < len(args) else 0xDEAD0000 + i)
        for i, r in enumerate(SAVED[:-1]):
            uc.reg_write(r, 0x5A5A0000 + i)
        uc.reg_write(UC_ARM_REG_R12, 0x12121212)
        self.ignore = None
        if regs is not None:
            for i, r in enumerate(REGS + SAVED[:-1] + [UC_ARM_REG_R12]):
                uc.reg_write(r, regs[i])
            sp = regs[13]
            # the callee's own stack frame (below the entry sp) is dead after the
            # return and laid out differently by different compilers: ignore it
            self.ignore = (sp - 0x4000, sp)
        uc.reg_write(UC_ARM_REG_SP, sp)
        uc.reg_write(UC_ARM_REG_LR, MAGIC_LR)
        uc.reg_write(UC_ARM_REG_CPSR, 0x1F)  # system mode, ARM state
        try:
            uc.emu_start(addr | (1 if thumb else 0), MAGIC_LR, count=MAX_INSNS)
        except UcError as e:
            if not self.fault:
                self.fault = f"cpu error: {e}"
        pc = uc.reg_read(UC_ARM_REG_PC)
        if pc != MAGIC_LR and not self.fault:
            self.fault = f"did not return (pc={pc:#x}, instruction limit?)"
        regs = [uc.reg_read(r) for r in (UC_ARM_REG_R0, UC_ARM_REG_R1)] + [uc.reg_read(r) for r in SAVED]
        # memory: every touched page, plus the argument buffers
        mem = {page: bytes(uc.mem_read(page, self.PAGE)) for page in self.dirty
               if not (STACK <= page < STACK + STACK_SIZE) and self.mapped(page)}
        mem[SCRATCH] = bytes(uc.mem_read(SCRATCH, 0x10000))
        if self.ignore:
            lo, hi = self.ignore
            for page in list(mem):
                if isinstance(page, int) and page < hi and page + self.PAGE > lo:
                    b = bytearray(mem[page])
                    a, z = max(lo, page) - page, min(hi, page + self.PAGE) - page
                    b[a:z] = bytes(z - a)
                    mem[page] = bytes(b)
        mem["ignore"] = self.ignore
        mem["vram_byte_writes"] = self.vram_byte_writes
        return self.fault, regs, mem

    def same_memory(self, m1, m2):
        """Compare two call results page by page; untouched pages are pristine."""
        if m1["vram_byte_writes"] != m2["vram_byte_writes"]:
            return (f"8-bit writes to VRAM/palette/OAM (ignored by DS hardware): "
                    f"{m1['vram_byte_writes']} (original) vs {m2['vram_byte_writes']} (C)")
        ign = m1.get("ignore")
        for page in (set(m1) | set(m2)) - {"vram_byte_writes", "ignore"}:
            a = m1.get(page) or self.masked_pristine(page, ign)
            b = m2.get(page) or self.masked_pristine(page, ign)
            if a != b:
                off = self.first_difference(a, b, ign)
                if off is not None:
                    return f"memory differs at {page + off:#x}: {a[off]:#04x} (original) vs {b[off]:#04x} (C)"
        return None

    def same_word(self, orig, cand, ign=None):
        """Equal, or the C build's copy of a function stored where the original
        stores the original function (e.g. an update callback), or two pointers
        into the dead stack frames of the call (e.g. a stack pointer saved in a
        thread context: the C frames have other sizes)."""
        if orig == cand or (self.fnmap.get(cand, -1) | 1) == (orig | 1):
            return True
        for lo, hi in ((STACK, STACK + STACK_SIZE), ign or (0, 0)):
            if lo <= orig < hi and lo <= cand < hi:
                return True
        return False

    def first_difference(self, a, b, ign=None):
        n = min(len(a), len(b))
        j = 0
        while j < n:
            if a[j] == b[j]:
                j += 1
                continue
            w = j & ~3
            if w + 4 <= n:
                wa, wb = struct.unpack_from("<I", a, w)[0], struct.unpack_from("<I", b, w)[0]
                if self.same_word(wa, wb, ign):
                    j = w + 4
                    continue
            return j
        return None


# ---------------------------------------------------------------- inputs
class ArgGen:
    def __init__(self, rng):
        self.rng = rng
        self.buffers = bytearray(rng.randbytes(0x20000) + bytes(SCRATCH_SIZE - 0x20000))
        self.cursor = SCRATCH + 0x100
        self.vcursor = VRAM_BUF + 0x100
        self.names = {}
        self.patches = []   # (address, bytes) written outside the scratch buffers

    def value(self, s):
        rng = self.rng
        if s.startswith("$"):
            name, _, off = s[1:].partition("+")
            return (self.names[name] + (int(off, 0) if off else 0)) & 0xFFFFFFFF
        if re.fullmatch(r"-?(0x[0-9a-fA-F]+|\d+)", s):
            return int(s, 0) & 0xFFFFFFFF
        kind, *p = s.split(":")
        if kind in ("u8", "u16", "u32"):
            return rng.getrandbits({"u8": 8, "u16": 16, "u32": 32}[kind])
        if kind in ("s8", "s16", "s32"):
            bits = {"s8": 8, "s16": 16, "s32": 32}[kind]
            v = rng.getrandbits(bits)
            if v >= 1 << (bits - 1):
                v -= 1 << bits
            return v & 0xFFFFFFFF
        if kind == "int":
            return rng.randrange(int(p[0], 0), int(p[1], 0)) & 0xFFFFFFFF
        if kind == "pick":
            return int(rng.choice(p[0].split(",")), 0) & 0xFFFFFFFF
        if kind in ("ptr", "zero"):
            size = int(p[0], 0)
            align = int(p[1]) if len(p) > 1 else 1
            self.cursor = (self.cursor + 0x40 + 3) & ~3
            addr = self.cursor + (rng.randrange(0, 4) & ~(align - 1) if kind == "ptr" else 0)
            if kind == "zero":
                off = addr - SCRATCH
                self.buffers[off:off + size] = bytes(size)
            self.cursor = addr + size + 8
            return addr
        if kind == "bytes":
            n, z = int(p[0], 0), int(p[1], 0)
            self.cursor = (self.cursor + 0x40 + 3) & ~3
            addr = self.cursor + rng.randrange(0, 4)
            off = addr - SCRATCH
            self.buffers[off + n:off + n + z] = bytes(z)
            self.cursor = addr + n + z + 8
            return addr
        if kind == "vram":
            size = int(p[0], 0)
            align = int(p[1]) if len(p) > 1 else 1
            self.vcursor = (self.vcursor + 0x40 + 3) & ~3
            addr = self.vcursor + (rng.randrange(0, 4) & ~(align - 1))
            self.vcursor = addr + size + 8
            return addr
        if kind == "fifo":
            self.cursor = (self.cursor + 0x40) & ~3
            addr = self.cursor
            self.cursor += 4
            return addr
        if kind == "ram":
            return int(p[0], 0)
        sys.exit(f"unknown generator {s}")

    def setup(self, item):
        if item.startswith("$"):
            name, gen = item[1:].split("=", 1)
            self.names[name] = self.value(gen)
            return
        target, val = item[1:].split("=", 1)
        target, width = target.rsplit(":", 1)
        if target.startswith("$"):
            base, _, off = target[1:].partition("+")
            addr = self.names[base] + (int(off, 0) if off else 0)
        else:
            addr = int(target, 16)
        nbytes = int(width) // 8
        data = (self.value(val) & ((1 << (8 * nbytes)) - 1)).to_bytes(nbytes, "little")
        if SCRATCH <= addr < SCRATCH + 0x10000:
            off = addr - SCRATCH
            self.buffers[off:off + nbytes] = data
        else:
            self.patches.append((addr, data))


def gen_args(spec, setup, rng):
    g = ArgGen(rng)
    for item in setup:
        g.setup(item)
    args = [g.value(x) for x in spec]
    return args, bytes(g.buffers), g.patches


# ---------------------------------------------------------------- main
def fnmap(cand_syms, syms):
    """Address of each C-compiled function -> the original function's address."""
    return {addr: syms[name] for name, addr in cand_syms.items()
            if name.startswith("func_") and name in syms}


def run_test(machine, func, spec, opts, syms, modes, cand_syms, n, seed):
    rng = random.Random(f"{seed}:{func}:{opts['_label']}")
    orig_addr, thumb = syms[func], modes.get(func) == "thumb"
    faults = 0
    names = ["r0", "r1", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "sp"]
    stubs = [int(x, 16) for x in opts["stub"].split(",")] if "stub" in opts else []
    # a stubbed function that is itself decompiled is also stubbed in the C build
    by_addr = {a & ~1: n for n, a in syms.items()}
    stubs += [cand_syms[by_addr[a]] for a in stubs if by_addr.get(a) in cand_syms]
    machine.set_stubs(stubs)
    try:
        return _run_cases(machine, func, spec, opts, cand_syms, n, rng, orig_addr, thumb, names)
    finally:
        machine.set_stubs([])


def _run_cases(machine, func, spec, opts, cand_syms, n, rng, orig_addr, thumb, names):
    faults = 0
    for i in range(n):
        args, buffers, patches = gen_args(spec, opts["_setup"], rng)
        f1, r1, m1 = machine.call(orig_addr & ~1, thumb, args, buffers, patches)
        f2, r2, m2 = machine.call(cand_syms[func], False, args, buffers, patches)
        if f1 or f2:
            faults += 1
            if (f1 is None) != (f2 is None):
                return False, i, args, f"original: {f1 or 'ok'} / C: {f2 or 'ok'}", faults
            if opts.get("faultmem") and f1 == f2:
                why = machine.same_memory(m1, m2)
                if why:
                    return False, i, args, f"after the same fault ({f1}): {why}", faults
            continue
        check = list(range(opts["_ret"])) + list(range(2, len(r1)))
        if any(not machine.same_word(r1[k], r2[k]) for k in check):
            d = [f"{names[k]} {r1[k]:#x} vs {r2[k]:#x}" for k in check if r1[k] != r2[k]]
            return False, i, args, "registers differ: " + ", ".join(d), faults
        why = machine.same_memory(m1, m2)
        if why:
            return False, i, args, why, faults
    return True, n, None, None, faults


def load_capture(path):
    raw = zlib.decompress(open(path, "rb").read())
    if raw[:4] != b"CAP1":
        raise ValueError(f"{path}: not a capture")
    addr, frame = struct.unpack_from("<II", raw, 4)
    regs = list(struct.unpack_from("<16I", raw, 12))
    cpsr = struct.unpack_from("<I", raw, 76)[0]
    return addr, frame, regs, cpsr, raw[80:]


def run_replays(func, cand_code, cand_syms, syms, cap_dir, ret):
    """Replay every recorded call of `func`. Returns (ok, n, message)."""
    files = sorted(glob.glob(os.path.join(cap_dir, func, "*.bin.z")),
                   key=lambda p: int(os.path.basename(p).split(".")[0]))
    names = ["r0", "r1", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "sp"]
    for path in files:
        addr, frame, regs, cpsr, mem = load_capture(path)
        machine = Machine(mem, cand_code)
        machine.fnmap = fnmap(cand_syms, syms)
        f1, r1, m1 = machine.call(addr, bool(cpsr & 0x20), [], bytes(SCRATCH_SIZE), regs=regs)
        f2, r2, m2 = machine.call(cand_syms[func], False, [], bytes(SCRATCH_SIZE), regs=regs)
        tag = f"call recorded at frame {frame} ({os.path.basename(path)})"
        if (f1 is None) != (f2 is None):
            return False, len(files), f"{tag}: original: {f1 or 'ok'} / C: {f2 or 'ok'}"
        if f1:
            continue
        check = list(range(ret)) + list(range(2, len(r1)))
        if any(not machine.same_word(r1[k], r2[k]) for k in check):
            d = [f"{names[k]} {r1[k]:#x} vs {r2[k]:#x}" for k in check if r1[k] != r2[k]]
            return False, len(files), f"{tag}: registers differ: " + ", ".join(d)
        why = machine.same_memory(m1, m2)
        if why:
            return False, len(files), f"{tag}: {why}"
    return True, len(files), None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("names", nargs="*")
    ap.add_argument("--snapshot", action="append",
                    help="memory snapshot(s) to start from (default: build/ram_snapshot*.bin)")
    ap.add_argument("--cases", type=int, default=None, help="cases per test and snapshot")
    ap.add_argument("--max-cases", type=int, default=None,
                    help="cap the cases per test and snapshot (quick regression runs)")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--no-replay", action="store_true", help="skip recorded-call replays")
    ap.add_argument("--replay-only", action="store_true", help="only run recorded-call replays")
    a = ap.parse_args()
    cap_dir = os.path.join(ROOT, "build", "captures")

    snap_paths = a.snapshot or sorted(glob.glob(os.path.join(ROOT, "build", "ram_snapshot*.bin")))
    if not snap_paths:
        sys.exit("no memory snapshot found (build/ram_snapshot*.bin); run tools/make_snapshots.sh")
    if not os.path.exists(os.path.join(ROOT, "build/arm9_matching.elf")):
        sys.exit("run `python3 tools/build.py` first (needs build/arm9_matching.elf)")

    names = set(a.names)
    files = []
    for n in list(names):
        if n.endswith(".c"):
            files.append(os.path.abspath(n))
            names.discard(n)
    tests = find_tests(names)
    syms, modes = original_symbols()

    by_file = {}
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.c"), recursive=True)):
        if files and os.path.abspath(path) not in files:
            continue
        defs = find_definitions(path)
        if names and not (names & set(defs)):
            continue
        by_file[path] = [t for t in tests if t[1] == path]
    failures = 0
    replay_total = [0]
    untested = []
    with tempfile.TemporaryDirectory() as tmp:
        for path, ftests in by_file.items():
            cand_code, cand_syms = build_candidate(path, syms, tmp)
            machines = [] if a.replay_only else [Machine(open(sp, "rb").read(), cand_code) for sp in snap_paths]
            for m in machines:
                m.fnmap = fnmap(cand_syms, syms)
            if not a.no_replay:
                rets = find_definitions(path)
                if names:
                    rets = {k: v for k, v in rets.items() if k in names}
                for func in rets:
                    if func in cand_syms and os.path.isdir(os.path.join(cap_dir, func)):
                        ok, n, why = run_replays(func, cand_code, cand_syms, syms, cap_dir, rets[func])
                        replay_total[0] += 1
                        if ok:
                            print(f"PASS {func} [replay]: {n} recorded game calls identical")
                        else:
                            failures += 1
                            print(f"FAIL {func} [replay]: {why}")
            covered = {t[0] for t in ftests}
            if not a.no_replay:
                covered |= {f for f in find_definitions(path) if os.path.isdir(os.path.join(cap_dir, f))}
            untested += [f for f in find_definitions(path) if f not in covered and (not names or f in names)]
            if a.replay_only:
                continue
            for func, _, spec, opts in ftests:
                if files and os.path.abspath(path) not in files:
                    continue
                if func not in syms or func not in cand_syms:
                    print(f"SKIP {func}: not found in original symbols or candidate")
                    failures += 1
                    continue
                n = a.cases or int(opts.get("cases", 300))
                if a.max_cases:
                    n = min(n, a.max_cases)
                total, faults, result = 0, 0, None
                for sp, machine in zip(snap_paths, machines):
                    ok, i, args, why, f = run_test(machine, func, spec, opts, syms, modes, cand_syms, n, a.seed)
                    faults += f
                    if not ok:
                        result = (os.path.basename(sp), i, args, why)
                        break
                    total += n
                if result:
                    failures += 1
                    snap, i, args, why = result
                    print(f"FAIL {func} [{opts['_label']}]: {snap} case {i} "
                          f"args=({', '.join(hex(x) for x in args)}): {why}")
                else:
                    note = f", {faults} faulted identically in both" if faults else ""
                    print(f"PASS {func} [{opts['_label']}]: {total} cases identical "
                          f"over {len(snap_paths)} snapshot(s){note}")
    ran = [t for f in by_file.values() for t in f]
    nfunc = len({t[0] for t in ran}) 
    total = (0 if a.replay_only else len(ran)) + replay_total[0]
    if untested and not a.replay_only and not a.no_replay:
        print(f"\nNOT TESTED ({len(untested)}): " + ", ".join(untested))
    print(f"\n{total - failures}/{total} tests passed ({nfunc} functions with generated-input tests, "
          f"{replay_total[0]} with recorded game calls)")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
