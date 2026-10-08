/*
 * Palettes: buffered palette loads, colour cycling and fades.
 * ARM9 main, 0x0200b36c - 0x0200be9c (17 functions).
 *
 * Source file "Color.cpp". Per screen (index data_020df0fc):
 *   data_020da588 + scr*16: {u16 *buf, hw} for BG (+0) and OBJ (+8) palettes
 *   data_020da5a8 + scr*0x60: 12 colour cycles of 8 bytes
 *     {u8 flags, period, counter, first, current, last, count}
 *     flags: 1 on, 2 dirty, 4 OBJ palette, 0x10 ping-pong, 0x20 reverse
 *   data_020da768 + scr*0x120: 12 fades of 0x18 bytes
 *     {u16 flags, u8 period, steps, delay, delay reload, first, -, frames,
 *      frame, u16 count, u16 *frames, u16 *target, handle}
 *     flags: 1 on, 2 dirty, 4 OBJ palette, 8 16-colour target, 0x10 loop,
 *     0x20 delayed
 *   data_020da568[scr] active fades, data_020da570[scr] cycles,
 *   data_020da578[scr] + data_020da668: queued palette loads.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u32 data_020da568[];
extern u32 data_020da570[];
extern u32 data_020da578[];
extern u8 data_020da588[];
extern u8 data_020da5a8[];
extern u8 data_020da668[];
extern u8 data_020da768[];
extern const char data_020bcce8[];   /* "Color.cpp" */

s32 func_020a4e00(s32 num, s32 den);
void func_0201817c(u32 screen);
void func_0209396c(const void *p, u32 size);    /* DC_FlushRange */
void func_020912a8(void);
void func_020912f4(const void *src, u32 off, u32 size);
void func_02091368(void);
void func_0209138c(void);
void func_020913d8(const void *src, u32 off, u32 size);
void func_0209144c(void);
void func_02091470(void);
void func_020914c8(const void *src, u32 off, u32 size);
void func_0209153c(void);
void func_020915ac(void);
void func_02091610(const void *src, u32 off, u32 size);
void func_02091690(void);

/* this file's own functions, called before their definition */
void func_0200b588(u8 *f);
void func_0200b5f8(void);
void func_0200b650(void *c);
void func_0200b664(void);
void func_0200b6bc(const void *src, u32 idx, u32 n, u8 *pb);
void func_0200b710(u8 *pb, void *hw);
void func_0200b74c(const void *src, u32 slot, u32 offset, u32 count, s32 kind);
void func_0200bc8c(void);
void func_0200bcc4(s32 handle, u32 offset, u32 idx, u32 n);

#define SCR data_020df0fc
#define PALBUF(scr, obj) (data_020da588 + (scr) * 16 + ((obj) ? 8 : 0))

/* one step of a colour component toward its target */
static inline u32 Step(u32 d, u32 s, u32 k)
{
    return d + func_020a4e00(s - d, k);
}

/* 0x0200b36c: advance a palette fade by one frame
 * @difftest $T=ptr:0x80:2 $F=ptr:0x18:4 @$F:16=pick:0x1,0x5,0x9,0x21,0x11,0x31 @$F+2:8=int:0:4 @$F+3:8=int:0:4 @$F+4:8=int:0:2 @$F+6:8=int:0:0xe0 @$F+8:8=int:1:3 @$F+0xa:16=int:0:32 @$F+0xc:32=$T @$F+0x10:32=$T @$F+0x14:32=-1 $F */
void func_0200b36c(u8 *f)
{
    u32 go, flags, first, n;
    u16 *dst, *src;

    if (U16_AT(f, 0) & 0x20) {
        u32 old = f[4];
        f[4] = old - 1;
        if (old == 0) {
            f[4] = f[5];
            go = 1;
        } else {
            go = 0;
        }
    } else {
        go = 1;
    }
    if (f[3] == 0) {
        if (U16_AT(f, 0) & 0x10) {
            f[9]++;
            if (f[9] >= f[8])
                f[9] = 0;
            f[3] = f[2];
            PTR_AT(f, 0x10) = (u8 *)PTR_AT(f, 0xc) + f[9] * 32;
            return;
        }
        U16_AT(f, 0) &= ~1u;
        func_0200b588(f);
        return;
    }
    if (!go)
        return;
    flags = U16_AT(f, 0);
    first = f[6];
    dst = *(u16 **)PALBUF(SCR, flags & 4);
    if (flags & 8)
        src = (u16 *)PTR_AT(f, 0x10) + (first & 0xf);
    else
        src = (u16 *)PTR_AT(f, 0x10) + first;
    dst += first;
    n = U16_AT(f, 0xa);
    if (n != 0) {
        do {
            u32 s = *src, d = *dst;
            if (d != s) {
                u32 k = f[3];
                u32 r = (u16)Step(d & 0x1f, s & 0x1f, k);
                u32 g = Step((d & 0x3e0) >> 5, (s & 0x3e0) >> 5, k);
                u32 b = Step((d & 0x7c00) >> 10, (s & 0x7c00) >> 10, k);
                *dst = (r & 0x1f) | (((u16)g << 5) & 0x3e0) | (((u16)b << 10) & 0x7c00);
            }
            dst++;
            src++;
        } while (--n != 0);
    }
    U16_AT(f, 0) |= 2;
    f[3]--;
}

