/*
 * SoundDS.cpp, part 2: the sound effect filters (FIR presets and biquads
 * on ring buffers of recent samples), the capture/effect channels that run
 * the master mix through a filter, and mixer settings (channel count,
 * mixing rate, mode), the mixer's timer-driven update, reclaiming finished
 * direct sounds, and starting, stopping and setting up the mixer.
 * ARM9 main, 0x02021b50 - 0x02022ecc (19 functions).
 *
 * A filter is {u8 type (0-1 FIR presets, 3 low pass, 4 high pass, 5 band
 * pass, 6 notch, 7 none); u8 pad[3]; u8 +4 Q; u16 +6 frequency;
 * u32 +8 sample rate; +0x0c coefficients}. A ring buffer is {+0x00 data,
 * +0x04 position, +0x08 length, +0x0c recent samples, +0x2c 16-bit}.
 * data_020e2614 + 0x430 is the master filter, +0x45c/+0x48c its two ring
 * buffers (left, right), +0x42c capture flags (0x04 16-bit, 0x10 restart,
 * 0x20 stopped), +0x4bc/+0x4c0 the capture buffer and its size; +0x4d0 the
 * effect filter and +0x4fc/+0x52c its ring buffers; +0x00 the mix buffer
 * (left at +0, right at +0x1000), +0x04 the mix position, +0x0c flags
 * (0x04 stopped, 0x08 starting), +0x580..+0x582 the mix channels and
 * capture settings for SND_StartTimer. Timers 2 and 3 count mixed samples.
 *
 * Types 0-2 and above 7 leave the biquad coefficients of func_02022090
 * undefined in the original (uninitialised registers); here they are 0.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define REG16(a) (*(vu16 *)(a))
#define REG32(a) (*(vu32 *)(a))
#define REG_DIVCNT 0x04000280
#define REG_DIV_NUMER 0x04000290
#define REG_DIV_DENOM 0x04000298
#define REG_DIV_RESULT 0x040002a0

extern u8 func_02000f08[];           /* the decompressor; tables in its code */
extern const u8 data_020bddfc[];     /* FIR presets, 0x20 bytes each */
extern u8 data_020e2614[];
extern u8 data_020e2a14[];
extern u8 data_020e2a44[];           /* the master filter */
extern u8 data_020e2a70[];
extern u8 data_020e2a7c[];
extern u8 data_020e2aa0[];
extern u8 data_020e2aac[];
extern const char data_020bde3c[];   /* "SoundDS.cpp" */
extern u8 data_020e2314[];
extern u8 data_020e2a08[];
extern u8 data_020e2b14[];
extern u8 data_020e2b1c[];
extern u8 data_020e2b4c[];
extern u8 data_020e2ba4[];           /* the sequence player being run (u32) */
extern u8 data_020e2ba8[];

void func_020013b0(void *src, void *dst, u32 n, const void *coef, void *hist);   /* FIR, 16-bit */
void func_02001614(void *src, void *dst, u32 n, const void *coef, void *hist);   /* FIR, 8-bit */
void func_02020fb0(u32 n);
void func_02021160(void **p);
void func_0202119c(void);
void func_02021308(u32 ch);
void func_02027974(s32 i);
void func_02028638(void);
s32 func_0209589c(u32 tick);
void func_020959c0(u32 tick);
u32 func_020960b4(void);
void func_02095374(u32 a, u32 b, u32 c, u32 d);
void func_020953a8(u32 v);
void func_020954e4(u32 a, u32 b);
void func_02022bdc(void);
void func_02022d18(void);
void func_02027acc(u32 v);
void func_02027b9c(u32 v);
void func_0209396c(void *p, u32 size);   /* DC_FlushRange */
void func_020953d4(u32 ch, u32 fmt, const void *data, u32 loop, u32 start, u32 len, s32 vol,
                   u32 shift, u32 timer, s32 pan);
