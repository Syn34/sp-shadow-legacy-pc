/*
 * Level objects with an effect (part 2): drawing (with fading lights),
 * the per-type updates, construction and creation from the map.
 * ARM9 main, 0x02048628 - 0x020499c0 (35 functions).
 *
 * See Gates1.c for the object. Types by map object kind (func_020494dc):
 * 0 -> 3, 1/2/13/16/20 -> 0, 9/10 -> 1, 4 -> 2, 3 -> 4, 5-7 -> 5 (doors
 * that need 3/2/1 of counter 7), 8 -> 7, 11 -> 8 (save spot), 12 -> 9
 * (level exit), 15 -> 0xa, 19 -> 0xb, 17 -> 0xc (moves along a path).
 *
 * Testing: with fake objects; the effects, the animations, the lights and
 * the camera stubbed or faked where they need real data.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bf760[];
extern const fx32 data_020bfacc, data_020bfad0, data_020bfad4;   /* a box */
extern const fx32 data_020bfae0, data_020bfae4, data_020bfae8;   /* another box */
extern const fx32 data_020bfaf0;
extern const u32 data_020bfaf4[2], data_020bfafc[2], data_020bfb04[2], data_020bfb0c[2];
extern u8 data_020bfb1c[], data_020bfb58[], data_020bfbc4[];    /* vtables */
extern const u8 data_020bfb9c[];     /* name of the effect's node */
extern const char data_020bfba8[];   /* debug message */
extern u8 data_020d9f9c[];
extern s32 data_020ebca8;            /* fading speed */
extern u8 data_020ebcac[], data_020ebcc4[];

void func_0202de98(s32 *m);
void func_02017550(s32 *out, const u8 *cam);
void func_0202de68(s32 *m, const s32 *v);
void func_020022e4(void *self);
void func_0201751c(s32 *out, const u8 *cam);
void func_0202dc88(s32 *m, const s32 *rot);
void func_02002974(void *self);
void func_02005b98(s32 *out, const void *src);
void func_0202dc6c(s32 *m, const s32 *v);
void MI_Copy64B(const void *src, void *dest);
void *func_020062e0(void);
void func_02080e84(void *settings);
void func_0204e948(void *anim, u32 frame);
void func_020814c8(void *settings);
u8 *func_0204f220(void *anim, const void *name);
void func_02047c94(s32 *out, const u8 *m);
s32 func_0204ebb4(void *anim, u32 a, u32 b, u32 c);
void func_020088e4(s32 *out, s32 i, s32 k);
void func_020038d4(s32 *out, const s32 *in, s32 shift);
s32 func_0202df68(const s32 *v);
void func_0202dea4(s32 *v);
void func_0202ded8(s32 *v, const s32 *s);
void func_02070fc8(void *m);
s32 func_0204817c(u8 *self);
s32 func_020145ac(s32 a, s32 b, const char *fmt, ...);
u32 func_020186ec(void);
void func_0207163c(void *anim, u32 dt);
void func_0204ed80(void *anim, u32 id, u32 a);
void func_0204efec(void *anim, u32 dt);
void func_02008130(s32 group, s32 index);
void func_020483f8(u8 *self);
void func_02071838(void *anim);
void func_0204f964(void *anim);
void *func_020506c0(void *actor);
void func_0205b784(void *self);
void func_0207fe44(void *p);
void func_0205b7c8(void *self, s32 *box, void *a, void *b);
void func_02050764(void *actor, s32 *pos);
void func_0204fa44(void *anim, u32 kind);
void func_02071864(void *anim);
void func_02048474(u8 *self, u32 kind);
u8 *func_02008994(s32 i, s32 k);
s32 func_0200820c(u32 id, s32 kind);
void *func_0207fe74(u32 size);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
s32 func_0201a0a0(s32 *xz, void *obj);
void func_020477e8(u8 *self, u32 v);
void func_020477b8(u8 *self, s32 v);
void func_020477cc(u8 *self, s32 v);
void func_02047780(u8 *self, fx32 h);
fx32 func_020477a8(u8 *self);
u8 *func_020477b0(u8 *self);
void func_02047cb0(u8 *self, u8 *other);
void func_020477f0(u8 *self);
void func_02047828(u8 *self);
void func_020478dc(u8 *self);

