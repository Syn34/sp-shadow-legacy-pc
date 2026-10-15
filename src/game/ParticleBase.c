/*
 * The particle emitter base class (base of Emitter.c; constructor
 * func_02058ed0): 60 particles of 0xfc bytes from +0xb4 that are spawned
 * with a random direction and speed and live for a while.
 * ARM9 main, 0x02056288 - 0x02057464 (10 functions).
 *
 * Emitter fields: +0x3bc4 running, +0x3bc5 finished spawning, +0x3bc6
 * counts down spawns, +0x3bc8 spawns left, +0x3bcc the age (0x1000 = 1)
 * after which particles may respawn, +0x3bd4 the texture's resource slot,
 * +0x3bdc the object followed (+0x3be0 / +0x3bec its last two positions),
 * +0x3bf8 spawn offset, +0x3c08 spawn interval, +0x3c10 particles left to
 * start, +0x3c14 life, +0x3c18 size, +0x3c1c spin, +0x3c20 velocity,
 * +0x3c2c velocity spread angle, +0x3c30 / +0x3c38 scale range, +0x3c40
 * alpha, +0x3c44 alpha mode, +0x3c48 spawn on a sphere, +0x3c4c its
 * radius vector, +0x3c58 next spawn time, +0x3c60 rotate the offset,
 * +0x3c64 preset.
 * Particle fields (object layout): +0x90 velocity, +0x9c / +0xa0 spin,
 * +0xac / +0xb0 life, +0xb4 age, +0xb8 age at respawn, +0xbc alpha,
 * +0xc0 alpha mode, +0xc4 free, +0xc5 new, +0xc8 / +0xcc size, +0xd0
 * scale.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 *data_020ebd34;            /* resource slots, 0xc bytes each */
extern const fx32 g_FxTwoPi;

int rand(void);
u64 func_020a500c(u32 num, u32 den);
void func_0205bb8c(u8 *p);
void func_020150a0(s32 *out, const s32 *a, const s32 *b);
void func_02002bd8(fx32 *out, const fx32 *vec2, const fx32 *len);
void func_02005b34(void *dst, const void *src);
void func_02002164(void *v);
void func_020297d4(s32 *out, const s32 *v, const s32 *s);
void func_02005b98(s32 *out, const void *src);
void func_0202de98(s32 *m);
void func_02017550(s32 *out, const void *obj);
void func_0202de68(s32 *m, const s32 *v);
void func_020022e4(void *self);
void func_0201751c(s32 *out, const void *obj);
void func_0202dc88(s32 *m, const s32 *rot);
void func_02002974(void *self);
void func_0202dc6c(s32 *m, const s32 *v);
void MI_Copy64B(const void *src, void *dest);
void func_020022e8(s32 *out, const s32 *mtx44, const s32 *in);
void func_0205b958(u8 *p, const s32 *mtx);
void *func_020062e0(void);
void *func_020062d0(void);
void *func_02018208(void);
void func_02080f60(void *settings, u32 a, u32 b);
void func_02080e84(void *settings);
void func_02080bf4(void *settings, void *m);
s32 func_0208f070(s32 num, s32 den);
void func_02082e48(void *a, void *b);
u64 func_020822c8(void *timer);
u64 func_020822a8(void *timer, s32 ms);
void func_02047c94(s32 *out, const void *m);
void func_0202a104(s32 *out, const s32 *a, const s32 *b);
void func_02029790(s32 *out, const s32 *a, const s32 *b);
s32 func_0208eff8(s32 v);
s32 func_0202df68(const s32 *v);
void func_0202ded8(s32 *v, const s32 *s);
void func_0202dea4(s32 *v);
void func_0205bc6c(u8 *p, u32 dt);
void func_02071c2c(void *ref, u32 id);

void func_02056938(u8 *p, s32 x, s32 y, s32 z);
void func_02056a0c(u8 *self, u8 *p);
void func_02056ddc(u8 *self, u8 *p);

#define F(self, off) S32_AT(self, 0x3000 + (off))
#define FREE(self, i) S8_AT(self, (i) * 0xfc + 0x178)
#define PARTICLE(self, i) ((self) + 0xb4 + (i) * 0xfc)

/* a random angle in [-a, a] */
static s32 RandAngle(s32 a)
{
    s32 lo = -a;

    return lo + (s32)(func_020a500c((u32)rand() & 0x7fff, a - lo + 1) >> 32);
}

