/*
 * The touch-screen menu (spells, part 1): the side tabs, showing the menu
 * and closing it.
 * ARM9 main, 0x0202f330 - 0x0202f910 (4 functions).
 *
 * Sprites: five spell icons (0x34 bytes each from data_020e3b24) and
 * eleven 0x78-byte entries from data_020e3c28 whose +0x10 is a text
 * actor. data_020be2d0 is the menu's sound handle.
 *
 * Testing: func_0202f3dc runs with the background loader func_0200aad0
 * stubbed (it reads a file from the card).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const char data_020be3c8[];   /* "B_Spells_BKG.bin" */
extern s32 data_020be2d0;
extern s32 data_020be3bc;
extern u8 data_020e3a80;
extern s32 data_020e3a84;
extern u8 data_020e3a8c;
extern u8 data_020e3a90;
extern s32 data_020e3a94;
extern s32 data_020e3a98;
extern u8 data_020e3b24[];
extern u8 data_020e3c28[];
extern u8 data_020e4150[];
extern s32 data_020e6054;

u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
s32 func_0204a868(u32 counter);
void *func_02011f44(void);
void func_020748e0(void *sprite, const s32 *pos);
void func_0202040c(s32 handle);
void func_02019b08(u32 state, u32 screen);
s32 func_02019ad0(u32 state);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02020440(u32 v);
void func_02053d1c(void *p);

void func_0202f6dc(s32 *out, const u8 *pad);

#define ICON(i) (data_020e3b24 + (i) * 0x34)
#define ENTRY(i) (data_020e3c28 + (i) * 0x78)

/* 0x0202f330: a touch at `pos` on the side tabs switches to that menu
 * @difftest $P=ptr:8:4 @$P+0:32=int:0xd0:0x100 @$P+4:32=int:0:0xd0 $P cases=60 */
void func_0202f330(const u32 *pos)
{
    u32 st = 0;

    if (pos[0] > 0xde) {
        u32 row = pos[1] / 0x30;
        /* (an if-chain: a switch becomes a lookup table in .rodata) */
        if (row == 0)
            st = 0xe;
        else if (row == 1)
            st = 0x14;
        else if (row == 2)
            st = 0x11;
        else if (row == 3)
            st = 0x17;
    }
    if (st == 0)
        return;
    if (func_02019ad0(st) != 0)
        return;
    func_02020440(0x45);
    func_0201b770(1, 0x3f, 1);
    func_02019b08(st, 1);
}

/* an owned spell's icon follows the stylus */
static void ShowIcon(u32 spell, s32 i, s32 *p)
{
    if (func_0204a868(spell) != 0) {
        func_0202f6dc(p, func_02011f44());
        func_020748e0(ICON(i), p);
    }
}

/* 0x0202f3dc: show the spells menu on the touch screen
 * @difftest stub=0x0200aad0 cases=5
 * @difftest @020df0fc:32=pick:0,1 stub=0x0200aad0 cases=10 */
void func_0202f3dc(void)
{
    s32 old = data_020df0fc;
    s32 p[5][2];
    s32 i;

    if (old != 1)
        data_020df0fc = 1;
    func_0200aad0(0, (u32)data_020be3c8, 0, 0, 0, 0);
    ShowIcon(0x10, 0, p[0]);
    ShowIcon(0xe, 1, p[1]);
    ShowIcon(0xf, 2, p[2]);
    ShowIcon(0x11, 3, p[3]);
    ShowIcon(0x12, 4, p[4]);
    for (i = 0; i < 11; i++) {
        u8 *t = PTR_AT(ENTRY(i), 0x10);
        if (t != NULL)
            U32_AT(t, 4) &= ~8u;
    }
    func_0202040c(data_020be2d0);
    data_020be2d0 = -1;
    data_020e3a94 = 0;
    data_020e3a84 = 0;
    data_020e3a80 = 0;
    data_020e3a90 = 0;
    data_020e3a8c = 0;
    data_020e3a98 = 0;
    data_020be3bc = -1;
    data_020e6054 = 0;
    data_020df0fc = old;
}

/* 0x0202f6dc: the touch position
 * @difftest ptr:8:4 ptr:0x80:2 cases=10 */
void func_0202f6dc(s32 *out, const u8 *pad)
{
    u32 y = U16_AT(pad, 0x74);

    out[0] = U16_AT(pad, 0x72);
    out[1] = y;
}

/* 0x0202f6f0: close the spells menu
 * @difftest @020e3a90:8=pick:0,1 cases=10 */
void func_0202f6f0(void)
{
    s32 i;

    if ((s8)data_020e3a90 == 0) {
        void *o = PTR_AT(data_020deebc, 8);
        PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = o;
        U32_AT(PTR_AT(data_020deebc, 8), 0x38) &= ~0x200u;
    }
    for (i = 0; i < 5; i++)
        ICON(i)[0x14] = 0;
    for (i = 0; i < 11; i++) {
        if (PTR_AT(ENTRY(i), 0x10) != NULL) {
            func_020044a0(PTR_AT(ENTRY(i), 0x10));
            PTR_AT(ENTRY(i), 0x10) = NULL;
        }
    }
    func_02053d1c(data_020e4150);
    func_0202040c(data_020be2d0);
    data_020be2d0 = -1;
}