void func_02095510(u32 alarm, u32 tick, u32 period, void (*cb)(void), u32 arg);
void func_02095554(u32 cap, u32 fmt, void *buf, u32 len, u32 a, u32 b, u32 c);
void func_020955a0(u32 mask, u32 a, u32 b, u32 c);
void func_02095610(u32 mask, u32 a, u32 b, u32 c);
void func_02095930(u32 v);
void func_020208c8(u32 h);
u32 func_020211e8(u32 i);
u32 func_020a500c(u32 num, u32 den);   /* unsigned division */
s32 func_020a4e00(s32 num, s32 den);   /* signed division */

void func_02021b50(u8 *rb, void *dst, u32 n);
void func_02021c1c(u8 *f, u8 *rb, u32 n);
void func_02021e50(const s8 *src, s8 *dst, u32 n, const s16 *c, const s8 *hist);
void func_02021f68(const s16 *src, s16 *dst, u32 n, const s16 *c, const s16 *hist);
void func_02022090(u8 *f);
void func_02022218(u8 *f);
void func_02022270(void);
void func_020225d4(void);

#define D data_020e2614

/* 0x02021b50: copy the last `n` samples of a ring buffer
 * @difftest $R=ptr:0x30:4 $B=ptr:0x200:4 @$R+0:32=$B @$R+4:32=int:0:0x80 @$R+8:32=pick:0x80,0x100 @$R+0x2c:32=pick:0,1 $R ptr:0x40 int:0:0x20 */
void func_02021b50(u8 *rb, void *dst, u32 n)
{
    u32 bps = S32_AT(rb, 0x2c) == 1 ? 2 : 1, pos = U32_AT(rb, 4);

    if (pos == 0) {
        MI_CpuCopy8((u8 *)PTR_AT(rb, 0) + bps * (U32_AT(rb, 8) - n), dst, n * bps);
    } else if (pos < n) {
        MI_CpuCopy8((u8 *)PTR_AT(rb, 0) + bps * (U32_AT(rb, 8) + pos - n), dst, bps * (n - pos));
        pos = U32_AT(rb, 4);
        MI_CpuCopy8(PTR_AT(rb, 0), (u8 *)dst + bps * (n - pos), pos * bps);
    } else {
        MI_CpuCopy8((u8 *)PTR_AT(rb, 0) + bps * (pos - n), dst, n * bps);
    }
}

/* 0x02021c1c: filter the next `n` samples of a ring buffer in place
 * @difftest $F=ptr:0x20:2 @$F+0:8=pick:0,1,3,4,5,6 $R=ptr:0x30:4 $B=ptr:0x200:4 @$R+0:32=$B @$R+4:32=int:0:0x80 @$R+8:32=pick:0x80,0x100 @$R+0x2c:32=pick:0,1 $F $R int:0:0x40 */
void func_02021c1c(u8 *f, u8 *rb, u32 n)
{
    u32 bps = S32_AT(rb, 0x2c) == 1 ? 2 : 1, h = 0, cnt, rest, pos;
    u8 hist[0x20], *out;

    MI_CpuFill8(hist, 0, 0x20);
    if (f[0] < 2)
        h = 0x10;
    else if (f[0] < 7)
        h = 3;
    pos = U32_AT(rb, 4);
    out = (u8 *)PTR_AT(rb, 0) + pos * bps;
    if (pos + n >= U32_AT(rb, 8)) {
        cnt = U32_AT(rb, 8) - pos;
        U32_AT(rb, 4) = 0;
        rest = n - cnt;
    } else {
        U32_AT(rb, 4) += n;
        cnt = n;
        rest = 0;
    }
    if (n < h) {
        u32 i;
        if (S32_AT(rb, 0x2c) == 1) {
            s16 *d = (s16 *)hist;
            for (i = 0; i < h - n; i++)
                *d++ = S16_AT(rb, 0xc + n * 2 + i * 2);
            func_02021b50(rb, d, n);
        } else {
            s8 *d = (s8 *)hist;
            for (i = 0; i < h - n; i++)
                *d++ = S8_AT(rb, 0xc + n + i);
            func_02021b50(rb, d, n);
        }
    } else {
        func_02021b50(rb, hist, h);
    }
    if (f[0] < 2) {
        if (S32_AT(rb, 0x2c) == 1)
            func_020013b0(out, out, cnt, f + 0xc, rb + 0xc);
        else
            func_02001614(out, out, cnt, f + 0xc, rb + 0xc);
    } else if (S32_AT(rb, 0x2c) == 1) {
        func_02021f68((s16 *)out, (s16 *)out, cnt, (s16 *)(f + 0xc), (s16 *)(rb + 0xc));
    } else {
        func_02021e50((s8 *)out, (s8 *)out, cnt, (s16 *)(f + 0xc), (s8 *)(rb + 0xc));
    }
    MI_CpuCopy8(hist, rb + 0xc, h * bps);
    if (f[0] >= 3 && f[0] < 7)
        func_02021b50(rb, rb + 0xc + bps * 3, 2);
    if (rest == 0)
        return;
    func_02021c1c(f, rb, rest);
}

