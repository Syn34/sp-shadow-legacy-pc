/*
 * Two actor classes used by the boss battles (moving collision bodies with
 * a 3D node), and the small vector/matrix helpers they inline.
 * ARM9 main, 0x0202d058 - 0x0202e000 (44 functions).
 *
 * Both classes have the same methods; they differ only in the collision
 * shape: the hit test asks the other actor's method +0x30 (class A) or
 * +0x2c (class B), and the two collision queries call func_020611ac /
 * func_0205c978 (A) or func_020611b4 / func_0205c874 (B).
 *
 * Actor layout used here: +0x44 3D node (+0x04 position, +0x28 4x4
 * matrix, +0x8c "dirty" flag), +0x4c the actor it stands on, +0x5c
 * velocity, +0x68 position/rotation block (+0x90 velocity), +0x6c
 * position, +0xf4 position changed, +0xf8 direction, +0x104/+0x108
 * speed, +0x234 collision shape (C++ object), +0x238 world matrix.
 *
 * Methods are called with registers left from the caller: several forward
 * r1-r3 unchanged and leave the method address in the next register; the
 * C passes those along explicitly.
 *
 * Testing: the draw methods run with the engine's model update
 * func_02053460 stubbed (it needs a complete 3D model).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const s32 data_020bdea4[], data_020bdec4[], data_020bdeac[];   /* default direction */

void func_02005b98(s32 *out, const void *src);
void func_02029790(s32 *out, const s32 *a, const s32 *b);
void func_020297d4(s32 *out, const s32 *v, const s32 *s);
void func_02029860(s32 *out, const void *obj);
void func_0202a148(s32 *out, u8 *obj);
s32 func_0208eff8(s32 v);
s32 func_0208f070(s32 num, s32 den);
void func_0208ea6c(void *mtx44);
void func_0208edf0(s32 *dst, const s32 *src, fx32 x, fx32 y, fx32 z);
void func_02017538(s32 *out, const s32 *rot, const s32 *m);
void func_02017550(s32 *out, const u8 *cam);
void func_0201751c(s32 *out, const u8 *obj);
void func_020022e4(void *self);
void func_02002974(void *self);
void MI_Copy64B(const void *src, void *dest);
void func_02053460(void *obj, u32 dt);
void func_0205bf14(void *p);
s32 func_020611ac(void *other, void *shape, s32 *pos, s32 *pos2, s32 *k, void *hit);
s32 func_020611b4(void *other, void *shape, s32 *pos, s32 *pos2, s32 *k, void *hit);
s32 func_0205c978(void *other, void *shape, s32 *pos, s32 *pos2, s32 *k, void *hit);
s32 func_0205c874(void *other, void *shape, s32 *pos, s32 *pos2, s32 *k, void *hit);

void func_0202dc6c(s32 *m, const s32 *v);
void func_0202dc88(s32 *m, const s32 *rot);
void func_0202de68(s32 *m, const s32 *v);
void func_0202de98(s32 *m);
void func_0202dea4(s32 *v);
void func_0202ded8(s32 *v, const s32 *s);
void func_0202df18(s32 *out, const s32 *v, const s32 *s);
s32 func_0202df68(const s32 *v);
void func_0202dfd4(s32 *out, const u8 *o);
void func_0202dff0(s32 *out, const u8 *o);

#define NODE(self) ((u8 *)PTR_AT(self, 0x44))
#define SHAPE(self) ((self) + 0x234)

typedef s32 (*Query)(void *, void *, s32 *, s32 *, s32 *, void *);

static s32 FxSquare(s32 x)
{
    return (s32)(((s64)x * x + 0x800) >> 12);
}

/* ---------------------------------------------------------------- shared bodies */

static void ResetPos(u8 *self)
{
    u8 *n = NODE(self);

    S32_AT(self, 0x6c) = S32_AT(n, 4);
    S32_AT(self, 0x70) = S32_AT(n, 8);
    S32_AT(self, 0x74) = S32_AT(n, 0xc);
    self[0xf4] = 1;
    S32_AT(self, 0x104) = 0;
    S32_AT(self, 0x108) = 0;
}