void func_02048964(u16 *c, u32 b, u32 g, u32 r, u8 a);
u32 func_020489cc(s32 v, const s32 *lo, const s32 *hi);
s32 func_020489e8(s32 a, s32 b);
void func_02048a04(s32 *out, const u8 *g, s32 i);
void func_02048a38(u8 *self, u32 dt);
void func_02048bb4(u8 *self, u32 dt);
void func_02048c14(u8 *self, u32 dt);
void func_02048cf4(u8 *self, u32 dt);
void func_02048d9c(u8 *self, u32 dt);
void func_02048ef4(u8 *self, u32 dt);
void func_02049110(u8 *self);
u32 func_02049124(u8 *self);
void func_0204912c(u8 *self, u32 v);
u8 *func_02049134(u8 *self);
u8 *func_020491b4(u8 *self);
u8 *func_0204922c(u8 *self, u32 kind, u32 anim, s32 *pos);
u8 *func_02049394(u8 *self, u32 kind, s32 *pos);

#define ACTOR(self) ((self) != NULL ? (self) + 0x90 : NULL)
#define EFFECT(self) ((u8 *)PTR_AT(self, 0x274))

/* 0x02048628: draw the object (its lights dimmed by +0x284)
 * @difftest $S=zero:0x290 @$S+0x94:32=pick:0,8 @$S+0x8c:8=pick:0,1 @$S+0x284:32=int:0:0x1000 @$S+0x278:32=pick:0,1 $S stub=0x02080e84,0x0204e948,0x020814c8 cases=10
 * @difftest $V=zero:0x20 @$V+0x14:32=0x020343a8 $E=zero:0x20 @$E+0:32=$V $S=zero:0x290 @$S+0x94:32=8 @$S+0x284:32=int:0:0x1000 @$S+0x278:32=pick:0,2 @$S+0x274:32=$E $S stub=0x02080e84,0x0204e948,0x020814c8 cases=5 */
void func_02048628(u8 *self)
{
    u8 *g;
    s32 i, lo, hi;

    if (BITS(U32_AT(self, 0x94), 3, 1) == 0)
        return;
    if (S8_AT(self, 0x8c) == 1) {
        s32 t[3], r[3], s[3];

        func_0202de98((s32 *)(self + 0x28));
        func_02017550(t, self);
        func_0202de68((s32 *)(self + 0x28), t);
        func_020022e4(t);
        func_0201751c(r, self);
        func_0202dc88((s32 *)(self + 0x28), r);
        func_02002974(r);
        func_02005b98(s, self);
        func_0202dc6c((s32 *)(self + 0x28), s);
        func_020022e4(s);
    }
    g = func_020062e0();
    MI_Copy64B(self + 0x28, g + 0x3d8);
    U32_AT(g, 0x2cc) |= 4;
    lo = 0;
    hi = 0x1f;
    for (i = 0; i < 4; i++) {
        s32 c[3], f, lim0, lim1;
        u32 r, gr, b;
        u16 col, out;

        func_02048a04(c, func_020062e0(), i);
        f = S32_AT(self, 0x284);
        col = U16_AT((u8 *)func_020062e0() + i * 0x14, 0x320);
        r = func_020489e8(f, (col & 0x1f) << 12) >> 12;
        gr = func_020489e8(f, ((col >> 5) & 0x1f) << 12) >> 12;
        b = func_020489e8(f, ((col >> 10) & 0x1f) << 12) >> 12;
        lim0 = lo;
        lim1 = hi;
        r = func_020489cc(r, &lim0, &lim1) & 0xff;
        gr = func_020489cc(gr, &lim0, &lim1) & 0xff;
        b = func_020489cc(b, &lim0, &lim1) & 0xff;
        out = 0;
        func_02048964(&out, b, gr, r, (col >> 15) & 1);
        g = (u8 *)func_020062e0() + i * 0x14;
        U16_AT(g, 0x320) = out;
        S32_AT(g, 0x324) = c[0] >= 0x1000 ? 0xfff : c[0];
        S32_AT(g, 0x328) = c[1] >= 0x1000 ? 0xfff : c[1];
        S32_AT(g, 0x32c) = c[2] >= 0x1000 ? 0xfff : c[2];
    }
    func_02080e84(func_020062e0());
    func_0204e948(self + 0xf4, U32_AT(self, 0xcc));
    func_020814c8(func_020062e0());
    if (EFFECT(self) == NULL)
        return;
    if (U32_AT(self, 0x278) == 1) {
        u8 *m = func_0204f220(self + 0xf4, data_020bfb9c);
        s32 t[3];
        u8 *e;

        func_02047c94(t, m);
        e = EFFECT(self);
        S32_AT(e, 4) = t[0];
        S32_AT(e, 8) = t[1];
        S32_AT(e, 0xc) = t[2];
        e[0x8c] = 1;
    }
    {
        u8 *e = EFFECT(self);
        void (*m)(void *, void *) = VCALL(e, 0x14, void (*)(void *, void *));

        m(e, (void *)m);
    }
}

