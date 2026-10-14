/*
 * The contact manager (a singleton: data_020f53ac): a list of actors that
 * can touch each other. Each pair whose shapes meet (vtable +0x24) is told
 * about the other (vtable +0x20), and an actor can poke the ones in front
 * of it within a distance and an angle.
 * ARM9 main, 0x02054468 - 0x020548c4 (13 functions).
 *
 * The manager is {vtable data_020c08dc, list (vtable data_020c08f4, base
 * data_020c047c: {vtable, data, size, capacity})}; the singleton base has
 * vtable data_020c0910. This-adjusting entry points subtract 4.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c047c[], data_020c08dc[], data_020c08f4[], data_020c0910[];   /* vtables */
extern const fx32 data_020c0878;     /* the height difference counted in distances */
extern void *data_020f53ac;          /* the manager */
extern const fx32 g_FxPi, g_FxTwoPi;

u32 func_02038b28(void *list);
void *func_02038b1c(void *list, u32 i);
void func_02019838(u8 *mgr);
s32 func_0208f30c(s32 x, s32 y);
u8 *func_02050d38(u8 *list);
void func_0207fe44(void *p);

void func_02054658(u8 *self);
u8 *func_02054770(u8 *self);
u8 *func_0205472c(u8 *self);
void func_02054804(u8 *self, void *p);
void func_02054820(u8 *self);

#define COUNT(self) ((s32)func_02038b28((self) + 4))
#define AT(self, i) ((u8 *)func_02038b1c((self) + 4, i))
#define CALL(o, off) (*(s32 (**)(u8 *, u8 *))(*(u8 **)(o) + (off)))

/* 0x02054468: empty the list
 * @difftest ptr:0x14:4 cases=3 */
void func_02054468(u8 *self)
{
    func_02019838(self + 4);
}

/* an angle into (lo, hi] */
static s32 Wrap(s32 a, s32 lo, s32 hi, s32 turn)
{
    while (a < lo)
        a += turn;
    while (a > hi)
        a -= turn;
    return a;
}

/* 0x02054478: poke the actors within `dist` (squared) of `actor` in
 * direction `dir` (within `spread`)
 * @difftest $T=zero:0x28 @$T+0x20:32=0x02043814 $E=zero:0x48 @$E:32=$T @$E+0x28:32=int:-0x8000:0x8000 @$E+0x2c:32=int:-0x8000:0x8000 $F=zero:0x48 @$F:32=$T @$F+0x28:32=int:-0x8000:0x8000 @$F+0x2c:32=int:-0x8000:0x8000 $D=zero:8 @$D:32=$E @$D+4:32=$F $S=zero:0x14 @$S+8:32=$D @$S+0xc:32=int:0:3 $S $F int:-0x8000:0x8000 int:0:0x10000 int:0:0x4000 cases=40 */
void func_02054478(u8 *self, u8 *actor, s32 dir, s32 dist, s32 spread)
{
    s32 i;

    for (i = 0; i < COUNT(self); i++) {
        u8 *o = AT(self, i);
        s32 dz, dx, c, a, b, d;

        if (o == actor)
            continue;
        dz = S32_AT(o, 0x2c) - S32_AT(actor, 0x2c);
        dx = S32_AT(o, 0x28) - S32_AT(actor, 0x28);
        c = data_020c0878;
        if (FxMul(dz, dz) + (FxMul(dx, dx) + FxMul(c, c)) > dist)
            continue;
        a = Wrap(func_0208f30c(dx, dz), g_FxZero, g_FxTwoPi, g_FxTwoPi);
        b = Wrap(dir, g_FxZero, g_FxTwoPi, g_FxTwoPi);
        d = Wrap(a - b, -g_FxPi, g_FxPi, g_FxTwoPi);
        if (d < 0)
            d = -d;
        if (d >= spread)
            continue;
        CALL(o, 0x20)(o, actor);
    }
}

