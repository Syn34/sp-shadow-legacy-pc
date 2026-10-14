/*
 * The actor base class (0x48 bytes, at +0x90 of most game objects): a task
 * (func_020592a8 / func_0205924c) with a collision box, a position and the
 * previous one, flags, a map object reference and an optional collision
 * shape object at +0x44.
 * ARM9 main, 0x02050430 - 0x02050a08 (18 functions).
 *
 * Layout: +0x00 vtable (data_020c037c), +0x10 box {x0, z0, x1, z1} (whole
 * units, relative to the position), +0x28 position {x, z}, +0x30 the
 * previous position, +0x38 flags (bit 0 has a map object, 1 to be removed,
 * 14 set at creation), +0x3c collision mask (0x1f), +0x40 map object index,
 * +0x42 map object group, +0x44 the shape object (0x1a8 bytes; its +0x14
 * heading, +0xac).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c037c[];           /* vtable */
extern const s32 data_020c0354, data_020c0358, data_020c035c, data_020c0360, data_020c0364,
    data_020c0368, data_020c036c, data_020c0370;   /* the default box and position */

void *func_0207fe74(u32 size);
void func_0207fe44(void *p);
void *func_0205382c(void *e);
void func_020038d4(s32 *out, const s32 *in, s32 shift);
void *func_020062c0(void);
void func_02017abc(void *list, void *item);
void func_0205924c(void *task);
void func_020592a8(void *task);

s8 func_020504e0(u8 *a, const s32 *box);

/* 0x02050430: turn the shape object to `h`
 * @difftest $A=zero:0x48 $O=ptr:0x100:4 @$A+0x44:32=$O $A int:0:0x6487 cases=5 */
void func_02050430(u8 *a, fx32 h)
{
    u8 *o = PTR_AT(a, 0x44);
    s32 x = S32_AT(o, 0x10), z = S32_AT(o, 0x18);

    S32_AT(o, 0x10) = x;
    S32_AT(o, 0x14) = h;
    S32_AT(o, 0x18) = z;
    o[0x8c] = 1;
}

/* 0x0205045c: the shape object's heading
 * @difftest $A=zero:0x48 $O=ptr:0x100:4 @$A+0x44:32=$O $A cases=3 */
fx32 func_0205045c(u8 *a)
{
    return S32_AT(PTR_AT(a, 0x44), 0x14);
}

/* 0x02050468: the shape object's +0xac (0 without one)
 * @difftest $A=zero:0x48 $A cases=2
 * @difftest $A=zero:0x48 $O=ptr:0x100:4 @$A+0x44:32=$O $A cases=3 */
u32 func_02050468(u8 *a)
{
    u8 *o = PTR_AT(a, 0x44);

    return o != NULL ? U32_AT(o, 0xac) : 0;
}

/* 0x0205047c: delete the shape object
 * @difftest $A=zero:0x48 $A cases=2
 * @difftest $A=zero:0x48 $T=zero:0x10 @$T+4:32=0x02043814 $O=zero:0x10 @$O:32=$T @$A+0x44:32=$O $A cases=3 */
void func_0205047c(void *actor)
{
    u8 *a = actor;
    u8 *o = PTR_AT(a, 0x44);

    if (o == NULL)
        return;
    (*(void (**)(void *))(*(u8 **)o + 4))(o);
    PTR_AT(a, 0x44) = NULL;
}

/* 0x020504b8: give the actor a shape object
 * @difftest ptr:0x48:4 stub=0x0207fe74:0 cases=3
 * @difftest ptr:0x48:4 stub=0x0207fe74:0x1234,0x0205382c:0x5678 cases=3 */
void func_020504b8(void *actor)
{
    u8 *p = func_0207fe74(0x1a8);

    if (p != NULL)
        p = func_0205382c(p);
    PTR_AT(actor, 0x44) = p;
}

