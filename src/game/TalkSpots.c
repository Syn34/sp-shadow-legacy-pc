/*
 * Talk spots: map objects of actor class 2 (0x108 bytes) where the hero
 * can press A to get a message (+0xe4), optionally running script
 * +0xe6 (func_020091c0); after it the spot rests for +0xfc ticks or is
 * removed. While the hero stands there without pressing A, a hint
 * (func_02072418, by the spot's kind +0xe0) is shown.
 * ARM9 main, 0x0204ab18 - 0x0204b36c (16 functions).
 *
 * The object is an actor (vtable data_020bfd04) with a model at +0x4c
 * (vtable data_020bfd50; this-adjusting destructors at 0x0204b34c..).
 * +0xdc rest after use, +0xe8 needs A, +0xec..+0xf8 the hint box,
 * +0x100 time left, +0x104 the state (0 idle, 1 used, 2 resting, 3 done).
 *
 * Testing: with fake objects; the messages, the hints, the scripts and
 * the camera stubbed or faked where they need real data.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bfd04[], data_020bfd50[];   /* vtables */

void func_020038d4(s32 *out, const s32 *in, s32 shift);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
s32 func_02011bc8(void);
void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
u8 *func_02011f34(void);
void func_02072418(void *obj, u32 a, u32 fbits);
u32 func_020186ec(void);
void *func_02009394(void);
void func_0205f6e4(void *obj, u32 v);
s32 func_0205f578(void *obj, u32 v);
void func_020091c0(s32 script);
void func_0201c17c(s32 *out, s32 *pos);
void *func_020062e0(void);
void *func_020062c0(void);
void *func_020062b0(void);
void func_020040f4(u8 *dst, const u8 *src);
void func_02082ce8(void *cam);
void func_02017abc(void *list, void *item);
void func_02008130(s32 group, s32 index);
void func_0205b784(void *self);
void func_0205b8c8(void *self);
void *func_020506c0(void *actor);
void func_0207fe44(void *p);
void func_02050764(void *actor, s32 *pos);
u8 *func_02008994(s32 i, s32 k);
void *func_0207fe74(u32 size);
s32 func_0201a0a0(s32 *xz, void *obj);

u32 func_0204abd0(u8 *self);
void func_0204abd8(u8 *self, u32 v);
void func_0204af10(u8 *self);
u8 *func_0204b06c(u8 *self);
u8 *func_0204b0bc(u8 *self);
u8 *func_0204b104(u8 *self, const u8 *def, s32 *pos);
u8 *func_0204b290(s32 id, s32 k);

/* 0x0204ab18: the hint box in the world
 * @difftest ptr:16:4 $S=zero:0x108 @$S+0xec:32=int:-8:0 @$S+0xf0:32=int:-8:0 @$S+0xf4:32=int:0:8 @$S+0xf8:32=int:0:8 @$S+0x28:32=s32 @$S+0x2c:32=s32 $S cases=20 */
void func_0204ab18(s32 *out, u8 *self)
{
    s32 xz[2], p[2];

    out[0] = S32_AT(self, 0xec);
    out[1] = S32_AT(self, 0xf0);
    out[2] = S32_AT(self, 0xf4);
    out[3] = S32_AT(self, 0xf8);
    xz[0] = S32_AT(self, 0x28);
    xz[1] = S32_AT(self, 0x2c);
    func_020038d4(p, xz, 0xc);
    out[1] += p[0];
    out[3] += p[0];
    out[0] += p[1];
    out[2] += p[1];
}

/* 0x0204abbc: remove the spot
 * @difftest ptr:0x108 cases=3 */
void func_0204abbc(u8 *self)
{
    U32_AT(self, 0x38) |= 2;
}

/* 0x0204abd0: the state
 * @difftest ptr:0x108 cases=3 */
u32 func_0204abd0(u8 *self)
{
    return U32_AT(self, 0x104);
}

/* 0x0204abd8: set the state
 * @difftest ptr:0x108 int:0:4 cases=3 */
void func_0204abd8(u8 *self, u32 v)
{
    U32_AT(self, 0x104) = v;
}

/* 0x0204abe0: the hero in the spot
 * @difftest $S=zero:0x108 @$S+0x38:32=pick:0,0,8 @$S+0xe8:8=pick:0,1,1 @$S+0xe0:32=int:0:7 $O=zero:0x60 @$O+0xc:32=pick:0,0,0,1 @$O+0x38:32=pick:0x800000,0x800000,0 $K=zero:0x80 @020f6f30:32=$K @$K+0x5c:32=pick:0,1 $S $O stub=0x02011bc8:0,0x02072418 cases=100
 * @difftest $S=zero:0x108 $O=zero:0x60 @$O+0x38:32=0x800000 $S $O stub=0x02011bc8:1,0x02072418 cases=3 */
