/*
 * A second kind of resource slot reference (8 bytes, vtable
 * data_020c0ca0): the index (0: none) of one of 0x13f reference-counted
 * slots (data_020f53b4 -> data_020f53b8, 0xc bytes each: {resource, ?,
 * users}), set up by the first constructor.
 * ARM9 main, 0x020590c4 - 0x0205924c (5 functions).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c0ca0[];           /* vtable */
extern s8 data_020f53b0;             /* the table is set up */
extern u8 *data_020f53b4;            /* the table */
extern u8 data_020f53b8[];

void func_0207fe44(void *p);

/* 0x020590c4: let go of the slot (freeing it with its last user)
 * @difftest $T=zero:0x30 @$T+8:32=int:0:3 @$T+0x14:32=int:0:3 @$T+0x20:32=int:0:3 @$T+0xc:32=u32 @$T+0x18:32=u32 @020f53b4:32=$T $R=zero:8 @$R+4:32=pick:-1,0,1,2 $R cases=20 */
void func_020590c4(u8 *r)
{
    s32 i = S32_AT(r, 4);
    u8 *e;

    if (i <= 0)
        return;
    e = data_020f53b4 + i * 0xc;
    if (U32_AT(e, 8) == 0)
        return;
    U32_AT(e, 8)--;
    e = data_020f53b4 + S32_AT(r, 4) * 0xc;
    if (U32_AT(e, 8) == 0)
        U32_AT(e, 0) = 0;
    S32_AT(r, 4) = 0;
}

/* 0x0205914c: destructor
 * @difftest $T=zero:0x30 @$T+0x14:32=int:0:3 @020f53b4:32=$T $R=zero:8 @$R+4:32=pick:0,1 $R cases=5 */
u8 *func_0205914c(u8 *r)
{
    PTR_AT(r, 0) = data_020c0ca0;
    func_020590c4(r);
    return r;
}

/* 0x02059170: deleting destructor
 * @difftest $T=zero:0x30 @$T+0x14:32=int:0:3 @020f53b4:32=$T $R=zero:8 @$R+4:32=pick:0,1 $R stub=0x0207fe44 cases=5 */
u8 *func_02059170(u8 *r)
{
    PTR_AT(r, 0) = data_020c0ca0;
    func_020590c4(r);
    func_0207fe44(r);
    return r;
}

/* 0x0205919c: destructor (another copy)
 * @difftest $T=zero:0x30 @$T+0x14:32=int:0:3 @020f53b4:32=$T $R=zero:8 @$R+4:32=pick:0,1 $R cases=5 */
u8 *func_0205919c(u8 *r)
{
    PTR_AT(r, 0) = data_020c0ca0;
    func_020590c4(r);
    return r;
}

/* 0x020591c0: constructor (no slot); sets the table up the first time
 * @difftest ptr:8:4 @020f53b0:8=pick:0,1 cases=4 */
u8 *func_020591c0(u8 *r)
{
    u32 i;

    PTR_AT(r, 0) = data_020c0ca0;
    S32_AT(r, 4) = 0;
    if (data_020f53b0 != 0)
        return r;
    data_020f53b4 = data_020f53b8;
    for (i = 0; i < 0x13f; i++) {
        U32_AT(data_020f53b4 + i * 0xc, 8) = 0;
        U32_AT(data_020f53b4 + i * 0xc, 0) = 0;
    }
    data_020f53b0 = 1;
    return r;
}
