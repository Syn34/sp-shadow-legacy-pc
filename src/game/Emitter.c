/*
 * Particle effect players: an object (base func_02058ed0, vtables
 * data_020c0548 and data_020c0570 at +0xac) with 60 particles of 0xfc
 * bytes from +0xb4 that plays the frames of its effect data (EffectRef at
 * +0x3c7c, see EffectData.c): each frame places, turns and scales the
 * particles its keys name, scaled by +0x3c78.
 * ARM9 main, 0x02051634 - 0x02051e08 (14 functions).
 *
 * Fields: +0x3bc4 playing, +0x3bc5 looping, +0x3bcc, +0x3c58 start time,
 * +0x3c68 length, +0x3c6c time into the frame, +0x3c70 frame, +0x3c74
 * particles in use, +0x3c78 scale, +0x3c80 the effect's slot. A particle
 * (object layout) has +0xc4 free, +0xc5, +0xb4 its index. This-adjusting
 * entry points subtract 0xac.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c0548[], data_020c0570[];   /* vtables */
extern u8 *data_020ebd34;            /* resource slots, 0xc bytes each */

void *func_020062d0(void);
u64 func_020822c8(void *timer);
s32 func_0208f070(s32 num, s32 den);
void func_02056a0c(u8 *self, u8 *p);
void func_02056ddc(u8 *self, u8 *p);
void func_0205bc6c(u8 *p, u32 dt);
void func_02052dac(u8 *ref, u32 id);
u8 *func_02052ce4(u8 *ref);
u8 *func_02052ee4(u8 *ref);
void func_02058ed0(void *self);
void func_020589f8(void *self);
void func_0207fe44(void *p);

void func_020516bc(u8 *self, u32 k);
void func_020517fc(u8 *self);
u8 *func_02051ce8(u8 *self);
u8 *func_02051d34(u8 *self);

#define EFFECT(self) ((u8 *)PTR_AT(data_020ebd34 + U32_AT(self, 0x3c80) * 0xc, 0))
#define FREE(self, i) S8_AT(self, (i) * 0xfc + 0x178)

/* 0x02051634: start playing
 * @difftest zero:0x3d00 cases=3 */
void func_02051634(u8 *self)
{
    u64 t;

    self[0x3bc4] = 1;
    self[0x3bc5] = 0;
    t = func_020822c8(func_020062d0());
    U32_AT(self, 0x3c58) = (u32)t;
    U32_AT(self, 0x3c5c) = (u32)(t >> 32);
    U32_AT(self, 0x3c70) = 0;
}

/* 0x02051674: set up particle `p` (it lives as long as the effect)
 * @difftest $X=zero:8 @$X+4:32=int:0:20 $T=zero:0x18 @$T:32=$X @$T+0xc:32=$X @020ebd34:32=$T $S=zero:0x3d00 @$S+0x3c80:32=int:0:2 $S ptr:0x100:4 cases=5 */
void func_02051674(u8 *self, u8 *p)
{
    s32 n = S32_AT(EFFECT(self), 4);

    p[0xc4] = 0;
    p[0xc5] = 1;
    S32_AT(p, 0xac) = n * 0x87;
    S32_AT(p, 0xb0) = n * 0x87;
}

/* 0x020516bc: use the particles of frame `k` only
 * @difftest $K=zero:0x40 @$K:32=int:0:60 @$K+0x20:32=int:0:60 $R=zero:0x18 @$R:32=$K @$R+4:32=int:0:3 @$R+0xc:32=$K @$R+0x10:32=int:0:3 $X=zero:8 @$X:32=$R $T=zero:0xc @$T:32=$X @020ebd34:32=$T $S=zero:0x3d00 $S int:0:2 cases=10 */
void func_020516bc(u8 *self, u32 k)
{
    u8 *e;
    s32 j;
    u32 i;

    for (i = 0; i < 0x3c; i++)
        FREE(self, i) = 1;
    e = EFFECT(self);
    for (j = 0; j < S32_AT((u8 *)PTR_AT(e, 0) + k * 0xc, 4); j++) {
        u32 n = U32_AT(PTR_AT((u8 *)PTR_AT(e, 0) + k * 0xc, 0), j * 0x20);

        FREE(self, n) = 0;
    }
    U32_AT(self, 0x3c74) = S32_AT((u8 *)PTR_AT(e, 0) + k * 0xc, 4);
}

/* 0x0205176c: start a free particle
 * @difftest $T=zero:0x20 @$T+0x1c:32=0x02043814 $S=zero:0x3d00 @$S:32=$T @$S+0x178:8=pick:0,1 @$S+0x274:8=pick:0,1 $S cases=8 */
