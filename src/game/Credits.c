/*
 * The credits: pages of text lines (game texts 0x123..0x208, pages
 * separated by "PageBreak") faded in and out on the touch screen, then the
 * title (or, after the true ending, the epilogue).
 * ARM9 main, 0x0203df2c - 0x0203e3d4 (6 functions).
 *
 * data_020bed70 is the state (0 fading in and holding, 1 next page, 2
 * waiting), data_020bed74 the next text, data_020eb754 / data_020eb750 the
 * fade and hold timers.
 *
 * Testing: with the state switch, the music player and the background
 * loader stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT / colour value passed by value in a register */

extern Cnt data_020aaca0, data_020aaca4, data_020aaca8;
extern s32 data_020bed70;
extern u32 data_020bed74;
extern const char data_020bed78[];   /* "PageBreak" */
extern const char data_020bed84[];   /* "NintendoLicense_BKG.bin" */
extern const char data_020bed9c[];   /* "Logo_Amaze_TS.bin" */
extern u8 data_020d9f9c[];
extern s8 data_020df114;
extern s32 data_020eb750;
extern s32 data_020eb754;

void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
void func_02019640(u32 v);
void func_02019660(u32 v);
const char *func_02028a20(u32 id);
s32 func_02080bc4(const char *a, const char *b);                /* strcmp */
u32 func_0201341c(u32 tile, s32 x, s32 y, const u8 *str, u32 align);
void func_02020194(void);
void func_020202f0(u32 v);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af18(s32 layer);
void func_0200af08(s32 layer, u32 v);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_02013a5c(Cnt color);

void func_0203e0e8(void);

/* 0x0203df2c: the credits' per-frame update
 * @difftest @020bed70:32=int:0:3 @020bed74:32=pick:0x123,0x208,0x209,0x20a @020eb750:32=pick:-5,5 @020eb754:32=pick:-5,5 @020df114:8=pick:0,1 @020da09c:8=pick:0,1 stub=0x02019b08 cases=100 */
void func_0203df2c(void)
{
    switch (data_020bed70) {
    case 0:
        data_020eb754 -= func_020822c0(func_020062d0());
        if (data_020eb754 > 0)
            return;
        data_020eb750 = 0x266;
        func_0200af08(0, 0);
        if (data_020bed74 < 0x209) {
            data_020bed70 = 2;
            return;
        }
        if (data_020df114 != 0 && data_020d9f9c[0x100] == 1) {
            func_0201b770(2, 0x3f, 1);
            func_02019b08(0x1e, 0);
            func_02019b08(0x14, 1);
            func_02019660(9);
            func_02019640(4);
            return;
        }
        func_0201b770(2, 0x3f, 1);
        func_02019b08(3, 0);
        func_02019b08(8, 1);
        return;
    case 1:
        func_0203e0e8();
        data_020bed70 = 0;
        return;
    case 2:
        data_020eb750 -= func_020822c0(func_020062d0());
        if (data_020eb750 > 0)
            return;
        data_020eb754 = 0x4000;
        data_020bed70 = 1;
        return;
    }
}

/* 0x0203e0e8: draw the next page, centred vertically
 * @difftest @020bed74:32=pick:0x123,0x150,0x1a0,0x200,0x207,0x209 cases=30 */
void func_0203e0e8(void)
{
    u32 start = data_020bed74, tile = 1;
    const char *s;
    s32 y;

    /* count the page's lines */
    data_020bed74 = start + 1;
    s = func_02028a20(start);
    while (func_02080bc4(s, data_020bed78) != 0 && data_020bed74 < 0x209)
        s = func_02028a20(data_020bed74++);
    y = ((s32)(0x10 - (data_020bed74 - start)) >> 1) * 0xc;
    data_020bed74 = start;
    data_020bed74 = start + 1;

    s = func_02028a20(start);
    while (func_02080bc4(s, data_020bed78) != 0 && data_020bed74 <= 0x209) {
        tile = func_0201341c(tile, 0x80, y, (const u8 *)s, 1);
        s = func_02028a20(data_020bed74++);
        y += 0xc;
    }
}

/* 0x0203e204: set up the credits
 * @difftest stub=0x0200aad0 cases=5 */
void func_0203e204(void)
{
    Cnt cnt;

    func_0200afb0(0, 0);
    func_0200b168(3, data_020aaca4.v);
    func_0200af18(3);
    func_0200b0ac(3);
    cnt = data_020aaca8;
    func_0200b168(0, cnt.v);
    func_0200b0ac(0);
    func_02013a5c(cnt);
    func_0200af08(0, 0);
    func_0201391c(1, 0, -1);
    data_020eb754 = 0x4000;
    data_020bed74 = 0x123;
    func_0200aad0(3, (u32)data_020bed84, 0, 0, 0, 0);
    func_0200b098(2);
    func_0200b098(1);
}

/* 0x0203e308: leave (stopping the music)
 * @difftest stub=0x02020194 cases=5 */
void func_0203e308(void)
{
    func_0200b098(0);
    func_0200b098(2);
    func_0200b098(1);
    func_0200b098(3);
    func_02020194();
}

/* 0x0203e340
 * @difftest cases=3 */
void func_0203e340(void)
{
}

/* 0x0203e344: the top screen during the credits (and their music)
 * @difftest stub=0x0200aad0,0x020202f0 cases=5 */
void func_0203e344(void)
{
    func_0200afb0(0, 0);
    func_0200b168(3, data_020aaca0.v);
    func_0200af18(3);
    func_0200aad0(3, (u32)data_020bed9c, 0, 0, 0, 0);
    func_0200b098(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b0ac(3);
    func_020202f0(0x21);
}
