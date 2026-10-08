/*
 * SoundDS.cpp, part 3: the tracker-style music player's voices: resetting
 * voices and tracks, pitch (bends, vibrato, auto-vibrato, pitch envelope),
 * volume and pan (with their envelopes), envelope steps, releasing voices
 * and the per-tick voice update.
 * ARM9 main, 0x02022ecc - 0x020241b8 (16 functions).
 *
 * The player being run is *data_020e2ba4 (players are listed at
 * data_020e2ba8 + 8):
 *   +0x00 samples (0x24 bytes each: +0x00 data, +0x04 volume, +0x05 flags,
 *   +0x08 length, +0x0c/+0x10 loop start/end, +0x23 vibrato wave),
 *   +0x04 instruments (0x20 bytes each: +0x04 fade-out, +0x06 pan
 *   separation, +0x07 pan centre, +0x08 volume, +0x14/+0x18/+0x1c volume,
 *   pan and pitch envelopes), +0x2c volume, +0x2f flags (0x08 linear
 *   frequencies, 0x10 fine vibrato), +0x38/+0x39/+0x3a volume, target and
 *   fade step, +0x3b player id (the owner in data_020e2614 + 0x55c),
 *   +0x3c tracks (0x24 bytes each: +0x01 portamento on, +0x02/+0x03 volume
 *   column and effect, +0x06 volume, +0x08 voice, +0x0c target rate, +0x11
 *   portamento speed, +0x15 vibrato depth, +0x21 vibrato wave, +0x22
 *   active effects), +0x93c voices (0x20 bytes each: +0x00 rate, +0x04
 *   note, +0x05 instrument, +0x06 volume, +0x07 pan, +0x09 sample, +0x0a
 *   track, +0x0b flags (0x02 released, 0x04 fading, 0x08/0x10/0x20 volume,
 *   pan and pitch envelope on), +0x0d/+0x0e auto-vibrato depth and phase,
 *   +0x0f vibrato phase, +0x11 bits 0-1 pitch/volume need updating, +0x12
 *   fade-out volume, +0x14/+0x18/+0x1c envelope {u16 time; u8 point}).
 * An envelope is {u16 *times; s8 *values; u8 +0x08 flags (0x02 sustain,
 * 0x04 loop); u8 +0x09 points; +0x0a/+0x0b sustain, +0x0c/+0x0d loop}.
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

extern const u8 data_020b58dc[];     /* u16 */
extern const u8 data_020b5f1c[];     /* u16 */
extern u8 data_020bde4c[];           /* u32 *: rate factors (up) */
extern u8 data_020bde50[];           /* u32 *: fine rate factors (up) */
extern u8 data_020bde58[];           /* s8 *[4]: vibrato waves */
extern u8 data_020e2314[];
extern u8 data_020e231c[];
extern u8 data_020e2320[];
extern u8 data_020e2322[];
extern u8 data_020e2324[];
extern u8 data_020e2328[];
extern u8 data_020e2614[];
extern u8 data_020e2b70[];
extern u8 data_020e2ba4[];

u32 func_02019910(u32 a, u32 b);   /* fixed-point multiply */
void func_02021308(u32 ch);
s32 func_020a4e00(s32 num, s32 den);

u32 func_02023040(u32 rate, s32 bend);
void func_02023274(u32 v, s32 k);
void func_02023778(u32 bits);
void func_02023828(u32 v);
void func_02023b44(u32 v);
void func_02023d60(u32 v, u32 mode);

#define D data_020e2614
#define PLAYER() ((u8 *)PTR_AT(data_020e2ba4, 0))
#define VOICE(p, v) ((p) + 0x93c + (v) * 0x20)
#define TRACK(p, t) ((p) + 0x3c + (t) * 0x24)
#define CHAN(v) (data_020e2314 + (v) * 0x18)

/* set bits of the 2-bit "needs update" field (+0x11) */
#define MARK(e, b) ((e)[0x11] = ((e)[0x11] & ~3) | ((((e)[0x11] & 3) | (b)) & 3))
#define UNMARK(e, b) ((e)[0x11] = ((e)[0x11] & ~3) | ((((e)[0x11] & 3) & ~(b)) & 3))

