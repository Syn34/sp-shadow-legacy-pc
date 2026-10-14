/*
 * Bystanders: characters of actor class 7 placed on the map that face the
 * hero with a random turn. Kind 0 can be talked to with A (in the hero's
 * first form); kinds 1 and 2 only notice him when he comes close. Map
 * objects of kind 2 are not made.
 * ARM9 main, 0x0204c528 - 0x0204cb4c (9 functions).
 *
 * Actor callbacks: +0x94 per frame (func_0204c528), +0x98 removal
 * (func_0204c80c), +0x9c collision (func_0204c6d8 / func_0204c604 /
 * func_0204c600).
 *
 * Testing: with fake actors; the hint, the model set-up and the card
 * stubbed where they need real objects.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcb94[];           /* RTTI: an actor */
extern const fx32 data_020bffc8;
extern const char data_020bffcc[], data_020bfff8[], data_020c001c[];   /* debug messages */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */
extern s32 data_020ebd28;            /* noticing distance (squared) */

void func_02053460(void *obj, u32 dt);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
u32 func_020186ec(void);
void *func_020a6efc(void *p, void *from, void *to, s32 v);
u8 *func_02011f34(void);
void *func_02011f44(void);
void func_02072418(void *obj, u32 a, u32 fbits);
s32 func_02081d34(void *pad, u32 key);
u8 *func_02008994(s32 i, s32 k);
u8 *func_02005d5c(s32 type);
s32 func_0200820c(u32 id, s32 kind);
void func_02003c74(void *actor, s32 x, s32 y);
s32 func_0201a0a0(s32 *xz, void *obj);
void *func_02016f24(void *self);
void func_0201751c(s32 *out, const u8 *cam);
s64 func_020a4c50(s64 a, s64 b);     /* signed 64-bit divide */

void func_0204c528(u8 *self);
s32 func_0204c54c(u8 *self, u8 *other);
void func_0204c600(void);
void func_0204c604(u8 *self, u8 *other);
void func_0204c6d8(u8 *self, u8 *other);
void func_0204c7a4(u8 *self, u32 kind);
void func_0204c80c(void);

#define KIND(a) (data_020c3b0c + S32_AT(a, 8) * 0x48)

/* 0x0204c528: per-frame update (the model's animation)
 * @difftest $O=zero:0x200 $S=zero:0x120 @$S+0x44:32=$O $S stub=0x02053460 cases=3 */
void func_0204c528(u8 *self)
{
    u8 *o = PTR_AT(self, 0x44);

    func_02053460(o, func_020822c0(func_020062d0()));
}

/* 0x0204c54c: is `other` close enough to be noticed?
 * @difftest $S=zero:0x120 @$S+0x54:32=int:-0x100000:0x100000 @$S+0x58:32=int:-0x100000:0x100000 $O=zero:0x120 @$O+0x54:32=int:-0x100000:0x100000 @$O+0x58:32=int:-0x100000:0x100000 @020ebd28:32=int:0:0x4000000 $S $O cases=60
 * @difftest ptr:0x120 0 cases=2 */
s32 func_0204c54c(u8 *self, u8 *other)
{
    s32 dz, dx, dy;

    if (other == NULL)
        return 0;
    dz = (S32_AT(self, 0x58) - S32_AT(other, 0x58)) >> 4;
    dx = (S32_AT(other, 0x54) - S32_AT(self, 0x54)) >> 4;
    dy = data_020bffc8;
    return FxMul(dz, dz) + (FxMul(dx, dx) + FxMul(dy, dy)) <= data_020ebd28;
}

/* 0x0204c600: the collision callback of other kinds
 * @difftest cases=3 */
void func_0204c600(void)
{
}

/* 0x0204c604: touched: notice the hero when he is close
 * @difftest $S=zero:0x120 @$S+0x24:32=int:-0x80000:0x80000 @$S+0x38:32=pick:0,8 $O=zero:0x120 @$O+0xc:32=pick:0,0,1 @$O+8:32=pick:1,1,2 @$O+0x24:32=int:-0x80000:0x80000 @$S+0x54:32=int:-0x100000:0x100000 @$O+0x54:32=int:-0x100000:0x100000 @020ebd28:32=int:0:0x4000000 $S $O stub=0x020186ec:0,0x020a6efc cases=60
 * @difftest $S=zero:0x120 $O=zero:0x120 @$O+8:32=1 $S $O stub=0x020186ec:1 cases=3 */
void func_0204c604(u8 *self, u8 *other)
{
    s32 d;

    if (S32_AT(other, 0xc) != 0)
        return;
    if (S32_AT(other, 8) != 1)
        return;
    if (func_020186ec() != 0)
        return;
    d = (S32_AT(other, 0x24) - S32_AT(self, 0x24)) >> 4;
    if (d < 0)
        d = -d;
    if (d >= 0x4000)
        return;
    if (other != NULL)
        other = func_020a6efc(other, data_020bcb88, data_020bcb94, -1);
    if (func_0204c54c(self, other) != 0)
        U32_AT(self, 0x38) |= 8;
    else
        U32_AT(self, 0x38) &= ~8u;
}

/* 0x0204c6d8: touched: the hero can talk with A
 * @difftest $S=zero:0x120 @$S+0x24:32=int:-0x80000:0x80000 $O=zero:0x120 @$O+0xc:32=pick:0,0,1 @$O+8:32=pick:1,1,2 @$O+0x24:32=int:-0x80000:0x80000 $K=zero:0x80 @020f6f30:32=$K @$K+0x5c:32=pick:0,1 $S $O stub=0x020186ec:0,0x02072418 cases=40
 * @difftest $S=zero:0x120 $O=zero:0x120 @$O+8:32=1 $S $O stub=0x020186ec:1 cases=3 */
