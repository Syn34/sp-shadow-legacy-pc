/*
 * Switch zones: map objects of actor class 0xb (model kind 0x34). The
 * hero standing in a zone's box while attacking (with counter 9 set)
 * raises a hero flag; when the switch state (func_020186ec) changes, the
 * zone shows one of two messages (+0x4c / +0x4d).
 * ARM9 main, 0x02044594 - 0x02044b44 (10 functions).
 *
 * Vtable data_020bf7f4; func_02044830 is the shared set-up of such
 * actors (list, model and size).
 *
 * Testing: with fake actors; the actor and model set-up and the message
 * system stubbed where they need real objects.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020b6cc4;
extern const s32 data_020bf7c4, data_020bf7c8, data_020bf7cc, data_020bf7d0;   /* the box */
extern u8 data_020bf7f4[];           /* the zone's vtable */
extern const char data_020bf840[];   /* debug message */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */

void func_020038d4(s32 *out, const s32 *in, s32 shift);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
s32 func_0204a868(u32 counter);
u8 *func_02011f34(void);
void func_02072418(void *obj, u32 a, u32 fbits);
u32 func_020186ec(void);
s32 func_020186cc(void);
void *func_02009394(void);
void func_0205f6e4(void *obj, u32 v);
void func_02044428(u8 *self);
void *func_020062b0(void);
void func_02017abc(void *list, void *item);
void func_020506a0(void *actor, s32 group, s32 index);
void func_020504b8(void *actor);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
void func_02008130(s32 group, s32 index);
void *func_020506c0(void *actor);
void func_0207fe44(void *p);
void func_02050890(void *actor, s32 *pos, s32 *box);
void *func_0207fe74(u32 size);
u8 *func_02008994(s32 i, s32 k);
s32 func_0201a0a0(s32 *xz, void *obj);

void func_02044594(u8 *self, u8 *other);
void func_02044830(u8 *self, u32 kind, s32 index, u32 b);
u8 *func_02044994(u8 *self, s32 *pos);

/* 0x02044594: the hero in the zone while attacking
 * @difftest $S=zero:0x60 @$S+0x10:32=int:-0x8000:0 @$S+0x14:32=int:-0x8000:0 @$S+0x18:32=int:0:0x8000 @$S+0x1c:32=int:0:0x8000 @$S+0x28:32=int:-0x100000:0x100000 @$S+0x2c:32=int:-0x100000:0x100000 $O=zero:0x60 @$O+0xc:32=pick:0,0,0,1 @$O+8:32=pick:1,1,1,2 @$O+0x38:32=pick:0x800000,0x800000,0 @$O+0x28:32=int:-0x100000:0x100000 @$O+0x2c:32=int:-0x100000:0x100000 $S $O stub=0x0204a868:1,0x02072418 cases=200
 * @difftest $S=zero:0x60 $O=zero:0x60 @$O+8:32=1 @$O+0x38:32=0x800000 $S $O stub=0x0204a868:0,0x02072418 cases=5 */
void func_02044594(u8 *self, u8 *other)
{
    s32 xz[2], p[2], q[2], b[4];
    s32 in1, in2, in3;

    if (S32_AT(other, 0xc) != 0)
        return;
    if (S32_AT(other, 8) != 1)
        return;
    if (func_0204a868(9) == 0)
        return;
    if ((s8)BITS(U32_AT(other, 0x38), 23, 1) == 0)
        return;
    b[0] = S32_AT(self, 0x10);
    b[1] = S32_AT(self, 0x14);
    b[2] = S32_AT(self, 0x18);
    b[3] = S32_AT(self, 0x1c);
    xz[0] = S32_AT(self, 0x28);
    xz[1] = S32_AT(self, 0x2c);
    func_020038d4(p, xz, 0xc);
    b[1] += p[0];
    b[3] += p[0];
    b[0] += p[1];
    b[2] += p[1];
    func_020038d4(q, (s32 *)(other + 0x28), 0xc);
    in1 = q[0] > b[1] && q[0] < b[3];
    in2 = in1 && q[1] < b[2];
    in3 = in2 && q[1] > b[0];
    if (in3 == 0)
        return;
    data_020deebc[0x20 / 4] |= 4;
    func_02072418(func_02011f34(), 6, 0x45000000);
}

/* 0x02044730: a zone touched
 * @difftest $S=zero:0x60 @$S+0x24:32=int:-0x80000:0x80000 $O=zero:0x60 @$O+0xc:32=1 @$O+0x24:32=int:-0x80000:0x80000 $S $O cases=20 */
void func_02044730(u8 *self, u8 *other)
{
    s32 d = (S32_AT(other, 0x24) - S32_AT(self, 0x24)) >> 4;

    if (d < 0)
        d = -d;
    if (d >= 0x4000)
        return;
    func_02044594(self, other);
}

/* 0x02044770: the model's method +0xc
 * @difftest $T=zero:0x20 @$T+0xc:32=0x020343a8 $O=zero:0x10 @$O+0:32=$T $S=zero:0x60 @$S+0x44:32=$O $S cases=3 */
void func_02044770(u8 *self)
{
    u8 *o = PTR_AT(self, 0x44);
    void (*m)(void *, void *) = VCALL(o, 0xc, void (*)(void *, void *));

    m(o, (void *)m);
}

/* 0x02044794: the model's method +8
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x10 @$O+0:32=$T $S=zero:0x60 @$S+0x44:32=$O $S cases=3 */
void func_02044794(u8 *self)
{
    u8 *o = PTR_AT(self, 0x44);
    void (*m)(void *, void *) = VCALL(o, 8, void (*)(void *, void *));

    m(o, (void *)m);
}