/* rotate (x, z) of v by angle `a` about y */
static void TurnY(s32 *v, s32 a)
{
    u32 idx = FxRadToIdx(a) >> 4;
    s32 s = FX_SinCosTable_[idx * 2], c = FX_SinCosTable_[idx * 2 + 1];
    s32 x = v[0], z = v[2];

    v[0] = FxMul(x, c) + FxMul(z, s);
    v[2] = FxMul(z, c) - FxMul(x, s);
}

/* rotate (y, z) of v by angle `a` about x */
static void TurnX(s32 *v, s32 a)
{
    u32 idx = FxRadToIdx(a) >> 4;
    s32 s = FX_SinCosTable_[idx * 2], c = FX_SinCosTable_[idx * 2 + 1];
    s32 y = v[1], z = v[2];

    v[1] = FxMul(y, c) - FxMul(z, s);
    v[2] = FxMul(z, c) + FxMul(y, s);
}

/* the object's matrix (rebuilt if dirty) */
static void Matrix(u8 *o, s32 *m)
{
    if (S8_AT(o, 0x8c) == 1) {
        s32 t[3], r[3], s[3];

        func_0202de98((s32 *)(o + 0x28));
        func_02017550(t, o);
        func_0202de68((s32 *)(o + 0x28), t);
        func_020022e4(t);
        func_0201751c(r, o);
        func_0202dc88((s32 *)(o + 0x28), r);
        func_02002974(r);
        func_02005b98(s, o);
        func_0202dc6c((s32 *)(o + 0x28), s);
        func_020022e4(s);
    }
    MI_Copy64B(o + 0x28, m);
}

/* 0x02056288: spawn particle `p`
 * @difftest $S=zero:0x3d00 @$S+0x3c14:32=int:1:0x10000 @$S+0x3c18:32=u32 @$S+0x3c20:32=int:-0x2000:0x2000 @$S+0x3c24:32=int:-0x2000:0x2000 @$S+0x3c28:32=int:-0x2000:0x2000 @$S+0x3c2c:32=pick:0,0x800 @$S+0x3c48:32=pick:0,1 @$S+0x3bcc:32=pick:0,0x1000 @$S+0x3c60:8=pick:0,1 $S ptr:0xfc:4 stub=0x0205bb8c cases=40 */
void func_02056288(u8 *self, u8 *p)
{
    s32 life, a[2], b[2], v[3];

    func_0205bb8c(p);
    life = F(self, 0xc14);
    p[0xc4] = 0;
    p[0xc5] = 1;
    S32_AT(p, 0xac) = life;
    S32_AT(p, 0xb0) = life;
    S32_AT(p, 0xc8) = F(self, 0xc18);
    S32_AT(p, 0xcc) = F(self, 0xc18);
    func_020150a0(a, (s32 *)(self + 0x3c38), (s32 *)(self + 0x3c30));
    func_02002bd8(b, a, (s32 *)(p + 0xac));
    func_02005b34(p + 0xd0, b);
    func_02002164(b);
    func_02002164(a);
    func_02056938(p, F(self, 0xc30), F(self, 0xc34), 0);
    S32_AT(p, 0x9c) = F(self, 0xc1c) + S32_AT(self, 0x9c);
    S32_AT(p, 0xa0) = S32_AT(p, 0x9c);
    S32_AT(p, 0xa4) = 0;
    S32_AT(p, 0xbc) = F(self, 0xc40);
    S32_AT(p, 0xc0) = F(self, 0xc44);
    v[0] = F(self, 0xc20);
    v[1] = F(self, 0xc24);
    v[2] = F(self, 0xc28);
    if (F(self, 0xc2c) != 0) {
        TurnY(v, RandAngle(F(self, 0xc2c)));
        TurnX(v, RandAngle(F(self, 0xc2c)));
    }
    S32_AT(p, 0x90) = v[0];
    S32_AT(p, 0x94) = v[1];
    S32_AT(p, 0x98) = v[2];
    if (F(self, 0xc48) == 0) {
        if (F(self, 0xbcc) == 0x1000) {
            s32 o[3], pa[3], pb[3], pc[3];

            if (S8_AT(self, 0x3c60) != 0) {
                s32 t[3];

                func_020297d4(t, v, (s32 *)(self + 0x3bf8));
                o[0] = t[0];
                o[1] = t[1];
                func_02005b98(pa, self);
                func_02005b98(pb, self);
                func_02005b98(pc, self);
                S32_AT(p, 4) = o[0] + pa[0];
                S32_AT(p, 8) = o[1] + pb[1];
                S32_AT(p, 0xc) = F(self, 0xc00) + pc[2];
            } else {
                func_02005b98(pa, self);
                func_02005b98(pb, self);
                func_02005b98(pc, self);
                S32_AT(p, 4) = F(self, 0xbf8) + pa[0];
                S32_AT(p, 8) = F(self, 0xbfc) + pb[1];
                S32_AT(p, 0xc) = F(self, 0xc00) + pc[2];
            }
            p[0x8c] = 1;
        } else {
            s32 t[3];

            func_020297d4(t, v, (s32 *)(self + 0x3c00));
            S32_AT(p, 4) = t[0];
            S32_AT(p, 8) = F(self, 0xbfc);
            S32_AT(p, 0xc) = t[2];
            p[0x8c] = 1;
        }
    } else {
        s32 d[3], pa[3], x, y, z;
        u32 n = g_FxTwoPi + 1;

        d[0] = F(self, 0xc4c);
        d[1] = F(self, 0xc50);
        d[2] = F(self, 0xc54);
        TurnY(d, (s32)(func_020a500c((u32)rand() & 0x7fff, n) >> 32));
        TurnX(d, (s32)(func_020a500c((u32)rand() & 0x7fff, n) >> 32));
        func_02005b98(pa, self);
        x = F(self, 0xbf8) + (d[0] + pa[0]);
        func_02005b98(pa, self);
        y = F(self, 0xbfc) + (d[1] + pa[1]);
        func_02005b98(pa, self);
        z = F(self, 0xc00) + (d[2] + pa[2]);
        S32_AT(p, 4) = x;
        S32_AT(p, 8) = y;
        S32_AT(p, 0xc) = z;
        p[0x8c] = 1;
    }
    if (F(self, 0xbcc) == 0x1000) {
        S32_AT(p, 0xb8) = S32_AT(p, 0xb4);
        p[0xc5] = 0;
    }
}

