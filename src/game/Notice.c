/*
 * A notice screen: one of the game texts 5-7 on the touch screen (chosen
 * with func_0203ee8c, e.g. before a new game), shown for 0x2000 ticks in
 * the first case and then followed by the title.
 * ARM9 main, 0x0203ecd4 - 0x0203ef98 (6 functions).
 *
 * data_020ebb44 is the notice, data_020ebb48 its text and data_020ebb40
 * the time left.
 *
 * Testing: with the state switch and the background loader stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT / colour value passed by value in a register */

extern Cnt data_020aaca0, data_020aaca4, data_020aaca8;
extern const char data_020bee50[];   /* "NintendoLicense_BKG.bin" */
extern const char data_020bee68[];   /* "NintendoLicense_BKG.bin" */
extern s32 data_020ebb40;
extern s32 data_020ebb44;
extern u32 data_020ebb48;

void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
const char *func_02028a20(u32 id);
u32 func_020135a8(u32 tile, s32 x, s32 y, s32 maxw, const u8 *str, u32 align);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af18(s32 layer);
void func_0200af08(s32 layer, u32 v);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_02013a5c(Cnt color);

/* 0x0203ecd4: the notice's per-frame update
 * @difftest @020ebb44:32=int:0:3 @020ebb40:32=pick:-5,0,5 stub=0x02019b08 cases=30 */
void func_0203ecd4(void)
{
    if (data_020ebb44 != 0)
        return;
    data_020ebb40 -= func_020822c0(func_020062d0());
    if (data_020ebb40 > 0)
        return;
    func_0201b770(2, 0x3f, 1);
    func_02019b08(3, 0);
    func_02019b08(8, 1);
}

/* 0x0203ed70: set up the notice
 * @difftest @020ebb48:32=int:5:7 stub=0x0200aad0 cases=10 */
void func_0203ed70(void)
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
    func_0200aad0(3, (u32)data_020bee50, 0, 0, 0, 0);
    func_020135a8(1, 0x80, 0x48, 0xc4, (const u8 *)func_02028a20(data_020ebb48), 1);
    data_020ebb40 = 0x2000;
    func_0200b098(1);
    func_0200b098(2);
}

/* 0x0203ee8c: choose the notice
 * @difftest int:-1:4 cases=10 */
void func_0203ee8c(s32 k)
{
    data_020ebb44 = k;
    if (k == 0)
        data_020ebb48 = 5;
    else if (k == 1)
        data_020ebb48 = 6;
    else if (k == 2)
        data_020ebb48 = 7;
}

/* 0x0203eee0: leave
 * @difftest cases=3 */
void func_0203eee0(void)
{
    func_0200b098(0);
    func_0200b098(2);
    func_0200b098(1);
    func_0200b098(3);
}

/* 0x0203ef14
 * @difftest cases=3 */
void func_0203ef14(void)
{
}

/* 0x0203ef18: the top screen during the notice
 * @difftest stub=0x0200aad0 cases=5 */
void func_0203ef18(void)
{
    func_0200afb0(0, 0);
    func_0200b168(3, data_020aaca0.v);
    func_0200aad0(3, (u32)data_020bee68, 0, 0, 0, 0);
    func_0200b098(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b0ac(3);
}