/* 0x02021e50: biquad over 8-bit samples (hist: x[-2], x[-1], x[0], y[-1], y[0])
 * @difftest ptr:0x100 ptr:0x100 int:0:0x100 ptr:10:2 ptr:5 */
void func_02021e50(const s8 *src, s8 *dst, u32 n, const s16 *c, const s8 *hist)
{
    s32 x0 = 0, x1 = 0, x2 = 0, y0 = 0, y1 = 0;
    u32 i;

    if (hist != NULL) {
        x0 = hist[2];
        x1 = hist[1];
        x2 = hist[0];
        y0 = hist[4];
        y1 = hist[3];
    }
    for (i = 0; i < n; i++) {
        s32 s = src[i], acc;
        x2 = x1;
        x1 = x0;
        x0 = s;
        acc = (c[1] * x1 + c[0] * x0 + c[2] * x2 + c[3] * y0 + c[4] * y1) >> 14;
        if (acc >= 0x80)
            acc = 0x7f;
        else if (acc < -0x80)
            acc = -0x80;
        dst[i] = (s8)acc;
        y1 = y0;
        y0 = (s8)acc;
    }
}

/* 0x02021f68: biquad over 16-bit samples
 * @difftest ptr:0x200:2 ptr:0x200:2 int:0:0x100 ptr:10:2 ptr:10:2 */
void func_02021f68(const s16 *src, s16 *dst, u32 n, const s16 *c, const s16 *hist)
{
    s32 x0 = 0, x1 = 0, x2 = 0, y0 = 0, y1 = 0;
    u32 i;

    if (hist != NULL) {
        x0 = hist[2];
        x1 = hist[1];
        x2 = hist[0];
        y0 = hist[4];
        y1 = hist[3];
    }
    for (i = 0; i < n; i++) {
        s32 s = src[i], acc;
        x2 = x1;
        x1 = x0;
        x0 = s;
        acc = (c[1] * x1 + c[0] * x0 + c[2] * x2 + c[3] * y0 + c[4] * y1) >> 14;
        if (acc >= 0x8000)
            acc = 0x7fff;
        else if (acc < -0x8000)
            acc = -0x8000;
        dst[i] = (s16)acc;
        y1 = y0;
        y0 = (s16)acc;
    }
}

/* 0x02022090: biquad coefficients for a filter's type, frequency and Q
 * @difftest $F=ptr:0x18:4 @$F+0:8=pick:3,4,5,6 @$F+4:8=int:1:20 @$F+6:16=int:100:20000 @$F+8:32=pick:32768,16384,22050 $F */
