/*
 * Companion objects: actors of class 0x13 made at another actor's place
 * (kinds 0x8a, 0x9d and 0x9e; only one 0x9d and one 0x9e at a time,
 * data_020ebce0 / data_020ebcdc). They follow their animation and vanish
 * when it ends; actors of classes 4, 5, 6 and 0xc touching them get hold
 * of them.
 * ARM9 main, 0x02049ccc - 0x0204a1f4 (4 functions).
 *
 * Actor callbacks: +0x94 per frame (func_02049ccc), +0x98 removal
 * (func_02049e48), +0x9c collision (func_02049d48).
 *
 * Testing: with fake actors; the model animation stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */
extern s8 data_020ebcdc, data_020ebce0;

void func_02053460(void *obj, u32 dt);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
u8 *func_02005d5c(s32 type);
void func_02003c74(void *actor, s32 x, s32 y);
s32 func_0201a0a0(s32 *xz, void *obj);
void func_020645e4(void *look, u32 v);

void func_02049ccc(u8 *self);
void func_02049d48(u8 *self, u8 *other);
void func_02049e48(u8 *self);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define KIND(a) (data_020c3b0c + S32_AT(a, 8) * 0x48)
#define LINKS(a) ((u8 *)PTR_AT(PTR_AT(a, 0x44), 0xac))

/* the actor's appearance (its method +0x38) */
static u8 *Look(u8 *self)
{
    u8 *(*m)(void *, void *) = VCALL(self, 0x38, u8 *(*)(void *, void *));

    return m(self, (void *)m);
}

/* 0x02049ccc: per-frame update (vanish when the animation ends)
 * @difftest $O=zero:0x200 @$O+0x94:8=pick:0,1 $S=zero:0x120 @$S+0x44:32=$O @$S+0x38:32=pick:0,0x2000 $S stub=0x02053460 cases=10 */
void func_02049ccc(u8 *self)
{
    u8 *o = PTR_AT(self, 0x44);

    func_02053460(o, func_020822c0(func_020062d0()));
    if ((s8)BITS(U32_AT(self, 0x38), 13, 1) == 0) {
        U32_AT(self, 0x110) |= 8;
        return;
    }
    if (U8_AT(PTR_AT(self, 0x44), 0x94) == 0)
        U32_AT(self, 0x110) |= 8;
}

/* 0x02049d48: touched
 * @difftest $T=zero:0x40 @$T+0x38:32=0x020343a8 $S=zero:0x120 @$S+0x110:32=pick:0,0,8 @$S+0x24:32=int:-0x100000:0x100000 $O=zero:0x60 @$O+0:32=$T @$O+0xc:32=pick:0,1,4,5,6,0xc @$O+0x24:32=int:-0x100000:0x100000 $S $O cases=100 */
void func_02049d48(u8 *self, u8 *other)
{
    s32 c, d;

    if ((s8)BITS(U32_AT(self, 0x110), 3, 1) != 0)
        return;
    c = S32_AT(other, 0xc);
    if (c == 4 || c == 6 || c == 0xc || c == 5) {
        d = (S32_AT(other, 0x24) - S32_AT(self, 0x24)) >> 4;
        if (d < 0)
            d = -d;
        if (d >= 0xc000)
            return;
        if (c == 4)
            U32_AT(self, 0x38) |= 0x400;
        U32_AT(self, 0x38) |= 8;
        U32_AT(other, 0x38) |= 8;
        U32_AT(other, 0x38) |= 0x400;
        PTR_AT(Look(other), 8) = self;
        return;
    }
    if (c == 0)
        return;
    d = (S32_AT(other, 0x24) - S32_AT(self, 0x24)) >> 4;
    if (d < 0)
        d = -d;
    if (d < 0xc000)
        U32_AT(self, 0x38) |= 8;
}

/* 0x02049e48: the removal callback
 * @difftest $S=zero:0x120 @$S+8:32=pick:0x9d,0x9e,3 @020ebce0:8=pick:0,1 @020ebcdc:8=pick:0,1 $S cases=12 */
