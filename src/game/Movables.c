/*
 * Movable objects: map objects of actor class 4 that the hero can push
 * and pick up, and that push each other aside.
 * ARM9 main, 0x02041498 - 0x0204244c (8 functions).
 *
 * An object is created from a map object (func_0204207c); when its actor
 * kind (data_020c3b0c, 0x48 bytes each) names a second kind at +0x44, a
 * handle actor is made with it (func_02041e40) and kept at +0xa0. The
 * hero touching the handle pushes the object away from him; the hero
 * touching the object while facing it (or in state 0xa) takes it when
 * data_020df100 allows. An optional floating sprite (+0xec) rises until
 * the time at +0xf0.
 * Actor callbacks: +0x94 per frame (func_02041498), +0x98 removal
 * (func_02042078), +0x9c collision (func_02041838 / func_02041628).
 *
 * Testing: with fake actors and models; the model animation, the sound
 * and the sprite placement stubbed where they need real objects.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020bf5b8, data_020bf5bc, data_020bf5c0, data_020bf5c4, data_020bf5c8,
    data_020bf5cc, data_020bf5d0;
extern const char data_020bf5d4[], data_020bf600[], data_020bf630[], data_020bf654[],
    data_020bf684[];                 /* debug messages */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */
extern s32 data_020df100;            /* objects can be picked up */

void func_02053460(void *obj, u32 dt);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
u64 func_020822c8(void *timer);
void func_02003890(void *obj, s32 *pos);
void func_02003b98(void *obj, s32 x, s32 y);
void func_0202dea4(s32 *v);
void func_0202ded8(s32 *v, const s32 *s);
s32 func_0202df68(const s32 *v);
s32 func_02020440(u32 v);
void *func_020062e0(void);
s32 func_0208f30c(s32 x, s32 y);
void func_020156f4(s32 *out, const s32 *in, s32 sh);
u8 *func_02008994(s32 i, s32 k);
u8 *func_02005d5c(s32 type);
s32 func_0200820c(u32 id, s32 kind);
void func_02003c74(void *actor, s32 x, s32 y);
s32 func_0201a0a0(s32 *xz, void *obj);
fx32 func_02001ef4(u32 dir);

void func_02041498(u8 *self);
void func_02041554(u8 *self);
void func_02041628(u8 *self, u8 *other);
void func_02041838(u8 *self, u8 *other);
void func_02041dd8(u8 *self, u32 kind);
u8 *func_02041e40(u32 kind, u8 *owner);
void func_02042078(void);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define KIND(a) (data_020c3b0c + S32_AT(a, 8) * 0x48)
#define LINKS(a) ((u8 *)PTR_AT(PTR_AT(a, 0x44), 0xac))

static inline s32 Wrap(s32 a, s32 lo, s32 hi, s32 turn)
{
    while (a < lo)
        a += turn;
    while (a > hi)
        a -= turn;
    return a;
}

/* the actor's "taken" flag: bit 22 for objects with bit 21, else bit 10 */
static void Take(u8 *self)
{
    if ((s8)BITS(U32_AT(self, 0x38), 21, 1) != 0)
        U32_AT(self, 0x38) |= 0x400000;
    else
        U32_AT(self, 0x38) |= 0x400;
}

/* 0x02041498: per-frame update (the model's animation and the sprite)
 * @difftest $O=zero:0x200 $S=zero:0x110 @$S+0x44:32=$O @$S+4:32=pick:0,8 @$S+0x38:32=pick:0,0x80000,0x1000000,0x1200000,0x1080000 $S stub=0x02053460 cases=20 */
void func_02041498(u8 *self)
{
    u8 *o;

    if ((s8)BITS(U32_AT(self, 0x38), 24, 1) != 0) {
        U32_AT(self, 0x38) &= ~0x1000000u;
        Take(self);
    }
    if (BITS(U32_AT(self, 4), 3, 1) == 0 && (s8)BITS(U32_AT(self, 0x38), 19, 1) == 0)
        return;
    o = PTR_AT(self, 0x44);
    func_02053460(o, func_020822c0(func_020062d0()));
    func_02041554(self);
}

