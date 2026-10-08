/*
 * Fbfsub.cpp: animated background tiles (water, lava, torches...).
 * ARM9 main, 0x02011f54 - 0x020126d0 (11 functions).
 *
 * data_020dad5c holds up to 32 animations of 0x14 bytes, data_020dad60
 * of them in use:
 *   +0x00 owner actor, +0x04 animation (data_020b512c[+0x08]),
 *   +0x09 frame, +0x0a previous frame, +0x0b frames left,
 *   +0x0c flags (1 stopped, 2 running, 4 runs only while the game runs),
 *   +0x0e/+0x10 tile position.
 * An animation is {u32 count; frame *frames[]}; a frame is
 * {u16 n; u8 duration (0xff loop, 0xfe stop); u8 layer mask; {u16 x, y}[n]}
 * and copies n tiles from the given source positions to the animation's
 * place on the BG layers in the mask (bit 4 = the collision layer).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern void *data_020b512c[];        /* animations */
extern const char data_020bd048[];   /* "Fbfsub.cpp" */
extern u8 *data_020dad5c;
extern u32 data_020dad60;

u8 *func_02008994(s32 i, s32 k);
void func_02002218(void *actor, u32 v);
u32 func_020186ec(void);
u32 func_0201a000(u32 x, u32 y);
void func_0201a050(u32 x, u32 y, u32 v);
u32 func_0201bb08(u32 x, u32 y, u32 layer);
void func_0201bb24(u32 x, u32 y, u32 tile, u32 layer);

/* this file's own functions, called before their definition */
void func_02011f54(u8 *e);
u8 *func_02012150(u32 x, u32 y);
void func_020122c8(u32 x, u32 y);
void func_02012354(u32 x, u32 y);
void func_02012400(void *owner, u32 anim, u32 x, u32 y, u32 flags);

#define ANIM(i) (data_020dad5c + (i) * 0x14)
#define FRAME(e, f) ((u8 *)((void **)PTR_AT(e, 4))[(f) + 1])
#define TILE_POS(v) (((u32)(v) & 0x7ff) << 5)

/* 0x02011f54: show the current frame and step to the next one
 * @difftest $F=ptr:0x20:4 @$F+0:16=int:0:4 @$F+2:8=pick:1,2,0xfe @$F+3:8=u8 $A=ptr:0x10:4 @$A+4:32=$F @$A+8:32=$F $E=ptr:0x14:4 @$E+4:32=$A @$E+9:8=int:0:1 @$E+0xc:8=pick:0,1,2 $E cases=150 */
void func_02011f54(u8 *e)
{
    u8 *frame;
    u32 n, mask, i, x0, y0;

    e[0xa] = e[9];
    e[0xb] = FRAME(e, e[9])[2];
    frame = FRAME(e, e[9]);
    n = U16_AT(frame, 0);
    mask = frame[3];
    if (e[0xb] == 0xff) {
        e[9] = 0;
        e[0xb] = FRAME(e, e[9])[2];
        func_02011f54(e);
        return;
    }
    if (e[0xb] == 0xfe) {
        e[0xc] |= 1;
        return;
    }
    x0 = TILE_POS(U16_AT(frame, 4));
    y0 = TILE_POS(U16_AT(frame, 6));
    for (i = 0; i < n; i++) {
        u8 *t = FRAME(e, e[9]) + i * 4;
        u32 x = TILE_POS(U16_AT(t, 4)), y = TILE_POS(U16_AT(t, 6));
        u32 dx = x - x0, dy = y - y0, layer;
        for (layer = 0; layer < 4; layer++) {
            if ((1u << layer) & mask)
                func_0201bb24((u16)(U16_AT(e, 0xe) + dx), (u16)(U16_AT(e, 0x10) + dy),
                              (u16)func_0201bb08(x, y, layer), layer);
        }
        if ((1u << layer) & mask)
            func_0201a050((u16)(U16_AT(e, 0xe) + dx), (u16)(U16_AT(e, 0x10) + dy),
                          (u16)func_0201a000(x, y));
    }
    if (!(e[0xc] & 1))
        e[9]++;
}

/* 0x02012150: the animation at a tile position
 * @difftest $T=ptr:0x80:4 @020dad5c:32=$T @020dad60:32=int:0:6 @$T+0xe:16=pick:5,6 @$T+0x10:16=pick:7,8 @$T+0x22:16=pick:5,6 @$T+0x24:16=pick:7,8 @$T+0x36:16=pick:5,6 @$T+0x38:16=pick:7,8 pick:5,6 pick:7,8 */
u8 *func_02012150(u32 x, u32 y)
{
    u32 i;
    u8 *e = data_020dad5c;
    for (i = 0; i < data_020dad60; i++, e += 0x14) {
        if (U16_AT(e, 0xe) == x && U16_AT(e, 0x10) == y)
            return e;
    }
    return NULL;
}

/* 0x020121c0: remove all animations
 * @difftest $T=ptr:0x280:4 @020dad5c:32=$T */