/* 0x02048964: make a colour
 * @difftest ptr:2:2 int:0:0x40 int:0:0x40 int:0:0x40 int:0:3 cases=20 */
void func_02048964(u16 *c, u32 b, u32 g, u32 r, u8 a)
{
    *c = (*c & ~0x8000) | ((a & 1) << 15);
    *c = (*c & ~0x1f) | (r & 0x1f);
    *c = (*c & ~0x3e0) | ((g & 0x1f) << 5);
    *c = (*c & ~0x7c00) | ((b & 0x1f) << 10);
}

/* 0x020489cc: clamp
 * @difftest int:-0x40:0x40 $L=ptr:4:4 @$L+0:32=int:-8:8 $H=ptr:4:4 @$H+0:32=int:0:0x20 $L $H cases=40 */
u32 func_020489cc(s32 v, const s32 *lo, const s32 *hi)
{
    if (v < *lo)
        v = *lo;
    if (v > *hi)
        v = *hi;
    return v;
}

/* 0x020489e8: fx32 multiply
 * @difftest s32 s32 cases=40 */
s32 func_020489e8(s32 a, s32 b)
{
    return FxMul(a, b);
}

/* 0x02048a04: light `i` of the 3D state
 * @difftest ptr:12:4 ptr:0x380:4 int:0:4 cases=5 */
void func_02048a04(s32 *out, const u8 *g, s32 i)
{
    const u8 *l = g + i * 0x14;

    out[0] = S32_AT(l, 0x324);
    out[1] = S32_AT(l, 0x328);
    out[2] = S32_AT(l, 0x32c);
}

/* 0x02048a28: the animation's func_0204ebb4
 * @difftest zero:0x290 u32 u32 u32 stub=0x0204ebb4 cases=3 */
s32 func_02048a28(u8 *self, u32 a, u32 b, u32 c)
{
    return func_0204ebb4(self + 0xf4, a, b, c);
}

/* 0x02048a38: move along the path
 * @difftest $S=zero:0x290 @$S+0x27c:32=pick:2,2,1 @$S+0x1f4:32=pick:0x80,0x80,0 @$S+0xb8:32=int:-0x8000:0x8000 @$S+0xbc:32=int:-0x8000:0x8000 $S int:0:0x2000 stub=0x020088e4,0x02070fc8 cases=30 */