void func_0205176c(u8 *self)
{
    u32 i;

    for (i = 0; i < 0x3c; i++) {
        if (FREE(self, i) != 0)
            break;
    }
    if (i >= 0x3c)
        return;
    (*(void (**)(u8 *, u8 *))(*(u8 **)self + 0x1c))(self, self + 0xb4 + i * 0xfc);
    U32_AT(self, 0x3c74)++;
}

/* 0x020517fc: update the particles in use
 * @difftest $S=zero:0x3d00 @$S+0x178:8=1 @$S+0x274:8=pick:0,1 $S stub=0x02056a0c cases=5 */
void func_020517fc(u8 *self)
{
    u32 i;

    U32_AT(self, 0x3c74) = 0;
    for (i = 0; i < 0x3c; i++) {
        if (FREE(self, i) == 0) {
            func_02056a0c(self, self + 0xb4 + i * 0xfc);
            U32_AT(self, 0x3c74)++;
        }
    }
}

/* 0x02051880: play `dt` ticks
 * @difftest $S=zero:0x3d00 @$S+0x3bc4:8=0 $S u32 cases=2
 * @difftest $K=ptr:0x40:4 @$K:32=int:0:60 @$K+0x20:32=int:0:60 $R=zero:0x18 @$R:32=$K @$R+4:32=int:0:3 @$R+0xc:32=$K @$R+0x10:32=int:0:3 $X=zero:8 @$X:32=$R @$X+4:32=int:0:3 $T=zero:0xc @$T:32=$X @020ebd34:32=$T $V=zero:0x20 @$V+0x18:32=0x02043814 $S=zero:0x3d00 @$S:32=$V @$S+0x3bc4:8=1 @$S+0x3bc5:8=pick:0,1 @$S+0x3c70:32=int:0:2 @$S+0x3c74:32=2 @$S+0x3c78:32=int:0x800:0x2000 @$S+0x3c6c:32=int:0:0x200 @$S+0x22d:8=pick:0,1 $S int:0:0x100 stub=0x0205bc6c,0x02056ddc cases=40 */
void func_02051880(u8 *self, u32 dt)
{
    u8 *e;
    u32 k, i;
    s32 q, r;
    u8 on;

    if (S8_AT(self, 0x3bc4) == 0)
        return;
    e = EFFECT(self);
    k = U32_AT(self, 0x3c70);
    if (k >= U32_AT(e, 4)) {
        U32_AT(self, 0x3c74) = 0;
        self[0x3bc4] = 0;
        U32_AT(self, 0x3c70) = 0;
        return;
    }
    while (U32_AT((u8 *)PTR_AT(e, 0) + k * 0xc, 4) > U32_AT(self, 0x3c74)) {
        (*(void (**)(u8 *))(*(u8 **)self + 0x18))(self);
        k = U32_AT(self, 0x3c70);
    }
    if (U32_AT((u8 *)PTR_AT(e, 0) + k * 0xc, 4) < U32_AT(self, 0x3c74))
        func_020516bc(self, k);
    on = 0;
    if (U32_AT(self, 0x3c74) != 0)
        on = 1;
    for (i = 0; i < U32_AT(self, 0x3c74); i++) {
        u8 *kf = (u8 *)PTR_AT((u8 *)PTR_AT(e, 0) + U32_AT(self, 0x3c70) * 0xc, 0) + i * 0x20;
        u8 *p = self + i * 0xfc;
        s32 a, b, c, y, z;

        S32_AT(p, 0x150) = 0;
        S32_AT(p, 0x154) = 0;
        a = FxMul(S32_AT(kf, 0xc), S32_AT(self, 0x3c78));
        b = FxMul(S32_AT(kf, 0x10), S32_AT(self, 0x3c78));
        c = FxMul(S32_AT(kf, 0x8), S32_AT(self, 0x3c78));
        S32_AT(p, 0xb8) = c;
        S32_AT(p, 0xbc) = a;
        S32_AT(p, 0xc0) = b;
        p[0x140] = 1;
        S32_AT(p, 0x170) = S32_AT(kf, 4);
        a = FxMul(S32_AT(kf, 0x18), S32_AT(self, 0x3c78));
        y = S32_AT(p, 0xd4);
        z = S32_AT(p, 0xd8);
        S32_AT(p, 0xd0) = a;
        S32_AT(p, 0xd4) = y;
        S32_AT(p, 0xd8) = z;
        p[0x140] = 1;
        b = FxMul(S32_AT(kf, 0x1c), S32_AT(self, 0x3c78));
        z = S32_AT(p, 0xd8);
        S32_AT(p, 0xd0) = S32_AT(p, 0xd0);
        S32_AT(p, 0xd4) = b;
        S32_AT(p, 0xd8) = z;
        p[0x140] = 1;
        c = S32_AT(kf, 0x14);
        S32_AT(p, 0xc4) = g_FxZero;
        S32_AT(p, 0xc8) = g_FxZero;
        S32_AT(p, 0xcc) = c;
        p[0x140] = 1;
        func_0205bc6c(p + 0xb4, dt);
        if (S8_AT(p, 0x179) != 0 && S32_AT(p, 0x168) < S32_AT(self, 0x3bcc))
            func_02056ddc(self, p + 0xb4);
    }
    /* (as in the original: a looping effect with no particles stops) */
    if (S8_AT(self, 0x3bc5) != 0)
        self[0x3bc4] = on;
    U32_AT(self, 0x3c6c) += dt;
    q = func_0208f070(U32_AT(self, 0x3c6c), 0x87) >> 12;
    U32_AT(self, 0x3c70) += q;
    r = (s32)(q != 0 ? 0.5f + (float)(u32)SHL(q, 12) : (float)(u32)SHL(q, 12) - 0.5f);
    U32_AT(self, 0x3c6c) -= FxMul(r, 0x87);
}