/* 0x02056938: set the scale
 * @difftest ptr:0xfc:4 s32 s32 s32 cases=3 */
void func_02056938(u8 *p, s32 x, s32 y, s32 z)
{
    S32_AT(p, 0x1c) = x;
    S32_AT(p, 0x20) = y;
    S32_AT(p, 0x24) = z;
    p[0x8c] = 1;
}

/* 0x02056958: spawn a free particle
 * @difftest $T=zero:0x20 @$T+0x1c:32=0x02043814 $S=zero:0x3d00 @$S:32=$T @$S+0x178:8=pick:0,1 @$S+0x274:8=pick:0,1 @$S+0x3c10:32=int:-1:3 $S cases=10 */
void func_02056958(u8 *self)
{
    u32 i;

    for (i = 0; i < 0x3c; i++) {
        if (FREE(self, i) != 0)
            break;
    }
    if (i >= 0x3c)
        return;
    (*(void (**)(u8 *, u8 *))(*(u8 **)self + 0x1c))(self, PARTICLE(self, i));
    if (F(self, 0xc10) <= 0)
        return;
    F(self, 0xc10)--;
    if (F(self, 0xc10) == 0)
        self[0x3bc5] = 1;
}

/* 0x02056a0c: draw particle `p`
 * @difftest $X=zero:0x40 $T=zero:0xc @$T:32=$X @020ebd34:32=$T $S=zero:0x3d00 @$S+0x3c64:32=pick:0,1,3,0xb @$S+0x8c:8=pick:0,1 $P=zero:0xfc @$P+0xc5:8=pick:0,1 @$P+0x8c:8=pick:0,1 @$P+0xc0:32=int:0:6 @$P+0xb4:32=int:0:0x1800 @$P+0xb8:32=int:1:0x1000 @$P+0xbc:32=int:0:0x20 $S $P stub=0x0205b958,0x02080f60,0x02080e84,0x02080bf4 cases=40 */
void func_02056a0c(u8 *self, u8 *p)
{
    s32 m[16];
    u32 a;
    u8 *g;

    if (S8_AT(p, 0xc5) != 0) {
        s32 q[3], t[3], w[3];

        w[0] = 0;
        w[1] = 0;
        w[2] = 0;
        Matrix(self, m);
        func_02005b98(q, p);
        func_020022e8(t, m, q);
        w[0] = t[0];
        w[1] = t[1];
        w[2] = t[2];
        Matrix(p, m);
        m[12] = w[0];
        m[13] = w[1];
        m[14] = w[2];
    } else {
        Matrix(p, m);
    }
    switch (F(self, 0xc64)) {
    case 1:
    case 2:
    case 6:
    case 10:
    case 11:
        func_0205b958(p, m);
        return;
    }
    func_02080f60(func_020062e0(), 3, 1);
    func_02080f60(func_020062e0(), 4, 1);
    a = 0x1f;
    switch (U32_AT(p, 0xc0)) {
    case 0:
        a = U32_AT(p, 0xbc);
        break;
    case 1:
        a = FxMul(S32_AT(p, 0xb4), 0x1f000) >> 12;
        if ((s32)a <= 1)
            a = 1;
        break;
    case 2:
        if (S32_AT(p, 0xb4) <= 0x800) {
            a = FxMul(func_0208f070(S32_AT(p, 0xb4), 0x800), 0x1f000) >> 12;
            if ((s32)a <= 1)
                a = 1;
        }
        break;
    case 4:
        a = FxMul(func_0208f070(S32_AT(p, 0xb4), S32_AT(p, 0xb8)), 0x1f000) >> 12;
        if ((s32)a <= 10)
            a = 10;
        break;
    }
    U32_AT(func_020062e0(), 0x4a4) = a;
    g = func_020062e0();
    MI_Copy64B(m, g + 0x3d8);
    U32_AT(g, 0x2cc) |= 4;
    func_02080e84(func_020062e0());
    func_02080bf4(func_020062e0(), PTR_AT(data_020ebd34, F(self, 0xbd4) * 0xc));
}

