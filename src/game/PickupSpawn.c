/*
 * Making pickups: from map objects (func_02045dcc) or from item numbers
 * (func_02045ea8), and the names of the items. Actor kinds without a model
 * of their own (kind +8 == -1) become falling pickups (Pickup.c); others
 * are fixed actors of class 0xa that face the hero with a random turn and
 * use func_020044a0 when he comes close.
 * ARM9 main, 0x02045900 - 0x02046154 (11 functions).
 *
 * Testing: with fake actors; the actor and model set-up stubbed where it
 * needs real objects.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bf8a8[];           /* RTTI: the pickup */
extern const fx32 data_020bf974, data_020bf978, data_020bf97c, data_020bf980, data_020bf984;
extern const char data_020bf988[], data_020bf9b4[], data_020bf9d8[];   /* debug messages */
extern const char data_020bf9f4[];   /* no name */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */
extern s32 data_020ebca4;            /* pick-up distance (squared) */

int rand(void);
void func_02053460(void *obj, u32 dt);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
s32 func_02014260(void *item);
void func_020044a0(void *actor);
u8 *func_02005d5c(s32 type);
void func_02003c74(void *actor, s32 x, s32 y);
s32 func_0201a0a0(s32 *xz, void *obj);
void *func_02016f24(void *self);
void func_0201751c(s32 *out, const u8 *cam);
s64 func_020a4c50(s64 a, s64 b);     /* signed 64-bit divide */
u8 *func_02008994(s32 i, s32 k);
s32 func_0200820c(u32 id, s32 kind);
void *func_020a6efc(void *p, void *from, void *to, s32 v);
const char *func_02028a20(u32 id);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
void *func_0207fe74(u32 size);
u8 *func_0204553c(u8 *self, u32 kind, s32 *pos, u32 shrink);

void func_02045900(u8 *self);
void func_02045924(u8 *self, u8 *other);
void func_02045a14(u8 *self, u32 kind);
void func_02045fe4(void);
u8 *func_02045ed0(u32 id, s32 f5, s32 f6, s32 *pos, s8 shrink);
u8 *func_02045fe8(s32 id, s32 k);
u8 *func_02046070(u32 kind, s32 f5, s32 f6, s32 *pos, s8 shrink);

#define KIND(k) (data_020c3b0c + (k) * 0x48)

/* 0x02045900: per-frame update (the model's animation)
 * @difftest $O=zero:0x200 $S=zero:0x120 @$S+0x44:32=$O $S stub=0x02053460 cases=3 */
void func_02045900(u8 *self)
{
    u8 *o = PTR_AT(self, 0x44);

    func_02053460(o, func_020822c0(func_020062d0()));
}

/* 0x02045924: touched: use the item when the hero is close
 * @difftest $H=zero:0x60 @$H+0x54:32=int:-0x80000:0x80000 @$H+0x58:32=int:-0x80000:0x80000 @020deec0:32=$H $S=zero:0x120 @$S+0x54:32=int:-0x80000:0x80000 @$S+0x58:32=int:-0x80000:0x80000 $O=zero:0x60 @$O+0xc:32=pick:0,0,1 @020ebca4:32=int:0:0x4000000 $S $O stub=0x02014260:1,0x020044a0 cases=100
 * @difftest $H=zero:0x60 @020deec0:32=$H $S=zero:0x120 $O=zero:0x60 @020ebca4:32=0x100000 $S $O stub=0x02014260:0,0x020044a0 cases=5 */
void func_02045924(u8 *self, u8 *other)
{
    u8 *h;
    s32 dx, dy, dz;

    if (S32_AT(other, 0xc) != 0)
        return;
    h = (u8 *)PTR_AT(data_020deebc, 4);
    dy = data_020bf980;
    dz = (S32_AT(h, 0x58) - S32_AT(self, 0x58)) >> 4;
    dx = (S32_AT(h, 0x54) - S32_AT(self, 0x54)) >> 4;
    if (FxMul(dz, dz) + (FxMul(dx, dx) + FxMul(dy, dy)) > data_020ebca4)
        return;
    if (func_02014260(self) == 0)
        return;
    func_020044a0(self);
}

/* 0x02045a14: give the actor its model (actor kind `kind`)
 * @difftest $O=zero:0x200 $S=zero:0x60 @$S+0x44:32=$O $S int:0:3 stub=0x02053648 cases=5 */