void func_02049e48(u8 *self)
{
    if (S32_AT(self, 8) == 0x9d && data_020ebce0 != 0)
        data_020ebce0 = 0;
    if (S32_AT(self, 8) == 0x9e && data_020ebcdc != 0)
        data_020ebcdc = 0;
}

/* 0x02049e94: make a companion of actor kind `kind` at `owner`
 * @difftest pick:0x9d,0x9e,0x8a,3 $O=zero:0x120 @$O+0x54:32=int:0:0x100 @$O+0x58:32=int:0:0x100 $O @020ebce0:8=pick:0,1 @020ebcdc:8=pick:0,1 stub=0x02053648,0x02003c74,0x020645e4 cases=12 */
u8 *func_02049e94(u32 kind, u8 *owner)
{
    u8 *a, *o;
    s32 t[2], w, d, sc;

    if (kind == 0x9d) {
        if (data_020ebce0 != 0)
            return NULL;
        data_020ebce0 = 1;
    } else if (kind == 0x9e) {
        if (data_020ebcdc != 0)
            return NULL;
        data_020ebcdc = 1;
    }
    a = func_02005d5c(0x13);
    S32_AT(a, 0xc) = 0x13;
    S32_AT(a, 8) = kind;
    U32_AT(a, 4) &= ~8u;
    U32_AT(a, 0x38) &= ~0x40000u;
    U32_AT(a, 0x110) |= 0x100;
    U32_AT(a, 0x38) |= 4;
    U32_AT(a, 0x38) |= 0x20;
    U32_AT(a, 0x38) |= 0x40;
    PTR_AT(a, 0x98) = (void *)func_02049e48;
    PTR_AT(a, 0x94) = (void *)func_02049ccc;
    PTR_AT(a, 0x9c) = (void *)func_02049d48;
    func_02053648(PTR_AT(a, 0x44), S32_AT(a, 8), a, 0, 0);
    S32_AT(PTR_AT(a, 0x44), 0xa4) = S32_AT(KIND(a), 0x34);
    o = PTR_AT(a, 0x44);
    U32_AT(o, 0x18c) = (U32_AT(o, 0x18c) & ~1u) | ((s8)S32_AT(KIND(a), 0x30) & 1);
    w = S32_AT(KIND(a), 0x3c);
    d = S32_AT(KIND(a), 0x38);
    S32_AT(a, 0x10) = -w;
    S32_AT(a, 0x14) = -d;
    S32_AT(a, 0x18) = w;
    S32_AT(a, 0x1c) = d;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~4u) | ((S32_AT(KIND(a), 0x40) > 0) << 2);
    sc = S32_AT(KIND(a), 0x2c);
    o = PTR_AT(a, 0x44);
    S32_AT(o, 0x1c) = sc;
    S32_AT(o, 0x20) = sc;
    S32_AT(o, 0x24) = sc;
    o[0x8c] = 1;
    func_02003c74(a, S32_AT(owner, 0x54), S32_AT(owner, 0x58));
    S32_AT(a, 0x30) = S32_AT(a, 0x28);
    S32_AT(a, 0x34) = S32_AT(a, 0x2c);
    t[0] = S32_AT(a, 0x54);
    t[1] = S32_AT(a, 0x58);
    S32_AT(a, 0x24) = func_0201a0a0(t, a);
    S32_AT(a, 0x6c) = S32_AT(a, 0x24);
    S32_AT(a, 0x64) = S32_AT(a, 0x24);
    S32_AT(a, 0x50) = 0x2000;
    PTR_AT(LINKS(a), 4) = HERO();
    o = PTR_AT(a, 0x44);
    U32_AT(o, 0x18c) |= 2;
    o = PTR_AT(a, 0x44);
    S32_AT(o, 4) = 0;
    S32_AT(o, 8) = 0;
    S32_AT(o, 0xc) = 0;
    o[0x8c] = 1;
    S32_AT(LINKS(a), 0xc) = S32_AT(KIND(a), 0x20);
    if (kind == 0x9d)
        func_020645e4(Look(a), 5);
    else if (kind == 0x9e)
        func_020645e4(Look(a), 6);
    else if (kind == 0x8a)
        func_020645e4(Look(a), 4);
    return a;
}
