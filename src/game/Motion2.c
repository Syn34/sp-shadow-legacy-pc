/*
 * Motion.cpp, part 2: OAM entries and buffers, the per-screen OBJ palette
 * slots, clearing palettes/OAM/VRAM, and small appearance-loading wrappers.
 * ARM9 main, 0x0201a790 - 0x0201afd8 (29 functions).
 *
 * OBJ palette slots: data_020e1d70 + screen * 0x84: +0 u32 palette index
 * into a palette list, then 16 slots of 8 bytes at +4:
 *   +0 owner class (0xff: free), +1 owner kind, +2 users, +3 flags (bit 1
 *   kept), +4 palette list.
 * An actor using a slot has bit 10 of +0x110 set and the slot in bits
 * 12-15 of +0xac.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020df598[];           /* OAM buffers, see Motion1.c */
extern u8 data_020e1d70[];           /* OBJ palette slots */

void MIi_CpuClearFast(u32 data, void *dest, u32 size);
s32 func_0200941c(u32 a, u32 b, u32 c);
s32 func_02009488(u32 a, u32 b, u32 c);
void func_0201817c(u32 screen);
void func_0201a3cc(void);
void func_0201a420(void);
u32 func_0201b274(u32 a);
void func_0208fb78(void);
void func_0208ffd8(u32 banks);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void func_020a6dcc(void *arr, u32 n, u32 size, void *(*ctor)(void *), void *(*dtor)(void *));   /* __construct_array */

/* this file's own functions, called before their definition */
void *func_0201a964(u8 *obj);
void *func_0201a998(void *p);
void *func_0201a9e4(u32 *p);
s32 func_0201a9f4(const u32 *key);
void func_0201abb4(u8 *actor, const u32 *key);
void func_0201adfc(void);
void func_0201ae44(void);
void func_0201ae8c(void);
void func_0201aec8(void);
void func_0201aee8(void);

#define OAM_STATE() (data_020df598 + data_020df0fc * 0x13ec)
#define SLOTS() (data_020e1d70 + data_020df0fc * 0x84)
#define SLOT(i) (SLOTS() + 4 + (i) * 8)
#define USES_SLOT(a) ((s8)BITS(U32_AT(a, 0x110), 10, 1))

/* 0x0201a790: set the affine parameter index of an OAM entry
 * @difftest ptr:8:2 int:0:0x20 */
void func_0201a790(u16 *e, u32 v)
{
    e[1] &= ~0x3e00;
    e[1] |= v << 9;
}

/* 0x0201a7b0
 * @difftest ptr:8:2 */
u32 func_0201a7b0(const u16 *e) { return (u16)((e[1] >> 9) & 0x1f); }

/* 0x0201a7c8: put an entry in both OAM copies; -1 when OAM is full
 * @difftest $B=zero:0x800 @020df598:32=$B @020df0fc:32=pick:0 @020e0981:8=int:0x7c:0x82 int:0:0x80 ptr:8:4 */
s32 func_0201a7c8(u32 i, const u32 *entry)
{
    u8 *b;
    if (OAM_STATE()[0x13e9] > 0x7f)
        return -1;
    b = PTR_AT(OAM_STATE(), 0);
    U32_AT(b, i * 8) = entry[0];
    U32_AT(b + i * 8, 4) = entry[1];
    b = (u8 *)PTR_AT(OAM_STATE(), 0) + 0x400;
    U32_AT(b, i * 8) = entry[0];
    U32_AT(b + i * 8, 4) = entry[1];
    OAM_STATE()[0x13e9]++;
    return 0;
}

/* 0x0201a864: set up the OAM buffers of both screens */
void func_0201a864(void)
{
    func_0201817c(1);
    PTR_AT(OAM_STATE(), 0) = OAM_STATE() + 8;
    func_0201a420();
    func_0201a3cc();
    PTR_AT(OAM_STATE(), 4) = OAM_STATE() + 0x808;
    func_0201817c(0);
    PTR_AT(OAM_STATE(), 0) = OAM_STATE() + 8;
    func_0201a420();
    func_0201a3cc();
    PTR_AT(OAM_STATE(), 4) = OAM_STATE() + 0x808;
}