/* 0x02051bfc: stop: free all the particles
 * @difftest zero:0x3d00 cases=3 */
void func_02051bfc(u8 *self)
{
    u32 i;

    for (i = 0; i < 0x3c; i++)
        FREE(self, i) = 1;
    U32_AT(self, 0x3c70) = 0;
    U32_AT(self, 0x3c6c) = 0;
    self[0x3bc5] = 0;
    U32_AT(self, 0x3c74) = 0;
}

/* 0x02051c38: load effect `id`
 * @difftest $X=zero:8 @$X+4:32=int:-2:20 $T=zero:0x18 @$T:32=$X @$T+0xc:32=$X @020ebd34:32=$T $S=zero:0x3d00 @$S+0x3c80:32=int:0:2 $S u32 stub=0x02052dac cases=10 */
void func_02051c38(u8 *self, u32 id)
{
    s32 n, r;

    func_02052dac(self + 0x3c7c, id);
    n = S32_AT(EFFECT(self), 4);
    r = (s32)(n > 0 ? 0.5f + (float)SHL(n, 12) : (float)SHL(n, 12) - 0.5f);
    S32_AT(self, 0x3c68) = FxMul(r, 0x87);
    U32_AT(self, 0x3c6c) = 0;
}

/* 0x02051ce8: deleting destructor
 * @difftest zero:0x3d00 stub=0x02052ce4,0x020589f8,0x0207fe44 cases=3 */
u8 *func_02051ce8(u8 *self)
{
    PTR_AT(self, 0) = data_020c0548;
    PTR_AT(self, 0xac) = data_020c0570;
    func_02052ce4(self + 0x3c7c);
    func_020589f8(self);
    func_0207fe44(self);
    return self;
}

/* 0x02051d34: destructor
 * @difftest zero:0x3d00 stub=0x02052ce4,0x020589f8 cases=3 */
u8 *func_02051d34(u8 *self)
{
    PTR_AT(self, 0) = data_020c0548;
    PTR_AT(self, 0xac) = data_020c0570;
    func_02052ce4(self + 0x3c7c);
    func_020589f8(self);
    return self;
}

/* 0x02051d78: constructor
 * @difftest zero:0x3d00 stub=0x02058ed0,0x02052ee4 cases=3 */
u8 *func_02051d78(u8 *self)
{
    func_02058ed0(self);
    PTR_AT(self, 0) = data_020c0548;
    PTR_AT(self, 0xac) = data_020c0570;
    U32_AT(self, 0x3c68) = 0;
    U32_AT(self, 0x3c6c) = 0;
    U32_AT(self, 0x3c70) = 0;
    U32_AT(self, 0x3c74) = 0;
    U32_AT(self, 0x3c78) = 0x1000;
    func_02052ee4(self + 0x3c7c);
    return self;
}

/* The this-adjusting entry points (the object is 0xac before them). */

/* 0x02051dd8
 * @difftest $S=zero:0x3d00 $S+0xac stub=0x02052ce4,0x020589f8,0x0207fe44 cases=3 */
u8 *func_02051dd8(u8 *a)
{
    return func_02051ce8(a - 0xac);
}

/* 0x02051de8
 * @difftest $S=zero:0x3d00 $S+0xac stub=0x02052ce4,0x020589f8 cases=3 */
u8 *func_02051de8(u8 *a)
{
    return func_02051d34(a - 0xac);
}

/* 0x02051df8
 * @difftest $S=zero:0x3d00 $S+0xac stub=0x02056a0c cases=3 */
void func_02051df8(u8 *a)
{
    func_020517fc(a - 0xac);
}