void func_0204c6d8(u8 *self, u8 *other)
{
    s32 d;

    if (S32_AT(other, 0xc) != 0)
        return;
    if (S32_AT(other, 8) != 1)
        return;
    if (func_020186ec() != 0)
        return;
    d = (S32_AT(other, 0x24) - S32_AT(self, 0x24)) >> 4;
    if (d < 0)
        d = -d;
    if (d >= 0x4000)
        return;
    func_02072418(func_02011f34(), 4, 0x45000000);
    U32_AT(self, 0x38) |= 8;
    U8_AT(data_020deebc, 0x1f) = 1;
    if (func_02081d34(func_02011f44(), 0xa) != 0)
        U32_AT(self, 0x38) |= 0x400;
}

/* 0x0204c7a4: give the actor its model (actor kind `kind`)
 * @difftest $O=zero:0x200 $S=zero:0x60 @$S+0x44:32=$O $S int:0:3 stub=0x02053648 cases=5 */
void func_0204c7a4(u8 *self, u32 kind)
{
    u8 *o;

    func_02053648(PTR_AT(self, 0x44), kind, self, 0, 0);
    func_02004490(data_020bffcc);
    o = PTR_AT(self, 0x44);
    U32_AT(o, 0x18c) |= 2;
    o = PTR_AT(self, 0x44);
    S32_AT(o, 4) = 0;
    S32_AT(o, 8) = 0;
    S32_AT(o, 0xc) = 0;
    o[0x8c] = 1;
}

/* 0x0204c80c: the removal callback
 * @difftest cases=3 */
void func_0204c80c(void)
{
}

/* 0x0204c810: create bystander `id` (of map object list `k`)
 * @difftest int:0:4 int:0:2 stub=0x02053648,0x02003c74 cases=5 */
u8 *func_0204c810(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    u32 v = def[0xa];
    u8 *a;
    s32 t[2], w, d;

    if (v == 2)
        return NULL;
    a = func_02005d5c(S32_AT(def, 0));
    S32_AT(a, 0xc) = 7;
    if (v == 2)
        S32_AT(a, 8) = func_0200820c(1, 6);
    else
        S32_AT(a, 8) = func_0200820c(v, 6);
    U32_AT(a, 4) |= 8;
    U32_AT(a, 0x38) |= 0x40000;
    U32_AT(a, 0x110) &= ~0x100u;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x20u) | ((def[0xd] & 1) << 5);
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x40u) | ((def[0xe] & 1) << 6);
    PTR_AT(a, 0x98) = (void *)func_0204c80c;
    PTR_AT(a, 0x94) = (void *)func_0204c528;
    if (v == 0)
        PTR_AT(a, 0x9c) = (void *)func_0204c6d8;
    else if ((u16)(v - 1) <= 1)
        PTR_AT(a, 0x9c) = (void *)func_0204c604;
    else
        PTR_AT(a, 0x9c) = (void *)func_0204c600;
    w = S32_AT(KIND(a), 0x3c);
    d = S32_AT(KIND(a), 0x38);
    S32_AT(a, 0x10) = -w;
    S32_AT(a, 0x14) = -d;
    S32_AT(a, 0x18) = w;
    S32_AT(a, 0x1c) = d;
    U32_AT(a, 0x38) |= 4;
    func_02003c74(a, SHL(S16_AT(def, 4), 16), SHL(S16_AT(def, 6), 16));
    t[0] = S32_AT(a, 0x54);
    t[1] = S32_AT(a, 0x58);
    S32_AT(a, 0x24) = func_0201a0a0(t, a);
    S32_AT(a, 0x6c) = S32_AT(a, 0x24);
    S32_AT(a, 0x50) = 0;
    a[0x10d] = 0;
    func_0204c7a4(a, S32_AT(a, 8));
    func_02004490(data_020bfff8);
    if (PTR_AT(a, 0x44) != NULL) {
        s32 r[3], deg, sc;
        float f;
        u8 *o;

        func_0201751c(r, func_02016f24(data_020deebc));
        f = (float)(s32)((u32)U16_AT(def, 8) << 12);
        deg = (s32)(U16_AT(def, 8) != 0 ? 0.5f + f : f - 0.5f);
        r[1] += (s32)func_020a4c50((s64)deg * 0x3244, 0xb4000);
        o = PTR_AT(a, 0x44);
        S32_AT(o, 0x10) = r[0];
        S32_AT(o, 0x14) = r[1];
        S32_AT(o, 0x18) = r[2];
        o[0x8c] = 1;
        S32_AT(PTR_AT(a, 0x44), 0xa4) = S32_AT(KIND(a), 0x34);
        o = PTR_AT(a, 0x44);
        U32_AT(o, 0x18c) = (U32_AT(o, 0x18c) & ~1u) | ((s8)S32_AT(KIND(a), 0x30) & 1);
        sc = S32_AT(KIND(a), 0x2c);
        o = PTR_AT(a, 0x44);
        S32_AT(o, 0x1c) = sc;
        S32_AT(o, 0x20) = sc;
        S32_AT(o, 0x24) = sc;
        o[0x8c] = 1;
    }
    func_02004490(data_020c001c);
    return a;
}
