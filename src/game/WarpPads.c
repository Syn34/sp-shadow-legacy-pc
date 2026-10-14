/*
 * Warp pads (actor kind 0xd7, class 0x10): touching one switches it on
 * for good (flag data_020d9f9c[0x1a2 + i] for warp point i, the one of
 * data_020c3b78 matching the current state) with an animation and a
 * sound. When the hero arrives by warp (data_020df104), he is put on the
 * pad.
 * ARM9 main, 0x0204d9a0 - 0x0204e42c (22 functions).
 *
 * The object (0x1d4 bytes, vtable data_020c01dc) is a model with an actor
 * at +0x90 (vtable data_020c0204; this-adjusting entry points at
 * 0x0204e3cc..), a draw callback at +0xdc (vtables data_020c026c /
 * data_020bf760), an animation at +0xf4, and its flag index at +0x1d0.
 *
 * Testing: with fake objects; the animation, the lights, the sounds and
 * the camera stubbed or faked where they need real data.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020b6cc4;
extern const fx32 data_020c017c, data_020c0180, data_020c0184, data_020c0188, data_020c018c,
    data_020c0190, data_020c0194;    /* boxes */
extern const u32 data_020c0198[2], data_020c01a0[2];   /* the member functions */
extern u8 data_020c01dc[], data_020c0204[], data_020c026c[], data_020bf760[];   /* vtables */
extern const char data_020c0248[];   /* debug message */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */
extern u8 *data_020c3b78;            /* warp points, 0x10 bytes each */
extern u8 data_020d6b8c[];
extern u8 data_020d9f9c[];
extern s8 data_020df104;             /* arriving by warp */
extern u8 data_020f6e58[], data_020f6ea0[];

void func_0204ed80(void *anim, u32 id, u32 a);
void func_02020440(u32 v);
u32 func_020186ec(void);
u32 func_02019650(void);
void func_0201c17c(s32 *out, s32 *pos);
void *func_020062e0(void);
void *func_020062c0(void);
void *func_020062b0(void);
void func_020040f4(u8 *dst, const u8 *src);
void func_02082ce8(void *cam);
void *func_02019670(void);
void func_02043cc8(u8 *list, void *item);
void func_02017abc(void *list, void *item);
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
void func_02080f60(void *settings, u32 a, u32 b);
void func_02080e84(void *settings);
void func_0204e948(void *anim, u32 frame);
void func_0204ebb4(void *anim);
void func_0204efec(void *anim, u32 dt);
void func_02008130(s32 group, s32 index);
void func_0204f964(void *anim);
void *func_020506c0(void *actor);
void func_0205b784(void *self);
void func_0205b7c8(void *self, s32 *box, void *a, void *b);
void func_02050890(void *actor, s32 *pos, s32 *box);
void func_0204fa44(void *anim, u32 kind);
void func_0207fe44(void *p);
void *func_0207fe74(u32 size);
u8 *func_02008994(s32 i, s32 k);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
void func_020038d4(s32 *out, const s32 *in, s32 shift);
s32 func_0201a0a0(s32 *xz, void *obj);
void func_02003c74(void *actor, s32 x, s32 y);
void func_02017028(void *self);

void func_0204d9a0(u8 *self, fx32 h);
fx32 func_0204d9c8(u8 *self);
void func_0204d9d0(u8 *self, u8 *other);
s32 func_0204da88(u8 *self);
void func_0204dc68(u8 *self);
void func_0204dc6c(u8 *self, u32 kind);
void func_0204deac(u8 *self, u32 dt);
u8 *func_0204dedc(u8 *self);
u8 *func_0204df54(u8 *self);
u8 *func_0204dfc4(u8 *self, u32 kind, s32 *pos);

#define ACTOR(self) ((self) != NULL ? (self) + 0x90 : NULL)
#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define HERO_ACTOR() ((u8 *)PTR_AT(data_020deebc, 4))

/* 0x0204d9a0: turn the pad to `h`
 * @difftest zero:0x1d4 int:0:0x6487 cases=3 */
void func_0204d9a0(u8 *self, fx32 h)
{
    s32 x = S32_AT(self, 0x10), z = S32_AT(self, 0x18);

    S32_AT(self, 0x10) = x;
    S32_AT(self, 0x14) = h;
    S32_AT(self, 0x18) = z;
    self[0x8c] = 1;
}

/* 0x0204d9c8: the pad's direction
 * @difftest ptr:0x1d4 cases=3 */
fx32 func_0204d9c8(u8 *self)
{
    return S32_AT(self, 0x14);
}

