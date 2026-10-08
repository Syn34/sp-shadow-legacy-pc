/*
 * Actor.cpp, part 4: CActor virtual methods, update and constructor.
 * ARM9 main, 0x02007290 - 0x02007efc (14 functions).
 *
 * CActor vtable (data_020bcbb8): 0 ~CActor (func_02007d78), 1 deleting
 * destructor (func_02007d4c), 2 Update (func_020077c4), 3 SetActive
 * (func_02003f70). Actor +0x38 flags used here: bit 2 position changed,
 * bit 4, bit 13 on screen, bit 17/18 keep updating when off screen.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define A_FLAGS110(a) U32_AT(a, 0x110)
#define A_FLAGS38(a)  U32_AT(a, 0x38)

extern u8 data_020bcbb8[];           /* CActor vtable */
extern const s32 data_020bcb44, data_020bcb48, data_020bcb4c, data_020bcb50, data_020bcb54,
    data_020bcb58, data_020bcb5c, data_020bcb60, data_020bcb64, data_020bcb68;
extern s32 data_020f6f20[4];         /* 2D visible area */

void func_02002ecc(void *actor);
void func_02002fa4(void *actor);
void *func_020062e0(void);
s32 func_02019f58(const s32 *pos);
void func_0201ab30(void *actor);
void func_0201ab68(void *actor);
void func_0201c17c(s32 *out, const s32 *in);
void func_0205092c(void *actor);
void func_02050660(void *actor);
void *func_020506c0(void *actor);
void func_02051370(void *obj, s32 v);
s32 func_02051430(void *obj);
void func_02053440(void *model);
void func_02082ce8(void *cam);
void func_0207fe44(void *p);
void func_02007efc(void *actor);

/* this file's own functions, called before their definition */
void func_020073a0(void *a);
void func_02007490(void *a);
void func_02007520(void *a);
void func_02007af0(void *a);
void func_02007c20(void *a);

/* rectangle {top, left, bottom, right} inside the camera's visible box */
static s32 CameraSees(const s32 *r)
{
    u32 cam[0x2c0 / 4];
    s32 vis;
    func_020040f4((u8 *)cam, func_020062e0());
    if (S8_AT(cam, 0x284) != 0)
        vis = S32_AT(cam, 0x2ac) >= r[1] && S32_AT(cam, 0x2a4) <= r[3]
              && S32_AT(cam, 0x2a0) <= r[2] && S32_AT(cam, 0x2a8) >= r[0];
    else
        vis = 0;
    func_02082ce8(cam);
    return vis;
}

/* 0x02007290 */
void func_02007290(void *a, void *obj)
{
    if (S32_AT(a, 0xc) != 5)
        return;
    if (S32_AT(obj, 0x1c) == 0) {
        func_02051370(obj, BITS(A_FLAGS38(a), 4, 1) ? 1 : 0);
    } else if (func_02051430(obj) == 1) {
        A_FLAGS38(a) |= 0x10;
    } else {
        A_FLAGS38(a) &= ~0x10u;
    }
}

/* 0x02007330: move a dormant actor to the active list */
void func_02007330(void *a)
{
    PtrVec *from;
    A_FLAGS110(a) |= 8;
    if (BITS(A_FLAGS110(a), 1, 1))
        return;
    A_FLAGS110(a) |= 2;
    from = func_020061e0();
    func_020042b0(a, from, func_02006200());
}

/* 0x020073a0: integrate velocity and acceleration
 * @difftest $A=ptr:0x114:4 @$A+0x110:32=0 $A */
void func_020073a0(void *a)
{
    S32_AT(a, 0x70) += S32_AT(a, 0x7c);
    S32_AT(a, 0x74) += S32_AT(a, 0x80);
    S32_AT(a, 0x5c) = S32_AT(a, 0x100) + (S32_AT(a, 0x54) + S32_AT(a, 0x70));
    S32_AT(a, 0x60) = S32_AT(a, 0x104) + (S32_AT(a, 0x58) + S32_AT(a, 0x74));
    S32_AT(a, 0x100) = 0;
    S32_AT(a, 0x104) = 0;
    if (!BITS(A_FLAGS110(a), 8, 1))
        return;
    if (S32_AT(a, 0xc) == 0)
        func_02002ecc(a);
    else
        func_02002fa4(a);
}

/* 0x02007458
 * @difftest $M=ptr:0xa0:4 $A=ptr:0x48:4 @$A+0x44:32=$M $A */
s32 func_02007458(void *a) { return S32_AT(PTR_AT(a, 0x44), 0x9c); }

/* 0x02007464: run the actor's +0x9c callback
 * @difftest zero:0xa0 */
void func_02007464(void *a)
{
    void (*cb)(void *) = PTR_AT(a, 0x9c);
    if (cb != NULL)
        cb(a);
}