void func_0204abe0(u8 *self, u8 *other)
{
    if (S32_AT(other, 0xc) != 0)
        return;
    if (func_02011bc8() != 0)
        return;
    if ((s8)BITS(U32_AT(other, 0x38), 23, 1) == 0)
        return;
    if (BITS(U32_AT(self, 0x38), 3, 1) != 0)
        return;
    if (S8_AT(self, 0xe8) != 0 && func_02081d34(func_02011f44(), 0xa) == 0) {
        u32 k;

        switch (U32_AT(self, 0xe0)) {
        case 2:
            k = 3;
            break;
        case 3:
            k = 8;
            break;
        case 4:
            k = 9;
            break;
        case 5:
            k = 2;
            break;
        default:
            k = 1;
            break;
        }
        func_02072418(func_02011f34(), k, 0x45000000);
        return;
    }
    func_02072418(func_02011f34(), 0, 0x45000000);
    U32_AT(self, 0x38) |= 8;
    func_02072418(func_02011f34(), 0, 0x45000000);
    func_0204abd8(self, 1);
}

/* 0x0204ad88: per-frame update
 * @difftest $S=zero:0x108 @$S+4:32=pick:0,0,4 @$S+0x104:32=int:0:5 @$S+0x38:32=pick:0x20,0x40,0x60,0 @$S+0xdc:32=pick:0,1 @$S+0xfc:32=int:0:8 @$S+0x100:32=int:-2:4 @$S+0xe4:16=pick:-1,7 @$S+0xe6:16=int:0:0x200 $S int:0:4 stub=0x020186ec:0,0x0204af10,0x0205f6e4,0x0205f578:0,0x020091c0 cases=60
 * @difftest $S=zero:0x108 @$S+0x104:32=pick:0,2 @$S+0x38:32=0x40 $S int:0:4 stub=0x020186ec:1,0x0204af10,0x0205f578:1 cases=10 */
void func_0204ad88(u8 *self, s32 dt)
{
    if ((s8)BITS(U32_AT(self, 4), 2, 1) != 0)
        return;
    switch (func_0204abd0(self)) {
    case 0:
        if (!(BITS(U32_AT(self, 0x38), 6, 1) != 0 && func_020186ec() == 1)) {
            if (BITS(U32_AT(self, 0x38), 5, 1) == 0)
                return;
            if (func_020186ec() != 0)
                return;
        }
        func_0204af10(self);
        return;
    case 1:
        func_0205f6e4(func_02009394(), S16_AT(self, 0xe4));
        func_020091c0((s8)S16_AT(self, 0xe6));
        if (U32_AT(self, 0xdc) == 0) {
            func_0204abd8(self, 3);
        } else {
            S32_AT(self, 0x100) = S32_AT(self, 0xfc);
            func_0204abd8(self, 2);
        }
        func_0204af10(self);
        return;
    case 2:
        if (func_0205f578(func_02009394(), S16_AT(self, 0xe4)) == 0) {
            S32_AT(self, 0x100) -= dt;
            if (S32_AT(self, 0x100) < 0) {
                func_0204abd8(self, 0);
                U32_AT(self, 0x38) &= ~8u;
            }
        }
        func_0204af10(self);
        return;
    case 3:
        U32_AT(self, 0x38) |= 2;
        return;
    }
}

/* 0x0204af10: place the model and draw it if in view
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0x108 @$S+0x38:32=pick:0,4 @$S+0x28:32=int:-0x10000:0x10000 @$S+0x2c:32=int:-0x10000:0x10000 @$S+0x10:32=int:-0x10000:0 @$S+0x14:32=int:-0x10000:0 @$S+0x18:32=int:0:0x10000 @$S+0x1c:32=int:0:0x10000 $S stub=0x0201f490:0x020deebc,0x02017abc cases=20 */
void func_0204af10(u8 *self)
{
    s32 xz[2], p[3], b[4];
    u32 cam[0x2c0 / 4];
    s32 vis;

    if (BITS(U32_AT(self, 0x38), 2, 1) == 0)
        return;
    xz[0] = S32_AT(self, 0x28);
    xz[1] = S32_AT(self, 0x2c);
    func_0201c17c(p, xz);
    S32_AT(self, 0x50) = p[0];
    S32_AT(self, 0x54) = p[1];
    S32_AT(self, 0x58) = p[2];
    self[0xd8] = 1;
    b[0] = S32_AT(self, 0x10) + S32_AT(self, 0x54);
    b[1] = S32_AT(self, 0x14) + S32_AT(self, 0x50);
    b[2] = S32_AT(self, 0x18) + S32_AT(self, 0x54);
    b[3] = S32_AT(self, 0x1c) + S32_AT(self, 0x50);
    func_020040f4((u8 *)cam, func_020062e0());
    vis = 0;
    if (S8_AT(cam, 0x284) != 0) {
        vis = S32_AT(cam, 0x2ac) >= b[1] && S32_AT(cam, 0x2a4) <= b[3]
              && S32_AT(cam, 0x2a0) <= b[2] && S32_AT(cam, 0x2a8) >= b[0];
    }
    func_02082ce8(cam);
    if (vis == 0)
        return;
    func_02017abc((u8 *)func_020062c0() + 4, self);
}