/* 0x02041554: the floating sprite: rise until its time is up
 * @difftest zero:0x110 cases=3
 * @difftest $V=zero:0x10 @$V+0xc:32=0x020343a8 $E=zero:0xc0 @$E+0:32=$V @$E+0xb0:16=int:0:0x40 $S=zero:0x110 @$S+0xec:32=$E @$S+0xf0:32=pick:0,0xffffffff @$S+0xf4:32=pick:0,0x7fffffff @$S+0xf8:32=int:-0x10000:0x10000 @$S+0x54:32=int:-0x100000:0x100000 @$S+0x58:32=int:-0x100000:0x100000 $S cases=40 */
void func_02041554(u8 *self)
{
    u8 *o;
    s32 pos[2];
    u64 now;

    if (PTR_AT(self, 0xec) == NULL)
        return;
    now = func_020822c8(func_020062d0());
    if (now > Get64(self + 0xf0)) {
        void (*m)(void *, u32, void *);

        o = PTR_AT(self, 0xec);
        U32_AT(o, 4) &= ~8u;
        o = PTR_AT(self, 0xec);
        m = VCALL(o, 0xc, void (*)(void *, u32, void *));
        m(o, 1, (void *)m);
        return;
    }
    pos[0] = 0;
    pos[1] = 0;
    S32_AT(self, 0xf8) -= 0x666;
    func_02003890(self, pos);
    pos[1] = 0xc0 - pos[1];
    o = PTR_AT(self, 0xec);
    func_02003b98(o, pos[0] - (U16_AT(o, 0xb0) >> 1), pos[1] + (S32_AT(self, 0xf8) >> 12));
}

/* 0x02041628: a handle touched: the hero pushes its object away
 * @difftest $C=zero:0x10 $B=zero:0x100 @$B+0xac:32=$C @$B+0x10:32=int:0:0x1000 @$B+0x14:32=int:0:0x1000 @$B+0x18:32=int:0:0x1000 $P=zero:0x40 @$P+0x28:32=int:-0x100000:0x100000 @$P+0x2c:32=int:-0x100000:0x100000 $S=zero:0xb0 @$S+0x44:32=$B @$S+0xa0:32=$P @$S+0x38:32=pick:0,0x200000 $D=zero:0x10 $A=zero:0x20 @$A+0x14:32=int:0:0x6487 @$A+0xac:32=$D $O=zero:0x60 @$O+0xc:32=pick:0,4,2 @$O+8:32=pick:1,1,2 @$O+0x28:32=int:-0x100000:0x100000 @$O+0x2c:32=int:-0x100000:0x100000 @$O+0x38:32=pick:0,0x800 @$O+0x44:32=$A @$C+4:32=$O @020deec4:32=$O $S $O stub=0x02020440 cases=100
 * @difftest $C=zero:0x10 $B=zero:0x100 @$B+0xac:32=$C $P=zero:0x40 $S=zero:0xb0 @$S+0x44:32=$B @$S+0xa0:32=$P $D=zero:0x10 $A=zero:0x20 @$A+0xac:32=$D $O=zero:0x60 @$O+0xc:32=pick:0,4 @$O+8:32=pick:1,2 @$O+0x38:32=0x800 @$O+0x44:32=$A @$O+0x28:32=pick:0,0x1000 @020deec4:32=$O $S $O stub=0x02020440 cases=20 */