/* move along with the actor stood on, then turn the velocity into a
 * direction (+0xf8) and a speed (+0x104/+0x108) */
static void Update(u8 *self, s32 *dt)
{
    u8 *n = NODE(self);
    s32 a[3], b[3], c[3], d[3], e[3], f[3], g[3], h[3], i[3], j[3];
    s32 k, k2, len, m;

    if (n == NULL)
        return;
    if (PTR_AT(self, 0x4c) != NULL) {
        func_0202dff0(a, PTR_AT(self, 0x4c));
        k = S32_AT(PTR_AT(self, 0x4c), 0x104);
        func_020297d4(b, a, &k);
        func_02005b98(c, self + 0x68);
        func_02029790(d, c, b);
        S32_AT(self, 0x6c) = d[0];
        S32_AT(self, 0x70) = d[1];
        S32_AT(self, 0x74) = d[2];
        self[0xf4] = 1;
        func_02029860(e, self);
        func_02029790(f, e, b);
        S32_AT(n, 4) = f[0];
        S32_AT(n, 8) = f[1];
        S32_AT(n, 0xc) = f[2];
        n[0x8c] = 1;
    }
    func_0202dfd4(g, self + 0x68);
    k2 = S32_AT(self, 0x104);
    func_020297d4(h, g, &k2);
    if (func_0208eff8(FxSquare(S32_AT(self, 0x64))
                      + (FxSquare(S32_AT(self, 0x5c)) + FxSquare(S32_AT(self, 0x60)))) <= 0)
        return;
    func_020297d4(i, (s32 *)(self + 0x5c), dt);
    func_02029790(j, h, i);
    h[0] = j[0];
    h[1] = j[1];
    h[2] = j[2];
    len = func_0208eff8(FxSquare(j[2]) + (FxSquare(j[0]) + FxSquare(j[1])));
    m = func_0202df68(h);
    if (m > 1)
        func_0202ded8(h, &m);
    else
        func_0202dea4(h);
    S32_AT(self, 0xf8) = h[0];
    S32_AT(self, 0xfc) = h[1];
    S32_AT(self, 0x100) = h[2];
    S32_AT(self, 0x104) = len;
    S32_AT(self, 0x108) = len;
}

static void Init(u8 *self)
{
    u8 *n;
    s32 tmp[3];
    u8 *sh;
    void (*fn)(void *, void *, void *);

    func_0205bf14(self + 0x68);
    n = NODE(self);
    func_02005b98(tmp, n);
    sh = SHAPE(self);
    fn = VCALL(sh, 8, void (*)(void *, void *, void *));
    fn(sh, n, (void *)fn);
    S32_AT(self, 0x5c) = data_020bdea4[0];
    S32_AT(self, 0x60) = data_020bdec4[0];
    S32_AT(self, 0x64) = data_020bdeac[0];
}

/* place the 3D node and rebuild its matrix */
static void Draw(u8 *self, u32 dt)
{
    u8 *n = NODE(self);
    s32 p[3], t[3], r[3], s[3];

    if (n == NULL)
        return;
    func_02053460(n, dt);
    func_02005b98(p, self + 0x68);
    S32_AT(n, 4) = p[0];
    S32_AT(n, 8) = p[1];
    S32_AT(n, 0xc) = p[2];
    n[0x8c] = 1;
    S32_AT(self, 0x28) = S32_AT(n, 4);
    S32_AT(self, 0x2c) = S32_AT(n, 0xc);
    if ((s8)n[0x8c] == 1) {
        func_0202de98((s32 *)(n + 0x28));
        func_02017550(t, n);
        func_0202de68((s32 *)(n + 0x28), t);
        func_020022e4(t);
        func_0201751c(r, n);
        func_0202dc88((s32 *)(n + 0x28), r);
        func_02002974(r);
        func_02005b98(s, n);
        func_0202dc6c((s32 *)(n + 0x28), s);
        func_020022e4(s);
    }
    MI_Copy64B(n + 0x28, self + 0x238);
}

static void NodeMethod3(u8 *self)
{
    u8 *n = NODE(self);
    void (*fn)(void *, void *);

    if (n == NULL)
        return;
    fn = VCALL(n, 0xc, void (*)(void *, void *));
    fn(n, (void *)fn);
}

