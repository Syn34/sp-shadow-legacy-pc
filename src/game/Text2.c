/*
 * Text.cpp, part 2: floating score numbers ("popups") shown above the
 * characters, plus a few small helpers.
 * ARM9 main, 0x02029364 - 0x020298d8 (12 functions).
 *
 * Popups: data_020e371c, one per entry of the root actor list (8 slots
 * of 0x18 bytes): +0x00 u64 time to hide it, +0x08 its text actor, +0x0c
 * vertical offset (rises 3 pixels per frame), +0x10/+0x14 screen x, y.
 * The root actor list is g_GameRoot {items, count}; entry 1 is the hero.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: actor base */
extern u8 data_020bdf70[];           /* RTTI: C3dBaseActor */
extern u8 data_020bde90[];
extern const s32 data_020bde98[];    /* fx32 scale of the velocity lead */
extern const char data_020bded8[];   /* "0" */
extern char data_020bdedc[];         /* number text buffer */
extern u8 data_020e371c[];
extern PtrVec g_GameRoot;           /* the root actor list */

void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */
void *func_02001c28(u32 *args);
void func_02001c04(void *text, ...);
void func_02005b98(s32 *out, const void *src);
void func_020829ec(s32 *out, const void *cam, const s32 *pos);
void func_02082ce8(void *cam);
void func_02077f34(void *obj);
s8 func_0202a724(void);

void func_02029450(u8 *self);
void func_02029790(s32 *out, const s32 *a, const s32 *b);
void func_020297d4(s32 *out, const s32 *v, const s32 *s);
void func_02029844(s32 *out, const void *obj);
void func_02029860(s32 *out, const void *obj);

#define POPUP(i) (data_020e371c + (i) * 0x18)

/* 0x02029364
 * @difftest cases=2 */
void func_02029364(void)
{
}

/* 0x02029368
 * @difftest u8 cases=10 */
void func_02029368(u8 v)
{
    data_020bde90[0] = v;
}

/* 0x02029378
 * @difftest cases=5 */
void func_02029378(void)
{
    if (func_0202a724() == 0)
        return;
    func_02077f34(g_GameRoot.items[0]);
}

/* 0x020293b4: per-frame update of a popup: rise, or hide it when its time
 * is up
 * @difftest $X=ptr:0x18:4 @$X+8:32=0 $X cases=5
 * @difftest $A=ptr:0x200:4 $V=ptr:0x10:4 @$A+0:32=$V @$V+0xc:32=0x02070a18 $X=ptr:0x18:4 @$X+8:32=$A @$X+4:32=pick:0,0xffffffff $X cases=60 */
void func_020293b4(u8 *self)
{
    u8 *a;
    u64 t, end;

    if (PTR_AT(self, 8) == NULL)
        return;
    t = func_020822c8(func_020062d0());
    end = ((u64)U32_AT(self, 4) << 32) | U32_AT(self, 0);
    if (t > end) {
        a = PTR_AT(self, 8);
        U32_AT(a, 4) &= ~8u;
        a = PTR_AT(self, 8);
        ((void (*)(void *, s32))PTR_AT(PTR_AT(a, 0), 0xc))(a, 1);
        return;
    }
    S32_AT(self, 0xc) -= 3;
    a = PTR_AT(self, 8);
    func_02003b98(a, S32_AT(self, 0x10) - (U16_AT(a, 0xb0) >> 1), S32_AT(self, 0x14) + S32_AT(self, 0xc));
}

/* 0x02029450: (re)create the text actor of a popup
 * @difftest $X=zero:0x18 $X cases=10 */
void func_02029450(u8 *self)
{
    u32 args[1];
    u8 *a;

    if (PTR_AT(self, 8) != NULL) {
        func_020044a0(PTR_AT(self, 8));
        PTR_AT(self, 8) = NULL;
    }
    args[0] = (u32)data_020bded8;
    func_0201391c(1, 0, 0);
    PTR_AT(self, 8) = func_02001c28(args);
    a = PTR_AT(self, 8);
    U32_AT(a, 4) |= 8;
    a = PTR_AT(self, 8);
    U32_AT(a, 0x38) |= 0x2000;
    a = PTR_AT(self, 8);
    U32_AT(a, 0x110) |= 4;
    func_0201ac94(PTR_AT(self, 8), 8);
    func_02003b98(PTR_AT(self, 8), 0x80, 0x60);
}

/* 0x02029500: show `value` above actor `who` (NULL or not in the list:
 * the slot after the last actor); for anyone but the hero it is placed
 * ahead of the hero's position by 10 frames of the hero's velocity
 * @difftest $T=zero:16 @$T+4:32=0x020bdf70 $W=ptr:0x100:4 @$W+0:32=$T+8 $S=ptr:0x100:4 $R=ptr:0x10:4 @020e36c8:32=$R @020e36cc:32=2 @$R+0:32=$S $H=ptr:0x100:4 @$H+0x44:32=$S @$R+4:32=$H @020e3754:32=0 $W int:-2000:100000 cases=40
 * @difftest $T=zero:16 @$T+4:32=0x020bdf70 $W=ptr:0x100:4 @$W+0:32=$T+8 $S=ptr:0x100:4 $R=ptr:0x10:4 @020e36c8:32=$R @020e36cc:32=2 @$R+0:32=$S @$R+4:32=$W @$W+0x44:32=$S @020e373c:32=0 $W int:-2000:100000 cases=40 */