void func_02048a38(u8 *self, u32 dt)
{
    s32 a[2], n[2], m[2], v[3], len, d;

    if (S32_AT(self, 0x27c) != 2)
        return;
    if ((s8)BITS(U32_AT(self, 0x1f4), 7, 1) == 0)
        return;
    a[0] = S32_AT(self, 0xb8);
    a[1] = S32_AT(self, 0xbc);
    func_020088e4(n, self[0x254], self[0x255]);
    func_020038d4(m, n, 4);
    n[0] = m[0];
    n[1] = m[1];
    v[0] = n[0] - a[0];
    v[1] = data_020bfaf0;
    v[2] = n[1] - a[1];
    d = FxMul(v[2], v[2]) + (FxMul(v[0], v[0]) + FxMul(v[1], v[1]));
    len = func_0202df68(v);
    if (len > 1)
        func_0202ded8(v, &len);
    else
        func_0202dea4(v);
    if (d < 0x2000) {
        func_02070fc8(self + 0x1d0);
        S32_AT(self, 0x27c) = 1;
        return;
    }
    a[0] += v[0];
    a[1] += v[2];
    S32_AT(self, 0xb8) = a[0];
    S32_AT(self, 0xbc) = a[1];
}

/* 0x02048bb4: update (save spots, exits)
 * @difftest $S=zero:0x290 @$S+0x27c:32=pick:0,1,2 @$S+0xc8:32=pick:0,8 $S int:0:0x2000 stub=0x0204817c cases=10 */
void func_02048bb4(u8 *self, u32 dt)
{
    switch (S32_AT(self, 0x27c)) {
    case 1:
        if (BITS(U32_AT(self, 0xc8), 3, 1) == 0) {
            U32_AT(self, 0x94) |= 8;
            S32_AT(self, 0x27c) = 2;
        }
        func_0204817c(self);
        return;
    case 2:
        func_0204817c(self);
        return;
    }
}

/* delete the object's effect */
static void DropEffect(u8 *self)
{
    u8 *e = EFFECT(self);

    if (e != NULL) {
        void (*m)(void *, void *) = VCALL(e, 4, void (*)(void *, void *));
        m(e, (void *)m);
    }
    PTR_AT(self, 0x274) = NULL;
}

/* run the object's effect */
static void RunEffect(u8 *self, u32 dt)
{
    u8 *e = EFFECT(self);
    void (*m)(void *, u32, void *) = VCALL(e, 0xc, void (*)(void *, u32, void *));

    m(e, dt, (void *)m);
}

/* 0x02048c14: update (doors)
 * @difftest $V=zero:0x20 @$V+4:32=0x020343a8 @$V+0xc:32=0x020343a8 $E=zero:0x3c68 @$E+0:32=$V @$E+0x3bc4:8=pick:0,1 $S=zero:0x290 @$S+0x27c:32=pick:0,1,2,2,4 @$S+0xcc:32=int:0:4 @$S+0x274:32=$E $S int:0:0x2000 stub=0x0204817c cases=30 */
void func_02048c14(u8 *self, u32 dt)
{
    switch (S32_AT(self, 0x27c)) {
    case 1:
        func_0204817c(self);
        return;
    case 2:
        func_0204817c(self);
        RunEffect(self, dt);
        if (U32_AT(self, 0xcc) > 1)
            U32_AT(self, 0xcc)--;
        if (S8_AT(EFFECT(self), 0x3bc4) != 0)
            return;
        if (U32_AT(self, 0xcc) > 1)
            return;
        S32_AT(self, 0x27c) = 4;
        DropEffect(self);
        return;
    case 4:
        U32_AT(self, 0xc8) |= 2;
        return;
    }
}

/* 0x02048cf4: update (types 1 and 2)
 * @difftest $V=zero:0x20 @$V+4:32=0x020343a8 @$V+0xc:32=0x020343a8 $E=zero:0x3c68 @$E+0:32=$V @$E+0x3bc4:8=pick:0,1 $S=zero:0x290 @$S+0x27c:32=pick:0,1,2 @$S+0x274:32=$E $S int:0:0x2000 stub=0x0204817c cases=20 */
void func_02048cf4(u8 *self, u32 dt)
{
    switch (S32_AT(self, 0x27c)) {
    case 1:
        func_0204817c(self);
        return;
    case 2:
        func_0204817c(self);
        RunEffect(self, dt);
        if (S8_AT(EFFECT(self), 0x3bc4) != 0)
            return;
        S32_AT(self, 0x27c) = 1;
        DropEffect(self);
        return;
    }
}