void func_02022090(u8 *f)
{
    s32 idx, cs, sn, cos4, a, b0 = 0, b1 = 0, b2 = 0, a0 = 0, a1 = 0, a2 = 0;
    u32 q;

    idx = func_020a500c(U16_AT(f, 6) << 15, U32_AT(f, 8));
    idx = (s32)((idx << 1) & 0xffff) >> 4;
    cs = FX_SinCosTable_[idx * 2 + 1];
    sn = FX_SinCosTable_[idx * 2];
    cos4 = cs << 2;
    q = f[4];
    a = func_020a4e00(sn << 2, q);
    switch (f[0]) {
    case 3:
        b1 = 0x4000 - cos4;
        b0 = b1 >> 1;
        b2 = b0;
        a0 = a + 0x4000;
        a1 = -(cos4 << 1);
        a2 = 0x4000 - a;
        break;
    case 4:
        b0 = (cos4 + 0x4000) >> 1;
        b2 = b0;
        b1 = -(cos4 + 0x4000);
        a0 = a + 0x4000;
        a1 = -(cos4 << 1);
        a2 = 0x4000 - a;
        break;
    case 5:
        b2 = -(s32)q * a;
        b0 = q * a;
        a0 = a + 0x4000;
        a1 = -(cos4 << 1);
        a2 = 0x4000 - a;
        b1 = 0;
        break;
    case 6:
        b1 = -(cos4 << 1);
        b0 = 0x4000;
        b2 = 0x4000;
        a1 = b1;
        a0 = a + 0x4000;
        a2 = 0x4000 - a;
        break;
    }
    S16_AT(f, 0xc) = func_020a4e00(SHL(b0, 14), a0);
    S16_AT(f, 0xe) = func_020a4e00(SHL(b1, 14), a0);
    S16_AT(f, 0x10) = func_020a4e00(SHL(b2, 14), a0);
    S16_AT(f, 0x12) = func_020a4e00(SHL(-a1, 14), a0);
    S16_AT(f, 0x14) = func_020a4e00(SHL(-a2, 14), a0);
}

/* 0x02022218: set up a filter's coefficients
 * @difftest $F=ptr:0x18:4 @$F+0:8=pick:0,1,3,4,5,6,7,9 @$F+4:8=int:1:20 @$F+6:16=int:100:20000 @$F+8:32=pick:32768,16384,22050 $F */
void func_02022218(u8 *f)
{
    u32 t = f[0];
    if (t < 2) {
        MI_CpuCopy8(data_020bddfc + t * 0x20, f + 0xc, 0x20);
        return;
    }
    if (t >= 7)
        return;
    func_02022090(f);
}

/* 0x02022270: capture alarm: filter what was captured
 * @difftest $B=ptr:0x2000:4 @020e2ad0:32=$B @020e2ad4:32=pick:0x40,0x80 @020e2a40:32=pick:0,4 @020e2a44:8=pick:3,4,5,6 $P=ptr:0x400:4 @020e2a70:32=$P @020e2a74:32=int:0:0x40 @020e2a78:32=0x100 @020e2a9c:32=pick:0,1 $Q=ptr:0x400:4 @020e2aa0:32=$Q @020e2aa4:32=int:0:0x40 @020e2aa8:32=0x100 @020e2acc:32=pick:0,1 cases=40 */
void func_02022270(void)
{
    u32 n = U32_AT(D, 0x4c0) >> 1;
    if (U32_AT(D, 0x42c) & 4)
        n >>= 1;
    func_02021c1c(data_020e2a44, data_020e2a70, n);
    func_02021c1c(data_020e2a44, data_020e2aa0, n);
    func_0209396c(PTR_AT(D, 0x4bc), 0x2000);
}

/* 0x020222d4: (re)start the capture channels that run the master mix
 * through the master filter */