/* 0x02022ecc: reset voice `v` (0xff: none) */
void func_02022ecc(u32 v)
{
    u8 *e;
    s32 i;

    if (v == 0xff)
        return;
    e = VOICE(PLAYER(), v);
    U32_AT(e, 0) = 0;
    e[5] = 0;
    e[6] = 0;
    e[7] = 0;
    e[8] = 0;
    e[9] = 0;
    e[0xa] = 0xff;
    e[0xb] = 0;
    e[0xf] = 0;
    e[0xe] = 0;
    U16_AT(e, 0xc) = 0;
    e[0x11] &= ~0xc;
    e[0x11] &= ~3;
    U16_AT(e, 0x12) = 0x400;
    for (i = 0; i < 3; i++) {
        e[0x16 + i * 4] = 0;
        U16_AT(e, 0x14 + i * 4) = 0;
    }
}

/* 0x02022f68: reset track `t` (0xff: none) */
void func_02022f68(u32 t)
{
    u8 *r;

    if (t == 0xff)
        return;
    r = TRACK(PLAYER(), t);
    r[0] = 0;
    r[1] = 0;
    r[2] = 0;
    r[3] = 0;
    r[4] = 0;
    r[5] = 0;
    r[6] = 0;
    r[7] = 0;
    r[8] = 0xff;
    r[9] = 0;
    r[0xa] = 0;
    r[0xb] = 0;
    r[0x10] = 0;
    r[0x11] = 0;
    U32_AT(r, 0xc) = 0;
    r[0x14] = 0;
    r[0x15] = 0;
    r[0x1e] = 0;
    r[0x18] = 0;
    r[0x19] = 0;
    U16_AT(r, 0x12) = 0;
    r[0x16] = 0;
    r[0x1a] = 0;
    r[0x1b] = 0;
    r[0x17] = 0;
    r[0x1f] &= ~0xf;
    r[0x1f] &= ~0xf0;
    r[0x21] &= ~3;
    r[0x21] &= ~0xc;
    r[0x21] &= ~0x30;
}

/* 0x02023040: a rate bent by `bend` (1/64 semitones; linear or Amiga
 * frequencies by the player's flags) */
u32 func_02023040(u32 rate, s32 bend)
{
    if (bend == 0)
        return rate;
    if (PLAYER()[0x2f] & 8) {
        if (bend > 0) {
            if (bend > 0x3ff)
                bend = 0x3ff;
            rate = func_02019910(rate, ((u32 *)PTR_AT(data_020bde4c, 0))[bend >> 2]);
            return func_02019910(rate, ((u32 *)PTR_AT(data_020bde50, 0))[bend & 3]);
        }
        if (bend >= 0)
            return rate;
        bend = -bend;
        if (bend > 0x3ff)
            bend = 0x3ff;
        rate = func_02019910(rate, ((const u16 *)data_020b5f1c)[bend >> 2]);
        return func_02019910(rate, ((const u16 *)data_020b58dc)[bend & 3]);
    }
    {
        s64 den = 0xda7600 - (s64)bend * (s64)rate;
        u64 num;
        if (den <= 0)
            return 0x1fffe000;
        num = (u64)rate * 0xda7600;
        REG16(REG_DIVCNT) = 2;
        REG32(REG_DIV_NUMER) = (u32)num;
        REG32(REG_DIV_NUMER + 4) = (u32)(num >> 32);
        REG32(REG_DIV_DENOM) = (u32)den;
        REG32(REG_DIV_DENOM + 4) = (u32)((u64)den >> 32);
        while (REG16(REG_DIVCNT) & 0x8000)
            ;
        return REG32(REG_DIV_RESULT);
    }
}

/* 0x020231c0: step the player's volume fade */
void func_020231c0(void)
{
    u8 *p = PLAYER();
    u32 step = p[0x3a], target, cur;

    if (step == 0)
        return;
    target = p[0x39];
    cur = p[0x38];
    if (cur > target) {
        if ((s32)(cur - step) <= (s32)target) {
            p[0x38] = target;
            PLAYER()[0x3a] = 0;
        } else {
            p[0x38] -= step;
        }
    } else if (cur < target) {
        if ((s32)(cur + step) >= (s32)target) {
            p[0x38] = target;
            PLAYER()[0x3a] = 0;
        } else {
            p[0x38] += step;
        }
    } else {
        p[0x3a] = 0;
    }
    func_02023778(2);
}

