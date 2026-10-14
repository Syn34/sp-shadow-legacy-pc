/*
 * Counting scenery: animated scenery (Scenery.c) that shows its count
 * (+0x196, saved with the game) as its animation frame; when the count
 * runs out it shows message +0x194 and fades away.
 * ARM9 main, 0x0204c148 - 0x0204c528 (16 functions).
 *
 * Vtables data_020bff5c (model) / data_020bff80 (actor at +0x90); +0x18c
 * the time it was made plus func_02082290(timer, 1).
 *
 * Testing: with fake objects; the base class parts stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bff5c[], data_020bff80[];   /* vtables */

void func_020513d0(void *stream, const void *buf, u32 n);
void func_02051464(void *stream, void *buf, u32 n);
void func_0205ed6c(void *anim, s32 frame);
void func_0204b3e8(u8 *self);
void func_0204b4d4(u8 *self, s32 dt);
void *func_02009394(void);
void func_0205f6e4(void *obj, u32 v);
u8 *func_0204b90c(u8 *self);
u8 *func_0204ba68(u8 *self, u32 a, u32 b, s32 *pos);
void func_0207fe44(void *p);
void *func_0207fe74(u32 size);
u8 *func_02008994(s32 i, s32 k);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
s32 func_0201a0a0(s32 *xz, void *obj);

void func_0204c148(u8 *self, u8 *stream);
void func_0204c1b4(u8 *self);
void func_0204c1c0(u8 *self);
void func_0204c1dc(u8 *self, u32 v);
void func_0204c1e8(u8 *self, u32 v);
void func_0204c218(u8 *self, s32 dt);
u8 *func_0204c278(u8 *self);
u8 *func_0204c2b0(u8 *self);
u8 *func_0204c2e0(u8 *self, u32 a, u32 b, s32 *pos);

#define ACTOR(self) ((self) != NULL ? (self) + 0x90 : NULL)

/* 0x0204c148: save (or load) the count
 * @difftest zero:0x198 0x020ebbc8 @020ebbe4:32=pick:0,1 stub=0x020513d0,0x02051464 cases=4 */
void func_0204c148(u8 *self, u8 *stream)
{
    if (S32_AT(stream, 0x1c) == 0) {
        func_020513d0(stream, self + 0x196, 1);
        return;
    }
    func_02051464(stream, self + 0x196, 1);
}

/* 0x0204c1b4: count one
 * @difftest $S=zero:0x198 @$S+0x196:8=u8 $S cases=3 */
void func_0204c1b4(u8 *self)
{
    func_0204c1c0(self);
}

/* 0x0204c1c0: count one
 * @difftest $S=zero:0x198 @$S+0x196:8=u8 $S cases=3 */
void func_0204c1c0(u8 *self)
{
    self[0x196]--;
}

/* 0x0204c1dc: set the message
 * @difftest zero:0x198 u16 cases=3 */
void func_0204c1dc(u8 *self, u32 v)
{
    U16_AT(self, 0x194) = v;
}

/* 0x0204c1e8: set the count
 * @difftest zero:0x198 u8 cases=3 */
void func_0204c1e8(u8 *self, u32 v)
{
    self[0x196] = v;
}

/* 0x0204c1f0: draw at the count's frame
 * @difftest $T=zero:0x10 @$T+4:32=0x020343a8 $S=zero:0x198 @$S+0xdc:32=$T @$S+0x196:8=u8 $S stub=0x0205ed6c,0x02080e84 cases=3 */
void func_0204c1f0(u8 *self)
{
    func_0205ed6c(self + 0xdc, (u32)self[0x196] << 12);
    func_0204b3e8(self);
}

/* 0x0204c218: per-frame update
 * @difftest $S=zero:0x198 @$S+0x180:32=pick:1,1,3 @$S+0x196:8=pick:0,0,2 @$S+0x194:16=7 $S int:0:4 stub=0x0205f6e4,0x0204b4d4 cases=10 */
void func_0204c218(u8 *self, s32 dt)
{
    if (S32_AT(self, 0x180) == 1 && self[0x196] == 0) {
        S32_AT(self, 0x180) = 2;
        func_0205f6e4(func_02009394(), S16_AT(self, 0x194));
    }
    func_0204b4d4(self, dt);
}