void func_02041628(u8 *self, u8 *other)
{
    s32 v[3], len;

    if (S32_AT(other, 0xc) == 0) {
        u8 *b, *p;

        if (S32_AT(other, 8) != 1)
            other = HERO();
        if (PTR_AT(LINKS(self), 4) != other)
            return;
        U32_AT(self, 0x38) |= 8;
        U32_AT(self, 0x38) |= 0x400;
        U32_AT(self, 0x38) |= 0x100;
        if ((s8)BITS(U32_AT(other, 0x38), 11, 1) == 0)
            return;
        b = PTR_AT(self, 0x44);
        {
            s32 x = S32_AT(b, 0x10), z = S32_AT(b, 0x18), h = S32_AT(PTR_AT(other, 0x44), 0x14);

            S32_AT(b, 0x10) = x;
            S32_AT(b, 0x14) = h;
            S32_AT(b, 0x18) = z;
            b[0x8c] = 1;
        }
        p = PTR_AT(self, 0xa0);
        v[0] = S32_AT(p, 0x28) - S32_AT(other, 0x28);
        v[1] = data_020bf5b8;
        v[2] = S32_AT(p, 0x2c) - S32_AT(other, 0x2c);
        len = func_0202df68(v);
        if (len > 1)
            func_0202ded8(v, &len);
        else
            func_0202dea4(v);
        S32_AT(self, 0x70) = SHL(FxMul(v[0], 0x6000), 4);
        S32_AT(self, 0x74) = SHL(FxMul(v[2], 0x6000), 4);
        PTR_AT(LINKS(self), 4) = PTR_AT(self, 0xa0);
        func_02020440(0x8f);
        return;
    }
    if (S32_AT(other, 0xc) != 4)
        return;
    if (PTR_AT(LINKS(self), 4) != other)
        return;
    U32_AT(other, 0x38) |= 8;
    U32_AT(other, 0x38) |= 0x400;
    PTR_AT(LINKS(other), 8) = self;
    U32_AT(self, 0x38) |= 0x400;
}

/* the object's push radius, made fx32 through the game's float routines */
static s32 Radius(s32 r)
{
    float f = (float)SHL(r, 12);

    return (s32)(r > 0 ? 0.5f + f : f - 0.5f);
}

/* 0x02041838: an object touched: other objects push it aside, the hero
 * takes it when facing it
 * @difftest $A=zero:0x20 $B=zero:0x20 @$A+4:32=int:-0x4000:0x4000 @$A+8:32=int:-0x4000:0x4000 @$B+4:32=int:-0x4000:0x4000 @$B+8:32=int:-0x4000:0x4000 @$B+0x14:32=int:0:0x6487 $S=zero:0x120 @$S+0x44:32=$B @$S+8:32=pick:4,0x13,0x101,0xf7 $O=zero:0x60 @$O+0xc:32=4 @$O+0x44:32=$A $S $O cases=150
 * @difftest $A=zero:0x20 $B=zero:0x20 $S=zero:0x120 @$S+0x44:32=$B @$S+8:32=4 $O=zero:0x60 @$O+0xc:32=4 @$O+0x44:32=$A @$S+0xa0:32=$O $S $O cases=5
 * @difftest $T=zero:0x40 @$T+0x38:32=0x020343a8 @$T+0x3c:32=0x020343a8 $C=zero:0x10 $D=zero:0x10 $A=zero:0x20 @$A+0xac:32=$C $B=zero:0x20 @$B+0xac:32=$D $S=zero:0x120 @$S+0:32=$T @$S+0x44:32=$B @$S+0x24:32=int:-0x100000:0x100000 @$S+0x28:32=int:-0x100000:0x100000 @$S+0x2c:32=int:-0x100000:0x100000 @$S+0x38:32=pick:0,0x200000 $O=zero:0x80 @$O+0:32=$T @$O+0xc:32=pick:0,0,0,2 @$O+8:32=1 @$O+0x44:32=$A @$O+0x24:32=int:-0x100000:0x100000 @$O+0x28:32=int:-0x100000:0x100000 @$O+0x2c:32=int:-0x100000:0x100000 @$O+0x38:32=pick:0,0x100,0x100 @$O+0x64:16=pick:0xa,3 @$O+0x66:16=int:0:0x100 @020df100:32=pick:0,1,1 $S $O cases=200
 * @difftest $T=zero:0x40 @$T+0x38:32=0x020343a8 @$T+0x3c:32=0x020343a8 $C=zero:0x10 $D=zero:0x10 $A=zero:0x20 @$A+0xac:32=$C $B=zero:0x20 @$B+0xac:32=$D $S=zero:0x120 @$S+0:32=$T @$S+0x44:32=$B @$S+0x24:32=int:-0x80000:0x80000 $H=zero:0x80 @$H+0:32=$T @$H+0x44:32=$A @$H+0x24:32=int:-0x80000:0x80000 @$H+0x38:32=0x100 @$H+0x64:16=pick:0xa,3 @020deec4:32=$H @020df100:32=1 $O=zero:0x80 @$O+8:32=2 $S $O cases=50 */