/* 0x0204d9d0: touched: switch the pad on
 * @difftest $S=zero:0x1d4 @$S+0xb4:32=int:-0x80000:0x80000 @$S+0x1d0:32=pick:0x1a2,0x1a3 @020da13e:8=pick:0,1 $O=zero:0x60 @$O+0xc:32=pick:0,0,1 @$O+8:32=pick:1,1,2 @$O+0x24:32=int:-0x80000:0x80000 $S $O stub=0x0204ed80,0x02020440 cases=40 */
void func_0204d9d0(u8 *self, u8 *other)
{
    s32 d;
    u8 *f;

    if (S32_AT(other, 0xc) != 0)
        return;
    if (S32_AT(other, 8) != 1)
        return;
    d = (S32_AT(other, 0x24) - S32_AT(self, 0xb4)) >> 4;
    if (d < 0)
        d = -d;
    if (d >= 0x4000)
        return;
    U32_AT(self, 0xc8) |= 8;
    f = data_020d9f9c + S32_AT(self, 0x1d0);
    if (*f != 0)
        return;
    *f = 1;
    func_0204ed80(self + 0xf4, 0x1b1, 0);
    func_02020440(0x4e);
}

/* 0x0204da88: place the pad and draw it if in view (1 if drawn)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0x1d4 @$S+0xc8:32=pick:0,0x20,0x40 @$S+0xb8:32=int:-0x10000:0x10000 @$S+0xbc:32=int:-0x10000:0x10000 @$S+0xa0:32=int:-0x10:0 @$S+0xa4:32=int:-0x10:0 @$S+0xa8:32=int:0:0x10 @$S+0xac:32=int:0:0x10 $S stub=0x0201f490:0x020deebc,0x020186ec:0,0x02043cc8,0x02017abc cases=30 */
s32 func_0204da88(u8 *self)
{
    s32 xz[2], p[3], b[4];
    u32 cam[0x2c0 / 4];
    s32 vis;

    if (!(BITS(U32_AT(self, 0xc8), 6, 1) != 0 && func_020186ec() == 1)) {
        if (BITS(U32_AT(self, 0xc8), 5, 1) == 0)
            return 0;
        if (func_020186ec() != 0)
            return 0;
    }
    xz[0] = S32_AT(self, 0xb8);
    xz[1] = S32_AT(self, 0xbc);
    func_0201c17c(p, xz);
    S32_AT(self, 4) = p[0];
    S32_AT(self, 8) = p[1];
    S32_AT(self, 0xc) = p[2];
    self[0x8c] = 1;
    b[0] = S32_AT(self, 8) + SHL(S32_AT(self, 0xa0), 12);
    b[1] = S32_AT(self, 4) + SHL(S32_AT(self, 0xa4), 12);
    b[2] = S32_AT(self, 8) + SHL(S32_AT(self, 0xa8), 12);
    b[3] = S32_AT(self, 4) + SHL(S32_AT(self, 0xac), 12);
    func_020040f4((u8 *)cam, func_020062e0());
    vis = 0;
    if (S8_AT(cam, 0x284) != 0) {
        vis = S32_AT(cam, 0x2ac) >= b[1] && S32_AT(cam, 0x2a4) <= b[3]
              && S32_AT(cam, 0x2a0) <= b[2] && S32_AT(cam, 0x2a8) >= b[0];
    }
    func_02082ce8(cam);
    if (vis == 0) {
        U32_AT(self, 0xc8) &= ~0x2000u;
        return 0;
    }
    func_02043cc8((u8 *)func_02019670() + 4, self + 0xdc);
    func_02017abc((u8 *)func_020062c0() + 4, ACTOR(self));
    U32_AT(self, 0xc8) |= 0x2000;
    return 1;
}

/* 0x0204dc68
 * @difftest ptr:0x1d4 cases=3 */
void func_0204dc68(u8 *self)
{
}

/* 0x0204dc6c: set up a pad of actor kind `kind`
 * @difftest $S=zero:0x1d4 $S pick:0xd7,3 stub=0x02017abc,0x0204ed80 cases=5 */