/* 0x020447b8: per-frame update (draw; a message when the switch changes)
 * @difftest $S=zero:0x60 @$S+0x4c:8=int:1:9 @$S+0x4d:8=int:1:9 @$S+0x4e:8=pick:0,1,2 $S stub=0x020186ec:1,0x020186cc:0,0x0205f6e4 cases=10
 * @difftest $S=zero:0x60 @$S+0x4e:8=pick:0,1 $S stub=0x020186ec:0,0x020186cc:1,0x0205f6e4 cases=5 */
void func_020447b8(u8 *self)
{
    if (BITS(U32_AT(self, 4), 3, 1) != 0)
        func_02044428(self);
    if (self[0x4e] == func_020186ec())
        return;
    if (func_020186cc() != 0)
        return;
    if (self[0x4e] == 1)
        func_0205f6e4(func_02009394(), self[0x4d]);
    else
        func_0205f6e4(func_02009394(), self[0x4c]);
    self[0x4e] = func_020186ec();
}

/* 0x02044830: set up an actor of model kind `kind` (object index `index`)
 * @difftest $O=zero:0x200 $S=zero:0x60 @$S+0x44:32=$O $S int:0:0x40 int:-2:0x100 int:0:4 stub=0x02017abc,0x020506a0,0x020504b8,0x02053648 cases=10 */
void func_02044830(u8 *self, u32 kind, s32 index, u32 b)
{
    u8 *o;
    s32 sc;

    U32_AT(self, 0x38) |= 4;
    func_02017abc((u8 *)func_020062b0() + 4, self);
    func_020506a0(self, (s8)kind, (s16)index);
    func_020504b8(self);
    func_02053648(PTR_AT(self, 0x44), kind, self, b, 0);
    o = PTR_AT(self, 0x44);
    S32_AT(o, 4) = 0;
    S32_AT(o, 8) = 0;
    S32_AT(o, 0xc) = 0;
    o[0x8c] = 1;
    o = PTR_AT(self, 0x44);
    sc = S32_AT(data_020c3b0c + kind * 0x48, 0x2c);
    S32_AT(o, 0x1c) = sc;
    S32_AT(o, 0x20) = sc;
    S32_AT(o, 0x24) = sc;
    o[0x8c] = 1;
    o = PTR_AT(self, 0x44);
    S32_AT(o, 0x10) = data_020b6cc4;
    S32_AT(o, 0x14) = g_FxZero;
    S32_AT(o, 0x18) = g_FxZero;
    o[0x8c] = 1;
}

/* 0x02044924: the zone's deleting destructor
 * @difftest $S=zero:0x60 @$S+0x40:16=int:0:0x10 @$S+0x42:8=int:0:3 $S stub=0x02008130,0x020506c0,0x0207fe44 cases=3 */
u8 *func_02044924(u8 *self)
{
    PTR_AT(self, 0) = data_020bf7f4;
    func_02008130(S8_AT(self, 0x42), S16_AT(self, 0x40));
    func_020506c0(self);
    func_0207fe44(self);
    return self;
}

/* 0x02044960: the zone's destructor
 * @difftest $S=zero:0x60 @$S+0x40:16=int:0:0x10 @$S+0x42:8=int:0:3 $S stub=0x02008130,0x020506c0 cases=3 */
u8 *func_02044960(u8 *self)
{
    PTR_AT(self, 0) = data_020bf7f4;
    func_02008130(S8_AT(self, 0x42), S16_AT(self, 0x40));
    func_020506c0(self);
    return self;
}

/* 0x02044994: the zone's constructor (at `pos`)
 * @difftest zero:0x60 ptr:8 stub=0x02050890 cases=3 */
u8 *func_02044994(u8 *self, s32 *pos)
{
    s32 box[4];

    box[0] = data_020bf7d0;
    box[1] = data_020bf7c4;
    box[2] = data_020bf7cc;
    box[3] = data_020bf7c8;
    func_02050890(self, pos, box);
    PTR_AT(self, 0) = data_020bf7f4;
    self[0x4f] = 0;
    return self;
}

/* 0x02044a0c: create zone `id` (of map object list `k`)
 * @difftest int:0:4 int:0:2 stub=0x02050890,0x02044830 cases=5 */
u8 *func_02044a0c(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    s32 v[2], t[2], u[2];
    u8 *p;

    v[0] = S16_AT(def, 4);
    v[1] = S16_AT(def, 6);
    func_02003b08(t, v, 0xc);
    v[0] = t[0];
    v[1] = t[1];
    p = func_0207fe74(0x50);
    if (p != NULL)
        p = func_02044994(p, v);
    func_02003b08(t, v, 4);
    v[0] = t[0];
    v[1] = t[1];
    u[0] = t[0];
    u[1] = t[1];
    S32_AT(p, 0x24) = func_0201a0a0(u, NULL);
    func_02044830(p, 0x34, 0, 0);
    S32_AT(p, 0xc) = 0xb;
    S32_AT(p, 8) = 0x34;
    U32_AT(p, 0x38) = (U32_AT(p, 0x38) & ~0x20u) | ((def[8] & 1) << 5);
    U32_AT(p, 0x38) = (U32_AT(p, 0x38) & ~0x40u) | ((def[9] & 1) << 6);
    p[0x4c] = def[0xa];
    p[0x4d] = def[0xb];
    p[0x4e] = func_020186ec();
    U32_AT(p, 4) |= 8;
    func_02004490(data_020bf840);
    return p;
}