/* 0x02023274: step envelope `k` (0 volume, 1 pan, 2 pitch) of voice `v` */
void func_02023274(u32 v, s32 k)
{
    u8 *p, *e, *env, *st;
    s32 ls = -1, le = 0;

    if (v == 0xff)
        return;
    p = PLAYER();
    e = VOICE(p, v);
    env = PTR_AT((u8 *)PTR_AT(p, 4) + e[5] * 0x20 + k * 4, 0x14);
    st = e + 0x14 + k * 4;
    if (!(e[0xb] & 2)) {
        u32 f = env[8];
        if (f & 4) {
            ls = env[0xc];
            le = env[0xd];
        } else if (f & 2) {
            ls = env[0xa];
            le = env[0xb];
        }
    } else if (env[8] & 2) {
        ls = env[0xa];
        le = env[0xb];
    }
    if (ls == le && (s32)st[2] == ls) {
        st[2] = ls;
        U16_AT(st, 0) = ((u16 *)PTR_AT(env, 0))[st[2]];
        return;
    }
    U16_AT(st, 0) += 1;
    if (U16_AT(st, 0) < ((u16 *)PTR_AT(env, 0))[st[2] + 1])
        return;
    st[2] += 1;
    if (ls != -1) {
        if ((s32)st[2] < le)
            return;
        st[2] = ls;
        U16_AT(st, 0) = ((u16 *)PTR_AT(env, 0))[st[2]];
        return;
    }
    {
        s32 last = env[9] - 1;
        if ((s32)st[2] < last)
            return;
        st[2] = last;
        U16_AT(st, 0) = ((u16 *)PTR_AT(env, 0))[st[2]];
        if (k != 0)
            return;
        if (((s8 *)PTR_AT(env, 4))[st[2]] != 0) {
            e[0xb] |= 4;
            return;
        }
        func_02023d60(v, 0);
    }
}

/* 0x02023450: step the portamento of track `t`
 * @difftest $P=ptr:0xd40:4 @020e2ba4:32=$P $S=ptr:0x2400:4 @$P+0:32=$S $I=ptr:0x2000:4 @$P+4:32=$I $M=ptr:0x100 @$I+0x2c:32=$M @$I+0x30:32=$M @$I+0x34:32=0 @$I+0x38:32=0 @$I+0x3c:32=0 $O=ptr:0x100 @$P+8:32=$O @$P+0x26:8=1 @$P+0x3d:8=1 @$P+0x3c:8=int:0:0x80 @$P+0x44:8=pick:0,1,0xff @$P+0x41:8=int:0:0x41 @$P+0x9f5:8=int:0:16 @$P+0xa15:8=int:0:16 @$P+0x2f:8=pick:0,8 @$P+0x48:32=int:0x100:0x100000 @$P+0x4d:8=u8 @$P+0x93c:32=int:0x100:0x100000 @$P+0x95c:32=int:0x100:0x100000 pick:0 cases=100 */
void func_02023450(u32 t)
{
    u8 *p = PLAYER(), *r = TRACK(p, t), *e;
    u32 v = r[8];

    if (v == 0xff)
        return;
    if (r[1] == 0)
        return;
    e = VOICE(p, v);
    if (U32_AT(e, 0) > U32_AT(r, 0xc)) {
        U32_AT(e, 0) = func_02023040(U32_AT(e, 0), -(r[0x11] << 2));
        if (U32_AT(e, 0) < U32_AT(r, 0xc))
            U32_AT(e, 0) = U32_AT(r, 0xc);
        MARK(e, 1);
        return;
    }
    if (U32_AT(e, 0) >= U32_AT(r, 0xc))
        return;
    U32_AT(e, 0) = func_02023040(U32_AT(e, 0), r[0x11] << 2);
    if (U32_AT(e, 0) > U32_AT(r, 0xc))
        U32_AT(e, 0) = U32_AT(r, 0xc);
    MARK(e, 1);
}

/* 0x02023574: volume slide (x: 0x0y down by y, 0xy0 up by y)
 * @difftest ptr:1 u8 int:0:0x41 */
void func_02023574(u8 *vol, u32 x, u32 max)
{
    if (!(x & 0xf0)) {
        if (*vol > x)
            *vol -= x;
        else
            *vol = 0;
        return;
    }
    if (x & 0xf)
        return;
    *vol += (s32)x >> 4;
    if (*vol > max)
        *vol = max;
}