/* 0x020504e0: whether the actor's box (at its position) overlaps `box`
 * @difftest $A=zero:0x48 @$A+0x10:32=int:-4:0 @$A+0x14:32=int:-4:0 @$A+0x18:32=int:0:4 @$A+0x1c:32=int:0:4 @$A+0x28:32=int:-0x8000:0x8000 @$A+0x2c:32=int:-0x8000:0x8000 $B=zero:0x10 @$B:32=int:-0x8000:0 @$B+4:32=int:-0x8000:0 @$B+8:32=int:0:0x8000 @$B+0xc:32=int:0:0x8000 $A $B cases=60 */
s8 func_020504e0(u8 *a, const s32 *box)
{
    s32 b[4], v[2], o[2];

    b[0] = S32_AT(a, 0x10);
    b[1] = S32_AT(a, 0x14);
    b[2] = S32_AT(a, 0x18);
    b[3] = S32_AT(a, 0x1c);
    v[0] = S32_AT(a, 0x28);
    v[1] = S32_AT(a, 0x2c);
    func_020038d4(o, v, 0xc);
    b[0] += o[1];
    b[1] += o[0];
    b[2] += o[1];
    b[3] += o[0];
    if (b[3] < box[1] || b[1] > box[3] || b[0] > box[2])
        return 0;
    return b[2] >= box[0];
}

/* 0x020505b8: whether two actors' boxes overlap
 * @difftest $A=zero:0x48 @$A+0x10:32=int:-4:0 @$A+0x14:32=int:-4:0 @$A+0x18:32=int:0:4 @$A+0x1c:32=int:0:4 @$A+0x28:32=int:-0x8000:0x8000 @$A+0x2c:32=int:-0x8000:0x8000 $B=zero:0x48 @$B+0x10:32=int:-4:0 @$B+0x14:32=int:-4:0 @$B+0x18:32=int:0:4 @$B+0x1c:32=int:0:4 @$B+0x28:32=int:-0x8000:0x8000 @$B+0x2c:32=int:-0x8000:0x8000 $A $B cases=60 */
s8 func_020505b8(u8 *a, u8 *other)
{
    s32 b[4], v[2], o[2];

    b[0] = S32_AT(a, 0x10);
    b[1] = S32_AT(a, 0x14);
    b[2] = S32_AT(a, 0x18);
    b[3] = S32_AT(a, 0x1c);
    v[0] = S32_AT(a, 0x28);
    v[1] = S32_AT(a, 0x2c);
    func_020038d4(o, v, 0xc);
    b[0] += o[1];
    b[1] += o[0];
    b[2] += o[1];
    b[3] += o[0];
    return func_020504e0(other, b);
}

/* 0x0205065c
 * @difftest ptr:0x48:4 cases=3 */
void func_0205065c(u8 *a)
{
}

/* 0x02050660: add the actor to the list of actors to process
 * @difftest ptr:0x48:4 stub=0x02017abc cases=3 */
void func_02050660(u8 *a)
{
    func_02017abc((u8 *)func_020062c0() + 4, a);
}

/* 0x02050680: forget the map object
 * @difftest ptr:0x48:4 cases=3 */
void func_02050680(void *actor)
{
    u8 *a = actor;

    S16_AT(a, 0x40) = -1;
    S8_AT(a, 0x42) = -1;
    U32_AT(a, 0x38) &= ~1u;
}

/* 0x020506a0: remember map object `index` of group `group`
 * @difftest ptr:0x48:4 s8 s16 cases=5 */
void func_020506a0(u8 *a, s8 group, s16 index)
{
    S8_AT(a, 0x42) = group;
    S16_AT(a, 0x40) = index;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~1u) | 1;
}

/* 0x020506c0: the actor's destructor
 * @difftest zero:0x48 stub=0x0205924c cases=3 */
u8 *func_020506c0(u8 *a)
{
    PTR_AT(a, 0) = data_020c037c;
    U32_AT(a, 0x38) = 0;
    func_0205047c(a);
    func_0205924c(a);
    return a;
}

/* 0x020506f4: the actor's deleting destructor
 * @difftest zero:0x48 stub=0x0205924c,0x0207fe44 cases=3 */
