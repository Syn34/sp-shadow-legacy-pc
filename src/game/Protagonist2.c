/*
 * CProtagonist, part 2: what happens when the hero touches another object
 * (standing on platforms and blocks, being pushed back by solid objects,
 * knock-back), with its small vector and rectangle helpers.
 * ARM9 main, 0x020145bc - 0x020157e0 (9 functions).
 *
 * Actor fields used here (see Actor1.c): +0x08 kind, +0x0c class,
 * +0x10 collision rect, +0x24 height, +0x28/+0x2c position, +0x38 flags,
 * +0x44 owner, +0x54/+0x58 previous and +0x5c/+0x60 new position,
 * +0x68 floor height, +0x70/+0x74 velocity, +0xa4 object stood on,
 * +0x100/+0x104 knock-back.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcef0[];           /* RTTI */
extern const fx32 data_020bd078, data_020bd07c, data_020bd080, data_020bd08c, data_020bd094;
extern u8 data_020bd1d8[];           /* RTTI: CTrigger */
extern u8 *data_020c3b0c;            /* object kinds, 0x48 bytes each */
extern u8 data_020d9f9c[];           /* script variables */
extern u8 data_020dee5c[], data_020dee8c[];

void func_02002164(void *v);         /* Vec2 destructor (empty) */
void func_02002bd8(fx32 *out, const fx32 *vec2, const fx32 *len);
void *func_020062e0(void);
void *func_02009394(void);
void func_0204ab18(s32 *rect, void *obj);
s32 func_0204a868(u32 counter);
s32 func_020461d4(void *obj);
s32 func_020504e0(void *self, const s32 *rect);
void func_0205f6b0(void *snd, void *sound);
s32 func_0208f30c(s32 x, s32 y);    /* FX_Atan2 */
void func_020177a8(void *obj);
void func_02015edc(void);
void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */

/* this file's own functions, called before their definition */
void func_020150a0(s32 *out, const s32 *a, const s32 *b);
void func_020150d4(u8 *self, u8 *other);
void func_02015244(s32 *out, const s32 *rect);
s32 func_02015280(const s32 *rect);
s32 func_02015290(u32 kind);
void func_02015310(u8 *self, u8 *other);
void func_020156f4(s32 *out, const s32 *in, s32 sh);

#define FLAG(a, bit) ((s8)BITS(U32_AT(a, 0x38), bit, 1))
#define HERO_FOCUS() PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4)

static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

static s32 RoundShl(s32 v, s32 sh)
{
    float f = (float)SHL(v, sh);
    return (s32)(v > 0 ? 0.5f + f : f - 0.5f);
}

/* undo this frame's movement */
static inline void StayPut(u8 *self)
{
    S32_AT(self, 0x5c) = S32_AT(self, 0x54);
    S32_AT(self, 0x60) = S32_AT(self, 0x58);
}

static void Normalize(s32 *v)
{
    s32 len = func_0208eff8(FxMul(v[0], v[0]) + FxMul(v[1], v[1]));
    if (len <= 1) {
        v[1] = 0;
        v[0] = 0;
    } else {
        s32 out[2];
        func_02002bd8(out, v, &len);
        v[0] = out[0];
        v[1] = out[1];
    }
}

/* is the hero moving away from `other` (direction of this frame's movement
   against the direction from `other` to the hero) */
static s32 MovingAway(u8 *self, u8 *other)
{
    s32 d[2], p[2], e[2];
    func_020150a0(d, (s32 *)(self + 0x5c), (s32 *)(self + 0x54));
    d[0] >>= 4;
    d[1] >>= 4;
    Normalize(d);
    p[0] = S32_AT(self, 0x54) >> 4;
    p[1] = S32_AT(self, 0x58) >> 4;
    func_020150a0(e, p, (s32 *)(other + 0x28));
    Normalize(e);
    return FxMul(d[0], e[0]) + FxMul(d[1], e[1]) > 0;
}

static inline s32 Dist2(s32 dx, s32 dy, s32 dz)
{
    return FxMul(dy, dy) + (FxMul(dx, dx) + FxMul(dz, dz));
}