/* 0x02048d9c: update (type 0: fades away)
 * @difftest $V=zero:0x20 @$V+4:32=0x020343a8 @$V+0xc:32=0x020343a8 $E=zero:0x3c68 @$E+0:32=$V @$E+0x3bc4:8=pick:0,1 $S=zero:0x290 @$S+0x27c:32=pick:0,1,2,2,3,4,5 @$S+0xcc:32=int:0:4 @$S+0x284:32=int:0:0x1000 @$S+0x274:32=$E @020ebca8:32=int:0:0x1000 $S int:0:0x2000 stub=0x0204817c:1,0x020145ac cases=30
 * @difftest $V=zero:0x20 @$V+4:32=0x020343a8 @$V+0xc:32=0x020343a8 $E=zero:0x3c68 @$E+0:32=$V @$E+0x3bc4:8=pick:0,1 $S=zero:0x290 @$S+0x27c:32=pick:0,1,2,2,3,4,5 @$S+0xcc:32=int:0:4 @$S+0x284:32=int:0:0x1000 @$S+0x274:32=$E @020ebca8:32=int:0:0x1000 $S int:0:0x2000 stub=0x0204817c:0,0x020145ac cases=30 */
void func_02048d9c(u8 *self, u32 dt)
{
    switch (S32_AT(self, 0x27c)) {
    case 1:
        func_0204817c(self);
        return;
    case 2:
        func_0204817c(self);
        RunEffect(self, dt);
        S32_AT(self, 0x284) -= FxMul(data_020ebca8, dt);
        func_020145ac(0, 0xa, data_020bfba8, S32_AT(self, 0x284));
        if (S8_AT(EFFECT(self), 0x3bc4) != 0)
            return;
        S32_AT(self, 0x27c) = 3;
        DropEffect(self);
        return;
    case 3:
        if (func_0204817c(self) != 0) {
            if (U32_AT(self, 0xcc) > 1)
                U32_AT(self, 0xcc)--;
            else
                S32_AT(self, 0x27c) = 4;
            return;
        }
        S32_AT(self, 0x27c) = 4;
        return;
    case 4:
        U32_AT(self, 0xc8) |= 2;
        return;
    }
}

/* run the animations */
static void Animate(u8 *self, u32 dt)
{
    func_0207163c(self + 0x1d0, dt);
    func_0204ed80(self + 0xf4, U32_AT(self, 0x1fc), U32_AT(self, 0x208));
}

/* 0x02048ef4: per-frame update
 * @difftest $S=zero:0x290 @$S+0xc8:32=pick:0x20,0x40,0x60,0 @$S+0x94:32=pick:0,0,4 @$S+0x278:32=int:0:0xe @$S+0x27c:32=pick:0,5 $S int:0:0x2000 stub=0x020186ec:0,0x0204817c,0x0207163c,0x0204ed80,0x0204efec,0x02048d9c,0x02048cf4,0x02048c14,0x02048bb4,0x02048a38 cases=60
 * @difftest $S=zero:0x290 @$S+0xc8:32=pick:0x20,0x40 @$S+0x278:32=int:0:0xe $S int:0:0x2000 stub=0x020186ec:1,0x0204817c,0x0207163c,0x0204ed80,0x0204efec,0x02048d9c,0x02048cf4,0x02048c14,0x02048bb4,0x02048a38 cases=20 */
void func_02048ef4(u8 *self, u32 dt)
{
    if (!(BITS(U32_AT(self, 0xc8), 6, 1) != 0 && func_020186ec() == 1)) {
        if (BITS(U32_AT(self, 0xc8), 5, 1) == 0)
            return;
        if (func_020186ec() != 0)
            return;
    }
    if ((s8)BITS(U32_AT(self, 0x94), 2, 1) != 0) {
        func_0204817c(self);
    } else {
        switch (U32_AT(self, 0x278)) {
        case 0:
            Animate(self, dt);
            func_02048d9c(self, dt);
            break;
        case 1:
        case 2:
            Animate(self, dt);
            func_02048cf4(self, dt);
            break;
        case 3:
        case 4:
        case 0xb:
            Animate(self, dt);
            func_0204817c(self);
            break;
        case 5:
            Animate(self, dt);
            func_02048c14(self, dt);
            break;
        case 7:
        case 0xa:
            func_0204efec(self + 0xf4, dt);
            func_0204817c(self);
            break;
        case 8:
        case 9:
            func_02048bb4(self, dt);
            break;
        case 0xc:
            func_02048a38(self, dt);
            func_0204817c(self);
            break;
        default:
            func_0204817c(self);
            break;
        }
    }
    U32_AT(self, 0xc8) &= ~0x400u;
    U32_AT(self, 0xc8) &= ~8u;
}

