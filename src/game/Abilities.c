/*
 * The hero's ability levels (data_020ebce4: 0x14 bytes, each 0-4, saved
 * with the game) and the attack buttons they allow.
 * ARM9 main, 0x0204a794 - 0x0204ab18 (10 functions).
 *
 * A new game starts with abilities 4, 7 and 8 at level 1. Ability 5 turns
 * the B attack (key 8 with key 0xb) into attack 0x10; otherwise keys 0xb,
 * 0xa and 0 give the attacks data_020bfcf0[0..2].
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const s8 data_020b6ae0[];
extern const u32 data_020bfcf0[3];   /* the attacks of keys 0xb, 0xa and 0 */
extern u8 data_020ebce4[0x14];       /* ability levels */

u32 func_02019b98(u32 v);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
void func_02051318(void *stream, s32 pos, s32 whence);
void func_02051464(void *stream, void *buf, u32 n);
void func_020513d0(void *stream, const void *buf, u32 n);
void *func_02011f44(void);
s32 func_02081cd0(void *pad, u32 key);
s32 func_02081d34(void *pad, u32 key);

s32 func_0204a868(u32 i);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))

/* 0x0204a794: set ability `i` to level `v` (1-3; 4 only for ability 2)
 * @difftest int:0:0x14 int:-1:6 stub=0x02019b98:3 cases=40
 * @difftest int:0:0x14 int:-1:6 stub=0x02019b98:0x17,0x0201b770,0x02019b08 cases=10 */
void func_0204a794(u32 i, s32 v)
{
    if (v < 4 && v > 0)
        data_020ebce4[i] = v;
    else if (i == 2 && v == 4)
        data_020ebce4[i] = v;
    if (func_02019b98(1) != 0x17)
        return;
    func_0201b770(1, 0x3f, 1);
    func_02019b08(0x17, 1);
}

/* 0x0204a80c: ability `i`'s level
 * @difftest int:0:0x14 cases=10 */
u32 func_0204a80c(u32 i)
{
    return data_020ebce4[i];
}

/* 0x0204a81c: raise (or lower) ability `i` by `d`
 * @difftest int:0:0x14 int:-2:3 @020ebce4:32=u32 @020ebce8:32=u32 @020ebcec:32=u32 @020ebcf0:32=u32 @020ebcf4:32=u32 cases=60
 * @difftest int:0:0x14 int:-2:3 @020ebce4:32=0x03020100 @020ebce8:32=0x04030201 @020ebcec:32=0x01000403 @020ebcf0:32=0x02040302 @020ebcf4:32=0x00010203 cases=60 */
void func_0204a81c(u32 i, s32 d)
{
    u32 c;

    if (d < 0) {
        c = data_020ebce4[i];
        if (c != 0) {
            data_020ebce4[i] = c + d;
            return;
        }
    }
    if (d <= 0)
        return;
    c = data_020ebce4[i];
    if (c < 4)
        data_020ebce4[i] = c + d;
}

/* 0x0204a868: does the hero have ability `i`?
 * @difftest int:0:0x14 cases=10 */
s32 func_0204a868(u32 i)
{
    return data_020ebce4[i] != 0;
}

/* 0x0204a88c: ability `i`'s level
 * @difftest int:0:0x14 cases=10 */
u32 func_0204a88c(u32 i)
{
    return data_020ebce4[i];
}

/* 0x0204a89c: the abilities in the save stream (new game: the first ones)
 * @difftest 0x020ebbc8 pick:0,1 @020ebbe4:32=pick:0,1 stub=0x020513d0,0x02051464 cases=6 */
void func_0204a89c(void *stream, s32 newgame)
{
    if (newgame != 0) {
        u8 *a = data_020ebce4;

        a[0] = 0;
        a[1] = 0;
        a[2] = 0;
        a[3] = 0;
        a[4] = 1;
        a[5] = 0;
        a[6] = 0;
        a[7] = 1;
        a[8] = 1;
        a[9] = 0;
        a[0xa] = 0;
        a[0xb] = 0;
        a[0xc] = 0;
        a[0xd] = 0;
        a[0xe] = 0;
        a[0xf] = 0;
        a[0x10] = 0;
        a[0x11] = 0;
        a[0x12] = 0;
        a[0x13] = 0;
        func_020513d0(stream, data_020ebce4, 0x14);
        return;
    }
    if (S32_AT(stream, 0x1c) == 0) {
        func_020513d0(stream, data_020ebce4, 0x14);
        return;
    }
    func_02051464(stream, data_020ebce4, 0x14);
}

/* 0x0204a96c: skip the abilities
 * @difftest 0x020ebbc8 stub=0x02051318 cases=3 */
void func_0204a96c(void *stream)
{
    func_02051318(stream, 0x14, 1);
}

/* the attacks of the buttons (`now`: just pressed rather than held) */
static u32 Attacks(s32 now)
{
    u32 r = 0;

    if ((s8)BITS(U32_AT(HERO(), 0x38), 23, 1) != 0) {
        if ((now ? func_02081d34(func_02011f44(), 0xb) : func_02081cd0(func_02011f44(), 0xb)) != 0) {
            if (func_02081cd0(func_02011f44(), 8) != 0 && func_0204a868(5) != 0)
                r |= 0x10;
            else
                r |= data_020bfcf0[0];
        }
        if ((now ? func_02081d34(func_02011f44(), 0xa) : func_02081cd0(func_02011f44(), 0xa)) != 0)
            r |= data_020bfcf0[1];
        if ((now ? func_02081d34(func_02011f44(), 0) : func_02081cd0(func_02011f44(), 0)) != 0)
            r |= data_020bfcf0[2];
    }
    return r;
}

/* 0x0204a980: the attacks of the buttons held
 * @difftest $H=zero:0x60 @$H+0x38:32=pick:0,0x800000 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x64:32=pick:0,1,0x10,0x11 @$K+0x5c:32=pick:0,1,0x10,0x11 @$K+0xc:32=pick:0,1,0x10,0x11 @$K+0x4c:32=pick:0,1,0x10,0x11 @020ebce9:8=pick:0,1 cases=60 */
u32 func_0204a980(void)
{
    return Attacks(0);
}

/* 0x0204aa44: the attacks of the buttons just pressed
 * @difftest $H=zero:0x60 @$H+0x38:32=pick:0,0x800000 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x64:32=pick:0,1,0x10,0x11 @$K+0x5c:32=pick:0,1,0x10,0x11 @$K+0xc:32=pick:0,1,0x10,0x11 @$K+0x4c:32=pick:0,1,0x10,0x11 @020ebce9:8=pick:0,1 cases=60 */
u32 func_0204aa44(void)
{
    return Attacks(1);
}

/* 0x0204ab08
 * @difftest int:0:8 cases=8 */
s32 func_0204ab08(u32 i)
{
    return data_020b6ae0[i];
}