void func_0204dc6c(u8 *self, u32 kind)
{
    s32 sc, n, i;

    sc = S32_AT(data_020c3b0c + kind * 0x48, 0x2c);
    S32_AT(self, 0x1c) = sc;
    S32_AT(self, 0x20) = sc;
    S32_AT(self, 0x24) = sc;
    self[0x8c] = 1;
    S32_AT(self, 4) = 0;
    S32_AT(self, 8) = 0;
    S32_AT(self, 0xc) = 0;
    self[0x8c] = 1;
    S32_AT(self, 0x10) = data_020b6cc4;
    S32_AT(self, 0x14) = g_FxZero;
    S32_AT(self, 0x18) = g_FxZero;
    self[0x8c] = 1;
    S32_AT(self, 0x9c) = 0x10;
    S32_AT(self, 0x98) = kind;
    U32_AT(self, 0xc8) = (U32_AT(self, 0xc8) & ~1u) | 1;
    U32_AT(self, 0xc8) |= 4;
    func_02017abc((u8 *)func_020062b0() + 4, ACTOR(self));
    n = S32_AT(data_020d6b8c, 0x9e4);
    for (i = 0; i < n; i++) {
        if (U32_AT(data_020c3b78 + i * 0x10, 0) == func_02019650())
            S32_AT(self, 0x1d0) = i + 0x1a2;
    }
    if (data_020d9f9c[S32_AT(self, 0x1d0)] != 0)
        func_0204ed80(self + 0xf4, 0x1b1, 0);
    else
        func_0204ed80(self + 0xf4, 0x1b2, 0);
}

/* 0x0204ddc8: draw the pad
 * @difftest $S=zero:0x1d4 @$S+0x8c:8=pick:0,1 @$S+0xcc:32=int:0:0x20 $S stub=0x02080f60,0x02080e84,0x0204e948 cases=5 */
void func_0204ddc8(u8 *self)
{
    u8 *g;

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
    func_02080f60(func_020062e0(), 0, 0);
    func_02080e84(func_020062e0());
    func_0204e948(self + 0xf4, U32_AT(self, 0xcc));
}

/* 0x0204de9c: the model's func_0204ebb4
 * @difftest zero:0x1d4 stub=0x0204ebb4 cases=3 */
void func_0204de9c(u8 *self)
{
    func_0204ebb4(self + 0xf4);
}

/* 0x0204deac: per-frame update
 * @difftest $S=zero:0x1d4 @$S+0xc8:32=pick:0,8 $S int:0:0x2000 stub=0x0204efec cases=5 */
void func_0204deac(u8 *self, u32 dt)
{
    func_0204efec(self + 0xf4, dt);
    func_0204da88(self);
    U32_AT(self, 0xc8) &= ~8u;
}

/* what both destructors do */
static void Destroy(u8 *self)
{
    PTR_AT(self, 0) = data_020c01dc;
    PTR_AT(self, 0x90) = data_020c0204;
    func_02008130(S8_AT(self, 0xd2), S16_AT(self, 0xd0));
    func_0204dc68(self);
    func_0204f964(self + 0xf4);
    PTR_AT(self, 0xdc) = data_020c026c;
    PTR_AT(self, 0xdc) = data_020bf760;
    func_020506c0(self + 0x90);
    func_0205b784(self);
}

/* 0x0204dedc: the pad's deleting destructor
 * @difftest zero:0x1d4 stub=0x02008130,0x0204f964,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_0204dedc(u8 *self)
{
    Destroy(self);
    func_0207fe44(self);
    return self;
}

/* 0x0204df54: the pad's destructor
 * @difftest zero:0x1d4 stub=0x02008130,0x0204f964,0x020506c0,0x0205b784 cases=3 */
u8 *func_0204df54(u8 *self)
{
    Destroy(self);
    return self;
}

/* 0x0204dfc4: the pad's constructor (actor kind `kind` at `pos`)
 * @difftest zero:0x1d4 pick:0xd7 ptr:8:4 stub=0x0205b7c8,0x02050890,0x0204fa44,0x02017abc,0x0204ed80 cases=3 */
u8 *func_0204dfc4(u8 *self, u32 kind, s32 *pos)
{
    s32 box3[3], box4[4];
    u8 *d = self + 0xdc;
    u32 m1[2], m2[2];

    box3[0] = data_020c0184;
    box3[1] = data_020c0180;
    box3[2] = data_020c0190;
    func_0205b7c8(self, box3, data_020f6e58, data_020f6ea0);
    box4[0] = data_020c0194;
    box4[1] = data_020c0188;
    box4[2] = data_020c017c;
    box4[3] = data_020c018c;
    func_02050890(self + 0x90, pos, box4);
    PTR_AT(self, 0) = data_020c01dc;
    PTR_AT(self, 0x90) = data_020c0204;
    m2[0] = data_020c01a0[0];
    m2[1] = data_020c01a0[1];
    m1[0] = data_020c0198[0];
    m1[1] = data_020c0198[1];
    PTR_AT(d, 0) = data_020bf760;
    PTR_AT(d, 0) = data_020c026c;
    PTR_AT(d, 4) = self;
    U32_AT(d, 8) = m1[0];
    U32_AT(d, 0xc) = m1[1];
    U32_AT(d, 0x10) = m2[0];
    U32_AT(d, 0x14) = m2[1];
    func_0204fa44(self + 0xf4, kind);
    S32_AT(self, 0x1d0) = 0;
    func_0204dc6c(self, kind);
    return self;
}

