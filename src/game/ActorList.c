/*
 * The list of running game objects (a singleton: data_020ebd2c) and the
 * object list class it is built on.
 * ARM9 main, 0x02050a08 - 0x02050ed8 (19 functions).
 *
 * The manager (0x18 bytes, vtable data_020c0448) is {vtable, list,
 * paused}: the list (vtable data_020c0460, base data_020c047c) is {vtable,
 * data, size, capacity} of object pointers. The objects' vtables have the
 * deleting destructor at +4, the update (ticks) at +8 and pause (on) at
 * +0xc; an object whose flag bit 1 (+0x38) is set is removed and deleted
 * at its next update. The singleton base (vtable data_020c0498) sets and
 * clears data_020ebd2c. This-adjusting entry points subtract 4.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c0448[], data_020c0460[], data_020c047c[], data_020c0498[];   /* vtables */
extern void *data_020ebd2c;          /* the manager */

void func_0207fe44(void *p);
u32 func_02038b28(void *list);
void *func_02038b1c(void *list, u32 i);
void func_02019724(u8 *list);
void func_02006acc(void *v);

s32 func_02050dd0(u8 *list, void *e);
u8 *func_02050d38(u8 *list);
void func_02050d60(u8 *self, void *p);
void func_02050d7c(u8 *self);

#define OBJCALL(o, off) (*(void (**)())(*(u8 **)(o) + (off)))

/* 0x02050a08: delete all the objects
 * @difftest $T=zero:0x10 @$T+4:32=0x02043814 $E=zero:0x48 @$E:32=$T $D=zero:0x10 @$D:32=$E @$D+8:32=$E $S=zero:0x18 @$S+8:32=$D @$S+0xc:32=int:0:4 $S stub=0x02019724 cases=8 */
void func_02050a08(u8 *self)
{
    u32 i;

    for (i = 0; (s32)i < (s32)func_02038b28(self + 4); i++) {
        u8 *e = func_02038b1c(self + 4, i);

        if (e != NULL)
            OBJCALL(e, 4)(e);
    }
    func_02019724(self + 4);
}

/* 0x02050a6c: resume all the objects
 * @difftest $T=zero:0x10 @$T+0xc:32=0x02043814 $E=zero:0x48 @$E:32=$T $D=zero:0x10 @$D:32=$E @$D+4:32=$E $S=zero:0x18 @$S+8:32=$D @$S+0xc:32=int:0:3 @$S+0x14:8=pick:0,1 $S cases=8 */
void func_02050a6c(u8 *self)
{
    u32 i;

    if (S8_AT(self, 0x14) != 1)
        return;
    self[0x14] = 0;
    for (i = 0; (s32)i < (s32)func_02038b28(self + 4); i++) {
        u8 *e = func_02038b1c(self + 4, i);

        OBJCALL(e, 0xc)(e, 0);
    }
}

/* 0x02050ae0: pause all the objects
 * @difftest $T=zero:0x10 @$T+0xc:32=0x02043814 $E=zero:0x48 @$E:32=$T $D=zero:0x10 @$D:32=$E @$D+4:32=$E $S=zero:0x18 @$S+8:32=$D @$S+0xc:32=int:0:3 @$S+0x14:8=pick:0,1 $S cases=8 */
void func_02050ae0(u8 *self)
{
    u32 i;

    if (S8_AT(self, 0x14) != 0)
        return;
    self[0x14] = 1;
    for (i = 0; (s32)i < (s32)func_02038b28(self + 4); i++) {
        u8 *e = func_02038b1c(self + 4, i);

        OBJCALL(e, 0xc)(e, 1);
    }
}

/* 0x02050b58: update all the objects by `dt` ticks, removing the ones marked
 * @difftest $T=zero:0x10 @$T+4:32=0x02043814 @$T+8:32=0x02043814 $E=zero:0x48 @$E:32=$T @$E+0x38:32=pick:0,2 $F=zero:0x48 @$F:32=$T @$F+0x38:32=pick:0,2 $D=zero:0x10 @$D:32=$E @$D+4:32=$F @$D+8:32=$E $S=zero:0x18 @$S+8:32=$D @$S+0xc:32=int:0:4 @$S+0x10:32=4 $S u32 cases=20 */
void func_02050b58(u8 *self, u32 dt)
{
    u32 i;

    for (i = 0; (s32)i < (s32)func_02038b28(self + 4); i++) {
        u8 *e = func_02038b1c(self + 4, i);

        if (BITS(U32_AT(e, 0x38), 1, 1) == 1) {
            func_02050dd0(self + 4, e);
            if (e != NULL)
                OBJCALL(e, 4)(e);
        } else {
            OBJCALL(e, 8)(e, dt);
        }
    }
}

/* 0x02050c04: the manager's deleting destructor
 * @difftest zero:0x18 stub=0x02006acc,0x0207fe44 cases=3 */
