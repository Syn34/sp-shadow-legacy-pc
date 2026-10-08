#!/usr/bin/env python3
"""Make dsd's delinked ELF objects link byte-identically with GNU ld.

dsd writes objects for Metrowerks' mwldarm. GNU ld needs three adjustments:

 1. Symbol table order: dsd puts some GLOBAL/WEAK undefined symbols before the
    first-global index (sh_info), so GNU ld treats them as local and can't
    resolve them. `objcopy` rewrites the table in the standard order.
 2. Thumb functions: GNU ld decides between BL/BLX (and sets bit 0 of function
    pointers) from bit 0 of a STT_FUNC symbol's value. dsd marks Thumb code
    with `$t` mapping symbols only, so we set bit 0 on functions that start in
    a `$t` region.
 3. R_ARM_ABS32 addends: GNU ld's ARM backend takes the addend of an
    R_ARM_ABS32 from the bytes at the relocation site (REL style) and ignores
    the RELA addend field, while dsd stores the addend in the RELA field and
    sometimes leaves the original absolute address in place. We write the RELA
    addend into the relocation site and clear the RELA field.
 4. Branch relocation types: dsd emits the legacy R_ARM_PC24 for every ARM
    branch. GNU ld only turns BL into BLX for Thumb targets (instead of adding
    an interworking veneer) for R_ARM_CALL, so unconditional BL/BLX become
    R_ARM_CALL and B/conditional branches become R_ARM_JUMP24.
 5. ELF header flags: marked as ARM EABI version 5 so the objects can be linked
    together with C code compiled by arm-none-eabi-gcc.

Usage: normalize_obj.py <in.o> <out.o>
"""
import bisect
import os
import struct
import subprocess
import sys

OBJCOPY = os.environ.get("OBJCOPY", "arm-none-eabi-objcopy")

src, dst = sys.argv[1], sys.argv[2]
subprocess.run([OBJCOPY, src, dst], check=True)

f = bytearray(open(dst, "rb").read())
shoff = struct.unpack_from("<I", f, 0x20)[0]
shentsize, shnum, shstrndx = struct.unpack_from("<HHH", f, 0x2E)
secs = [list(struct.unpack_from("<10I", f, shoff + i * shentsize)) for i in range(shnum)]
# fields: name type flags addr offset size link info addralign entsize


def cstr(off):
    return f[off:f.index(b"\0", off)].decode()


secname = [cstr(secs[shstrndx][4] + s[0]) for s in secs]
SHT_SYMTAB, SHT_REL, SHT_RELA = 2, 9, 4
symtab_i = next(i for i, s in enumerate(secs) if s[1] == SHT_SYMTAB)
symtab = secs[symtab_i]
strtab = secs[symtab[6]]
nsyms = symtab[5] // 16


def sym(i):
    off = symtab[4] + i * 16
    name, value, size, info, other, shndx = struct.unpack_from("<IIIBBH", f, off)
    return off, cstr(strtab[4] + name), value, size, info, shndx


# --- 2. Thumb bit on functions inside $t regions
maps = {}  # shndx -> sorted [(value, 't'/'a'/'d')]
for i in range(nsyms):
    _, name, value, _, info, shndx = sym(i)
    if name in ("$a", "$t", "$d") or name.startswith(("$a.", "$t.", "$d.")):
        maps.setdefault(shndx, []).append((value, name[1]))
for k in maps:
    maps[k].sort()

thumb_fixed = 0
for i in range(nsyms):
    off, name, value, size, info, shndx = sym(i)
    if (info & 0xF) != 2 or shndx == 0 or shndx >= 0xFF00:  # STT_FUNC, defined
        continue
    m = maps.get(shndx)
    if not m:
        continue
    j = bisect.bisect_right(m, (value, "~")) - 1
    if j >= 0 and m[j][1] == "t" and not (value & 1):
        struct.pack_into("<I", f, off + 4, value | 1)
        thumb_fixed += 1

# --- 3./4. R_ARM_ABS32 addends in place, branch relocation types
R_ARM_ABS32 = 2
R_ARM_PC24, R_ARM_CALL, R_ARM_JUMP24 = 1, 28, 29
abs_zeroed = 0
pc24_converted = 0
for s in secs:
    if s[1] not in (SHT_REL, SHT_RELA):
        continue
    target = secs[s[7]]
    entsize = 12 if s[1] == SHT_RELA else 8
    for k in range(s[5] // entsize):
        r_offset, r_info = struct.unpack_from("<II", f, s[4] + k * entsize)
        if (r_info & 0xFF) == R_ARM_PC24:
            insn = struct.unpack_from("<I", f, target[4] + r_offset)[0]
            cond, link = insn >> 28, (insn >> 24) & 1
            is_call = cond == 0xF or (cond == 0xE and link)
            new_type = R_ARM_CALL if is_call else R_ARM_JUMP24
            struct.pack_into("<I", f, s[4] + k * entsize + 4, (r_info & ~0xFF) | new_type)
            pc24_converted += 1
        if (r_info & 0xFF) == R_ARM_ABS32 and s[1] == SHT_RELA:
            p = target[4] + r_offset
            addend = struct.unpack_from("<i", f, s[4] + k * entsize + 8)[0]
            new_bytes = struct.pack("<i", addend)
            if f[p:p + 4] != new_bytes or addend:
                f[p:p + 4] = new_bytes
                struct.pack_into("<i", f, s[4] + k * entsize + 8, 0)
                abs_zeroed += 1

# --- 5. EABI version 5 header flag
struct.pack_into("<I", f, 0x24, 0x05000000)

open(dst, "wb").write(f)
if os.environ.get("VERBOSE"):
    print(f"{os.path.basename(src)}: thumb funcs marked={thumb_fixed}, abs32 addends moved in-place={abs_zeroed}, pc24 converted={pc24_converted}")
