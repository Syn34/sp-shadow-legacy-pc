/*
 * References to loaded particle effect data (a resource slot reference,
 * see ResSlots.c, vtable data_020c0618): the first reference to an effect
 * loads it (EffectData.c), the last one frees it.
 * ARM9 main, 0x02052c14 - 0x02052f10 (4 functions).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c0618[];           /* vtable */
extern const char data_020c063c[], data_020c0674[], data_020c0698[], data_020c06bc[];   /* debug messages */
extern u8 *data_020ebd34;            /* resource slots, 0xc bytes each {data, ?, users} */

void *func_0207fe74(u32 size);
void func_0207fe44(void *p);
u8 *func_0205216c(u8 *e);
u8 *func_02052208(u8 *v);
void func_02051f80(u8 *e, u32 id);
u8 *func_02050f64(u8 *r);
u8 *func_02050fd8(u8 *r);

#define SLOT(i) (data_020ebd34 + (i) * 0xc)

/* what both destructors do */
static void Release(u8 *r)
{
    s32 i;

    PTR_AT(r, 0) = data_020c0618;
    func_02004490(data_020c063c);
    i = S32_AT(r, 4);
    if (i > -1 && U32_AT(SLOT(i), 8) == 1) {
        u8 *d;

        func_02004490(data_020c0674);
        d = PTR_AT(r, 8);
        if (d != NULL) {
            func_0205216c(d);
            func_0207fe44(d);
        }
        func_02004490(data_020c0698);
    }
    func_02004490(data_020c06bc);
    func_02050f64(r);
}

/* 0x02052c14: deleting destructor
 * @difftest $T=zero:0x18 @$T+8:32=int:0:3 @$T+0x14:32=int:0:3 @020ebd34:32=$T $R=zero:0xc @$R+4:32=pick:-1,0,1 $R stub=0x0205216c,0x02050f64,0x0207fe44 cases=10 */
u8 *func_02052c14(u8 *r)
{
    Release(r);
    func_0207fe44(r);
    return r;
}

/* 0x02052ce4: destructor
 * @difftest $T=zero:0x18 @$T+8:32=int:0:3 @$T+0x14:32=int:0:3 @020ebd34:32=$T $R=zero:0xc @$R+4:32=pick:-1,0,1 @$R+8:32=pick:0,4 $R stub=0x0205216c,0x02050f64,0x0207fe44 cases=10 */
u8 *func_02052ce4(u8 *r)
{
    Release(r);
    return r;
}

/* 0x02052dac: refer to effect `id` (loading it if no one else does)
 * @difftest $T=zero:0x18 @$T:32=u32 @$T+0xc:32=u32 @$T+8:32=int:0:2 @$T+0x14:32=int:0:2 @020ebd34:32=$T $R=zero:0xc @$R+8:32=pick:0,4 $R pick:-1,0,1 stub=0x0207fe74:0x1234,0x02052208:0x1234,0x02051f80 cases=20 */
void func_02052dac(u8 *r, s32 id)
{
    s32 i;

    S32_AT(r, 4) = id;
    i = S32_AT(r, 4);
    if (i > -1 && U32_AT(SLOT(i), 8) == 0) {
        if (PTR_AT(r, 8) == NULL) {
            u8 *p = func_0207fe74(0xc);

            if (p != NULL)
                p = func_02052208(p);
            PTR_AT(r, 8) = p;
            func_02051f80(PTR_AT(r, 8), id);
        }
        if (S32_AT(r, 4) <= -1)
            return;
        PTR_AT(SLOT(S32_AT(r, 4)), 0) = PTR_AT(r, 8);
        U32_AT(SLOT(S32_AT(r, 4)), 8)++;
        return;
    }
    if (i <= -1) {
        PTR_AT(r, 8) = NULL;
        return;
    }
    U32_AT(SLOT(i), 8)++;
    PTR_AT(r, 8) = PTR_AT(SLOT(S32_AT(r, 4)), 0);
}

/* 0x02052ee4: constructor
 * @difftest ptr:0xc:4 stub=0x02050fd8 cases=3 */
u8 *func_02052ee4(u8 *r)
{
    func_02050fd8(r);
    PTR_AT(r, 0) = data_020c0618;
    PTR_AT(r, 8) = NULL;
    return r;
}