static s32 HitTest(u8 *self, void *other, void *hit, u32 slot)
{
    s32 (*fn)(void *, void *, void *, void *) = VCALL(other, slot, s32 (*)(void *, void *, void *, void *));

    return fn(other, SHAPE(self), hit, (void *)fn);
}

static s32 Collide(u8 *self, void *other, u8 *hit, Query q)
{
    s32 pos[3], pos2[3];
    s32 k = 0, r;
    u8 *sh;
    s32 (*fn)(void *, s32, void *);

    func_02029860(pos, self);
    func_0202a148(pos2, self);
    r = q(other, SHAPE(self), pos, pos2, &k, hit);
    sh = SHAPE(self);
    fn = VCALL(sh, 0x1c, s32 (*)(void *, s32, void *));
    S32_AT(hit, 0x34) = fn(sh, k, (void *)fn);
    return r;
}

static u32 ShapeCall1(u8 *self, u32 a, u32 slot)
{
    u8 *sh = SHAPE(self);
    u32 (*fn)(void *, u32, void *) = VCALL(sh, slot, u32 (*)(void *, u32, void *));

    return fn(sh, a, (void *)fn);
}

/* ---------------------------------------------------------------- class A */

/* 0x0202d058
 * @difftest cases=2 */
void func_0202d058(void)
{
}

/* 0x0202d05c: copy the node's position into the actor
 * @difftest $N=ptr:0x100:4 $S=ptr:0x240:4 @$S+0x44:32=$N $S cases=20 */
void func_0202d05c(u8 *self)
{
    ResetPos(self);
}

/* 0x0202d0a0
 * @difftest cases=2 */
void func_0202d0a0(void)
{
}

/* 0x0202d0a4: per-frame movement
 * @difftest $N=ptr:0x100:4 $S=ptr:0x240:4 @$S+0x44:32=$N @$S+0x4c:32=0 $S s32 cases=40
 * @difftest $N=ptr:0x100:4 $O=ptr:0x240:4 $S=ptr:0x240:4 @$S+0x44:32=$N @$S+0x4c:32=$O $S s32 cases=40 */
void func_0202d0a4(u8 *self, s32 dt)
{
    Update(self, &dt);
}

/* 0x0202d2e8: initialise
 * @difftest $V=zero:0x20 @$V+8:32=0x02070a18 $N=ptr:0x100:4 $S=zero:0x300 @$S+0x44:32=$N @$S+0x234:32=$V $S cases=20 */
void func_0202d2e8(u8 *self)
{
    Init(self);
}

/* 0x0202d35c: draw
 * @difftest $N=zero:0x100 $S=ptr:0x280:4 @$S+0x44:32=$N $S u32 stub=0x02053460 cases=20 */
void func_0202d35c(u8 *self, u32 dt)
{
    Draw(self, dt);
}

/* 0x0202d458
 * @difftest $S=ptr:0x60:4 @$S+0x44:32=0 $S cases=5
 * @difftest $V=zero:0x20 @$V+0xc:32=0x02070a18 $N=zero:0x40 @$N+0:32=$V $S=ptr:0x60:4 @$S+0x44:32=$N $S cases=20 */
void func_0202d458(u8 *self)
{
    NodeMethod3(self);
}

/* 0x0202d48c
 * @difftest cases=2 */
s32 func_0202d48c(void)
{
    return 0;
}

/* 0x0202d494: hit test against `other`
 * @difftest $V=zero:0x40 @$V+0x30:32=0x02070a18 $O=zero:0x10 @$O+0:32=$V ptr:0x240:4 $O ptr:0x40:4 cases=20 */
s32 func_0202d494(u8 *self, void *other, void *hit)
{
    return HitTest(self, other, hit, 0x30);
}

/* 0x0202d4c0
 * @difftest $V=zero:0x40 @$V+0x1c:32=0x02070a18 $N=ptr:0x100:4 $S=zero:0x240 @$S+0x234:32=$V @$S+0x44:32=$N $S ptr:0x80:4 ptr:0x40:4 cases=20 */