/* 0x0201a93c: destroy the OAM states
 * @difftest cases=5 */
void func_0201a93c(void)
{
    func_020a6d58(data_020df598, 2, 0x13ec, (void *(*)(void *))func_0201a964);
}

/* 0x0201a964: OAM state destructor
 * @difftest zero:0x1400 */
void *func_0201a964(u8 *obj)
{
    func_020a6d58(obj + 0x1008, 0x20, 0x18, func_0201a998);
    return obj;
}

/* 0x0201a998
 * @difftest u32 */
void *func_0201a998(void *p) { return p; }

/* 0x0201a99c: OAM state constructor */
void *func_0201a99c(u8 *obj)
{
    func_020a6dcc(obj + 0x1008, 0x20, 0x18, (void *(*)(void *))func_0201a9e4, func_0201a998);
    return obj;
}

/* 0x0201a9e4
 * @difftest ptr:8:4 */
void *func_0201a9e4(u32 *p)
{
    p[0] = 0;
    p[1] = 0;
    return p;
}

/* 0x0201a9f4: the palette slot holding `key`, else the first free one
 * (0xff when none)
 * @difftest $K=ptr:4:4 @$K+0:32=pick:1,2,3 $T=ptr:0x84:4 @020df0fc:32=pick:0 @$T+0:32=pick:0 $K */
s32 func_0201a9f4(const u32 *key)
{
    s32 found = 0xff;
    u32 i;
    for (i = 0; i <= 0xf; i++) {
        u8 *s = SLOT(i);
        if (s[3] & 2)
            continue;
        if (s[0] == 0xff) {
            if (found == 0xff)
                found = i;
            continue;
        }
        if (*key == *(u32 *)PTR_AT(s, 4)) {
            found = i;
            break;
        }
    }
    return found;
}

/* 0x0201aa88: release the actor's palette slot
 * @difftest $A=ptr:0x114:4 @$A+0x110:32=pick:0,0x400 $A @020df0fc:32=int:0:1 */
void func_0201aa88(void *obj)
{
    u8 *actor = obj;
    u32 slot;
    u8 *s;
    if (USES_SLOT(actor) == 0)
        return;
    U32_AT(actor, 0x110) &= ~0x400u;
    slot = (U32_AT(actor, 0xac) << 16) >> 28;
    s = SLOT(slot);
    if (s[2] == 0)
        return;
    s[2]--;
    if (s[2] != 0)
        return;
    if (!(SLOT(slot)[3] & 2))
        SLOT(slot)[0] = 0xff;
}

/* 0x0201ab30
 * @difftest $A=ptr:0x114:4 @$A+0x110:32=pick:0,0x400 $A @020df0fc:32=int:0:1 */
void func_0201ab30(u8 *actor)
{
    if (USES_SLOT(actor) == 0)
        return;
    func_0201aa88(actor);
}

/* 0x0201ab68: give the actor a slot for its own palette list (+0xe4) unless
 * it has one
 * @difftest @020df0fc:32=pick:0 @020e1d70:32=pick:0 $K=ptr:8:4 @$K+0:32=pick:0x02100000 $A=ptr:0x114:4 @$A+0x110:32=pick:0,0x400 @$A+0xe4:32=$K $A cases=60
 * @difftest $A=ptr:0x114:4 @$A+0x110:32=pick:0,0x400 @$A+0xe4:32=pick:0 $A cases=20 */
void func_0201ab68(u8 *actor)
{
    if (USES_SLOT(actor) != 0)
        return;
    if (U32_AT(actor, 0xe4) == 0)
        return;
    func_0201abb4(actor, PTR_AT(actor, 0xe4));
}

/* 0x0201abb4: load palette `key` into a slot for the actor
 * @difftest @020df0fc:32=pick:0 @020e1d70:32=pick:0 $A=ptr:0x114:4 @$A+0x110:32=pick:0,0x400 $K=ptr:8:4 @$K+0:32=pick:0x02100000 $A $K cases=60 */