void func_020121c0(void)
{
    MI_CpuFill8(data_020dad5c, 0, 0x280);
    data_020dad60 = 0;
}

/* 0x020121fc: run the animations
 * @difftest cases=50 */
void func_020121fc(void)
{
    u32 i;
    for (i = 0; i < data_020dad60; i++) {
        if (ANIM(i)[0xb] == 0)
            continue;
        if (func_020186ec() != 1) {
            if (func_020186ec() == 1)
                continue;
            if (ANIM(i)[0xc] & 4)
                continue;
        }
        if (!(ANIM(i)[0xc] & 2))
            continue;
        ANIM(i)[0xb]--;
        if (ANIM(i)[0xb] == 0)
            func_02011f54(ANIM(i));
    }
}

/* 0x020122c8: (re)start the animation at a tile position */
void func_020122c8(u32 x, u32 y)
{
    u8 *e = func_02012150(x, y);
    e[0xc] &= ~1;
    if (e[9] > U32_AT(PTR_AT(e, 4), 0))
        e[9] = 0;
    e[0xb] = FRAME(e, e[9])[2];
    if (e[0xb] != 0xff)
        return;
    e[9] = 0;
    e[0xb] = FRAME(e, e[9])[2];
}

/* 0x02012354: stop the animation at a tile position */
void func_02012354(u32 x, u32 y)
{
    u8 *e = func_02012150(x, y);
    e[0xc] |= 1;
}

/* 0x0201237c: pause or resume the animations of an actor
 * @difftest $T=ptr:0x80:4 @020dad5c:32=$T @020dad60:32=int:0:6 @$T+0:32=pick:1,2 @$T+0x14:32=pick:1,2 @$T+0x28:32=pick:1,2 pick:1,2 pick:0,1 */
void func_0201237c(void *owner, s32 on)
{
    u32 i;
    for (i = 0; i < data_020dad60; i++) {
        u8 *e = ANIM(i);
        if (PTR_AT(e, 0) == owner) {
            if (on != 0)
                e[0xc] |= 2;
            else
                e[0xc] &= ~2;
        }
    }
}

/* 0x02012400: add an animation
 * @difftest $T=ptr:0x80:4 @020dad5c:32=$T @020dad60:32=int:0:5 u32 int:0:8 u32 u32 u32 */
void func_02012400(void *owner, u32 anim, u32 x, u32 y, u32 flags)
{
    u8 *e;
    PTR_AT(ANIM(data_020dad60), 0) = owner;
    PTR_AT(ANIM(data_020dad60), 4) = data_020b512c[anim];
    e = ANIM(data_020dad60);
    e[8] = anim;
    e[9] = 0;
    e[0xa] = 0;
    e[0xb] = 1;
    e[0xc] = flags;
    U16_AT(e, 0xe) = x;
    U16_AT(e, 0x10) = y;
    data_020dad60++;
}

/* 0x020124dc: update of the actor that owns animations
 * @difftest $A=ptr:0x120:4 @$A+0x10e:8=pick:0,1,5 $A */
void func_020124dc(u8 *self)
{
    switch (self[0x10e]) {
    case 1:
        U32_AT(self, 0x110) |= 8;
        break;
    case 2:
        func_02002218(self, 0);
        break;
    case 3:
        func_020122c8(S32_AT(self, 0x54) >> 16, S32_AT(self, 0x58) >> 16);
        func_02002218(self, 0);
        break;
    case 4:
        func_02012354(S32_AT(self, 0x54) >> 16, S32_AT(self, 0x58) >> 16);
        func_02002218(self, 0);
        break;
    }
}

/* 0x02012580: spawn object `index` of `group` as an animated tile owner */
u8 *func_02012580(s32 group, s32 index)
{
    u8 *d = func_02008994((s8)group, index);
    u8 *a = func_02005cd0(S32_AT(d, 0));
    u32 flags = 0;

    func_02003b98(a, S16_AT(d, 4), S16_AT(d, 6));
    U32_AT(a, 0x38) |= 0x40000;
    U32_AT(a, 0x38) &= ~4u;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x20u) | ((d[0xb] & 1u) << 5);
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x40u) | ((d[0xc] & 1u) << 6);
    if (BITS(U32_AT(a, 0x38), 6, 1) && d[0xd] != 0)
        flags = (flags | 4) & 0xff;
    if (d[0xe] != 0)
        flags = (flags | 2) & 0xff;
    func_02012400(a, U16_AT(d, 8), S16_AT(d, 4), S16_AT(d, 6), flags);
    if (d[0xa] == 0)
        func_02012354(S16_AT(d, 4), S16_AT(d, 6));
    else
        func_020122c8(S16_AT(d, 4), S16_AT(d, 6));
    PTR_AT(a, 0x94) = (void *)func_020124dc;
    return a;
}

/* 0x0201268c: allocate the animation table */
void func_0201268c(void)
{
    data_020dad5c = func_0207ff48(0x280, data_020bd048, 0x46);
    data_020dad60 = 0;
}