/* 0x020235bc: fine volume slide (0xfy down, 0xyf up); 0x10 if x was a
 * plain slide
 * @difftest ptr:1 u8 int:0:0x41 */
u32 func_020235bc(u8 *vol, u32 x, u32 max)
{
    u32 ret = 0, lo = x & 0xf, hi;
    s32 d = 0;

    if (lo == 0 || (hi = x & 0xf0) == 0) {
        ret = 0x10;
        if ((x & 0xf0) == 0xf0)
            d = 0xf;
        else if (lo == 0xf)
            d = -0xf;
    } else if (lo == 0xf) {
        d = (s32)x >> 4;
    } else if (hi == 0xf0) {
        d = -(s32)lo;
    }
    if (d != 0) {
        *vol += d;
        if (*vol > max) {
            if (d < 0)
                *vol = 0;
            else
                *vol = max;
        }
    }
    return ret;
}

/* 0x02023648: release all the player's voices */
void func_02023648(void)
{
    s32 i;
    for (i = D[0x10] - 1; i >= 0; i--) {
        if (PLAYER()[0x3b] == D[0x55c + i])
            func_02023d60(i & 0xff, 0);
    }
}

/* 0x020236b4: free the player's voices whose channel has finished */
void func_020236b4(void)
{
    s32 i;
    for (i = D[0x10] - 1; i >= 0; i--) {
        u8 *p = PLAYER(), *e = VOICE(p, i);
        u32 t = e[0xa];
        if (t == 0xff)
            continue;
        if (U32_AT(CHAN(i), 0) != 0)
            continue;
        if (p[0x3b] != D[0x55c + i])
            continue;
        if ((u32)i == p[t * 0x24 + 0x44])
            p[t * 0x24 + 0x44] = 0xff;
        e[0xa] = 0xff;
        D[0x55c + i] = 3;
    }
}

/* 0x02023778: mark the player's playing voices for a pitch (1) and/or
 * volume (2) update
 * @difftest $P=ptr:0xd40:4 @020e2ba4:32=$P @$P+0x3b:8=int:0:4 pick:1,2,3 cases=60 */
void func_02023778(u32 bits)
{
    s32 i;
    for (i = D[0x10] - 1; i >= 0; i--) {
        u8 *p = PLAYER();
        if (p[0x3b] != D[0x55c + i])
            continue;
        if (U32_AT(CHAN(i), 0) == 0)
            continue;
        p[0x94d + i * 0x20] = (p[0x94d + i * 0x20] & ~3) | ((bits | (p[0x94d + i * 0x20] & 3)) & 3);
    }
}