void func_02041838(u8 *self, u8 *other)
{
    s32 v[3], out[3], d, ang, h, diff, dir;

    if (S32_AT(other, 0xc) == 4) {
        u8 *a, *b;
        s32 r, lim, dx, dz;
        u32 idx;

        r = Radius(S32_AT(KIND(self), 0x38));
        lim = FxMul(FxMul(r, r), S32_AT(func_020062e0(), 0x370));
        if (PTR_AT(self, 0xa0) != NULL && PTR_AT(self, 0xa0) == other)
            return;
        a = PTR_AT(other, 0x44);
        b = PTR_AT(self, 0x44);
        dz = S32_AT(b, 8) - S32_AT(a, 8);
        dx = S32_AT(a, 4) - S32_AT(b, 4);
        v[0] = dx;
        v[1] = data_020bf5d0;
        v[2] = dz;
        if (FxMul(dz, dz) + (FxMul(dx, dx) + FxMul(v[1], v[1])) > lim)
            return;
        ang = func_0208f30c(dx, dz);
        h = S32_AT(PTR_AT(self, 0x44), 0x14);
        ang = Wrap(ang, g_FxZero, g_FxTwoPi, g_FxTwoPi);
        diff = ang - Wrap(h, g_FxZero, g_FxTwoPi, g_FxTwoPi);
        diff = Wrap(diff, -g_FxPi, g_FxPi, g_FxTwoPi);
        if (diff < g_FxZero)
            dir = Wrap(g_FxPi_2 + h, g_FxZero, g_FxTwoPi, g_FxTwoPi);
        else
            dir = Wrap(h - g_FxPi_2, g_FxZero, g_FxTwoPi, g_FxTwoPi);
        idx = FxRadToIdx(dir) >> 4;
        v[0] = FxMul(FX_SinCosTable_[idx * 2], 0x1000);
        v[1] = FxMul(FX_SinCosTable_[idx * 2 + 1], 0x1000);
        func_020156f4(out, v, 4);
        S32_AT(self, 0x100) = out[0];
        S32_AT(self, 0x104) = out[1];
        return;
    }
    if (S32_AT(other, 0xc) != 0)
        return;
    if (S32_AT(other, 8) != 1)
        other = HERO();
    d = (S32_AT(other, 0x24) - S32_AT(self, 0x24)) >> 4;
    if (d < 0)
        d = -d;
    if (d >= 0x8000)
        return;
    U32_AT(self, 0x38) |= 8;
    if ((s8)BITS(U32_AT(other, 0x38), 8, 1) == 0)
        return;
    v[0] = S32_AT(self, 0x28) - S32_AT(other, 0x28);
    v[1] = data_020bf5c0;
    v[2] = S32_AT(self, 0x2c) - S32_AT(other, 0x2c);
    ang = func_0208f30c(v[0], v[2]);
    {
        s32 (*m)(void *, void *) = VCALL(other, 0x3c, s32 (*)(void *, void *));
        h = m(other, (void *)m);
    }
    ang = Wrap(ang, g_FxZero, g_FxTwoPi, g_FxTwoPi);
    h = Wrap(h, g_FxZero, g_FxTwoPi, g_FxTwoPi);
    diff = Wrap(ang - h, -g_FxPi, g_FxPi, g_FxTwoPi);
    if (diff < 0)
        diff = -diff;
    if (diff > g_FxPi_2) {
        u8 *(*m)(void *, void *) = VCALL(other, 0x38, u8 *(*)(void *, void *));
        if (U16_AT(m(other, (void *)m), 0x64) != 0xa)
            return;
    }
    if (data_020df100 == 0)
        return;
    {
        u8 *(*m)(void *, void *) = VCALL(other, 0x38, u8 *(*)(void *, void *));
        u16 t = U16_AT(m(other, (void *)m), 0x66);

        m = VCALL(self, 0x38, u8 *(*)(void *, void *));
        U16_AT(m(self, (void *)m), 0x66) = t;
    }
    Take(self);
    PTR_AT(LINKS(self), 8) = other;
    PTR_AT(LINKS(other), 4) = self;
}