/* 0x0200b588: stop a fade
 * @difftest $F=ptr:0x18:4 @$F+0x14:32=-1 $F */
void func_0200b588(u8 *f)
{
    u32 scr;
    if (S32_AT(f, 0x14) != -1) {
        func_020129ac(S32_AT(f, 0x14));
        S32_AT(f, 0x14) = -1;
    }
    MI_CpuFill8(f, 0, 0x18);
    S32_AT(f, 0x14) = -1;
    scr = SCR;
    if (data_020da568[scr] != 0)
        data_020da568[scr]--;
}

/* 0x0200b5f8: stop all fades of the current screen */
void func_0200b5f8(void)
{
    u32 i;
    data_020da568[SCR] = 0;
    for (i = 0; i < 12; i++)
        func_0200b588(data_020da768 + SCR * 0x120 + i * 0x18);
}

/* 0x0200b650: clear a colour cycle
 * @difftest ptr:8 */
void func_0200b650(void *c) { MI_CpuFill8(c, 0, 8); }

/* 0x0200b664: clear all colour cycles of the current screen
 * @difftest */
void func_0200b664(void)
{
    u32 i;
    data_020da570[SCR] = 0;
    for (i = 0; i < 12; i++)
        func_0200b650(data_020da5a8 + SCR * 0x60 + i * 8);
}

/* 0x0200b6bc: load n colours at idx into a palette buffer and the hardware
 * @difftest $B=ptr:0x200:2 $H=ptr:0x200:2 $P=ptr:8:4 @$P:32=$B @$P+4:32=$H ptr:0x40 int:0:32 int:0:32 $P */
void func_0200b6bc(const void *src, u32 idx, u32 n, u8 *pb)
{
    u16 *d = *(u16 **)pb + idx;
    MI_CpuCopy8(src, d, n * 2);
    func_0207fed8(3, (u8 *)PTR_AT(pb, 4) + idx * 2, d, n * 2, 0x10);
}

/* 0x0200b710: allocate a palette buffer for the given hardware palette */
void func_0200b710(u8 *pb, void *hw)
{
    PTR_AT(pb, 0) = func_0207ff48(0x200, data_020bcce8, 0x50b);
    PTR_AT(pb, 4) = hw;
}

/* 0x0200b74c: load an extended palette slot (kind 0: BG, 1: OBJ) */
void func_0200b74c(const void *src, u32 slot, u32 offset, u32 count, s32 kind)
{
    u8 *tmp = NULL;
    u32 size;
    if ((u32)src & 3) {
        tmp = func_0207ff70(count * 2, data_020bcce8, 0x2da);
        MI_CpuCopy8(src, tmp, count * 2);
        src = tmp;
    }
    size = count * 2;
    func_0209396c(src, size);
    if (SCR == 0) {
        if (kind == 0) {
            func_02091690();
            func_02091610(src, (offset + (slot << 12)) * 2, size);
            func_020915ac();
        } else if (kind == 1) {
            func_0209153c();
            func_020914c8(src, (offset + (slot << 12)) * 2, size);
            func_02091470();
        }
    } else {
        if (kind == 0) {
            func_0209144c();
            func_020913d8(src, (offset + (slot << 12)) * 2, size);
            func_0209138c();
        } else if (kind == 1) {
            func_02091368();
            func_020912f4(src, (offset + (slot << 12)) * 2, size);
            func_020912a8();
        }
    }
    if (tmp != NULL)
        func_0207ff14(tmp, data_020bcce8, 0x317);
}

/* 0x0200b88c: perform the queued OBJ palette loads and free their data */
void func_0200b88c(void)
{
    u32 scr = SCR, i = 0;
    if (data_020da578[scr] != 0) {
        do {
            u8 *e = data_020da668 + scr * 128 + i * 8;
            func_0200b6bc(PTR_AT(e, 4), U16_AT(e, 0), U16_AT(e, 2), PALBUF(scr, 1));
            func_0207ff14(PTR_AT(data_020da668 + SCR * 128 + i * 8, 4), data_020bcce8, 0x2b2);
            i++;
            scr = SCR;
        } while (i < data_020da578[scr]);
    }
    data_020da578[scr] = 0;
}

