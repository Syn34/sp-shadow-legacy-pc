/*
 * Backdrops: distant scenery models (actor class 0xf) that follow the
 * camera with parallax, placed relative to the level's origin (the level
 * table data_020c3b60, 0x60 bytes per level: +0x30 / +0x34 origin, +0x38
 * depth, +0x54 parallax). Kinds 0x79-0x7d change their animation once
 * data_020d9f9c[0xef] is set.
 * ARM9 main, 0x0204cb4c - 0x0204d9a0 (19 functions).
 *
 * The object (0xfc bytes, vtable data_020c009c) is a model with an actor
 * at +0x90 (vtable data_020c00bc; this-adjusting entry points at
 * 0x0204d960..), a draw callback at +0xdc (vtables data_020c0140 /
 * data_020bf760) and an animated model at +0xf4 (func_0204f9a4 with the
 * kind's entry of data_020c3b84); +0xf8 the animation is to change.
 *
 * Testing: with fake objects; the models, the lights and the camera
 * stubbed or faked where they need real data.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020c0038, data_020c003c, data_020c0040, data_020c0044, data_020c0048,
    data_020c004c, data_020c0050;    /* boxes */
extern const u32 data_020c0054[2], data_020c005c[2];   /* the member functions */
extern u8 data_020c009c[], data_020c00bc[], data_020c0140[], data_020bf760[];   /* vtables */
extern const char data_020c0100[], data_020c0118[];   /* debug messages */
extern u8 *data_020c3b84;            /* models by kind, 0xc bytes each */
extern u8 data_020d9f9c[];
extern u8 data_020df1bc[];
extern u8 data_020f6e58[], data_020f6ea0[];

void *func_0201f490(void);
void func_0201c428(s32 *out);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
s32 func_0201a0a0(s32 *xz, void *obj);
s32 func_0200be9c(void *z);
void func_02003798(s32 *out, const s32 *in, const s32 *scale);
void func_02005c1c(s32 *out, const s32 *a, const s32 *b);
void func_02005b98(s32 *out, const void *src);
void *func_020062e0(void);
void *func_020062b0(void);
void func_020040f4(u8 *dst, const u8 *src);
void func_02082ce8(void *cam);
void *func_02019670(void);
void func_02043cc8(u8 *list, void *item);
void func_0204f964(void *anim);
void *func_0204f9a4(void *p, void *a, void *b);
void func_0207fe44(void *p);
void *func_0207fe74(u32 size);
void func_0204ed80(void *anim, u32 id, u32 a);
void func_02017abc(void *list, void *item);
void func_0202de98(s32 *m);
void func_02017550(s32 *out, const u8 *cam);
void func_0202de68(s32 *m, const s32 *v);
void func_020022e4(void *self);
void func_0201751c(s32 *out, const u8 *cam);
void func_0202dc88(s32 *m, const s32 *rot);
void func_02002974(void *self);
void func_0202dc6c(s32 *m, const s32 *v);
void MI_Copy64B(const void *src, void *dest);
void func_02080e84(void *settings);
void func_0204e948(void *anim, u32 frame);
s32 func_0204ebb4(void *anim, u32 a, u32 b, u32 c);
u32 func_020186ec(void);
void func_02008130(s32 group, s32 index);
void *func_020506c0(void *actor);
void func_0205b784(void *self);
void func_0205b7c8(void *self, s32 *box, void *a, void *b);
void func_02050764(void *actor, s32 *pos);
u8 *func_02008994(s32 i, s32 k);

void func_0204cb4c(u8 *self);
s32 func_0204cb50(u8 *self);
void func_0204d060(u8 *self);
void func_0204d0ac(u8 *self, u32 kind);
void func_0204d2ec(u8 *self);
u8 *func_0204d390(u8 *self);
u8 *func_0204d400(u8 *self);
u8 *func_0204d468(u8 *self, u32 kind, s32 *pos);

#define ACTOR(self) ((self) != NULL ? (self) + 0x90 : NULL)
#define LEVEL_DATA() (data_020c3b60 + (func_020436f0(data_020bf6a0)[0xf] >> 1) * 0x60)

/* an int made fx32 through the game's float routines, rounded */
static float FxF(s32 v)
{
    float f = (float)SHL(v, 12);

    return v > 0 ? 0.5f + f : f - 0.5f;
}

