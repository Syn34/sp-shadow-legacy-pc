/*
 * Resource slot references: a small base class (8 bytes, vtable
 * data_020c04a8) holding the index of a slot in a shared table of 0xc83
 * reference-counted slots (data_020ebd34 -> data_020ebd38, 0xc bytes each:
 * {resource, ?, count}), set up once by the first constructor.
 * ARM9 main, 0x02050ed8 - 0x02051068 (5 functions).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c04a8[];           /* vtable */
extern s8 data_020ebd30;             /* the table is set up */
extern u8 *data_020ebd34;            /* the table */
extern u8 data_020ebd38[];

void func_0207fe44(void *p);

/* 0x02050ed8: let go of the slot (freeing it with its last reference)
 * @difftest $T=zero:0x30 @$T+8:32=int:0:3 @$T+0x14:32=int:0:3 @$T:32=u32 @$T+0xc:32=u32 @020ebd34:32=$T $R=zero:8 @$R+4:32=pick:-1,0,1 $R cases=20 */
void func_02050ed8(u8 *r)
{
    s32 i = S32_AT(r, 4);
    u8 *e;

    if (i <= -1)
        return;
    e = data_020ebd34 + i * 0xc;
    if (U32_AT(e, 8) == 0)
        return;
    U32_AT(e, 8)--;
    e = data_020ebd34 + S32_AT(r, 4) * 0xc;
    if (U32_AT(e, 8) == 0)
        U32_AT(e, 0) = 0;
    S32_AT(r, 4) = -1;
}

/* 0x02050f64: destructor
 * @difftest $T=zero:0x30 @$T+8:32=int:0:3 @$T+0x14:32=int:0:3 @020ebd34:32=$T $R=zero:8 @$R+4:32=pick:-1,0,1 $R cases=5 */
u8 *func_02050f64(u8 *r)
{
    PTR_AT(r, 0) = data_020c04a8;
    func_02050ed8(r);
    return r;
}

/* 0x02050f88: deleting destructor
 * @difftest $T=zero:0x30 @$T+8:32=int:0:3 @$T+0x14:32=int:0:3 @020ebd34:32=$T $R=zero:8 @$R+4:32=pick:-1,0,1 $R stub=0x0207fe44 cases=5 */
u8 *func_02050f88(u8 *r)
{
    PTR_AT(r, 0) = data_020c04a8;
    func_02050ed8(r);
    func_0207fe44(r);
    return r;
}

/* 0x02050fb4: destructor (another copy)
 * @difftest $T=zero:0x30 @$T+8:32=int:0:3 @$T+0x14:32=int:0:3 @020ebd34:32=$T $R=zero:8 @$R+4:32=pick:-1,0,1 $R cases=5 */
u8 *func_02050fb4(u8 *r)
{
    PTR_AT(r, 0) = data_020c04a8;
    func_02050ed8(r);
    return r;
}

/* 0x02050fd8: constructor (no slot); sets the table up the first time
 * @difftest ptr:8:4 @020ebd30:8=pick:0,1 cases=4 */
u8 *func_02050fd8(u8 *r)
{
    u32 i;

    PTR_AT(r, 0) = data_020c04a8;
    S32_AT(r, 4) = -1;
    if (data_020ebd30 != 0)
        return r;
    data_020ebd34 = data_020ebd38;
    for (i = 0; i < 0xc83; i++) {
        U32_AT(data_020ebd34 + i * 0xc, 8) = 0;
        U32_AT(data_020ebd34 + i * 0xc, 0) = 0;
    }
    data_020ebd30 = 1;
    return r;
}