void func_02045a14(u8 *self, u32 kind)
{
    u8 *o;

    func_02053648(PTR_AT(self, 0x44), kind, self, 0, 0);
    func_02004490(data_020bf988);
    o = PTR_AT(self, 0x44);
    U32_AT(o, 0x18c) |= 2;
    o = PTR_AT(self, 0x44);
    S32_AT(o, 4) = 0;
    S32_AT(o, 8) = 0;
    S32_AT(o, 0xc) = 0;
    o[0x8c] = 1;
}

/* a random number 0..90 */
static u32 Rand91(void)
{
    return ((u32)rand() & 0x7fff) % 0x5b;
}

/* a random turn, 0..90 degrees in fx32 radians */
static s32 RandomTurn(void)
{
    float f;
    s32 deg;

    if (Rand91() != 0)
        f = 0.5f + (float)(u32)(Rand91() << 12);
    else
        f = (float)(u32)(Rand91() << 12) - 0.5f;
    deg = (s32)f;
    return (s32)func_020a4c50((s64)deg * 0x3244, 0xb4000);
}

/* 0x02045a7c: create a fixed item actor (actor kind `kind`, type `type`)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:0x40 int:0:3 int:0:4 int:0:2 int:0:2 int:-0x1000000:0x1000000 int:-0x1000000:0x1000000 stub=0x02053648,0x02003c74 cases=5 */
u8 *func_02045a7c(u32 id, u32 kind, s32 type, s32 f5, s32 f6, s32 x, s32 y)
{
    u8 *a = func_02005d5c(type);
    s32 t[2];

    S32_AT(a, 0xc) = 0xa;
    S32_AT(a, 8) = kind;
    U32_AT(a, 4) |= 8;
    U32_AT(a, 0x38) |= 0x40000;
    U32_AT(a, 0x110) &= ~0x100u;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x20u) | ((f5 & 1) << 5);
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x40u) | (((s8)f6 & 1) << 6);
    PTR_AT(a, 0x98) = (void *)func_02045fe4;
    PTR_AT(a, 0x94) = (void *)func_02045900;
    PTR_AT(a, 0x9c) = (void *)func_02045924;
    S32_AT(a, 0x10) = data_020bf978;
    S32_AT(a, 0x14) = data_020bf984;
    S32_AT(a, 0x18) = data_020bf97c;
    S32_AT(a, 0x1c) = data_020bf974;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~4u) | ((S32_AT(KIND(kind), 0x40) > 0) << 2);
    func_02003c74(a, x, y);
    t[0] = S32_AT(a, 0x54);
    t[1] = S32_AT(a, 0x58);
    S32_AT(a, 0x24) = func_0201a0a0(t, a);
    S32_AT(a, 0x6c) = S32_AT(a, 0x24);
    S32_AT(a, 0x50) = 0;
    a[0x10d] = 0;
    func_02045a14(a, S32_AT(a, 8));
    func_02004490(data_020bf9b4);
    if (PTR_AT(a, 0x44) != NULL) {
        s32 r[3];
        u8 *o;
        s32 sc;

        func_0201751c(r, func_02016f24(data_020deebc));
        r[1] += RandomTurn();
        o = PTR_AT(a, 0x44);
        S32_AT(o, 0x10) = r[0];
        S32_AT(o, 0x14) = r[1];
        S32_AT(o, 0x18) = r[2];
        o[0x8c] = 1;
        S32_AT(PTR_AT(a, 0x44), 0xa4) = S32_AT(KIND(S32_AT(a, 8)), 0x34);
        o = PTR_AT(a, 0x44);
        U32_AT(o, 0x18c) = (U32_AT(o, 0x18c) & ~1u) | ((s8)S32_AT(KIND(S32_AT(a, 8)), 0x30) & 1);
        sc = S32_AT(KIND(S32_AT(a, 8)), 0x2c);
        o = PTR_AT(a, 0x44);
        S32_AT(o, 0x1c) = sc;
        S32_AT(o, 0x20) = sc;
        S32_AT(o, 0x24) = sc;
        o[0x8c] = 1;
    }
    func_02004490(data_020bf9d8);
    return a;
}