/* 0x020145bc: the hero touched `other`
 * @difftest $S=ptr:0x120:4 $O=ptr:0x120:4 $W=ptr:0xb0:4 $X=ptr:0x10:4 @$S+0x44:32=$W @$W+0xac:32=$X @$O+0:32=pick:0x020bcbb8 @$O+0xc:32=pick:2,4,5,6,0xc,0xd,0xe,0x10,0x11,3 @$O+8:32=pick:0x70,0x96,0xd3,0x11b,0x11d,0xf0,0xf9,5 @$O+0x38:32=u32 @$S+0x38:32=u32 @$S+0x24:32=int:-0x30000:0x30000 @$O+0x24:32=int:-0x30000:0x30000 @$S+0xa4:32=pick:0 $S $O cases=300 */
void func_020145bc(u8 *self, u8 *other)
{
    u32 cls = U32_AT(other, 0xc);

    if (cls - 4 <= 1) {
        s32 d = (S32_AT(other, 0x24) - S32_AT(self, 0x24)) >> 4;
        if (d < 0)
            d = -d;
        if (d >= 0xc000 && FLAG(other, 19) == 0)
            return;
        if (FLAG(self, 8) && FLAG(other, 21)) {
            if (func_0204a868(3)) {
                if (data_020d9f9c[0x179] == 0) {
                    data_020d9f9c[0x179] = 1;
                    func_0205f6b0(func_02009394(), data_020dee5c);
                }
            } else {
                if (data_020d9f9c[0x17a] == 0) {
                    data_020d9f9c[0x17a] = 1;
                    func_0205f6b0(func_02009394(), data_020dee8c);
                }
            }
        }
        if (FLAG(other, 19) == 0 && U32_AT(other, 8) != 0xf9)
            func_02015310(self, other);
        U32_AT(self, 0x38) |= 8;
        if (FLAG(other, 8) == 0)
            return;
        U32_AT(other, 0x38) &= ~0x100u;
        if (FLAG(other, 19) == 0) {
            s32 v;
            PTR_AT(PTR_AT(PTR_AT(self, 0x44), 0xac), 4) = other;
            v = S32_AT(VCallR1(other, 0x38), 0x70);
            if (v < 0)
                v = -v;
            if (v <= g_Fx3Pi_4)
                U32_AT(self, 0x38) |= 0x400;
        } else {
            U32_AT(self, 0x38) |= 0x400;
        }
        PTR_AT(PTR_AT(PTR_AT(self, 0x44), 0xac), 8) = other;
        return;
    }

    if ((cls == 0xc && BITS(U32_AT(other, 0x38), 2, 1)) ||
        (cls == 6 && BITS(U32_AT(other, 0x38), 2, 1))) {
        s32 h = func_02015290(U32_AT(other, 8));
        if (h > 0) {
            s32 top = RoundShl(h, 16) + S32_AT(other, 0x24);
            if (S32_AT(self, 0x24) >= top) {
                u32 old = U32_AT(self, 0x38);
                s8 was;
                U32_AT(self, 0x38) = old | 0x20000;
                U32_AT(self, 0x110) |= 0x1000;
                was = (s8)BITS(old, 17, 1);
                if (PTR_AT(self, 0xa4) != NULL && PTR_AT(self, 0xa4) != other) {
                    u8 *a = PTR_AT(self, 0xa4);
                    s32 t1 = data_020bd078, t2 = data_020bd07c;
                    s32 da = Dist2(S32_AT(a, 0x28) - S32_AT(self, 0x28),
                                   S32_AT(a, 0x2c) - S32_AT(self, 0x2c), t2);
                    s32 dothr = Dist2(S32_AT(other, 0x28) - S32_AT(self, 0x28),
                                      S32_AT(other, 0x2c) - S32_AT(self, 0x2c), t1);
                    if (dothr < da) {
                        S32_AT(self, 0x68) = top;
                        PTR_AT(data_020deebc[1], 0xa4) = other;
                    }
                } else {
                    S32_AT(self, 0x68) = top;
                    PTR_AT(data_020deebc[1], 0xa4) = other;
                }
                if (was != FLAG(self, 17))
                    data_020deebc[0x20 / 4] |= 0x10;
            } else if (!MovingAway(self, other)) {
                StayPut(self);
            }
        } else if (HERO_FOCUS() != other) {
            if (U32_AT(other, 0xc) == 0xc) {
                if (!MovingAway(self, other))
                    StayPut(self);
            } else {
                func_020150d4(self, other);
            }
        }
        U32_AT(self, 0x38) |= 8;
        return;
    }

    if (cls == 2) {
        u8 *o = other;
        s32 rect[4];
        if (other != NULL)
            o = func_020a6efc(other, data_020bcb88, data_020bd1d8, -1);
        if ((s8)o[0xe9] == 0)
            return;
        func_0204ab18(rect, o);
        if (func_020504e0(self, rect))
            StayPut(self);
        return;
    }
    if (cls - 0xd <= 1) {
        StayPut(self);
        return;
    }
    if (U32_AT(other, 8) == 0x70 ||
        (FLAG(other, 23) && (U32_AT(other, 8) == 0x11d || U32_AT(other, 8) == 0xf0))) {
        func_02015310(self, other);
        return;
    }
    if (cls == 0x11 && FLAG(other, 14)) {
        s32 h = S32_AT(self, 0x24), pos[2], tmp[2];
        void *o;
        pos[0] = S32_AT(other, 0x28);
        pos[1] = S32_AT(other, 0x2c);
        func_02003b08(tmp, pos, 4);
        pos[0] = tmp[0];
        pos[1] = tmp[1];
        o = other;
        if (other != NULL)
            o = func_020a6efc(other, data_020bcb88, data_020bcef0, -1);
        if (h >= func_020461d4(o))
            return;
        if (MovingAway(self, other))
            return;
        if (HERO_FOCUS() == other)
            return;
        if (U32_AT(other, 8) != 0x11b)
            StayPut(self);
        return;
    }
    if (cls == 0x10)
        data_020deebc[0x20 / 4] |= 2;
}