u8 *func_020506f4(u8 *a)
{
    PTR_AT(a, 0) = data_020c037c;
    U32_AT(a, 0x38) = 0;
    func_0205047c(a);
    func_0205924c(a);
    func_0207fe44(a);
    return a;
}

/* 0x02050730: the actor's destructor (another copy)
 * @difftest zero:0x48 stub=0x0205924c cases=3 */
u8 *func_02050730(u8 *a)
{
    PTR_AT(a, 0) = data_020c037c;
    U32_AT(a, 0x38) = 0;
    func_0205047c(a);
    func_0205924c(a);
    return a;
}

/* what the constructors share after the box */
static void InitRest(u8 *a, const s32 *pos)
{
    S32_AT(a, 0x28) = pos[0];
    S32_AT(a, 0x2c) = pos[1];
    S32_AT(a, 0x30) = pos[0];
    S32_AT(a, 0x34) = pos[1];
    U32_AT(a, 0x3c) = 0x1f;
    S16_AT(a, 0x40) = -1;
    S8_AT(a, 0x42) = -1;
    U32_AT(a, 0x44) = 0;
    U32_AT(a, 0x38) = 0;
    U32_AT(a, 0x38) |= 0x4000;
}

/* 0x02050764: constructor: no box, at `pos`
 * @difftest ptr:0x48:4 ptr:8:4 stub=0x020592a8 cases=3 */
u8 *func_02050764(u8 *a, const s32 *pos)
{
    func_020592a8(a);
    PTR_AT(a, 0) = data_020c037c;
    U32_AT(a, 0x10) = 0;
    U32_AT(a, 0x14) = 0;
    U32_AT(a, 0x18) = 0;
    U32_AT(a, 0x1c) = 0;
    InitRest(a, pos);
    return a;
}

/* 0x020507f4: constructor: box `box` at `pos`
 * @difftest ptr:0x48:4 ptr:8:4 ptr:0x10:4 stub=0x020592a8 cases=3 */
u8 *func_020507f4(u8 *a, const s32 *pos, const s32 *box)
{
    func_020592a8(a);
    PTR_AT(a, 0) = data_020c037c;
    S32_AT(a, 0x10) = box[0];
    S32_AT(a, 0x14) = box[1];
    S32_AT(a, 0x18) = box[2];
    S32_AT(a, 0x1c) = box[3];
    InitRest(a, pos);
    return a;
}

/* 0x02050890: constructor: box `box` at `pos` (another copy)
 * @difftest ptr:0x48:4 ptr:8:4 ptr:0x10:4 stub=0x020592a8 cases=3 */
u8 *func_02050890(u8 *a, const s32 *pos, const s32 *box)
{
    func_020592a8(a);
    PTR_AT(a, 0) = data_020c037c;
    S32_AT(a, 0x10) = box[0];
    S32_AT(a, 0x14) = box[1];
    S32_AT(a, 0x18) = box[2];
    S32_AT(a, 0x1c) = box[3];
    InitRest(a, pos);
    return a;
}

/* 0x0205092c: constructor: the default box and position
 * @difftest ptr:0x48:4 stub=0x020592a8 cases=3 */
u8 *func_0205092c(u8 *a)
{
    func_020592a8(a);
    PTR_AT(a, 0) = data_020c037c;
    S32_AT(a, 0x10) = data_020c0370;
    S32_AT(a, 0x14) = data_020c036c;
    S32_AT(a, 0x18) = data_020c0368;
    S32_AT(a, 0x1c) = data_020c0360;
    S32_AT(a, 0x28) = data_020c0364;
    S32_AT(a, 0x2c) = data_020c0354;
    S32_AT(a, 0x30) = data_020c0358;
    S32_AT(a, 0x34) = data_020c035c;
    U32_AT(a, 0x3c) = 0x1f;
    S16_AT(a, 0x40) = -1;
    S8_AT(a, 0x42) = -1;
    U32_AT(a, 0x44) = 0;
    U32_AT(a, 0x38) = 0;
    U32_AT(a, 0x38) |= 0x4000;
    return a;
}