/* 0x02041dd8: give an object its model (actor kind `kind`)
 * @difftest $O=zero:0x200 $S=zero:0x60 @$S+0x44:32=$O $S int:0:3 stub=0x02053648 cases=5 */
void func_02041dd8(u8 *self, u32 kind)
{
    u8 *o;

    func_02053648(PTR_AT(self, 0x44), kind, self, 0, 0);
    func_02004490(data_020bf5d4);
    o = PTR_AT(self, 0x44);
    U32_AT(o, 0x18c) |= 2;
    o = PTR_AT(self, 0x44);
    S32_AT(o, 4) = 0;
    S32_AT(o, 8) = 0;
    S32_AT(o, 0xc) = 0;
    o[0x8c] = 1;
}

/* 0x02041e40: make the handle (actor kind `kind`) of object `owner`
 * @difftest int:0:3 $O=zero:0x120 @$O+0xc:32=pick:0,1,2,3,4,5,6,7 @$O+0x38:32=pick:0,0x20,0x40,0x60 @$O+0x54:32=int:0:0x100 @$O+0x58:32=int:0:0x100 $O cases=16 */
u8 *func_02041e40(u32 kind, u8 *owner)
{
    u8 *a = func_02005d5c(S32_AT(owner, 0xc));
    u8 *o;
    s32 t[2];

    S32_AT(a, 0xc) = 4;
    S32_AT(a, 8) = kind;
    U32_AT(a, 4) &= ~8u;
    U32_AT(a, 0x38) |= 0x40000;
    U32_AT(a, 0x110) |= 0x100;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x20u) | (BITS(U32_AT(owner, 0x38), 5, 1) << 5);
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x40u) | (BITS(U32_AT(owner, 0x38), 6, 1) << 6);
    U32_AT(a, 0x38) |= 0x80000;
    PTR_AT(a, 0x98) = (void *)func_02042078;
    PTR_AT(a, 0x94) = (void *)func_02041498;
    PTR_AT(a, 0x9c) = (void *)func_02041628;
    S32_AT(a, 0x10) = data_020bf5cc;
    S32_AT(a, 0x14) = data_020bf5c4;
    S32_AT(a, 0x18) = data_020bf5c8;
    S32_AT(a, 0x1c) = data_020bf5bc;
    U32_AT(a, 0x38) |= 4;
    func_02041dd8(a, S32_AT(a, 8));
    S32_AT(PTR_AT(a, 0x44), 0xa4) = S32_AT(KIND(a), 0x34);
    o = PTR_AT(a, 0x44);
    U32_AT(o, 0x18c) = (U32_AT(o, 0x18c) & ~1u) | ((s8)S32_AT(KIND(a), 0x30) & 1);
    func_02003c74(a, S32_AT(owner, 0x54), S32_AT(owner, 0x58));
    t[0] = S32_AT(a, 0x54);
    t[1] = S32_AT(a, 0x58);
    S32_AT(a, 0x24) = func_0201a0a0(t, a);
    S32_AT(a, 0x6c) = S32_AT(a, 0x24);
    S32_AT(a, 0x50) = 0x20000;
    a[0x10d] = 0;
    PTR_AT(LINKS(a), 4) = HERO();
    func_02004490(data_020bf600);
    o = PTR_AT(a, 0x44);
    U32_AT(o, 0x18c) &= ~2u;
    S32_AT(LINKS(a), 0xc) = S32_AT(KIND(a), 0x20);
    PTR_AT(a, 0xa0) = owner;
    func_02004490(data_020bf630);
    return a;
}

/* 0x02042078: the removal callback
 * @difftest cases=3 */
void func_02042078(void)
{
}

/* 0x0204207c: create movable object `id` (of map object list `k`)
 * @difftest int:0:4 int:0:2 cases=5 */