/* 0x0204cb4c
 * @difftest ptr:0xfc cases=3 */
void func_0204cb4c(u8 *self)
{
}

/* 0x0204cb50: place the backdrop for the camera; draw it if in view
 * (1 if drawn)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0xfc @$S+0x94:32=pick:0,8 @$S+0xb4:32=int:-0x100000:0x100000 @$S+0xb8:32=int:-0x100000:0x100000 @$S+0xbc:32=int:-0x100000:0x100000 @$S+0xa0:32=int:-0x10:0 @$S+0xa4:32=int:-0x10:0 @$S+0xa8:32=int:0:0x10 @$S+0xac:32=int:0:0x10 $S stub=0x0201f490:0x020deebc,0x02043cc8 cases=30 */
s32 func_0204cb50(u8 *self)
{
    s32 c[2], v[2], s[2], t[2], w[2], u[2], p[3], b[4];
    s32 h, k, y, x, z, depth;
    u32 cam[0x2c0 / 4];
    float f;
    u8 *cm;
    s32 vis;

    if (BITS(U32_AT(self, 0x94), 3, 1) == 0)
        return 0;
    cm = func_0201f490();
    c[0] = S32_AT(cm, 0x28);
    c[1] = S32_AT(cm, 0x2c);
    func_0201c428(s);
    func_02003b08(t, c, 4);
    h = S32_AT(self, 0xb4);
    h = (h - func_0201a0a0(t, NULL)) >> 4;
    v[0] = S32_AT(self, 0xb8) - c[0];
    v[1] = S32_AT(self, 0xbc) - c[1];
    k = func_0200be9c(data_020e1f50);
    func_02003798(w, v, &k);
    v[0] = w[0];
    v[1] = w[1];
    func_02005c1c(u, v, c);
    v[0] = u[0];
    v[1] = u[1];
    y = s[1] - (v[1] >> 12) + (FxMul(func_0200be9c(data_020e1f50), h) >> 12);
    depth = FxMul((s32)FxF(S32_AT(LEVEL_DATA(), 0x54)), 0x29);
    f = FxF(S32_AT(LEVEL_DATA(), 0x38));
    func_02005b98(p, data_020df1bc);
    z = p[2] - FxMul((s32)f, depth);
    y = (s32)FxF(y);
    x = (s32)FxF(v[0] >> 12);
    y = (s32)FxF(y >> 12);
    S32_AT(self, 4) = x;
    S32_AT(self, 8) = y;
    S32_AT(self, 0xc) = z;
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
    U32_AT(self, 0xc8) |= 0x2000;
    func_02043cc8((u8 *)func_02019670() + 4, self + 0xdc);
    return 1;
}

/* 0x0204d060: delete the animated model
 * @difftest zero:0xfc cases=3
 * @difftest $P=zero:0xdc $S=zero:0xfc @$S+0xf4:32=$P $S stub=0x0204f964,0x0207fe44 cases=3 */
void func_0204d060(u8 *self)
{
    u8 *p = PTR_AT(self, 0xf4);

    if (p == NULL)
        return;
    if (p != NULL) {
        func_0204f964(p);
        func_0207fe44(p);
    }
    PTR_AT(self, 0xf4) = NULL;
}

/* 0x0204d0ac: set up a backdrop of kind `kind`
 * @difftest zero:0xfc pick:3,0x79,0x7b,0x7d,0x7e @020da08b:8=0 stub=0x0207fe74:0,0x0204ed80,0x02017abc cases=10 */
void func_0204d0ac(u8 *self, u32 kind)
{
    fx32 z;
    void *p;

    S32_AT(self, 0x1c) = 0x1000;
    S32_AT(self, 0x20) = 0x1000;
    S32_AT(self, 0x24) = 0;
    self[0x8c] = 1;
    z = g_FxZero;
    S32_AT(self, 0x10) = z;
    S32_AT(self, 0x14) = z;
    S32_AT(self, 0x18) = z;
    self[0x8c] = 1;
    S32_AT(self, 0x9c) = 0xf;
    S32_AT(self, 0x98) = kind;
    U32_AT(self, 0xc8) = (U32_AT(self, 0xc8) & ~1u) | 1;
    U32_AT(self, 0xc8) &= ~4u;
    S32_AT(self, 0xa0) = data_020c0044;
    S32_AT(self, 0xa4) = data_020c0050;
    S32_AT(self, 0xa8) = data_020c0048;
    S32_AT(self, 0xac) = data_020c003c;
    p = func_0207fe74(0xdc);
    if (p != NULL)
        p = func_0204f9a4(p, PTR_AT(data_020c3b84 + kind * 0xc, 0),
                          PTR_AT(data_020c3b84 + kind * 0xc, 4));
    PTR_AT(self, 0xf4) = p;
    if (data_020d9f9c[0xef] == 0 && kind >= 0x79 && kind <= 0x7d) {
        self[0xf8] = 1;
        func_0204ed80(PTR_AT(self, 0xf4), 0x2dd, 0);
    }
    func_02017abc((u8 *)func_020062b0() + 4, ACTOR(self));
}