/* 0x020150a0: out = a - b (2D)
 * @difftest ptr:8:4 ptr:8:4 ptr:8:4 */
void func_020150a0(s32 *out, const s32 *a, const s32 *b)
{
    s32 ax = a[0], bx = b[0], ay = a[1], by = b[1];
    out[0] = ax - bx;
    out[1] = ay - by;
}

/* 0x020150d4: push the hero out of an object's rectangle (shrunk by 3) */
void func_020150d4(u8 *self, u8 *other)
{
    s32 rect[4], x0, x1, y0, y1, c[2], pos[2], v[2];

    rect[0] = S32_AT(other, 0x10);
    rect[1] = S32_AT(other, 0x14);
    rect[2] = S32_AT(other, 0x18);
    rect[3] = S32_AT(other, 0x1c);
    x0 = rect[0] + 3;
    x1 = rect[2] - 3;
    y0 = rect[1] + 3;
    y1 = rect[3] - 3;
    if (func_02015280(rect) / 2 < -3) {
        func_02015244(c, rect);
        y1 = c[0];
        y0 = c[0];
        func_02002164(c);
    }
    if (func_02015280(rect) / 2 < -3) {
        func_02015244(c, rect);
        x1 = c[1];
        x0 = c[1];
        func_02002164(c);
    }
    func_020036e8(rect, &x0, &y0, &x1, &y1);
    pos[0] = S32_AT(other, 0x28);
    pos[1] = S32_AT(other, 0x2c);
    func_020038d4(v, pos, 0xc);
    rect[1] += v[0];
    rect[3] += v[0];
    rect[0] += v[1];
    rect[2] += v[1];
    if (func_020504e0(self, rect))
        StayPut(self);
}

/* 0x02015244: centre of a rectangle (y, x)
 * @difftest ptr:8:4 ptr:16:4 */
void func_02015244(s32 *out, const s32 *rect)
{
    s32 r1 = rect[1], r3 = rect[3], r0 = rect[0], r2 = rect[2];
    out[0] = r1 + ((r3 - r1) >> 1);
    out[1] = r0 + ((r2 - r0) >> 1);
}

/* 0x02015280: height of a rectangle
 * @difftest ptr:16:4 */
s32 func_02015280(const s32 *rect) { return rect[3] - rect[1]; }

/* 0x02015290: height of the objects of some kinds the hero can stand on
 * @difftest int:0x90:0xd8 */
s32 func_02015290(u32 kind)
{
    switch (kind) {
    case 0x96:
    case 0x99:
        return 0x20;
    case 0xd1:
        return 0x2c;
    case 0xd2:
        return 0x2c;
    case 0xd3:
        return 0x14;
    }
    return -1;
}