s32 func_0202d4c0(u8 *self, void *other, u8 *hit)
{
    return Collide(self, other, hit, func_020611ac);
}

/* 0x0202d540
 * @difftest $V=zero:0x40 @$V+0x1c:32=0x02070a18 $N=ptr:0x100:4 $S=zero:0x240 @$S+0x234:32=$V @$S+0x44:32=$N $S ptr:0x80:4 ptr:0x40:4 cases=20 */
s32 func_0202d540(u8 *self, void *other, u8 *hit)
{
    return Collide(self, other, hit, func_0205c978);
}

/* 0x0202d5c0: forward to the collision shape (methods +0x0c..+0x18)
 * @difftest $V=zero:0x40 @$V+0xc:32=0x02070a18 $S=zero:0x240 @$S+0x234:32=$V $S u32 cases=10 */
u32 func_0202d5c0(u8 *self, u32 a)
{
    return ShapeCall1(self, a, 0xc);
}

/* 0x0202d5e4
 * @difftest $V=zero:0x40 @$V+0x10:32=0x02070a18 $S=zero:0x240 @$S+0x234:32=$V $S u32 cases=10 */
u32 func_0202d5e4(u8 *self, u32 a)
{
    return ShapeCall1(self, a, 0x10);
}

/* 0x0202d608
 * @difftest $V=zero:0x40 @$V+0x14:32=0x02070a18 $S=zero:0x240 @$S+0x234:32=$V $S u32 u32 cases=10 */
u32 func_0202d608(u8 *self, u32 a, u32 b)
{
    u8 *sh = SHAPE(self);
    u32 (*fn)(void *, u32, u32, void *) = VCALL(sh, 0x14, u32 (*)(void *, u32, u32, void *));

    return fn(sh, a, b, (void *)fn);
}

/* 0x0202d62c
 * @difftest $V=zero:0x40 @$V+0x18:32=0x02070a18 $S=zero:0x240 @$S+0x234:32=$V $S u32 cases=10 */
u32 func_0202d62c(u8 *self, u32 a)
{
    return ShapeCall1(self, a, 0x18);
}

/* ---------------------------------------------------------------- class B */

/* 0x0202d650
 * @difftest cases=2 */
void func_0202d650(void)
{
}

/* 0x0202d654
 * @difftest $N=ptr:0x100:4 $S=ptr:0x240:4 @$S+0x44:32=$N $S cases=20 */
void func_0202d654(u8 *self)
{
    ResetPos(self);
}

/* 0x0202d698
 * @difftest cases=2 */
void func_0202d698(void)
{
}

/* 0x0202d69c
 * @difftest $N=ptr:0x100:4 $S=ptr:0x240:4 @$S+0x44:32=$N @$S+0x4c:32=0 $S s32 cases=40
 * @difftest $N=ptr:0x100:4 $O=ptr:0x240:4 $S=ptr:0x240:4 @$S+0x44:32=$N @$S+0x4c:32=$O $S s32 cases=40 */
void func_0202d69c(u8 *self, s32 dt)
{
    Update(self, &dt);
}

/* 0x0202d8e0
 * @difftest $V=zero:0x20 @$V+8:32=0x02070a18 $N=ptr:0x100:4 $S=zero:0x300 @$S+0x44:32=$N @$S+0x234:32=$V $S cases=20 */
void func_0202d8e0(u8 *self)
{
    Init(self);
}

/* 0x0202d954
 * @difftest $N=zero:0x100 $S=ptr:0x280:4 @$S+0x44:32=$N $S u32 stub=0x02053460 cases=20 */
void func_0202d954(u8 *self, u32 dt)
{
    Draw(self, dt);
}

/* 0x0202da50
 * @difftest $S=ptr:0x60:4 @$S+0x44:32=0 $S cases=5
 * @difftest $V=zero:0x20 @$V+0xc:32=0x02070a18 $N=zero:0x40 @$N+0:32=$V $S=ptr:0x60:4 @$S+0x44:32=$N $S cases=20 */
void func_0202da50(u8 *self)
{
    NodeMethod3(self);
}