/* 0x02056ddc: respawn particle `p` from where it is, flying away from the
 * followed object's movement
 * @difftest $S=zero:0x3d00 @$S+0x8c:8=pick:0,1 $P=ptr:0xfc:4 @$P+0x8c:8=pick:0,1 $S $P cases=10
 * @difftest $O=ptr:0x100:4 $S=zero:0x3d00 @$S+0x3bdc:32=$O $P=ptr:0xfc:4 @$P+0x8c:8=0 $S $P cases=10 */
void func_02056ddc(u8 *self, u8 *p)
{
    s32 m[16], w[3], q[3], t[3], d[3], o[3], v[3], c[3], len, n;

    w[0] = 0;
    w[1] = 0;
    w[2] = 0;
    Matrix(self, m);
    func_02005b98(q, p);
    func_020022e8(t, m, q);
    w[0] = t[0];
    w[2] = t[2];
    w[1] = t[1];
    S32_AT(p, 4) = w[0];
    S32_AT(p, 8) = w[1];
    S32_AT(p, 0xc) = w[2];
    p[0x8c] = 1;
    func_02005b98(q, self);
    d[0] = 0;
    d[1] = 0;
    d[2] = 0;
    if (PTR_AT(self, 0x3bdc) != NULL) {
        s32 r[3], e[3];

        F(self, 0xbe0) = F(self, 0xbec);
        F(self, 0xbe4) = F(self, 0xbf0);
        F(self, 0xbe8) = F(self, 0xbf4);
        func_02005b98(r, PTR_AT(self, 0x3bdc));
        F(self, 0xbec) = r[0];
        F(self, 0xbf0) = r[1];
        F(self, 0xbf4) = r[2];
        func_0202a104(e, (s32 *)(self + 0x3bec), (s32 *)(self + 0x3be0));
        d[0] = e[0];
        d[1] = e[1];
        d[2] = e[2];
    }
    func_02047c94(c, m);
    func_0202a104(o, w, c);
    func_02029790(v, o, d);
    len = func_0208eff8(FxMul(v[2], v[2]) + (FxMul(v[0], v[0]) + FxMul(v[1], v[1])));
    n = func_0202df68(v);
    if (n > 1)
        func_0202ded8(v, &n);
    else
        func_0202dea4(v);
    S32_AT(p, 0x90) = v[0];
    S32_AT(p, 0x94) = v[1];
    S32_AT(p, 0x98) = v[2];
    S32_AT(p, 0x9c) = len + S32_AT(p, 0x9c);
    S32_AT(p, 0xa0) = S32_AT(p, 0x9c);
    S32_AT(p, 0xb8) = S32_AT(p, 0xb4);
    p[0xc5] = 0;
}

/* 0x02057074: draw the particles in use
 * @difftest $S=zero:0x3d00 @$S+0x178:8=1 @$S+0x274:8=pick:0,1 $S stub=0x02056a0c cases=4 */
void func_02057074(u8 *self)
{
    u32 i;

    for (i = 0; i < 0x3c; i++) {
        if (FREE(self, i) == 0)
            func_02056a0c(self, PARTICLE(self, i));
    }
}

/* 0x020570e0: release the texture
 * @difftest $X=zero:0x40 $T=zero:0x18 @$T:32=0 @020ebd34:32=$T $S=zero:0x3d00 $S stub=0x02082e48 cases=1
 * @difftest $X=zero:0x40 $T=zero:0x18 @$T:32=$X @$T+0xc:32=0 @020ebd34:32=$T $S=zero:0x3d00 @$S+0x3bd4:32=int:0:2 $S stub=0x02082e48 cases=4 */