/* 0x02023828: volume and pan of voice `v` into its software channel */
void func_02023828(u32 v)
{
    u8 *p = PLAYER(), *e = VOICE(p, v), *ins, *envv = NULL, *envp = NULL;
    u32 ep = 0, fl;
    s32 vol, pan, pan0;

    if ((e[0x10] & 0x80) && e[0xa] != 0xff && p[e[0xa] * 0x24 + 0x3f] == 9) {
        U32_AT(data_020e2320, v * 0x18) = 0;
        return;
    }
    if (e[0xb] & 8) {
        const s8 *vals;
        const u16 *times;
        s32 t0, dt, dv;
        u32 t1;
        envv = PTR_AT((u8 *)PTR_AT(p, 4) + e[5] * 0x20, 0x14);
        ep = e[0x16];
        vals = PTR_AT(envv, 4);
        times = PTR_AT(envv, 0);
        t0 = times[ep];
        dt = U16_AT(e, 0x14) - t0;
        dv = vals[ep + 1] - vals[ep];
        t1 = times[ep + 1];
        REG16(REG_DIVCNT) = 0;
        REG32(REG_DIV_NUMER) = SHL(dt * dv, 8);
        REG32(REG_DIV_DENOM) = t1 - t0;
        REG32(REG_DIV_DENOM + 4) = 0;
    }
    p = PLAYER();
    {
        u8 *tr = p + e[0xa] * 0x24;
        u32 mv = D[0x58c] * p[0x2c];
        u8 *smp;
        s32 a, b;
        ins = (u8 *)PTR_AT(p, 4) + e[5] * 0x20;
        smp = (u8 *)PTR_AT(p, 0) + e[9] * 0x24;
        fl = e[0xb];
        a = smp[4] * (ins[8] * mv);
        b = U16_AT(e, 0x12) * (p[0x38] * tr[0x42]);
        vol = ((a >> 19) * b) >> 23;
    }
    if (fl & 8) {
        s32 q;
        while (REG16(REG_DIVCNT) & 0x8000)
            ;
        q = REG32(REG_DIV_RESULT) + SHL(((s8 *)PTR_AT(envv, 4))[ep], 8);
        vol = (vol * q * e[6]) >> 4;
    } else {
        vol = vol * (e[6] << 10);
    }
    if (fl & 0x10) {
        const s8 *vals;
        const u16 *times;
        s32 t0, dt, dv;
        u32 t1;
        envp = PTR_AT(ins, 0x18);
        ep = e[0x1a];
        vals = PTR_AT(envp, 4);
        times = PTR_AT(envp, 0);
        t0 = times[ep];
        dt = U16_AT(e, 0x18) - t0;
        dv = vals[ep + 1] - vals[ep];
        t1 = times[ep + 1];
        REG16(REG_DIVCNT) = 0;
        REG32(REG_DIV_NUMER) = dt * dv;
        REG32(REG_DIV_DENOM) = t1 - t0;
        REG32(REG_DIV_DENOM + 4) = 0;
    }
    p = PLAYER();
    fl = e[0xb];
    ins = (u8 *)PTR_AT(p, 4) + e[5] * 0x20;
    pan0 = e[7];
    pan = pan0 + (((s32)(e[4] - ins[7]) * (s8)ins[6]) >> 3);
    if (fl & 0x10) {
        s32 q, m;
        while (REG16(REG_DIVCNT) & 0x8000)
            ;
        q = REG32(REG_DIV_RESULT) + ((s8 *)PTR_AT(envp, 4))[ep];
        if ((u32)pan0 < 0x20)
            m = q * pan0;
        else
            m = q * (0x40 - pan0);
        pan += m >> 5;
    }
    if (pan < 0)
        pan = 0;
    else if (pan > 0x40)
        pan = 0x40;
    U16_AT(data_020e2320, v * 0x18) = (vol * (0x40 - pan)) >> 15;
    U16_AT(data_020e2322, v * 0x18) = (vol * pan) >> 15;
    UNMARK(e, 2);
}

/* 0x02023b44: pitch of voice `v` into its software channel */
void func_02023b44(u32 v)
{
    u8 *p = PLAYER(), *e = VOICE(p, v);
    u32 rate = U32_AT(e, 0), depth = e[0xd];
    s32 bend = 0;

    if (depth != 0) {
        u8 *smp = (u8 *)PTR_AT(p, 0) + e[9] * 0x24;
        const s8 *wave = ((const s8 **)data_020bde58)[smp[0x23]];
        bend += (s32)(depth * wave[e[0xe]]) >> 6;
    }
    if (e[0xa] != 0xff) {
        u8 *t = TRACK(p, e[0xa]);
        u32 fx = t[0x22];
        if (((fx & 0x10) && (t[3] == 8 || t[3] == 0x15)) ||
            ((fx & 0x20) && t[2] >= 0xcb && t[2] <= 0xd4)) {
            const s8 *wave = ((const s8 **)data_020bde58)[t[0x21] & 3];
            u32 sh = (p[0x2f] & 0x10) ? 7 : 8;
            bend += (t[0x15] * wave[e[0xf]]) >> sh;
        }
    }
    if (e[0xb] & 0x20) {
        u8 *env = PTR_AT((u8 *)PTR_AT(p, 4) + e[5] * 0x20, 0x1c);
        u32 ep = e[0x1e];
        const s8 *vals = PTR_AT(env, 4);
        const u16 *times = PTR_AT(env, 0);
        s32 v0 = vals[ep], dv = vals[ep + 1] - v0, t0 = times[ep], dt = U16_AT(e, 0x1c) - t0, q;
        s32 t1 = times[ep + 1];
        q = func_020a4e00(SHL(dt * dv, 8), t1 - t0);
        bend += SHL(q + SHL(v0, 8), 5) / 256;
    }
    rate = func_02023040(rate, bend);
    {
        u32 den = U16_AT(D, 0x12);
        REG16(REG_DIVCNT) = 2;
        REG32(REG_DIV_NUMER) = rate << 16;
        REG32(REG_DIV_NUMER + 4) = rate >> 16;
        REG32(REG_DIV_DENOM) = den;
        REG32(REG_DIV_DENOM + 4) = 0;
        while (REG16(REG_DIVCNT) & 0x8000)
            ;
        U32_AT(data_020e231c, v * 0x18) = REG32(REG_DIV_RESULT);
    }
    UNMARK(e, 1);
}