/* 0x02045dcc: create item `id` (of map object list `k`)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:4 int:0:2 stub=0x02053648,0x0204553c,0x0207fe74:0 cases=5 */
u8 *func_02045dcc(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    u32 kind16 = U16_AT(def, 8);
    s32 kk = func_0200820c(kind16, 3);

    if (S32_AT(KIND(kk), 4) == 0) {
        u8 *p = func_02045fe8(id, k);

        if (p != NULL)
            p = func_020a6efc(p, data_020bcb88, data_020bf8a8, -1);
        if (p != NULL)
            S32_AT(p, 0x9c) = 0xa;
        return p != NULL ? p + 0x90 : NULL;
    }
    return func_02045a7c(kind16, kk, S32_AT(def, 0), S8_AT(def, 0xc), S8_AT(def, 0xd),
                         SHL(S16_AT(def, 4), 16), SHL(S16_AT(def, 6), 16));
}

/* 0x02045ea8: create a pickup of item `item` at `pos`
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:0x40 int:0:2 int:0:2 zero:8 int:0:2 stub=0x0204553c cases=5 */
u8 *func_02045ea8(u32 item, s32 f5, s32 f6, s32 *pos, s8 shrink)
{
    u8 *p = func_02045ed0(item, f5, f6, pos, shrink);

    S32_AT(p, 0x108) = item;
    return p;
}

/* 0x02045ed0: create a pickup of item `id` at `pos`
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:0x40 int:0:2 int:0:2 zero:8 int:0:2 stub=0x0204553c cases=5 */
u8 *func_02045ed0(u32 id, s32 f5, s32 f6, s32 *pos, s8 shrink)
{
    u8 *p = func_02046070(func_0200820c(id, 3), f5, f6, pos, shrink);

    S32_AT(p, 0x9c) = 0xa;
    return p;
}

/* 0x02045f18: an item actor's name
 * @difftest $A=zero:0x120 @$A+0x40:16=-1 @$A+0x42:8=-1 @$A+0x108:32=pick:0,0xc,0xd,0x12,0x18,0x19,0x1a,0x30,0x39,0x3a,0x63,0x64 $A stub=0x020a6efc cases=30
 * @difftest $A=zero:0x120 @$A+0x40:16=int:0:3 @$A+0x42:8=int:0:2 $A cases=5 */
const char *func_02045f18(u8 *a)
{
    u32 id;

    if (S8_AT(a, 0x42) != -1 && S16_AT(a, 0x40) != -1) {
        id = U16_AT(func_02008994(S8_AT(a, 0x42), S16_AT(a, 0x40)), 8);
    } else {
        u8 *p = a;

        if (p != NULL)
            p = func_020a6efc(p, data_020bcb88, data_020bf8a8, -1);
        id = U32_AT(p, 0x108);
    }
    if (id >= 0xd && id <= 0x18)
        return func_02028a20(id + 0x6f);
    if (id >= 0x1a && id <= 0x39)
        return func_02028a20(id + 0x10);
    if (id != 0x63)
        return data_020bf9f4;
    return func_02028a20(0x4a);
}

/* 0x02045fe4: the removal callback
 * @difftest cases=3 */
void func_02045fe4(void)
{
}

/* 0x02045fe8: create a pickup from map object `id` (of list `k`)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:4 int:0:2 stub=0x0204553c,0x0207fe74:0 cases=5 */
u8 *func_02045fe8(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    s32 kk = func_0200820c(U16_AT(def, 8), 3);
    s32 v[2], t[2];
    u8 *p;

    v[0] = S16_AT(def, 4);
    v[1] = S16_AT(def, 6);
    func_02003b08(t, v, 0xc);
    v[0] = t[0];
    v[1] = t[1];
    p = func_02046070(kk, S8_AT(def, 0xc), S8_AT(def, 0xd), v, 0);
    return p != NULL ? p + 0x90 : NULL;
}

/* 0x02046070: create a pickup (actor kind `kind`) at `pos`
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:0x40 int:0:2 int:0:2 zero:8 int:0:2 stub=0x0204553c cases=10 */
u8 *func_02046070(u32 kind, s32 f5, s32 f6, s32 *pos, s8 shrink)
{
    u8 *p = NULL;
    s32 t[2], u[2];

    if (S32_AT(KIND(kind), 8) == -1) {
        u8 *q = func_0207fe74(0x134);

        if (q != NULL)
            p = func_0204553c(q, kind, pos, shrink);
    }
    func_02003b08(t, pos, 4);
    u[0] = t[0];
    u[1] = t[1];
    S32_AT(p, 0xb4) = func_0201a0a0(u, NULL);
    if (p != NULL) {
        U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x20u) | ((f5 & 1) << 5);
        U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x40u) | ((f6 & 1) << 6);
        U32_AT(p, 0xc8) &= ~0x80u;
    }
    return p;
}