void func_020222d4(void)
{
    u32 f = U32_AT(D, 0x42c), loop = 0, fmt, pcm16, rep = 0;

    if (f == 0)
        return;
    if (!(f & 0x20))
        func_020225d4();
    MI_CpuFill8(PTR_AT(D, 0x4bc), 0, U32_AT(D, 0x4c0) << 1);
    if (D[0x430] == 7) {
        D[0x582] &= ~2;
    } else {
        D[0x582] |= 2;
        func_02022218(data_020e2a44);
        f = U32_AT(D, 0x42c);
        if (f & 0x10) {
            u32 is16 = (f & 4) ? 1 : 0, buf = U32_AT(D, 0x4bc), len;
            U32_AT(D, 0x488) = is16;
            U32_AT(D, 0x45c) = buf;
            U32_AT(D, 0x460) = 0;
            len = func_020a500c(0x1000, is16 == 1 ? 2 : 1);
            U32_AT(D, 0x464) = len;
            MI_CpuFill8(data_020e2a7c, 0, 0x20);
            buf = U32_AT(D, 0x4bc);
            U32_AT(D, 0x4b8) = is16;
            U32_AT(D, 0x48c) = buf + 0x1000;
            U32_AT(D, 0x490) = 0;
            U32_AT(D, 0x494) = len;
            MI_CpuFill8(data_020e2aac, 0, 0x20);
        }
    }
    f = U32_AT(D, 0x42c) & ~0x10;
    U32_AT(D, 0x42c) = f;
    if (f & 4) {
        fmt = 0;
        pcm16 = 1;
    } else {
        fmt = 1;
        pcm16 = 0;
    }
    if ((f & 3) == 1)
        rep = 1;
    else if ((f & 3) == 2)
        rep = 0;
    func_02095930(6);
    func_02095554(0, fmt, PTR_AT(D, 0x4bc), U32_AT(D, 0x4c0) >> 2, 1, rep, rep);
    func_020953d4(1, pcm16, PTR_AT(D, 0x4bc), 1, 0, U32_AT(D, 0x4c0) >> 2, D[0x4ce], 0,
                  U16_AT(data_020e2a14, 0xcc), D[0x4cf]);
    func_02095554(1, fmt, (u8 *)PTR_AT(D, 0x4bc) + 0x1000, U32_AT(D, 0x4c0) >> 2, 1, rep, rep);
    func_020953d4(3, pcm16, (u8 *)PTR_AT(D, 0x4bc) + 0x1000, 1, 0, U32_AT(D, 0x4c0) >> 2, D[0x4ce], 0,
                  U16_AT(data_020e2a14, 0xcc), 0x7f - D[0x4cf]);
    if (D[0x430] != 7) {
        u32 half = U32_AT(D, 0x4c0) >> 1;
        s32 k = 0x20, t = U16_AT(data_020e2a14, 0xcc);
        if (U32_AT(D, 0x42c) & 4) {
            half = (s32)half >> 1;
            k >>= 1;
        }
        loop = 2;
        func_02095510(1, k * (t >> 5) + t, (s32)(t * half) / 32, func_02022270, 0);
    }
    if (U32_AT(D, 0xc) & 4)
        return;
    func_02095610(10, 3, loop, 0);
}

/* 0x020225d4: stop the capture channels
 * @difftest @020e2a40:32=pick:0,1,0x20,0x21 cases=20 */
void func_020225d4(void)
{
    u32 f = U32_AT(D, 0x42c);
    if (f == 0)
        return;
    if (f & 0x20)
        return;
    func_02095930(1);
    func_020955a0(10, 3, 2, 0);
    U32_AT(D, 0x42c) |= 0x20;
}

/* 0x02022640: set the music volume */
void func_02022640(u32 v)
{
    D[0x58c] = v;
    func_02027acc(4);
}

/* 0x0202265c: set the master volume of the direct sounds */
void func_0202265c(u32 v)
{
    s32 i;
    D[0x58d] = v;
    for (i = 15; i >= 0; i--)
        func_020208c8(func_020211e8(i));
}

/* 0x0202268c: use `n` software channels; the others are cut */
void func_0202268c(u32 n)
{
    s32 i;

    for (i = n; i < D[0x10]; i++) {
        u32 k = D[0x55c + i];
        if (k < 3) {
            u8 *base = PTR_AT(data_020e2ba8, 8 + k * 4);
            u8 *e = base + 0x93c + i * 0x20;
            if (e[0xa] != 0xff) {
                u8 *t = base + e[0xa] * 0x24;
                if ((u32)i == t[0x44])
                    t[0x44] = 0xff;
            }
            e[0xa] = 0xff;
        }
        func_02021308(i & 0xff);
    }
    D[0x10] = n;
    D[0x11] = n;
}