/* 0x02023d60: release voice `v` (mode & 0xf: 0 cut, 1 note off, 2 note off
 * with fade unless the volume envelope sustains, 3 fade; 0x10: keep the
 * track's voice link) */
void func_02023d60(u32 v, u32 mode)
{
    u32 keep = (mode & 0x10) ? 1 : 0, t;
    u8 *ch, *p, *e;

    if (v == 0xff)
        return;
    ch = CHAN(v);
    p = PLAYER();
    e = VOICE(p, v);
    if (U32_AT(ch, 0) != 0 && (mode & ~0x10u & 0xff) != 0) {
        u8 *smp = (u8 *)PTR_AT(p, 0) + e[9] * 0x24;
        if (smp[5] & 0x10) {
            U32_AT(data_020e2324, v * 0x18) = U32_AT(smp, 0) + U32_AT(smp, 0x10);
            U32_AT(data_020e2328, v * 0x18) = U32_AT(smp, 0x10) - U32_AT(smp, 0xc);
        } else {
            U32_AT(data_020e2324, v * 0x18) = U32_AT(smp, 0) + U32_AT(smp, 8);
            U32_AT(data_020e2328, v * 0x18) = 0;
        }
        if (U32_AT(ch, 0) >= U32_AT(data_020e2324, v * 0x18)) {
            func_02021308(v);
            if (e[0xa] != 0xff)
                PLAYER()[e[0xa] * 0x24 + 0x44] = 0xff;
            e[0xa] = 0xff;
            data_020e2b70[v] = 3;
            return;
        }
    }
    t = e[0xa];
    if (t != 0xff) {
        u8 *slot = p + 0x44 + t * 0x24;
        if (v == *slot && keep == 0)
            *slot = 0xff;
    }
    switch (mode & ~0x10u & 0xff) {
    case 0:
        func_02021308(v);
        e[0xa] = 0xff;
        data_020e2b70[v] = 3;
        break;
    case 1:
        e[0xb] |= 2;
        break;
    case 2: {
        u8 *env;
        e[0xb] |= 2;
        env = PTR_AT((u8 *)PTR_AT(PLAYER(), 4) + e[5] * 0x20, 0x14);
        if (env != NULL && !(env[8] & 2) && (e[0xb] & 8))
            break;
        e[0xb] |= 4;
        break;
    }
    case 3:
        e[0xb] |= 6;
        break;
    }
}

/* 0x02023fac: per-tick update of the player's voices (fade-out,
 * envelopes, pitch and volume) */
void func_02023fac(void)
{
    s32 i;
    for (i = D[0x10] - 1; i >= 0; i--) {
        u8 *p, *e;
        if (U32_AT(CHAN(i), 0) == 0)
            continue;
        p = PLAYER();
        e = VOICE(p, i);
        if (p[0x3b] != D[0x55c + i])
            continue;
        if (e[0xb] & 4) {
            u32 fade = U16_AT((u8 *)PTR_AT(p, 4) + e[5] * 0x20, 4);
            if (U16_AT(e, 0x12) >= fade) {
                U16_AT(e, 0x12) -= fade;
                MARK(e, 2);
            } else {
                U16_AT(e, 0x12) = 0;
                func_02023d60(i & 0xff, 0);
            }
        }
        if (e[0xb] & 8) {
            func_02023274(i & 0xff, 0);
            MARK(e, 2);
        }
        if (e[0xb] & 0x10) {
            func_02023274(i & 0xff, 1);
            MARK(e, 2);
        }
        if (e[0xb] & 0x20) {
            func_02023274(i & 0xff, 2);
            MARK(e, 1);
        }
        if ((e[0x11] & 3) & 1)
            func_02023b44(i & 0xff);
        if ((e[0x11] & 3) & 2)
            func_02023828(i & 0xff);
    }
}