/* 0x02007490: give the 3D model the actor's ground heights */
void func_02007490(void *a)
{
    s32 t[2], u[2], h0;
    if (!BITS(A_FLAGS110(a), 8, 1) || PTR_AT(a, 0x44) == NULL)
        return;
    func_020038d4(t, (s32 *)((u8 *)a + 0x54), 0x10);
    h0 = func_02019f58(t);
    func_020038d4(u, (s32 *)((u8 *)a + 0x5c), 0x10);
    u[0] = func_02019f58(u);
    S32_AT(PTR_AT(a, 0x44), 0x98) = h0;
    S32_AT(PTR_AT(a, 0x44), 0x9c) = u[0];
}

/* 0x02007520: update the "on screen" flag (+0x38 bit 13) */
void func_02007520(void *a)
{
    s32 r[4];
    A_FLAGS38(a) &= ~0x2000u;
    r[0] = S32_AT(a, 0x10);
    r[1] = S32_AT(a, 0x14);
    r[2] = S32_AT(a, 0x18);
    r[3] = S32_AT(a, 0x1c);
    if (PTR_AT(a, 0x44) != NULL) {
        u8 *m = PTR_AT(a, 0x44);
        s32 my = S32_AT(m, 8), mx = S32_AT(m, 4);
        r[1] += mx;
        r[3] += mx;
        r[0] += my;
        r[2] += my;
        if (CameraSees(r))
            A_FLAGS38(a) |= 0x2000;
        return;
    }
    if (BITS(A_FLAGS110(a), 2, 1)) {
        A_FLAGS38(a) |= 0x2000;
        return;
    }
    {
        s32 p[2];
        p[1] = 0;
        p[0] = 0;
        func_02003890(a, p);
        r[0] += p[1];
        r[2] += p[1];
        r[1] += p[0];
        r[3] += p[0];
        if (r[3] >= data_020f6f20[1] && r[1] <= data_020f6f20[3] && r[0] <= data_020f6f20[2]
            && r[2] >= data_020f6f20[0])
            A_FLAGS38(a) |= 0x2000;
    }
}

/* 0x02007708: draw the 3D model if on screen */
void func_02007708(void *a)
{
    u32 active = BITS(U32_AT(a, 4), 3, 1);
    u8 *link;
    if (active && BITS(A_FLAGS38(a), 13, 1)) {
        func_02053440(PTR_AT(a, 0x44));
        return;
    }
    if (!active || !BITS(A_FLAGS38(a), 17, 1))
        return;
    link = PTR_AT(a, 0xa4);
    if (link == NULL || !BITS(U32_AT(link, 0x38), 13, 1))
        return;
    func_02053440(PTR_AT(a, 0x44));
}

/* 0x020077c4: CActor::Update (vtable slot 2) */
void func_020077c4(void *a, s32 arg)
{
    u32 f = A_FLAGS110(a), g;

    if (BITS(f, 3, 1) && !BITS(f, 7, 1)) {
        func_020044a0(a);
        goto done;
    }
    func_02007520(a);
    if (PTR_AT(a, 0x44) != NULL)
        func_02007af0(a);
    else
        func_02007c20(a);

    g = A_FLAGS38(a);
    if ((BITS(g, 13, 1) || BITS(g, 18, 1) || BITS(g, 17, 1)) && !BITS(U32_AT(a, 4), 2, 1)) {
        if (BITS(U32_AT(a, 4), 3, 1)) {
            func_020073a0(a);
            func_02007490(a);
        }
        if (PTR_AT(a, 0x94) != NULL)
            ((void (*)(void *))PTR_AT(a, 0x94))(a);
    } else {
        S32_AT(a, 0x5c) = S32_AT(a, 0x54);
        S32_AT(a, 0x60) = S32_AT(a, 0x58);
    }

    g = A_FLAGS38(a);
    if (!BITS(g, 13, 1) && !BITS(g, 18, 1) && S32_AT(a, 0xc) == 4) {
        void *(*get)(void *, void *) = (void *(*)(void *, void *))(*(void ***)a)[0x38 / 4];
        u8 *o = get(a, (void *)get);
        if (!BITS(U32_AT(o, 0x24), 6, 1)) {
            s32 r[4], spawn[2], t[2];
            r[0] = SHL(S32_AT(a, 0x10), 12);
            r[1] = SHL(S32_AT(a, 0x14), 12);
            r[2] = SHL(S32_AT(a, 0x18), 12);
            r[3] = SHL(S32_AT(a, 0x1c), 12);
            spawn[0] = S32_AT(a, 0x30);
            spawn[1] = S32_AT(a, 0x34);
            func_0201c17c(t, spawn);
            r[0] += t[1];
            r[2] += t[1];
            r[1] += t[0];
            r[3] += t[0];
            if (!CameraSees(r))
                func_02003be8(a);
        }
    }

done:
    if (BITS(A_FLAGS38(a), 2, 1) && (BITS(A_FLAGS38(a), 13, 1) || BITS(A_FLAGS38(a), 17, 1))) {
        s32 t[2];
        func_020038d4(t, (s32 *)((u8 *)a + 0x5c), 4);
        S32_AT(a, 0x28) = t[0];
        S32_AT(a, 0x2c) = t[1];
        func_02050660(a);
    }
    A_FLAGS38(a) &= ~0x400000u;
    A_FLAGS38(a) &= ~0x400u;
    A_FLAGS38(a) &= ~0x8u;
}