/* 0x02049110: remove the object
 * @difftest ptr:0x290 cases=3 */
void func_02049110(u8 *self)
{
    U32_AT(self, 0xc8) |= 2;
}

/* 0x02049124: the map object kind
 * @difftest ptr:0x290 cases=3 */
u32 func_02049124(u8 *self)
{
    return U32_AT(self, 0x280);
}

/* 0x0204912c: set the map object kind
 * @difftest ptr:0x290 u32 cases=3 */
void func_0204912c(u8 *self, u32 v)
{
    U32_AT(self, 0x280) = v;
}

/* what both destructors do */
static void Destroy(u8 *self)
{
    PTR_AT(self, 0) = data_020bfb1c;
    PTR_AT(self, 0x90) = data_020bfb58;
    func_02008130(S8_AT(self, 0xd2), S16_AT(self, 0xd0));
    func_020483f8(self);
    func_02071838(self + 0x1d0);
    func_0204f964(self + 0xf4);
    PTR_AT(self, 0xdc) = data_020bfbc4;
    PTR_AT(self, 0xdc) = data_020bf760;
    func_020506c0(self + 0x90);
    func_0205b784(self);
}

/* 0x02049134: the object's deleting destructor
 * @difftest zero:0x290 stub=0x02008130,0x020483f8,0x02071838,0x0204f964,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_02049134(u8 *self)
{
    Destroy(self);
    func_0207fe44(self);
    return self;
}

/* 0x020491b4: the object's destructor
 * @difftest zero:0x290 stub=0x02008130,0x020483f8,0x02071838,0x0204f964,0x020506c0,0x0205b784 cases=3 */
u8 *func_020491b4(u8 *self)
{
    Destroy(self);
    return self;
}

/* the common part of the constructors */
static void Construct(u8 *self, u32 kind, u32 anim, s32 *pos, s32 *box, const u32 *m1,
                      const u32 *m2)
{
    u8 *d = self + 0xdc;
    u32 a[2], b[2];

    func_0205b7c8(self, box, data_020ebcac, data_020ebcc4);
    func_02050764(self + 0x90, pos);
    PTR_AT(self, 0) = data_020bfb1c;
    PTR_AT(self, 0x90) = data_020bfb58;
    b[0] = m2[0];
    b[1] = m2[1];
    a[0] = m1[0];
    a[1] = m1[1];
    PTR_AT(d, 0) = data_020bf760;
    PTR_AT(d, 0) = data_020bfbc4;
    PTR_AT(d, 4) = self;
    U32_AT(d, 8) = a[0];
    U32_AT(d, 0xc) = a[1];
    U32_AT(d, 0x10) = b[0];
    U32_AT(d, 0x14) = b[1];
    func_0204fa44(self + 0xf4, anim);
    func_02071864(self + 0x1d0);
    PTR_AT(self, 0x274) = NULL;
    S32_AT(self, 0x27c) = 1;
    S32_AT(self, 0x284) = 0x1000;
    U16_AT(self, 0x288) = 0;
    U16_AT(self, 0x28a) = 0;
    self[0x28c] = 0;
    func_02048474(self, kind);
}

/* 0x0204922c: the object's constructor (actor kind `kind`, animation
 * `anim`, at `pos`)
 * @difftest zero:0x290 int:0:0x130 int:0:0x10 ptr:8:4 stub=0x0205b7c8,0x02050764,0x0204fa44,0x02071864,0x02048474 cases=5 */