void func_02029500(void *who, s32 value)
{
    s32 scr[2];
    s32 pos[3], vel[3], d[3], sum[3];
    u32 cam[0x2c0 / 4];
    u8 *e, *a;
    void **p, **end;
    u32 i;
    u64 t;

    if (who != NULL)
        who = func_020a6efc(who, data_020bcb88, data_020bdf70, -1);
    i = 0;
    p = g_GameRoot.items;
    end = g_GameRoot.items + g_GameRoot.count;
    for (; p != end; p++, i++) {
        if (*p == who)
            break;
    }
    e = POPUP(i);
    if (PTR_AT(e, 8) == NULL) {
        func_02029450(e);
        a = PTR_AT(e, 8);
        U32_AT(a, 0x38) |= 0x20;
        a = PTR_AT(e, 8);
        U32_AT(a, 0x38) |= 0x40;
    }
    if (PTR_AT(e, 8) == NULL)
        return;
    func_02029860(pos, g_GameRoot.items[1]);
    if (who != g_GameRoot.items[1]) {
        func_02029844(vel, g_GameRoot.items[1]);   /* the hero's velocity */
        func_020297d4(d, vel, data_020bde98);
        func_02029790(sum, pos, d);
        pos[0] = sum[0];
        pos[1] = sum[1];
        pos[2] = sum[2];
        S32_AT(e, 0xc) = -5;
        func_0201391c(3, 0, 0);
    } else {
        func_0201391c(3, 2, 0);
        S32_AT(e, 0xc) = -30;
    }
    func_02028678(value, data_020bdedc);
    func_02001c04(PTR_AT(e, 8), data_020bdedc);
    a = PTR_AT(e, 8);
    U32_AT(a, 4) |= 8;
    a = PTR_AT(e, 8);
    U32_AT(a, 0x38) |= 0x2000;
    a = PTR_AT(e, 8);
    U32_AT(a, 0x110) |= 4;
    func_020040f4((u8 *)cam, func_020062e0());
    func_020829ec(scr, cam, pos);
    S32_AT(e, 0x10) = scr[0];
    S32_AT(e, 0x14) = 0xc0 - scr[1];
    a = PTR_AT(e, 8);
    func_02003b98(a, S32_AT(e, 0x10) - (U16_AT(a, 0xb0) >> 1), S32_AT(e, 0x14) + S32_AT(e, 0xc));
    t = func_020822c8(func_020062d0()) + 0xc9168;
    U32_AT(e, 0) = (u32)t;
    U32_AT(e, 4) = (u32)(t >> 32);
    func_02082ce8(cam);
}

/* 0x02029790: out = a + b (3 x fx32)
 * @difftest ptr:12:4 ptr:12:4 ptr:12:4 */
void func_02029790(s32 *out, const s32 *a, const s32 *b)
{
    s32 x = a[0] + b[0];
    s32 y = a[1] + b[1];
    s32 z = a[2] + b[2];

    out[0] = x;
    out[1] = y;
    out[2] = z;
}

/* 0x020297d4: out = v * s (3 x fx32, rounded)
 * @difftest ptr:12:4 ptr:12:4 ptr:4:4 */
void func_020297d4(s32 *out, const s32 *v, const s32 *s)
{
    s32 k = *s;
    s32 z = (s32)(((s64)v[2] * k + 0x800) >> 12);
    s32 y = (s32)(((s64)v[1] * k + 0x800) >> 12);
    s32 x = (s32)(((s64)v[0] * k + 0x800) >> 12);

    out[0] = x;
    out[1] = y;
    out[2] = z;
}

/* 0x02029844: an actor's velocity
 * @difftest ptr:12:4 ptr:0x60:4 */
void func_02029844(s32 *out, const void *obj)
{
    out[0] = S32_AT(obj, 0x50);
    out[1] = S32_AT(obj, 0x54);
    out[2] = S32_AT(obj, 0x58);
}

/* 0x02029860: an actor's position
 * @difftest $S=ptr:0x100:4 $O=ptr:0x48:4 @$O+0x44:32=$S ptr:12:4 $O */
void func_02029860(s32 *out, const void *obj)
{
    func_02005b98(out, PTR_AT(obj, 0x44));
}

/* 0x02029870: destroy all popups
 * @difftest cases=5 */
void func_02029870(void)
{
    s32 i;

    for (i = 0; i < 8; i++) {
        u8 *e = POPUP(i);
        if (PTR_AT(e, 8) != NULL) {
            func_020044a0(PTR_AT(e, 8));
            PTR_AT(e, 8) = NULL;
        }
    }
}

/* 0x020298b0: forget all popups
 * @difftest cases=5 */
void func_020298b0(void)
{
    s32 i;

    for (i = 0; i < 8; i++)
        PTR_AT(POPUP(i), 8) = NULL;
}