void func_0201abb4(u8 *actor, const u32 *key)
{
    s32 slot;
    u8 *s;
    if (USES_SLOT(actor) != 0)
        func_0201aa88(actor);
    slot = func_0201a9f4(key);
    U32_AT(actor, 0xac) = (U32_AT(actor, 0xac) & ~0xf000u) | ((slot & 0xf) << 12);
    s = SLOT(slot);
    s[0] = U32_AT(actor, 0xc);
    s[1] = U32_AT(actor, 8);
    s[2]++;
    PTR_AT(s, 4) = (void *)key;
    func_0200bd20(key[U32_AT(SLOTS(), 0)], slot << 4, 0x10);
    U32_AT(actor, 0x110) |= 0x400;
}

/* 0x0201ac94: make the actor use palette slot `slot`
 * @difftest $A=ptr:0x114:4 @$A+0x110:32=pick:0,0x400 $A pick:0,3,15,0xff @020df0fc:32=int:0:1 */
void func_0201ac94(void *obj, s32 slot)
{
    u8 *actor = obj;
    if (slot == 0xff)
        return;
    if (USES_SLOT(actor) != 0)
        func_0201aa88(actor);
    U32_AT(actor, 0xac) = (U32_AT(actor, 0xac) & ~0xf000u) | ((slot & 0xf) << 12);
    SLOT(slot)[2]++;
    U32_AT(actor, 0x110) |= 0x400;
}

/* 0x0201ad2c: load palette `key` into slot `slot` */
void func_0201ad2c(u32 slot, const u32 *key)
{
    u8 *s = SLOT(slot);
    s[0] = slot;
    PTR_AT(SLOT(slot), 4) = (void *)key;
    func_0200bd20(key[U32_AT(SLOTS(), 0)], slot << 4, 0x10);
}

/* 0x0201ad94: free all palette slots of the current screen
 * @difftest @020df0fc:32=int:0:1 */
void func_0201ad94(void)
{
    u32 i;
    MI_CpuFill8(SLOTS(), 0, 0x84);
    for (i = 0; i < 0x10; i++)
        SLOT(i)[0] = 0xff;
}

/* 0x0201adfc: clear both palette RAMs (BG and OBJ) */
void func_0201adfc(void)
{
    MIi_CpuClearFast(0, (void *)0x05000000, 0x400);
    MIi_CpuClearFast(0, (void *)0x05000400, 0x400);
}

/* 0x0201ae44: clear both OAMs */
void func_0201ae44(void)
{
    MIi_CpuClearFast(0xc0, (void *)0x07000000, 0x400);
    MIi_CpuClearFast(0xc0, (void *)0x07000400, 0x400);
}

/* 0x0201ae8c: clear all VRAM */
void func_0201ae8c(void)
{
    func_0208ffd8(0x1ff);
    MIi_CpuClearFast(0, (void *)0x06800000, 0xa4000);
    func_0208fb78();
}

/* 0x0201aec8 */
void func_0201aec8(void)
{
    func_0201ae8c();
    func_0201ae44();
    func_0201adfc();
}

/* 0x0201aee8
 * @difftest */
void func_0201aee8(void) {}

/* 0x0201aeec */
void func_0201aeec(void)
{
    func_0201aee8();
    func_0201aec8();
}

/* 0x0201af08 */
s32 func_0201af08(u32 a, u32 b, u32 c) { return func_0200941c(func_0201b274(a) + 0x20, c, b); }

/* 0x0201af38
 * @difftest u32 u32 u32 cases=20 */
s32 func_0201af38(u32 a, u32 b, u32 c) { return func_02009488(func_0201b274(a) + 0x20, c, b); }

/* 0x0201af68 */
s32 func_0201af68(u32 a, u32 b) { return func_0200941c(func_0201b274(a), 0x20, b); }

/* 0x0201af88 */
s32 func_0201af88(u32 a, u32 b) { return func_02009488(func_0201b274(a), 0x20, b); }

/* 0x0201afa8 */
s32 func_0201afa8(u32 a) { return func_0200941c(0, 0x10, a); }

/* 0x0201afc0 */
s32 func_0201afc0(u32 a) { return func_02009488(0, 0x10, a); }