/* 0x0200b93c: copy the changed palette ranges to the hardware */
void func_0200b93c(void)
{
    u32 i;
    for (i = 0; i < 12; i++) {
        u8 *c = data_020da5a8 + SCR * 0x60 + i * 8, *pb;
        u32 f = c[0], first, cur;
        if (!(f & 1) || !(f & 2))
            continue;
        c[0] = f & ~2u;
        first = c[3];
        cur = c[4];
        pb = PALBUF(SCR, c[0] & 4);
        if (cur == first) {
            func_0207fed8(3, (u8 *)PTR_AT(pb, 4) + first * 2, (u8 *)PTR_AT(pb, 0) + first * 2,
                          c[6] * 2, 0x10);
        } else {
            u32 a, n;
            func_0207fed8(3, (u8 *)PTR_AT(pb, 4) + cur * 2, (u8 *)PTR_AT(pb, 0) + first * 2,
                          (c[5] - cur + 1) * 2, 0x10);
            a = c[3];
            n = c[4] - a;
            func_0207fed8(3, (u8 *)PTR_AT(pb, 4) + a * 2, (u8 *)PTR_AT(pb, 0) + (c[5] - n + 1) * 2,
                          n * 2, 0x10);
        }
    }
    for (i = 0; i < 12; i++) {
        u8 *f = data_020da768 + SCR * 0x120 + i * 0x18, *pb;
        u32 fl = U16_AT(f, 0), first;
        if (!(fl & 1) || !(fl & 2))
            continue;
        U16_AT(f, 0) = fl & ~2u;
        first = f[6];
        pb = PALBUF(SCR, U16_AT(f, 0) & 4);
        func_0207fed8(3, (u8 *)PTR_AT(pb, 4) + first * 2, (u8 *)PTR_AT(pb, 0) + first * 2,
                      U16_AT(f, 0xa) * 2, 0x10);
    }
}

/* 0x0200bb0c: advance colour cycles and fades of the current screen */
void func_0200bb0c(void)
{
    u32 i, active;
    for (i = 0; i < 12; i++) {
        u8 *c = data_020da5a8 + SCR * 0x60 + i * 8;
        u32 f = c[0];
        if (!(f & 1))
            continue;
        if (c[2] != 0) {
            c[2]--;
            continue;
        }
        if (f & 0x20) {
            if (c[4] == c[3])
                c[4] = c[5];
            else
                c[4]--;
        } else {
            if (c[4] == c[5])
                c[4] = c[3];
            else
                c[4]++;
        }
        f = c[0];
        if (f & 0x10) {
            if (f & 0x20) {
                if (c[4] == c[3])
                    c[0] = f ^ 0x20;
            } else if (c[4] == c[5]) {
                c[0] = f ^ 0x20;
            }
        }
        c[0] |= 2;
        c[2] = c[1];
    }
    active = 0;
    for (i = 0; i < 12; i++) {
        u32 scr = SCR;
        u8 *e;
        if (active > data_020da568[scr])
            return;
        e = data_020da768 + scr * 0x120 + i * 0x18;
        if (U16_AT(e, 0) & 1) {
            func_0200b36c(e);
            active = (active + 1) & 0xff;
        }
    }
}

/* 0x0200bc8c: reset cycles, fades and queued loads of the current screen */
void func_0200bc8c(void)
{
    func_0200b664();
    func_0200b5f8();
    data_020da578[SCR] = 0;
}

/* 0x0200bcc4: load OBJ colours from a resource */
void func_0200bcc4(s32 handle, u32 offset, u32 idx, u32 n)
{
    u8 *res = func_02012a64(handle);
    func_0200b6bc(res + offset, idx, n, PALBUF(SCR, 1));
    func_020129ac(handle);
}

/* 0x0200bd20 */
void func_0200bd20(s32 handle, s32 idx, s32 n) { func_0200bcc4(handle, 0, idx, n); }

/* 0x0200bd38: load a BG extended palette slot */
void func_0200bd38(const void *src, u32 slot, u32 offset, u32 count)
{
    func_0200b74c(src, slot, offset, count, 0);
}

/* 0x0200bd58: load BG colours */
void func_0200bd58(const void *src, u32 idx, u32 n)
{
    func_0200b6bc(src, idx, n, PALBUF(SCR, 0));
}

/* 0x0200bd88: initialise the palettes of both screens */
void func_0200bd88(void)
{
    s32 screen;
    for (screen = 1; screen >= 0; screen--) {
        u32 i, scr;
        func_0201817c(screen);
        scr = SCR;
        for (i = 0; i < 12; i++)
            S32_AT(data_020da768 + scr * 0x120 + i * 0x18, 0x14) = -1;
        func_0200b710(data_020da588 + scr * 16, (void *)(0x05000000 + (scr << 10)));
        func_0200b710(data_020da588 + SCR * 16 + 8, (void *)(0x05000200 + (SCR << 10)));
        func_0200bc8c();
    }
}
