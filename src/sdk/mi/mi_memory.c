/*
 * NitroSDK MI (memory interface) CPU copy / fill routines.
 * ARM9 main, 0x02094d00 - 0x02095044.
 *
 * In the SDK these are hand-written ARM assembly, so there is no original C to
 * match; this is an equivalent C implementation. Names follow the NitroSDK API
 * and were assigned from behaviour and call sites.
 *
 * Several routines deliberately avoid 8-bit stores (they read-modify-write a
 * halfword instead) because the DS ignores byte writes to VRAM. Those halfword
 * accesses go through `vu16` (volatile) so the compiler cannot merge them back
 * into a single byte store -- GCC does exactly that otherwise, which
 * tools/difftest.py caught. A PC port can relax this; the DS build must not.
 *
 * Status: functionally equivalent, verified by tools/difftest.py and the
 * boot test. Not byte-matching (compiled with GCC instead of mwccarm).
 */
#include "types.h"

/* 0x02094d00 */
/* @difftest u16 ptr:520:2 int:0:512
 * @difftest u16 vram:520:2 int:0:512 */
void MIi_CpuClear16(u16 data, void *destp, u32 size)
{
    u8 *dest = (u8 *)destp;
    s32 i;
    for (i = 0; i < (s32)size; i += 2)
        *(u16 *)(dest + i) = data;
}

/* 0x02094d18 */
/* @difftest ptr:520:2 ptr:520:2 int:0:512
 * @difftest ptr:520:2 vram:520:2 int:0:512 */
void MIi_CpuCopy16(const void *srcp, void *destp, u32 size)
{
    const u8 *src = (const u8 *)srcp;
    u8 *dest = (u8 *)destp;
    s32 i;
    for (i = 0; i < (s32)size; i += 2)
        *(u16 *)(dest + i) = *(const u16 *)(src + i);
}

/* 0x02094d34 */
/* @difftest u32 ptr:520:4 int:0:512 */
void MIi_CpuClear32(u32 data, void *destp, u32 size)
{
    u32 *dest = (u32 *)destp;
    u32 *end = (u32 *)((u8 *)destp + size);
    while (dest < end)
        *dest++ = data;
}

/* 0x02094d48 */
/* @difftest ptr:520:4 ptr:520:4 int:0:512 */
void MIi_CpuCopy32(const void *srcp, void *destp, u32 size)
{
    const u32 *src = (const u32 *)srcp;
    u32 *dest = (u32 *)destp;
    u32 *end = (u32 *)((u8 *)destp + size);
    while (dest < end)
        *dest++ = *src++;
}

/* 0x02094d60: writes every word of src to the same (FIFO) address */
/* @difftest ptr:520:4 fifo int:0:512 */
void MIi_CpuSend32(const void *srcp, volatile void *destp, u32 size)
{
    const u32 *src = (const u32 *)srcp;
    const u32 *end = (const u32 *)((const u8 *)srcp + size);
    while (src < end)
        *(vu32 *)destp = *src++;
}

/* 0x02094d78: 32 bytes per iteration, then single words */
/* @difftest u32 ptr:1040:4 int:0:1024 */
void MIi_CpuClearFast(u32 data, void *destp, u32 size)
{
    u32 *dest = (u32 *)destp;
    u32 *end = (u32 *)((u8 *)destp + size);
    u32 *fastEnd = dest + (size >> 5) * 8;

    while (dest < fastEnd) {
        dest[0] = data; dest[1] = data; dest[2] = data; dest[3] = data;
        dest[4] = data; dest[5] = data; dest[6] = data; dest[7] = data;
        dest += 8;
    }
    while (dest < end)
        *dest++ = data;
}

/* 0x02094dc4 */
/* @difftest ptr:1040:4 ptr:1040:4 int:0:1024 */
void MIi_CpuCopyFast(const void *srcp, void *destp, u32 size)
{
    const u32 *src = (const u32 *)srcp;
    u32 *dest = (u32 *)destp;
    u32 *end = (u32 *)((u8 *)destp + size);
    u32 *fastEnd = dest + (size >> 5) * 8;

    while (dest < fastEnd) {
        dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2]; dest[3] = src[3];
        dest[4] = src[4]; dest[5] = src[5]; dest[6] = src[6]; dest[7] = src[7];
        dest += 8;
        src += 8;
    }
    while (dest < end)
        *dest++ = *src++;
}

static inline void CopyWords(const void *srcp, void *destp, int words)
{
    const u32 *src = (const u32 *)srcp;
    u32 *dest = (u32 *)destp;
    int i;
    for (i = 0; i < words; i++)
        dest[i] = src[i];
}

/* 0x02094dfc */
/* @difftest ptr:64:4 ptr:64:4 */
void MI_Copy36B(const void *src, void *dest) { CopyWords(src, dest, 9); }

/* 0x02094e18 */
/* @difftest ptr:64:4 ptr:64:4 */
void MI_Copy48B(const void *src, void *dest) { CopyWords(src, dest, 12); }