/* 0x02007af0: queue an on-screen model of kind 0x1f */
void func_02007af0(void *actor)
{
    void *self = actor;
    u32 cap;
    PtrVec *v;
    if (S32_AT(self, 0xc) != 4 || !BITS(A_FLAGS38(self), 13, 1) || BITS(A_FLAGS38(self), 18, 1))
        return;
    if (U8_AT(PTR_AT(self, 0x44), 0x94) != 0x1f)
        return;
    cap = func_020061c0()->cap;
    if ((u32)func_020061c0()->count >= cap)
        return;
    v = func_020061c0();
    if ((u32)v->count < (u32)v->cap) {
        v->count++;
        v->items[v->count - 1] = self;
    } else {
        func_020062f0(v, &self, 0);
    }
}

/* 0x02007c20: 2D part of the update: sprite VRAM, animation */
void func_02007c20(void *a)
{
    u32 f, g;
    if (!BITS(A_FLAGS38(a), 13, 1))
        func_02003740(a);
    f = A_FLAGS110(a);
    if (BITS(f, 9, 1)) {
        if (BITS(A_FLAGS38(a), 13, 1)) {
            if (!BITS(f, 10, 1))
                func_0201ab68(a);
        } else if (BITS(f, 10, 1)) {
            func_0201ab30(a);
        }
    }
    if (!BITS(U32_AT(a, 4), 3, 1) || BITS(U32_AT(a, 4), 2, 1))
        return;
    g = A_FLAGS38(a);
    if (!BITS(g, 13, 1) && !BITS(g, 18, 1) && !BITS(g, 17, 1))
        return;
    func_020030d0(a);
    if (U16_AT(a, 0xda) == 0)
        return;
    func_02007efc(a);
}

/* 0x02007d4c: CActor deleting destructor (vtable slot 1)
 * @difftest zero:0x114 cases=20 */
void *func_02007d4c(void *a)
{
    PTR_AT(a, 0) = data_020bcbb8;
    func_020506c0(a);
    func_0207fe44(a);
    return a;
}

/* 0x02007d78: CActor::~CActor (vtable slot 0) */
void *func_02007d78(void *a)
{
    PTR_AT(a, 0) = data_020bcbb8;
    func_020506c0(a);
    return a;
}

/* 0x02007d9c: CActor::CActor
 * @difftest ptr:0x114:4 */
void *func_02007d9c(void *a)
{
    func_0205092c(a);
    PTR_AT(a, 0) = data_020bcbb8;
    S32_AT(a, 0x4c) = -1;
    S32_AT(a, 0x50) = 0;
    S32_AT(a, 0x54) = data_020bcb4c;
    S32_AT(a, 0x58) = data_020bcb48;
    S32_AT(a, 0x5c) = data_020bcb68;
    S32_AT(a, 0x60) = data_020bcb64;
    S32_AT(a, 0x68) = 0;
    S32_AT(a, 0x6c) = 0;
    S32_AT(a, 0x70) = data_020bcb60;
    S32_AT(a, 0x74) = data_020bcb50;
    S32_AT(a, 0x78) = 0;
    S32_AT(a, 0x7c) = data_020bcb5c;
    S32_AT(a, 0x80) = data_020bcb58;
    S32_AT(a, 0x84) = 0;
    S32_AT(a, 0x94) = 0;
    S32_AT(a, 0x98) = 0;
    S32_AT(a, 0x9c) = 0;
    S32_AT(a, 0xa0) = 0;
    S32_AT(a, 0xa4) = 0;
    S32_AT(a, 0xc8) = 0;
    S32_AT(a, 0xcc) = 0;
    S32_AT(a, 0xd0) = 0;
    S32_AT(a, 0xd4) = 0;
    S32_AT(a, 0xe8) = 0;
    S32_AT(a, 0xec) = 0;
    S32_AT(a, 0xf0) = 0;
    S32_AT(a, 0xf4) = 0;
    S32_AT(a, 0xf8) = 0;
    S32_AT(a, 0x100) = data_020bcb44;
    S32_AT(a, 0x104) = data_020bcb54;
    U8_AT(a, 0x108) = 0;
    U8_AT(a, 0x109) = 0;
    U8_AT(a, 0x10a) = 0;
    U8_AT(a, 0x10b) = 0;
    U8_AT(a, 0x10c) = 0;
    U8_AT(a, 0x10d) = 0;
    U8_AT(a, 0x10e) = 0;
    MI_CpuFill8((u8 *)a + 0x88, 0, 0xc);
    MI_CpuFill8((u8 *)a + 0xa8, 0, 0x40);
    U32_AT(a, 0x110) = 0;
    return a;
}