/* 0x0202da84
 * @difftest cases=2 */
s32 func_0202da84(void)
{
    return 0;
}

/* 0x0202da8c
 * @difftest $V=zero:0x40 @$V+0x2c:32=0x02070a18 $O=zero:0x10 @$O+0:32=$V ptr:0x240:4 $O ptr:0x40:4 cases=20 */
s32 func_0202da8c(u8 *self, void *other, void *hit)
{
    return HitTest(self, other, hit, 0x2c);
}

/* 0x0202dab8
 * @difftest $V=zero:0x40 @$V+0x1c:32=0x02070a18 $N=ptr:0x100:4 $S=zero:0x240 @$S+0x234:32=$V @$S+0x44:32=$N $S ptr:0x80:4 ptr:0x40:4 cases=20 */
s32 func_0202dab8(u8 *self, void *other, u8 *hit)
{
    return Collide(self, other, hit, func_020611b4);
}

/* 0x0202db38
 * @difftest $V=zero:0x40 @$V+0x1c:32=0x02070a18 $N=ptr:0x100:4 $S=zero:0x240 @$S+0x234:32=$V @$S+0x44:32=$N $S ptr:0x80:4 ptr:0x40:4 cases=20
 * @difftest $V=zero:0x40 @$V+0x1c:32=0x02070a18 $N=ptr:0x100:4 $S=zero:0x240 @$S+0x234:32=$V @$S+0x44:32=$N $O=ptr:0x80:4 @$O+0x6a:8=0 $S $O ptr:0x40:4 cases=20 */
s32 func_0202db38(u8 *self, void *other, u8 *hit)
{
    return Collide(self, other, hit, func_0205c874);
}

/* 0x0202dbb8
 * @difftest $V=zero:0x40 @$V+0xc:32=0x02070a18 $S=zero:0x240 @$S+0x234:32=$V $S u32 cases=10 */
u32 func_0202dbb8(u8 *self, u32 a)
{
    return ShapeCall1(self, a, 0xc);
}

/* 0x0202dbdc
 * @difftest $V=zero:0x40 @$V+0x10:32=0x02070a18 $S=zero:0x240 @$S+0x234:32=$V $S u32 cases=10 */
u32 func_0202dbdc(u8 *self, u32 a)
{
    return ShapeCall1(self, a, 0x10);
}

/* 0x0202dc00
 * @difftest $V=zero:0x40 @$V+0x14:32=0x02070a18 $S=zero:0x240 @$S+0x234:32=$V $S u32 u32 cases=10 */
u32 func_0202dc00(u8 *self, u32 a, u32 b)
{
    u8 *sh = SHAPE(self);
    u32 (*fn)(void *, u32, u32, void *) = VCALL(sh, 0x14, u32 (*)(void *, u32, u32, void *));

    return fn(sh, a, b, (void *)fn);
}

/* 0x0202dc24
 * @difftest $V=zero:0x40 @$V+0x18:32=0x02070a18 $S=zero:0x240 @$S+0x234:32=$V $S u32 cases=10 */
u32 func_0202dc24(u8 *self, u32 a)
{
    return ShapeCall1(self, a, 0x18);
}

/* ---------------------------------------------------------------- helpers */

/* 0x0202dc48
 * @difftest cases=2 */
s32 func_0202dc48(void)
{
    return 0;
}

/* 0x0202dc50
 * @difftest cases=2 */
void func_0202dc50(void)
{
}

/* 0x0202dc54
 * @difftest ptr:0x70 cases=5 */
void func_0202dc54(u8 *o)
{
    o[0x6a] = 0;
}

/* 0x0202dc60
 * @difftest ptr:0x70 cases=5 */
void func_0202dc60(u8 *o)
{
    o[0x6a] = 1;
}

/* 0x0202dc6c: set a 4x4 matrix's translation
 * @difftest ptr:0x40:4 ptr:12:4 cases=10 */
void func_0202dc6c(s32 *m, const s32 *v)
{
    m[12] = v[0];
    m[13] = v[1];
    m[14] = v[2];
}