void func_020570e0(u8 *self)
{
    u8 *t = PTR_AT(data_020ebd34, F(self, 0xbd4) * 0xc);

    if (t == NULL)
        return;
    func_02082e48(func_02018208(), PTR_AT(PTR_AT(data_020ebd34, F(self, 0xbd4) * 0xc), 0x34));
}

/* 0x02057134: update by `dt` ticks: spawn on schedule, move the particles,
 * respawn old ones
 * @difftest $S=zero:0x3d00 @$S+0x3bc4:8=0 $S u32 cases=2
 * @difftest $T=zero:0x20 @$T+0x18:32=0x02043814 $S=zero:0x3d00 @$S:32=$T @$S+0x3bc4:8=1 @$S+0x3bc5:8=pick:0,1 @$S+0x3bc6:8=pick:0,1 @$S+0x3bc8:32=int:0:3 @$S+0x3bcc:32=int:0:0x1000 @$S+0x3c08:32=int:0:0x2000 @$S+0x3c58:32=pick:0,0xffffffff @$S+0x3c5c:32=pick:0,0x7fffffff @$S+0x178:8=1 @$S+0x274:8=pick:0,1 @$S+0x22d:8=pick:0,1 @$S+0x21c:32=int:0:0x1000 $S int:0:0x100 stub=0x0205bc6c,0x02056ddc cases=40 */
void func_02057134(u8 *self, u32 dt)
{
    u32 i;
    s8 any = 0;

    if (S8_AT(self, 0x3bc4) == 0)
        return;
    if (S8_AT(self, 0x3bc6) == 1 && S8_AT(self, 0x3bc5) == 0) {
        F(self, 0xbc8) -= dt;
        if (F(self, 0xbc8) <= 0)
            self[0x3bc5] = 1;
    }
    if (S8_AT(self, 0x3bc5) == 0) {
        u64 now = func_020822c8(func_020062d0());
        u64 next = ((u64)U32_AT(self, 0x3c5c) << 32) | U32_AT(self, 0x3c58);

        if (next <= now) {
            s32 ms;
            u64 t;

            (*(void (**)(u8 *))(*(u8 **)self + 0x18))(self);
            ms = (F(self, 0xc08) * 1000) >> 12;
            t = func_020822c8(func_020062d0());
            t += func_020822a8(func_020062d0(), ms);
            U32_AT(self, 0x3c58) = (u32)t;
            U32_AT(self, 0x3c5c) = (u32)(t >> 32);
        }
    }
    for (i = 0; i < 0x3c; i++) {
        u8 *q = self + i * 0xfc;

        if (FREE(self, i) != 0)
            continue;
        func_0205bc6c(PARTICLE(self, i), dt);
        if (S8_AT(q, 0x179) != 0 && S32_AT(q, 0x168) < F(self, 0xbcc)) {
            s32 left = 0x1000 - S32_AT(q, 0x168);
            s32 r = (s32)((u32)rand() & 0x7fff) % 0x65 > 0
                        ? (s32)(0.5f + (float)SHL((s32)((u32)rand() & 0x7fff) % 0x65, 12))
                        : (s32)((float)SHL((s32)((u32)rand() & 0x7fff) % 0x65, 12) - 0.5f);

            if (r * 0x29 * left > 0x59a)
                func_02056ddc(self, PARTICLE(self, i));
        }
        any = 1;
    }
    if (S8_AT(self, 0x3bc5) != 0)
        self[0x3bc4] = any;
}

/* 0x020573c4: use texture `id`
 * @difftest zero:0x3d00 u32 stub=0x02071c2c cases=3 */
void func_020573c4(u8 *self, u32 id)
{
    func_02071c2c(self + 0x3bd0, id);
}

/* 0x020573dc: move to the followed object
 * @difftest $S=zero:0x3d00 $S cases=2
 * @difftest $O=ptr:0x100:4 $S=zero:0x3d00 @$S+0x3bdc:32=$O $S cases=3 */
void func_020573dc(u8 *self)
{
    s32 p[3], r[3];

    if (PTR_AT(self, 0x3bdc) == NULL)
        return;
    func_02005b98(p, PTR_AT(self, 0x3bdc));
    S32_AT(self, 4) = p[0];
    S32_AT(self, 8) = p[1];
    S32_AT(self, 0xc) = p[2];
    self[0x8c] = 1;
    func_0201751c(r, PTR_AT(self, 0x3bdc));
    S32_AT(self, 0x10) = r[0];
    S32_AT(self, 0x14) = r[1];
    S32_AT(self, 0x18) = r[2];
    self[0x8c] = 1;
}