u8 *func_02050c04(u8 *self)
{
    PTR_AT(self, 0) = data_020c0448;
    PTR_AT(self, 4) = data_020c0460;
    func_02050d38(self + 4);
    func_02050d7c(self);
    func_0207fe44(self);
    return self;
}

/* 0x02050c48: the manager's destructor
 * @difftest zero:0x18 stub=0x02006acc cases=3 */
u8 *func_02050c48(u8 *self)
{
    PTR_AT(self, 0) = data_020c0448;
    PTR_AT(self, 4) = data_020c0460;
    func_02050d38(self + 4);
    func_02050d7c(self);
    return self;
}

/* 0x02050c84: the manager's constructor
 * @difftest ptr:0x18:4 cases=3 */
u8 *func_02050c84(u8 *self)
{
    u8 *l = self + 4;

    func_02050d60(self, self);
    PTR_AT(l, 0) = data_020c047c;
    U32_AT(l, 4) = 0;
    U32_AT(l, 8) = 0;
    U32_AT(l, 0xc) = 0;
    PTR_AT(self, 0) = data_020c0448;
    PTR_AT(self, 4) = data_020c0460;
    self[0x14] = 0;
    return self;
}

/* 0x02050ce0: the list's destructor
 * @difftest zero:0x10 stub=0x02006acc cases=3 */
u8 *func_02050ce0(u8 *list)
{
    PTR_AT(list, 0) = data_020c047c;
    func_02006acc(list + 4);
    return list;
}

/* 0x02050d08: the list's deleting destructor
 * @difftest zero:0x10 stub=0x02006acc,0x0207fe44 cases=3 */
u8 *func_02050d08(u8 *list)
{
    PTR_AT(list, 0) = data_020c047c;
    func_02006acc(list + 4);
    func_0207fe44(list);
    return list;
}

/* 0x02050d38: the list's destructor (another copy)
 * @difftest zero:0x10 stub=0x02006acc cases=3 */
u8 *func_02050d38(u8 *list)
{
    PTR_AT(list, 0) = data_020c047c;
    func_02006acc(list + 4);
    return list;
}

/* 0x02050d60: the singleton's constructor: `p` is the instance
 * @difftest ptr:4:4 u32 cases=3 */
void func_02050d60(u8 *self, void *p)
{
    PTR_AT(self, 0) = data_020c0498;
    data_020ebd2c = p;
}

/* 0x02050d7c: the singleton's destructor
 * @difftest ptr:4:4 cases=3 */
void func_02050d7c(u8 *self)
{
    PTR_AT(self, 0) = data_020c0498;
    data_020ebd2c = NULL;
}

/* 0x02050d9c: the singleton's deleting destructor
 * @difftest ptr:4:4 stub=0x0207fe44 cases=3 */
u8 *func_02050d9c(u8 *self)
{
    PTR_AT(self, 0) = data_020c0498;
    data_020ebd2c = NULL;
    func_0207fe44(self);
    return self;
}

/* 0x02050dd0: remove object `e` from the list; whether it was there
 * @difftest $D=zero:0x10 @$D+4:32=1 @$D+8:32=5 @$D+0xc:32=6 $L=zero:0x10 @$L+4:32=$D @$L+8:32=int:0:4 @$L+0xc:32=4 $L pick:0,1 cases=10 */
s32 func_02050dd0(u8 *list, void *e)
{
    void **it = PTR_AT(list, 4);
    void **end = it + U32_AT(list, 8);

    while (it != end && *it != e)
        it++;
    if (it == end)
        return 0;
    func_02006240((struct PtrVec *)(list + 4), it, 0);
    return 1;
}

/* 0x02050e78: the singleton's destructor (another copy)
 * @difftest ptr:4:4 cases=3 */
void func_02050e78(u8 *self)
{
    PTR_AT(self, 0) = data_020c0498;
    data_020ebd2c = NULL;
}

/* The manager's this-adjusting entry points (for its list's vtable). */

/* 0x02050e98
 * @difftest $S=zero:0x18 $S+4 stub=0x02006acc cases=3 */
u8 *func_02050e98(u8 *l)
{
    return func_02050c48(l - 4);
}

/* 0x02050ea8
 * @difftest $S=zero:0x18 $S+4 u32 cases=3 */
void func_02050ea8(u8 *l, u32 dt)
{
    func_02050b58(l - 4, dt);
}

/* 0x02050eb8
 * @difftest $S=zero:0x18 $S+4 stub=0x02019724 cases=3 */
void func_02050eb8(u8 *l)
{
    func_02050a08(l - 4);
}

/* 0x02050ec8
 * @difftest $S=zero:0x18 $S+4 stub=0x02006acc,0x0207fe44 cases=3 */
u8 *func_02050ec8(u8 *l)
{
    return func_02050c04(l - 4);
}