/* 0x0202dc88: apply rotations about y, x and z (fx32 radians) to a 4x4
 * matrix, skipping zero angles
 * @difftest ptr:0x40:4 ptr:12:4
 * @difftest ptr:0x40:4 $R=ptr:12:4 @$R+0:32=pick:0,0x1922,0x3244 @$R+4:32=pick:0,0x6488 @$R+8:32=pick:0,0x1000 $R cases=60 */
void func_0202dc88(s32 *m, const s32 *rot)
{
    s32 r[16], out[16];
    s32 s, c;

    if (rot[1] != 0) {
        s = FX_SIN_RAD(rot[1]);
        c = FX_COS_RAD(rot[1]);
        func_0208ea6c(r);
        r[0] = c;
        r[2] = -s;
        r[8] = s;
        r[10] = c;
        func_02017538(out, r, m);
        MI_Copy64B(out, m);
    }
    if (rot[0] != 0) {
        s = FX_SIN_RAD(rot[0]);
        c = FX_COS_RAD(rot[0]);
        func_0208ea6c(r);
        r[5] = c;
        r[6] = s;
        r[9] = -s;
        r[10] = c;
        func_02017538(out, r, m);
        MI_Copy64B(out, m);
    }
    if (rot[2] != 0) {
        s = FX_SIN_RAD(rot[2]);
        c = FX_COS_RAD(rot[2]);
        func_0208ea6c(r);
        r[0] = c;
        r[1] = s;
        r[4] = -s;
        r[5] = c;
        func_02017538(out, r, m);
        MI_Copy64B(out, m);
    }
}

/* 0x0202de68: translate a matrix
 * @difftest ptr:0x40:4 ptr:12:4 cases=20 */
void func_0202de68(s32 *m, const s32 *v)
{
    func_0208edf0(m, m, v[0], v[1], v[2]);
}

/* 0x0202de98: identity
 * @difftest ptr:0x40:4 cases=5 */
void func_0202de98(s32 *m)
{
    func_0208ea6c(m);
}

/* 0x0202dea4: the default direction
 * @difftest ptr:12:4 cases=5 */
void func_0202dea4(s32 *v)
{
    v[0] = data_020bdea4[0];
    v[1] = data_020bdec4[0];
    v[2] = data_020bdeac[0];
}

/* 0x0202ded8: v /= *s
 * @difftest ptr:12:4 $D=ptr:4:4 @$D+0:32=int:0x800:0x100000 $D */
void func_0202ded8(s32 *v, const s32 *s)
{
    s32 t[3];

    func_0202df18(t, v, s);
    v[0] = t[0];
    v[1] = t[1];
    v[2] = t[2];
}

/* 0x0202df18: out = v / *s
 * @difftest ptr:12:4 ptr:12:4 $D=ptr:4:4 @$D+0:32=int:0x800:0x100000 $D */
void func_0202df18(s32 *out, const s32 *v, const s32 *s)
{
    s32 z = func_0208f070(v[2], *s);
    s32 y = func_0208f070(v[1], *s);
    s32 x = func_0208f070(v[0], *s);

    out[0] = x;
    out[1] = y;
    out[2] = z;
}

/* 0x0202df68: length of a vector
 * @difftest ptr:12:4
 * @difftest $V=ptr:12:4 @$V+0:32=int:-0x100000:0x100000 @$V+4:32=int:-0x100000:0x100000 @$V+8:32=int:-0x100000:0x100000 $V */
s32 func_0202df68(const s32 *v)
{
    return func_0208eff8(FxSquare(v[2]) + (FxSquare(v[0]) + FxSquare(v[1])));
}

/* 0x0202dfd4: an actor's velocity (+0x90)
 * @difftest ptr:12:4 ptr:0xa0:4 cases=10 */
void func_0202dfd4(s32 *out, const u8 *o)
{
    out[0] = S32_AT(o, 0x90);
    out[1] = S32_AT(o, 0x94);
    out[2] = S32_AT(o, 0x98);
}

/* 0x0202dff0
 * @difftest ptr:12:4 ptr:0x108:4 cases=10 */
void func_0202dff0(s32 *out, const u8 *o)
{
    func_0202dfd4(out, o + 0x68);
}