/* what both destructors do */
static void Destroy(u8 *self)
{
    PTR_AT(self, 0) = data_020bfd04;
    PTR_AT(self, 0x4c) = data_020bfd50;
    func_02008130(S8_AT(self, 0x42), S16_AT(self, 0x40));
    func_0205b784(self + 0x4c);
    func_020506c0(self);
}

/* 0x0204b06c: the spot's deleting destructor
 * @difftest zero:0x108 stub=0x02008130,0x0205b784,0x020506c0,0x0207fe44 cases=3 */
u8 *func_0204b06c(u8 *self)
{
    Destroy(self);
    func_0207fe44(self);
    return self;
}

/* 0x0204b0bc: the spot's destructor
 * @difftest zero:0x108 stub=0x02008130,0x0205b784,0x020506c0 cases=3 */
u8 *func_0204b0bc(u8 *self)
{
    Destroy(self);
    return self;
}

/* 0x0204b104: the spot's constructor (map object data `def`, at `pos`)
 * @difftest zero:0x108 ptr:0x14 ptr:8:4 stub=0x02050764,0x0205b8c8,0x02017abc cases=20 */
u8 *func_0204b104(u8 *self, const u8 *def, s32 *pos)
{
    s32 a, b, c, d;
    float f;

    func_02050764(self, pos);
    func_0205b8c8(self + 0x4c);
    PTR_AT(self, 0) = data_020bfd04;
    PTR_AT(self, 0x4c) = data_020bfd50;
    S32_AT(self, 0xec) = 0;
    S32_AT(self, 0xf0) = 0;
    S32_AT(self, 0xf4) = 0;
    S32_AT(self, 0xf8) = 0;
    a = (s8)def[0];
    b = (s8)def[1];
    S32_AT(self, 0x10) = -b;
    S32_AT(self, 0x14) = -a;
    S32_AT(self, 0x18) = b;
    S32_AT(self, 0x1c) = a;
    c = (s8)def[5];
    d = (s8)def[4];
    S32_AT(self, 0xec) = -c;
    S32_AT(self, 0xf0) = -d;
    S32_AT(self, 0xf4) = c;
    S32_AT(self, 0xf8) = d;
    S32_AT(self, 0x20) = 0x20000;
    U32_AT(self, 0xdc) = def[6];
    U32_AT(self, 0xe0) = def[0x11];
    self[0xe8] = def[7];
    U16_AT(self, 0xe4) = def[0xa];
    U16_AT(self, 0xe6) = def[9];
    self[0xe9] = def[0xf];
    f = (float)(s32)((u32)U16_AT(def, 2) << 12);
    S32_AT(self, 0xfc) = (s32)(U16_AT(def, 2) != 0 ? 0.5f + f : f - 0.5f);
    U32_AT(self, 0x104) = 0;
    U32_AT(self, 0x38) = (U32_AT(self, 0x38) & ~0x20u) | ((def[0xd] & 1) << 5);
    U32_AT(self, 0x38) = (U32_AT(self, 0x38) & ~0x40u) | ((def[0xe] & 1) << 6);
    U32_AT(self, 0x38) |= 4;
    S32_AT(self, 0xc) = 2;
    func_02017abc((u8 *)func_020062b0() + 4, self);
    return self;
}

/* 0x0204b290: create talk spot `id` (of map object list `k`)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:4 int:0:2 stub=0x0204b104,0x0207fe74:0 cases=5 */
u8 *func_0204b290(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    s32 v[2], t[2], u[2];
    u8 *p;

    v[0] = S16_AT(def, 4);
    v[1] = S16_AT(def, 6);
    func_02003b08(t, v, 0xc);
    v[0] = t[0];
    v[1] = t[1];
    p = func_0207fe74(0x108);
    if (p != NULL)
        p = func_0204b104(p, def + 8, v);
    func_02003b08(t, v, 4);
    v[0] = t[0];
    v[1] = t[1];
    u[0] = t[0];
    u[1] = t[1];
    S32_AT(p, 0x24) = func_0201a0a0(u, NULL);
    return p;
}

/* 0x0204b340
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:4 int:0:2 stub=0x0204b104,0x0207fe74:0 cases=3 */
u8 *func_0204b340(s32 id, s32 k)
{
    return func_0204b290(id, k);
}

/* 0x0204b34c: the model's deleting destructor (this-adjusting)
 * @difftest $S=zero:0x108 $S+0x4c stub=0x02008130,0x0205b784,0x020506c0,0x0207fe44 cases=3 */
u8 *func_0204b34c(u8 *m)
{
    return func_0204b06c(m - 0x4c);
}

/* 0x0204b35c: the model's destructor (this-adjusting)
 * @difftest $S=zero:0x108 $S+0x4c stub=0x02008130,0x0205b784,0x020506c0 cases=3 */
u8 *func_0204b35c(u8 *m)
{
    return func_0204b0bc(m - 0x4c);
}