/* 0x0204d218: draw the backdrop
 * @difftest $S=zero:0xfc @$S+0x8c:8=pick:0,1 @$S+0xcc:32=int:0:0x20 $S stub=0x02080e84,0x0204e948 cases=5 */
void func_0204d218(u8 *self)
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
    func_02080e84(func_020062e0());
    func_0204e948(PTR_AT(self, 0xf4), U32_AT(self, 0xcc));
}

/* 0x0204d2dc: the animated model's func_0204ebb4
 * @difftest zero:0xfc u32 u32 u32 stub=0x0204ebb4 cases=3 */
s32 func_0204d2dc(u8 *self, u32 a, u32 b, u32 c)
{
    return func_0204ebb4(PTR_AT(self, 0xf4), a, b, c);
}

/* 0x0204d2ec: per-frame update
 * @difftest $S=zero:0xfc @$S+0xc8:32=pick:0,0x20,0x40,0x60 @$S+0xf8:8=pick:0,1 @$S+0x98:32=pick:3,0x79,0x7d @020da08b:8=pick:0,1 $S stub=0x020186ec:0,0x0204ed80,0x0204cb50 cases=30
 * @difftest $S=zero:0xfc @$S+0xc8:32=0x40 @$S+0xf8:8=1 @$S+0x98:32=0x7a @020da08b:8=1 $S stub=0x020186ec:1,0x0204ed80,0x0204cb50 cases=5 */
void func_0204d2ec(u8 *self)
{
    if (!(BITS(U32_AT(self, 0xc8), 6, 1) != 0 && func_020186ec() == 1)) {
        if (BITS(U32_AT(self, 0xc8), 5, 1) == 0)
            return;
        if (func_020186ec() != 0)
            return;
    }
    if (data_020d9f9c[0xef] == 1 && S8_AT(self, 0xf8) != 0 && S32_AT(self, 0x98) >= 0x79
        && S32_AT(self, 0x98) <= 0x7d) {
        self[0xf8] = 0;
        func_0204ed80(PTR_AT(self, 0xf4), 0x2de, 0);
    }
    func_0204cb50(self);
}

/* what both destructors do */
static void Destroy(u8 *self)
{
    PTR_AT(self, 0) = data_020c009c;
    PTR_AT(self, 0x90) = data_020c00bc;
    func_02008130(S8_AT(self, 0xd2), S16_AT(self, 0xd0));
    func_0204d060(self);
    PTR_AT(self, 0xdc) = data_020c0140;
    PTR_AT(self, 0xdc) = data_020bf760;
    func_020506c0(self + 0x90);
    func_0205b784(self);
}

/* 0x0204d390: the backdrop's deleting destructor
 * @difftest zero:0xfc stub=0x02008130,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_0204d390(u8 *self)
{
    Destroy(self);
    func_0207fe44(self);
    return self;
}

/* 0x0204d400: the backdrop's destructor
 * @difftest zero:0xfc stub=0x02008130,0x020506c0,0x0205b784 cases=3 */
u8 *func_0204d400(u8 *self)
{
    Destroy(self);
    return self;
}

/* 0x0204d468: the backdrop's constructor (kind `kind` at `pos`)
 * @difftest zero:0xfc int:0:0x80 ptr:8:4 stub=0x0205b7c8,0x02050764,0x0207fe74:0,0x0204ed80,0x02017abc cases=5 */