/* 0x0204c278: the deleting destructor
 * @difftest zero:0x198 stub=0x0204b90c,0x0207fe44 cases=3 */
u8 *func_0204c278(u8 *self)
{
    PTR_AT(self, 0) = data_020bff5c;
    PTR_AT(self, 0x90) = data_020bff80;
    func_0204b90c(self);
    func_0207fe44(self);
    return self;
}

/* 0x0204c2b0: the destructor
 * @difftest zero:0x198 stub=0x0204b90c cases=3 */
u8 *func_0204c2b0(u8 *self)
{
    PTR_AT(self, 0) = data_020bff5c;
    PTR_AT(self, 0x90) = data_020bff80;
    func_0204b90c(self);
    return self;
}

/* 0x0204c2e0: the constructor (animations `a` / `b`, at `pos`)
 * @difftest zero:0x198 u32 u32 ptr:8:4 stub=0x0204ba68 cases=3 */
u8 *func_0204c2e0(u8 *self, u32 a, u32 b, s32 *pos)
{
    u64 t, d;

    func_0204ba68(self, a, b, pos);
    PTR_AT(self, 0) = data_020bff5c;
    PTR_AT(self, 0x90) = data_020bff80;
    t = func_020822c8(func_020062d0());
    d = func_02082290(func_020062d0(), 1);
    Put64(self + 0x18c, t + d);
    return self;
}

/* 0x0204c33c: create counting scenery `id` (of map object list `k`)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:4 int:0:2 stub=0x0207fe74:0 cases=5 */
u8 *func_0204c33c(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    s32 v[2], t[2], u[2];
    u32 var;
    u8 *p = NULL;

    v[0] = S16_AT(def, 4);
    v[1] = S16_AT(def, 6);
    func_02003b08(t, v, 0xc);
    v[0] = t[0];
    v[1] = t[1];
    var = def[0xd];
    if (var == 0) {
        p = func_0207fe74(0x198);
        if (p != NULL)
            p = func_0204c2e0(p, 0x81a, 0x81b, v);
    } else if (var == 1) {
        p = func_0207fe74(0x198);
        if (p != NULL)
            p = func_0204c2e0(p, 0x80f, 0x810, v);
    } else if (var == 2) {
        p = func_0207fe74(0x198);
        if (p != NULL)
            p = func_0204c2e0(p, 0x818, 0x819, v);
    }
    func_02003b08(t, v, 4);
    v[0] = t[0];
    v[1] = t[1];
    u[0] = t[0];
    u[1] = t[1];
    S32_AT(p, 0xb4) = func_0201a0a0(u, NULL);
    U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x20u) | ((def[0xa] & 1) << 5);
    U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x40u) | ((def[0xb] & 1) << 6);
    U32_AT(p, 0x184) = var;
    func_0204c1e8(p, def[8]);
    func_0204c1dc(p, def[9]);
    S32_AT(p, 0x9c) = 0xe;
    return ACTOR(p);
}

/* The actor's this-adjusting entry points (the object is 0x90 before it). */

/* 0x0204c4d8
 * @difftest $S=zero:0x198 $S+0x90 stub=0x0204b90c,0x0207fe44 cases=3 */
u8 *func_0204c4d8(u8 *a)
{
    return func_0204c278(a - 0x90);
}

/* 0x0204c4e8
 * @difftest $S=zero:0x198 $S+0x90 0x020ebbc8 @020ebbe4:32=pick:0,1 stub=0x020513d0,0x02051464 cases=3 */
void func_0204c4e8(u8 *a, u8 *stream)
{
    func_0204c148(a - 0x90, stream);
}

/* 0x0204c4f8
 * @difftest $S=zero:0x198 @$S+0x196:8=u8 $S+0x90 cases=3 */
void func_0204c4f8(u8 *a)
{
    func_0204c1b4(a - 0x90);
}

/* 0x0204c508
 * @difftest $S=zero:0x198 @$S+0x180:32=3 $S+0x90 int:0:4 stub=0x0204b4d4 cases=3 */
void func_0204c508(u8 *a, s32 dt)
{
    func_0204c218(a - 0x90, dt);
}

/* 0x0204c518
 * @difftest $S=zero:0x198 $S+0x90 stub=0x0204b90c cases=3 */
u8 *func_0204c518(u8 *a)
{
    return func_0204c2b0(a - 0x90);
}