/* 0x02094e3c */
/* @difftest ptr:64:4 ptr:64:4 */
void MI_Copy64B(const void *src, void *dest) { CopyWords(src, dest, 16); }

/* 0x02094e68: byte fill that only ever issues 16/32-bit stores */
/* @difftest ptr:300 u8 int:0:256
 * @difftest vram:300 u8 int:0:256 */
void MI_CpuFill8(void *destp, u8 data, u32 size)
{
    u8 *dest = (u8 *)destp;
    u32 fill = data;

    if (size == 0)
        return;

    if ((u32)dest & 1) {
        /* odd start: merge into the high byte of the halfword before */
        vu16 *h = (vu16 *)(dest - 1);
        *h = (u16)((*h & 0x00FF) | (fill << 8));
        dest++;
        if (--size == 0)
            return;
    }

    if (size >= 2) {
        u32 words;

        fill |= fill << 8;
        if ((u32)dest & 2) {
            *(u16 *)dest = (u16)fill;
            dest += 2;
            size -= 2;
            if (size == 0)
                return;
        }
        fill |= fill << 16;

        words = size & ~3u;
        if (words != 0) {
            u32 *w = (u32 *)dest;
            u32 *end = (u32 *)(dest + words);
            size -= words;
            do {
                *w++ = fill;
            } while (w < end);
            dest = (u8 *)w;
        }
        if (size & 2) {
            *(u16 *)dest = (u16)fill;
            dest += 2;
        }
    }

    if (size & 1) {
        /* odd tail: merge into the low byte of the last halfword */
        vu16 *h = (vu16 *)dest;
        *h = (u16)((*h & 0xFF00) | (fill & 0xFF));
    }
}

/* 0x02094efc: byte copy that only ever issues 16/32-bit loads and stores */
/* @difftest ptr:300 ptr:300 int:0:256
 * @difftest ptr:300 vram:300 int:0:256 */
void MI_CpuCopy8(const void *srcp, void *destp, u32 size)
{
    const u8 *src = (const u8 *)srcp;
    u8 *dest = (u8 *)destp;

    if (size == 0)
        return;

    if ((u32)dest & 1) {
        /* odd destination: write one byte into the high half of dest-1 */
        vu16 *h = (vu16 *)(dest - 1);
        u32 b;
        if ((u32)src & 1)
            b = *(const u16 *)(src - 1) >> 8;
        else
            b = *(const u16 *)src;
        *h = (u16)((*h & 0x00FF) | (b << 8));
        src++;
        dest++;
        if (--size == 0)
            return;
    }

    if (((u32)dest ^ (u32)src) & 1) {
        /* dest is now even and src odd: shift bytes through halfwords */
        const u16 *s = (const u16 *)((u32)src & ~1u);
        u16 *d = (u16 *)dest;
        u32 carry = *s++ >> 8;
        u32 cur;

        while (size >= 2) {
            cur = carry | ((u32)*s++ << 8);
            *d++ = (u16)cur;
            carry = cur >> 16;
            size -= 2;
        }
        if (size & 1)
            *(vu16 *)d = (u16)((*(vu16 *)d & 0xFF00) | carry);
        return;
    }

    if (((u32)dest ^ (u32)src) & 2) {
        /* same byte parity, different word alignment: copy halfwords */
        u32 halves = size & ~1u;
        if (halves != 0) {
            u16 *d = (u16 *)dest;
            u16 *end = (u16 *)(dest + halves);
            const u16 *s = (const u16 *)src;
            size -= halves;
            do {
                *d++ = *s++;
            } while (d < end);
            dest = (u8 *)d;
            src = (const u8 *)s;
        }
    } else if (size >= 2) {
        u32 words;
        if ((u32)dest & 2) {
            *(u16 *)dest = *(const u16 *)src;
            dest += 2;
            src += 2;
            size -= 2;
            if (size == 0)
                return;
        }
        words = size & ~3u;
        if (words != 0) {
            u32 *d = (u32 *)dest;
            u32 *end = (u32 *)(dest + words);
            const u32 *s = (const u32 *)src;
            size -= words;
            do {
                *d++ = *s++;
            } while (d < end);
            dest = (u8 *)d;
            src = (const u8 *)s;
        }
        if (size & 2) {
            *(u16 *)dest = *(const u16 *)src;
            dest += 2;
            src += 2;
        }
    }

    if (size & 1) {
        vu16 *h = (vu16 *)dest;
        *h = (u16)((*h & 0xFF00) | (*(const vu16 *)src & 0xFF));
    }
}

/* 0x0209502c (Thumb in the original) */
/* @difftest ptr:40:4 */
void MI_Zero36B(void *destp)
{
    u32 *dest = (u32 *)destp;
    int i;
    for (i = 0; i < 9; i++)
        dest[i] = 0;
}

/* 0x0209503c: atomic exchange using the ARMv5 SWP instruction */
/* @difftest u32 fifo */
u32 MI_SwapWord(u32 data, vu32 *destp)
{
    u32 old;
    __asm__ volatile("swp %0, %1, [%2]" : "=&r"(old) : "r"(data), "r"(destp) : "memory");
    return old;
}