u8 *func_0204922c(u8 *self, u32 kind, u32 anim, s32 *pos)
{
    s32 box[3];

    box[0] = data_020bfacc;
    box[1] = data_020bfad0;
    box[2] = data_020bfad4;
    Construct(self, kind, anim, pos, box, data_020bfaf4, data_020bfafc);
    return self;
}

/* 0x02049378: the draw callback's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_02049378(u8 *self)
{
    PTR_AT(self, 0) = data_020bfbc4;
    PTR_AT(self, 0) = data_020bf760;
    return self;
}

/* 0x02049394: the object's constructor (actor kind `kind` at `pos`)
 * @difftest zero:0x290 int:0:0x130 ptr:8:4 stub=0x0205b7c8,0x02050764,0x0204fa44,0x02071864,0x02048474 cases=5 */
u8 *func_02049394(u8 *self, u32 kind, s32 *pos)
{
    s32 box[3];

    box[0] = data_020bfae8;
    box[1] = data_020bfae4;
    box[2] = data_020bfae0;
    Construct(self, kind, kind, pos, box, data_020bfb04, data_020bfb0c);
    return self;
}

/* 0x020494dc: create object `id` (of map object list `k`)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:4 int:0:2 stub=0x0204922c,0x02049394,0x0207fe74:0 cases=5 */
u8 *func_020494dc(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    s32 v[2], t[2], u[2];
    u32 kind16;
    s32 kk;
    u8 *p;

    v[0] = S16_AT(def, 4);
    v[1] = S16_AT(def, 6);
    func_02003b08(t, v, 0xc);
    v[0] = t[0];
    v[1] = t[1];
    kind16 = U16_AT(def, 8);
    if (kind16 == 0x11 && data_020d9f9c[0x34] == 1)
        return NULL;
    kk = func_0200820c((u16)kind16, 5);
    if (kind16 == 3) {
        s32 a = func_0200820c(0, 5);

        p = func_0207fe74(0x290);
        if (p != NULL)
            p = func_0204922c(p, kk, a, v);
    } else if (kind16 == 0xf) {
        s32 a = func_0200820c(0xf, 5);

        p = func_0207fe74(0x290);
        if (p != NULL)
            p = func_0204922c(p, kk, a, v);
    } else {
        p = func_0207fe74(0x290);
        if (p != NULL)
            p = func_02049394(p, kk, v);
    }
    func_02003b08(t, v, 4);
    v[0] = t[0];
    v[1] = t[1];
    u[0] = t[0];
    u[1] = t[1];
    S32_AT(p, 0xb4) = func_0201a0a0(u, NULL);
    func_0204912c(p, kind16);
    U32_AT(p, 0x94) |= 8;
    switch (kind16) {
    case 0:
        func_020477e8(p, 3);
        break;
    case 1: case 2: case 13: case 16: case 20:
        func_020477e8(p, 0);
        break;
    case 9: case 10:
        func_020477e8(p, 1);
        break;
    case 4:
        func_020477e8(p, 2);
        break;
    case 3:
        func_020477e8(p, 4);
        break;
    case 5: case 6: case 7:
        func_020477e8(p, 5);
        break;
    case 8:
        func_020477e8(p, 7);
        S32_AT(p, 0x27c) = 2;
        break;
    case 11:
        func_020477e8(p, 8);
        S32_AT(p, 0x27c) = 0;
        U32_AT(p, 0x94) &= ~8u;
        break;
    case 12:
        func_020477e8(p, 9);
        S32_AT(p, 0x27c) = 0;
        break;
    case 15:
        func_020477e8(p, 0xa);
        S32_AT(p, 0x27c) = 2;
        break;
    case 19:
        func_020477e8(p, 0xb);
        S32_AT(p, 0x27c) = 2;
        break;
    case 17:
        func_020477e8(p, 0xc);
        S32_AT(p, 0x27c) = 1;
        break;
    }
    U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x20u) | ((def[0xc] & 1) << 5);
    U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x40u) | ((def[0xd] & 1) << 6);
    func_020477cc(p, def[0xa]);
    func_020477b8(p, def[0xb]);
    p[0x28c] = def[0xf];
    return ACTOR(p);
}