/* 0x02054658: tell each pair of actors that touch about each other
 * @difftest $T=zero:0x28 @$T+0x20:32=0x02043814 @$T+0x24:32=0x020343a8 $E=zero:0x48 @$E:32=$T $F=zero:0x48 @$F:32=$T $D=zero:0xc @$D:32=$E @$D+4:32=$F @$D+8:32=$E $S=zero:0x14 @$S+8:32=$D @$S+0xc:32=int:0:4 $S cases=8 */
void func_02054658(u8 *self)
{
    s32 i, j;

    for (i = 0; i < COUNT(self) - 1; i++) {
        u8 *a = AT(self, i);

        for (j = i + 1; j < COUNT(self); j++) {
            u8 *b = AT(self, j);

            if (CALL(a, 0x24)(a, b) != 0) {
                CALL(a, 0x20)(a, b);
                CALL(b, 0x20)(b, a);
            }
        }
    }
}

/* 0x0205472c: the manager's deleting destructor
 * @difftest zero:0x14 stub=0x02006acc,0x0207fe44 cases=3 */
u8 *func_0205472c(u8 *self)
{
    PTR_AT(self, 0) = data_020c08dc;
    PTR_AT(self, 4) = data_020c08f4;
    func_02050d38(self + 4);
    func_02054820(self);
    func_0207fe44(self);
    return self;
}

/* 0x02054770: the manager's destructor
 * @difftest zero:0x14 stub=0x02006acc cases=3 */
u8 *func_02054770(u8 *self)
{
    PTR_AT(self, 0) = data_020c08dc;
    PTR_AT(self, 4) = data_020c08f4;
    func_02050d38(self + 4);
    func_02054820(self);
    return self;
}

/* 0x020547ac: the manager's constructor
 * @difftest ptr:0x14:4 cases=3 */
u8 *func_020547ac(u8 *self)
{
    u8 *l = self + 4;

    func_02054804(self, self);
    PTR_AT(l, 0) = data_020c047c;
    U32_AT(l, 4) = 0;
    U32_AT(l, 8) = 0;
    U32_AT(l, 0xc) = 0;
    PTR_AT(self, 0) = data_020c08dc;
    PTR_AT(self, 4) = data_020c08f4;
    return self;
}

/* 0x02054804: the singleton's constructor: `p` is the instance
 * @difftest ptr:4:4 u32 cases=3 */
void func_02054804(u8 *self, void *p)
{
    PTR_AT(self, 0) = data_020c0910;
    data_020f53ac = p;
}

/* 0x02054820: the singleton's destructor
 * @difftest ptr:4:4 cases=3 */
void func_02054820(u8 *self)
{
    PTR_AT(self, 0) = data_020c0910;
    data_020f53ac = NULL;
}

/* 0x02054840: the singleton's deleting destructor
 * @difftest ptr:4:4 stub=0x0207fe44 cases=3 */
u8 *func_02054840(u8 *self)
{
    PTR_AT(self, 0) = data_020c0910;
    data_020f53ac = NULL;
    func_0207fe44(self);
    return self;
}

/* 0x02054874: the singleton's destructor (another copy)
 * @difftest ptr:4:4 cases=3 */
void func_02054874(u8 *self)
{
    PTR_AT(self, 0) = data_020c0910;
    data_020f53ac = NULL;
}

/* The manager's this-adjusting entry points. */

/* 0x02054894
 * @difftest $S=zero:0x14 $S+4 stub=0x02006acc cases=3 */
u8 *func_02054894(u8 *l)
{
    return func_02054770(l - 4);
}

/* 0x020548a4
 * @difftest $S=zero:0x14 $S+4 cases=3 */
void func_020548a4(u8 *l)
{
    func_02054658(l - 4);
}

/* 0x020548b4
 * @difftest $S=zero:0x14 $S+4 stub=0x02006acc,0x0207fe44 cases=3 */
u8 *func_020548b4(u8 *l)
{
    return func_0205472c(l - 4);
}