u8 *func_0204d468(u8 *self, u32 kind, s32 *pos)
{
    s32 box[3];
    u8 *d = self + 0xdc;
    u32 m1[2], m2[2];

    box[0] = data_020c0038;
    box[1] = data_020c004c;
    box[2] = data_020c0040;
    func_0205b7c8(self, box, data_020f6e58, data_020f6ea0);
    func_02050764(self + 0x90, pos);
    PTR_AT(self, 0) = data_020c009c;
    PTR_AT(self, 0x90) = data_020c00bc;
    m2[0] = data_020c005c[0];
    m2[1] = data_020c005c[1];
    m1[0] = data_020c0054[0];
    m1[1] = data_020c0054[1];
    PTR_AT(d, 0) = data_020bf760;
    PTR_AT(d, 0) = data_020c0140;
    PTR_AT(d, 4) = self;
    U32_AT(d, 8) = m1[0];
    U32_AT(d, 0xc) = m1[1];
    U32_AT(d, 0x10) = m2[0];
    U32_AT(d, 0x14) = m2[1];
    PTR_AT(self, 0xf4) = NULL;
    self[0xf8] = 0;
    func_0204d0ac(self, kind);
    return self;
}

/* 0x0204d580: the draw callback's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_0204d580(u8 *self)
{
    PTR_AT(self, 0) = data_020c0140;
    PTR_AT(self, 0) = data_020bf760;
    return self;
}

/* 0x0204d59c: create backdrop `id` (of map object list `k`)
 * @difftest int:0:4 int:0:2 stub=0x0207fe74:0 cases=5 */
u8 *func_0204d59c(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    s32 v[2], t[2];
    u32 kind16;
    u8 *p;

    v[0] = S16_AT(def, 4);
    v[1] = S16_AT(def, 6);
    func_02003b08(t, v, 0xc);
    v[0] = t[0];
    v[1] = t[1];
    kind16 = U16_AT(def, 8);
    if (data_020d9f9c[0xef] == 1 && kind16 + 1 >= 0x79 && kind16 + 1 <= 0x7d)
        return NULL;
    v[1] -= (s32)FxF(S32_AT(LEVEL_DATA(), 0x34));
    v[0] -= (s32)FxF(S32_AT(LEVEL_DATA(), 0x30));
    p = func_0207fe74(0xfc);
    if (p != NULL)
        p = func_0204d468(p, kind16 + 1, v);
    func_02004490(data_020c0100, def[0xe]);
    S32_AT(p, 0xb4) = def[0xe];
    S32_AT(p, 0x1c) = 0x2000;
    S32_AT(p, 0x20) = 0x2000;
    S32_AT(p, 0x24) = 0x2000;
    p[0x8c] = 1;
    U32_AT(p, 0xc8) |= 0x40000;
    U32_AT(p, 0x94) |= 8;
    U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x20u) | ((def[0xc] & 1) << 5);
    U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x40u) | ((def[0xd] & 1) << 6);
    func_02004490(data_020c0118);
    return ACTOR(p);
}

/* 0x0204d870: the draw callback's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_0204d870(u8 *self)
{
    PTR_AT(self, 0) = data_020c0140;
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

/* 0x0204d8a0: call the draw callback's first member function
 * @difftest $D=zero:0x18 @$D+8:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+8:32=8 @$D+0xc:32=pick:1,9 $D cases=5 */
void func_0204d8a0(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 8) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 8);
}

/* 0x0204d900: call the draw callback's second member function
 * @difftest $D=zero:0x18 @$D+0x10:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+0x10:32=8 @$D+0x14:32=pick:1,9 $D cases=5 */
void func_0204d900(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 0x10) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 0x10);
}

/* The actor's this-adjusting entry points (the object is 0x90 before it). */

/* 0x0204d960
 * @difftest $S=zero:0xfc $S+0x90 stub=0x02008130,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_0204d960(u8 *a)
{
    return func_0204d390(a - 0x90);
}

/* 0x0204d970
 * @difftest $S=zero:0xfc $S+0x90 cases=3 */
void func_0204d970(u8 *a)
{
    func_0204cb4c(a - 0x90);
}

/* 0x0204d980
 * @difftest $S=zero:0xfc $S+0x90 stub=0x020186ec:1 cases=3 */
void func_0204d980(u8 *a)
{
    func_0204d2ec(a - 0x90);
}

/* 0x0204d990
 * @difftest $S=zero:0xfc $S+0x90 stub=0x02008130,0x020506c0,0x0205b784 cases=3 */
u8 *func_0204d990(u8 *a)
{
    return func_0204d400(a - 0x90);
}