/* 0x02049820: the draw callback's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_02049820(u8 *self)
{
    PTR_AT(self, 0) = data_020bfbc4;
    PTR_AT(self, 0) = data_020bf760;
    func_0207fe44(self);
    return self;
}

/* call member function pointer `m` ({function, adjustment * 2 + virtual}) on `obj` */
static void CallMember(u8 *obj, const u8 *m)
{
    s32 adj = S32_AT(m, 4);
    u8 *t = obj + (adj >> 1);
    void (*f)(void *, void *);

    if (adj & 1)
        f = *(void (**)(void *, void *))(*(u8 **)t + U32_AT(m, 0));
    else
        f = (void (*)(void *, void *))PTR_AT(m, 0);
    f(t, (void *)f);
}

/* 0x02049850: call the draw callback's first member function
 * @difftest $D=zero:0x18 @$D+8:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+8:32=8 @$D+0xc:32=pick:1,9 $D cases=5 */
void func_02049850(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 8) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 8);
}

/* 0x020498b0: call the draw callback's second member function
 * @difftest $D=zero:0x18 @$D+0x10:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+0x10:32=8 @$D+0x14:32=pick:1,9 $D cases=5 */
void func_020498b0(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 0x10) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 0x10);
}

/* The actor's this-adjusting entry points (the object is 0x90 before it). */

/* 0x02049910
 * @difftest $S=zero:0x290 $S+0x90 stub=0x02008130,0x020483f8,0x02071838,0x0204f964,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_02049910(u8 *a)
{
    return func_02049134(a - 0x90);
}

/* 0x02049920
 * @difftest $S=zero:0x290 $S+0x90 int:0:0x6487 cases=3 */
void func_02049920(u8 *a, fx32 h)
{
    func_02047780(a - 0x90, h);
}

/* 0x02049930
 * @difftest $S=zero:0x290 $S+0x90 cases=3 */
fx32 func_02049930(u8 *a)
{
    return func_020477a8(a - 0x90);
}

/* 0x02049940
 * @difftest $S=zero:0x290 $S+0x90 cases=3 */
u8 *func_02049940(u8 *a)
{
    return func_020477b0(a - 0x90);
}

/* 0x02049950
 * @difftest $S=zero:0x290 $S+0x90 cases=3 */
void func_02049950(u8 *a)
{
    func_02049110(a - 0x90);
}

/* 0x02049960
 * @difftest $S=zero:0x290 $O=zero:0x60 @$O+0xc:32=1 $S+0x90 $O cases=3 */
void func_02049960(u8 *a, u8 *other)
{
    func_02047cb0(a - 0x90, other);
}

/* 0x02049970
 * @difftest $S=zero:0x290 $S+0x90 stub=0x0204ed80 cases=3 */
void func_02049970(u8 *a)
{
    func_020477f0(a - 0x90);
}

/* 0x02049980
 * @difftest $S=zero:0x290 $S+0x90 stub=0x0204ed80,0x02047b08 cases=3 */
void func_02049980(u8 *a)
{
    func_02047828(a - 0x90);
}

/* 0x02049990
 * @difftest $S=zero:0x290 @$S+0x278:32=3 $S+0x90 cases=3 */
void func_02049990(u8 *a)
{
    func_020478dc(a - 0x90);
}

/* 0x020499a0
 * @difftest $S=zero:0x290 $S+0x90 int:0:0x2000 stub=0x020186ec:0 cases=3 */
void func_020499a0(u8 *a, u32 dt)
{
    func_02048ef4(a - 0x90, dt);
}

/* 0x020499b0
 * @difftest $S=zero:0x290 $S+0x90 stub=0x02008130,0x020483f8,0x02071838,0x0204f964,0x020506c0,0x0205b784 cases=3 */
u8 *func_020499b0(u8 *a)
{
    return func_020491b4(a - 0x90);
}