/* 0x02022748: set the mixing rate */
void func_02022748(u32 rate)
{
    u32 on;

    if (rate == U16_AT(D, 0x12))
        return;
    if (U32_AT(D, 0xc) & 4) {
        on = 1;
    } else {
        on = 0;
        func_02022d18();
    }
    U16_AT(D, 0x12) = rate;
    REG16(REG_DIVCNT) = 0;
    REG32(REG_DIV_NUMER) = 0xffb0ff;
    REG32(REG_DIV_DENOM) = rate;
    REG32(REG_DIV_DENOM + 4) = 0;
    while (REG16(REG_DIVCNT) & 0x8000)
        ;
    U16_AT(data_020e2b14, 0x8a) = (REG32(REG_DIV_RESULT) + 0x10) & ~0x1fu;
    func_02027b9c(4);
    if (on)
        return;
    func_02022bdc();
}

/* 0x02022818: set the mixer mode (bit 0 picks one of two tables that are
 * copied into the mixer's code)
 * @difftest u32 */
void func_02022818(u32 mode)
{
    if (mode & 1)
        MI_CpuCopy8(func_02000f08 + 0x278, func_02000f08 + 0x218, 0x30);
    else
        MI_CpuCopy8(func_02000f08 + 0x248, func_02000f08 + 0x218, 0x30);
    U32_AT(D, 0xc) = mode;
}

/* 0x0202286c: mix what the timers say has been played since the last call */
void func_0202286c(void)
{
    s32 n, i;
    u32 save;

    if (U32_AT(D, 0xc) & 8) {
        REG16(0x0400010c) = 0;
        REG16(0x0400010e) = 0x84;
        REG16(0x04000108) = 0x10000 - (U16_AT(data_020e2b14, 0x8a) << 3);
        REG16(0x0400010a) = 0x81;
        n = U16_AT(data_020e2b14, 0x88) >> 1;
        U16_AT(data_020e2b14, 0x88) = 0;
    } else {
        u32 t = REG16(0x0400010c);
        s32 d = t - U16_AT(data_020e2b14, 0x88);
        if (d < 0)
            d += 0x10000;
        n = SHL(d, 8) & 0x7ff;
        U16_AT(data_020e2b14, 0x88) = t;
    }
    save = U32_AT(data_020e2ba4, 0);
    while (n > 0) {
        s32 cnt = n;
        u32 pos, end;
        for (i = 0; i < 3; i++) {
            u8 *p = PTR_AT(data_020e2ba8, 8 + i * 4);
            s32 t;
            if (p == NULL || p[0x28] != 1)
                continue;
            if (S32_AT(p, 0x1c) <= 0x40000) {
                func_02027974(i);
                p = PTR_AT(data_020e2ba8, 8 + i * 4);
                S32_AT(p, 0x1c) += S32_AT(p, 0x18);
            }
            t = S32_AT(PTR_AT(data_020e2ba8, 8 + i * 4), 0x1c) >> 16;
            if (t < cnt)
                cnt = t & ~3;
        }
        pos = U32_AT(D, 4);
        end = pos + cnt;
        if (end >= 0x800) {
            cnt = 0x800 - pos;
            end = 0;
        }
        if (cnt > 0) {
            func_02020fb0(cnt);
            n -= cnt;
            for (i = 0; i < 3; i++) {
                u8 *p = PTR_AT(data_020e2ba8, 8 + i * 4);
                if (p != NULL && p[0x28] == 1)
                    S32_AT(p, 0x1c) -= SHL(cnt, 16);
            }
        }
        U32_AT(D, 4) = end;
    }
    func_0209396c(PTR_AT(D, 0), 0x2000);
    U32_AT(data_020e2ba4, 0) = save;
    if (!(U32_AT(D, 0xc) & 8))
        return;
    {
        u32 m = U32_AT(D, 0xc) & ~0xcu;
        u32 a = D[0x580], b = D[0x581], c = D[0x582];
        U32_AT(D, 0xc) = m;
        func_02095610(a, b, c, 0);
    }
}