u8 *func_0204207c(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    u32 kind = U16_AT(def, 8);
    u8 *a, *o;
    s32 t[2], w, d, sc;
    fx32 r;

    a = func_02005d5c(S32_AT(def, 0));
    S32_AT(a, 0xc) = 4;
    S32_AT(a, 8) = func_0200820c(kind, 1);
    U32_AT(a, 4) |= 8;
    U32_AT(a, 0x38) &= ~0x40000u;
    U32_AT(a, 0x110) |= 0x100;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x20u) | ((def[0x21] & 1) << 5);
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x40u) | ((def[0x22] & 1) << 6);
    {
        void (*m)(void *, u32, void *) = VCALL(a, 0xc, void (*)(void *, u32, void *));
        m(a, 0, (void *)m);
    }
    PTR_AT(a, 0x98) = (void *)func_02042078;
    PTR_AT(a, 0x94) = (void *)func_02041498;
    PTR_AT(a, 0x9c) = (void *)func_02041838;
    func_02041dd8(a, S32_AT(a, 8));
    S32_AT(PTR_AT(a, 0x44), 0xa4) = S32_AT(KIND(a), 0x34);
    o = PTR_AT(a, 0x44);
    U32_AT(o, 0x18c) = (U32_AT(o, 0x18c) & ~1u) | ((s8)S32_AT(KIND(a), 0x30) & 1);
    w = S32_AT(KIND(a), 0x38);
    d = S32_AT(KIND(a), 0x3c);
    S32_AT(a, 0x10) = -d;
    S32_AT(a, 0x14) = -w;
    S32_AT(a, 0x18) = d;
    S32_AT(a, 0x1c) = w;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~4u) | ((S32_AT(KIND(a), 0x40) > 0) << 2);
    sc = S32_AT(KIND(a), 0x2c);
    o = PTR_AT(a, 0x44);
    S32_AT(o, 0x1c) = sc;
    S32_AT(o, 0x20) = sc;
    S32_AT(o, 0x24) = sc;
    o[0x8c] = 1;
    func_02003c74(a, SHL(S16_AT(def, 4), 16), SHL(S16_AT(def, 6), 16));
    S32_AT(a, 0x30) = S32_AT(a, 0x28);
    S32_AT(a, 0x34) = S32_AT(a, 0x2c);
    t[0] = S32_AT(a, 0x54);
    t[1] = S32_AT(a, 0x58);
    S32_AT(a, 0x24) = func_0201a0a0(t, a);
    S32_AT(a, 0x6c) = S32_AT(a, 0x24);
    S32_AT(a, 0x64) = S32_AT(a, 0x24);
    S32_AT(a, 0x50) = S32_AT(KIND(a), 0x28);
    a[0x10d] = def[0x1e];
    r = func_02001ef4(a[0x10d]);
    o = PTR_AT(a, 0x44);
    {
        s32 x = S32_AT(o, 0x10), z = S32_AT(o, 0x18);

        S32_AT(o, 0x10) = x;
        S32_AT(o, 0x14) = r;
        S32_AT(o, 0x18) = z;
        o[0x8c] = 1;
    }
    PTR_AT(LINKS(a), 4) = HERO();
    func_02004490(data_020bf654);
    S32_AT(LINKS(a), 0xc) = S32_AT(KIND(a), 0x20);
    if (S32_AT(KIND(a), 0x44) != 0)
        PTR_AT(a, 0xa0) = func_02041e40(S32_AT(KIND(a), 0x44), a);
    switch (S32_AT(a, 8)) {
    case 4: case 0x13: case 0x14:
    case 0x101: case 0x102: case 0x103: case 0x104:
        U32_AT(a, 0x38) |= 0x800;
        break;
    }
    switch (S32_AT(a, 8)) {
    case 0xf7: case 0xfa: case 0xfb: case 0xfc:
    case 0x101: case 0x102: case 0x103: case 0x104: case 0x10a:
        U32_AT(a, 0x38) |= 0x200000;
        break;
    }
    func_02004490(data_020bf684);
    return a;
}