/* 0x0204e128: the draw callback's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_0204e128(u8 *self)
{
    PTR_AT(self, 0) = data_020c026c;
    PTR_AT(self, 0) = data_020bf760;
    return self;
}

/* 0x0204e144: create warp pad `id` (of map object list `k`)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:4 int:0:2 @020df104:8=0 stub=0x0207fe74:0 cases=5 */
u8 *func_0204e144(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    s32 v[2], t[2], u[2];
    u8 *p;

    v[0] = S16_AT(def, 4);
    v[1] = S16_AT(def, 6);
    func_02003b08(t, v, 0xc);
    v[0] = t[0];
    v[1] = t[1];
    p = func_0207fe74(0x1d4);
    if (p != NULL)
        p = func_0204dfc4(p, 0xd7, v);
    U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x20u) | ((def[8] & 1) << 5);
    U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x40u) | ((def[9] & 1) << 6);
    func_02003b08(t, v, 4);
    v[0] = t[0];
    v[1] = t[1];
    u[0] = t[0];
    u[1] = t[1];
    S32_AT(p, 0xb4) = func_0201a0a0(u, NULL);
    func_02004490(data_020c0248);
    if (data_020df104 != 0) {
        s32 w[2], n[2];
        u8 *a;

        func_02003c74(HERO_ACTOR(), v[0], v[1]);
        w[0] = v[0];
        w[1] = v[1];
        S32_AT(HERO_ACTOR(), 0x24) = func_0201a0a0(w, HERO_ACTOR()) + 0x1e0000;
        a = HERO_ACTOR();
        S32_AT(a, 0x6c) = S32_AT(a, 0x24);
        func_020038d4(n, v, 4);
        v[0] = n[0];
        v[1] = n[1];
        S32_AT(HERO(), 0x30) = n[0];
        S32_AT(HERO(), 0x34) = v[1];
        func_02017028(data_020deebc);
    }
    return ACTOR(p);
}

/* 0x0204e2dc: the draw callback's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_0204e2dc(u8 *self)
{
    PTR_AT(self, 0) = data_020c026c;
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

/* 0x0204e30c: call the draw callback's first member function
 * @difftest $D=zero:0x18 @$D+8:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+8:32=8 @$D+0xc:32=pick:1,9 $D cases=5 */
void func_0204e30c(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 8) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 8);
}

/* 0x0204e36c: call the draw callback's second member function
 * @difftest $D=zero:0x18 @$D+0x10:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+0x10:32=8 @$D+0x14:32=pick:1,9 $D cases=5 */
void func_0204e36c(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 0x10) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 0x10);
}

/* The actor's this-adjusting entry points (the object is 0x90 before it). */

/* 0x0204e3cc
 * @difftest $S=zero:0x1d4 $S+0x90 stub=0x02008130,0x0204f964,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_0204e3cc(u8 *a)
{
    return func_0204dedc(a - 0x90);
}

/* 0x0204e3dc
 * @difftest $S=zero:0x1d4 $S+0x90 int:0:0x6487 cases=3 */
void func_0204e3dc(u8 *a, fx32 h)
{
    func_0204d9a0(a - 0x90, h);
}

/* 0x0204e3ec
 * @difftest $S=zero:0x1d4 $S+0x90 cases=3 */
fx32 func_0204e3ec(u8 *a)
{
    return func_0204d9c8(a - 0x90);
}

/* 0x0204e3fc
 * @difftest $S=zero:0x1d4 $O=zero:0x60 @$O+0xc:32=1 $S+0x90 $O cases=3 */
void func_0204e3fc(u8 *a, u8 *other)
{
    func_0204d9d0(a - 0x90, other);
}

/* 0x0204e40c
 * @difftest $S=zero:0x1d4 $S+0x90 int:0:0x2000 stub=0x0204efec cases=3 */
void func_0204e40c(u8 *a, u32 dt)
{
    func_0204deac(a - 0x90, dt);
}

/* 0x0204e41c
 * @difftest $S=zero:0x1d4 $S+0x90 stub=0x02008130,0x0204f964,0x020506c0,0x0205b784 cases=3 */
u8 *func_0204e41c(u8 *a)
{
    return func_0204df54(a - 0x90);
}