/* 0x02022a6c: free the direct sounds that have finished */
void func_02022a6c(void)
{
    u8 *w = data_020e2a08;
    s32 i;

    for (i = 15; i >= 0; i--, w -= 0x38) {
        u32 f, st;
        if (U32_AT(w, 0) == 0)
            continue;
        f = w[0xb];
        st = (u8)f >> 1;
        if ((f & 1) == 1) {
            u32 bit = 1 << w[0xa];
            switch (st) {
            case 0:
                if (w[5] & 0x10)
                    break;
                if (func_0209589c(U32_AT(w, 0x1c)) != 1)
                    break;
                if (bit & func_020960b4())
                    break;
                func_02021160((void **)w);
                U16_AT(data_020e2b14, 0x7c) |= bit;
                break;
            case 2:
                if (func_0209589c(U32_AT(w, 0x1c)) != 1)
                    break;
                func_02021160((void **)w);
                U16_AT(data_020e2b14, 0x7c) |= bit;
                break;
            case 3:
                func_020959c0(U32_AT(w, 0x1c));
                func_02021160((void **)w);
                U16_AT(data_020e2b14, 0x7c) |= bit;
                break;
            }
        } else if (st == 0) {
            if (U32_AT(data_020e2314, w[0xa] * 0x18) == 0) {
                func_02021160((void **)w);
                D[0x55c + w[0xa]] = 3;
            }
        }
    }
}

/* 0x02022bdc: start the mixer (if stopped) */
void func_02022bdc(void)
{
    u32 t;

    if (!(U32_AT(D, 0xc) & 4))
        return;
    MI_CpuFill8(PTR_AT(D, 0), 0, 0x2000);
    t = D[0x4d0];
    U16_AT(data_020e2b14, 0x88) = 0x800;
    {
        u32 f = U32_AT(D, 0xc);
        U32_AT(D, 4) = 0;
        U32_AT(D, 0x584) = 0;
        U32_AT(D, 0xc) = f | 8;
    }
    if (t != 7) {
        U32_AT(D, 0x500) = 0;
        MI_CpuFill8(data_020e2b1c, 0, 0x20);
        U32_AT(D, 0x530) = 0;
        MI_CpuFill8(data_020e2b4c, 0, 0x20);
    }
    func_02095930(7);
    func_020953d4(0, 1, PTR_AT(D, 0), 1, 0, 0x400, 0x7f, 0, U16_AT(data_020e2b14, 0x8a), 0);
    func_020953d4(2, 1, (u8 *)PTR_AT(D, 0) + 0x1000, 1, 0, 0x400, 0x7f, 0, U16_AT(data_020e2b14, 0x8a), 0x7f);
    func_020222d4();
}

/* 0x02022d18: stop the mixer (if running) */
void func_02022d18(void)
{
    if (U32_AT(D, 0xc) & 4)
        return;
    func_02095930(1);
    func_020955a0(D[0x580], D[0x581], D[0x582], 0);
    REG16(0x0400010a) = 0;
    REG16(0x0400010e) = 0;
    U32_AT(D, 0xc) |= 4;
}

/* 0x02022d94: set up the sound system */
void func_02022d94(void)
{
    s32 i;
    u8 *p;

    func_02028638();
    MI_CpuFill8(D, 0, 0x590);
    MI_CpuFill8(data_020e2314, 0, 0x300);
    PTR_AT(D, 0) = func_0207ff70(0x2000, data_020bde3c, 0xbd);
    p = func_0207ff70(0x368, data_020bde3c, 0xcc);
    PTR_AT(D, 8) = p;
    PTR_AT(D, 8) = (void *)(((u32)p + 0x1ff) & ~0x1ffu);
    func_0202119c();
    D[0x10] = 0x10;
    D[0x11] = 0x10;
    D[0x58c] = 0x7f;
    D[0x58d] = 0x7f;
    D[0x581] = 0;
    D[0x582] = 0;
    D[0x580] = 5;
    {
        u32 m = D[0x580];
        D[0x4d0] = 7;
        U16_AT(data_020e2b14, 0x7c) = ~m;
    }
    for (i = 0; i < 0x20; i++)
        D[0x55c + i] = 3;
    func_02095930(3);
    func_020953a8(0x7f);
    func_02095374(0, 0, 0, 0);
    func_020954e4(0xffff, 0);
    func_02022818(7);
    func_02022748(0x8000);
}