/* wrap an angle into [lo, hi] */
static inline s32 Wrap(s32 a, s32 lo, s32 hi, s32 turn)
{
    while (a < lo)
        a += turn;
    while (a > hi)
        a -= turn;
    return a;
}

/* 0x02015310: knock the hero back sideways from `other` when they overlap
 * @difftest $S=ptr:0x120:4 $O=ptr:0x120:4 $W=ptr:0x20:4 @$S+8:32=int:0:0x100 @$O+8:32=int:0:0x100 @$S+0x28:32=int:0:0x8000 @$S+0x2c:32=int:0:0x8000 @$O+0x28:32=int:0:0x8000 @$O+0x2c:32=int:0:0x8000 @$S+0x44:32=$W @$W+0x14:32=int:-30000:30000 @$S+0x70:32=pick:0,5 @$S+0x74:32=pick:0,5 $S $O cases=200
 * @difftest $S=ptr:0x120:4 $O=ptr:0x120:4 $W=ptr:0x20:4 @$S+8:32=int:0:0x100 @$O+8:32=int:0:0x100 @$S+0x28:32=pick:0 @$S+0x2c:32=pick:0 @$O+0x28:32=int:-0x800:0x800 @$O+0x2c:32=int:-0x800:0x800 @$S+0x44:32=$W @$W+0x14:32=int:-30000:30000 @$S+0x70:32=pick:5,-7 @$S+0x74:32=pick:5 $S $O cases=200 */
void func_02015310(u8 *self, u8 *other)
{
    u8 *kinds = data_020c3b0c;
    s32 r, dx, dy, c, lim, eq, a, heading, diff, dir;
    s32 v[3], out[3];
    u32 idx;

    r = RoundShl(S32_AT(kinds + U32_AT(other, 8) * 0x48, 0x38), 12);
    r = RoundShl(S32_AT(kinds + U32_AT(self, 8) * 0x48, 0x38), 12) + r;
    lim = FxMul(FxMul(r, r), S32_AT(func_020062e0(), 0x370));
    c = data_020bd08c;
    dy = S32_AT(other, 0x2c) - S32_AT(self, 0x2c);
    dx = S32_AT(other, 0x28) - S32_AT(self, 0x28);
    if (Dist2(dx, dy, c) > lim)
        return;
    eq = S32_AT(self, 0x70) == data_020bd080 && S32_AT(self, 0x74) == data_020bd094;
    if ((s8)((s8)eq == 0) == 0)
        return;

    a = func_0208f30c(dx, dy);
    heading = S32_AT(PTR_AT(self, 0x44), 0x14);
    a = Wrap(a, g_FxZero, g_FxTwoPi, g_FxTwoPi);
    diff = a - Wrap(heading, g_FxZero, g_FxTwoPi, g_FxTwoPi);
    diff = Wrap(diff, -g_FxPi, g_FxPi, g_FxTwoPi);
    if (diff < g_FxZero)
        dir = Wrap(g_FxPi_2 + heading, g_FxZero, g_FxTwoPi, g_FxTwoPi);
    else
        dir = Wrap(heading - g_FxPi_2, g_FxZero, g_FxTwoPi, g_FxTwoPi);

    idx = FxRadToIdx(dir) >> 4;
    v[0] = FxMul(FX_SinCosTable_[idx * 2], 0x2000);
    v[1] = FxMul(FX_SinCosTable_[idx * 2 + 1], 0x2000);
    v[2] = dy;
    func_020156f4(out, v, 4);
    S32_AT(self, 0x100) = out[0];
    S32_AT(self, 0x104) = out[1];
}

static inline s32 ShlSigned(s32 v, s32 sh)
{
    if (v < 0)
        return -SHL(-v, sh);
    return SHL(v, sh);
}

/* 0x020156f4: out = in << sh (3D, magnitudes shifted)
 * @difftest ptr:12:4 ptr:12:4 int:0:8 */
void func_020156f4(s32 *out, const s32 *in, s32 sh)
{
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    out[0] = ShlSigned(in[0], sh);
    out[1] = ShlSigned(in[1], sh);
    out[2] = ShlSigned(in[2], sh);
}

/* 0x020157bc */
void func_020157bc(void)
{
    func_020177a8(data_020deebc);
    func_02015edc();
}
