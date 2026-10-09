/*
 * The boot logos: the Nintendo licence notice and the publisher and
 * developer logos, each faded in, held and faded out, then the title (or,
 * without a save, the opening).
 * ARM9 main, 0x0203a22c - 0x0203a948 (7 functions).
 *
 * data_020bea84 is the logo state (1 licence, 2 publisher, 3 / 4 their
 * captions, 5 developer, 6 holding until data_020e8818 runs out, then
 * data_020e8814); data_020e881c is the fade-in time left (0x5000 ticks)
 * and data_020e8810 set when a new state starts. data_020beac4 cycles the
 * top screen's logo (func_0203a834).
 *
 * Testing: the update runs with the state switch, the backup read
 * func_02009488 and the opening set-up func_0203ee8c stubbed; the set-ups
 * with the background loader func_0200aad0 stubbed (they read from the
 * card).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT / colour value passed by value in a register */

extern Cnt data_020aaca0, data_020aaca4, data_020aaca8;
extern s32 data_020bea84;
extern const char data_020bea88[];   /* "NintendoLicense_BKG.bin" */
extern const char data_020beaa0[];   /* "Logo_VUG_BS.bin" */
extern const char data_020beab0[];   /* "Logo_Amaze_BS.bin" */
extern s32 data_020beac4;
extern const char data_020beac8[];   /* "NintendoLicense_BKG.bin" */
extern const char data_020beae0[];   /* "Logo_VUG_TS.bin" */
extern const char data_020beaf0[];   /* "Logo_Amaze_TS.bin" */
extern s8 data_020e8810;
extern s32 data_020e8814;
extern s32 data_020e8818;
extern s32 data_020e881c;

void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
s32 func_02009488(u32 a, u32 b, void *c);
void func_0203ee8c(s32 a);
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

void func_0203a5b8(void);

static void Caption(u32 text)
{
    func_020135a8(1, 0x80, 0x3c, 0xd0, (const u8 *)func_02028a20(text), 1);
}

/* fade the screens out and start states 1 / 2 (the title) */
static void ToTitle(void)
{
    data_020e8810 = 0;
    data_020e881c = 0x5000;
    func_0201b770(2, 0x3f, 1);
    func_02019b08(1, 0);
    func_02019b08(2, 1);
}

/* 0x0203a22c: leave
 * @difftest cases=3 */
void func_0203a22c(void)
{
    func_0200b098(0);
    func_0200b098(2);
    func_0200b098(1);
    func_0200b098(3);
}

/* 0x0203a260: the logos' per-frame update
 * @difftest @020bea84:32=int:0:7 @020e8810:8=pick:0,1 @020e8814:32=int:1:5 @020e8818:32=pick:-5,0,5,0x7fffffff @020e881c:32=pick:-5,0,5,0x7fffffff stub=0x02019b08,0x02009488:0,0x0203ee8c cases=120
 * @difftest @020bea84:32=5 @020e8810:8=0 @020e881c:32=pick:-5,0 stub=0x02019b08,0x02009488:1,0x0203ee8c cases=10 */
void func_0203a260(void)
{
    u8 tmp[8];

    switch ((u32)data_020bea84) {
    case 1:
        func_0203a5b8();
        if (data_020bea84 != 6)
            return;
        data_020e8810 = 1;
        data_020e8814 = 2;
        return;
    case 2:
        if (data_020e8810 != 0) {
            ToTitle();
            return;
        }
        func_0203a5b8();
        if (data_020bea84 == 6)
            data_020e8814 = 3;
        return;
    case 3:
        if (data_020e8810 == 1) {
            data_020e8810 = 0;
            Caption(1);
            return;
        }
        func_0203a5b8();
        if (data_020bea84 == 6)
            data_020e8814 = 5;
        return;
    case 4:
        if (data_020e8810 == 1) {
            data_020e8810 = 0;
            Caption(3);
            return;
        }
        func_0203a5b8();
        if (data_020bea84 != 6)
            return;
        data_020e8810 = 1;
        data_020bea84 = 5;
        return;
    case 5:
        if (data_020e8810 != 0) {
            ToTitle();
            return;
        }
        func_0203a5b8();
        if (data_020bea84 != 6)
            return;
        /* no save yet: the opening */
        if (func_02009488(0, 1, tmp) == 0) {
            func_0203ee8c(1);
            func_0201b770(2, 0x3f, 1);
            func_02019b08(0x25, 0);
            func_02019b08(0x26, 1);
        } else {
            func_0201b770(2, 0x3f, 1);
            func_02019b08(3, 0);
            func_02019b08(8, 1);
        }
        return;
    case 6: {
        s32 st;
        data_020e8818 -= func_020822c0(func_020062d0());
        if (data_020e8818 > 0)
            return;
        st = data_020e8814;
        data_020e881c = 0x5000;
        data_020e8810 = 1;
        data_020bea84 = st;
        return;
    }
    }
}

/* 0x0203a5b8: fade in, then hold (state 6)
 * @difftest @020e881c:32=pick:-5,0,5,0x7fffffff cases=20 */
void func_0203a5b8(void)
{
    data_020e881c -= func_020822c0(func_020062d0());
    if (data_020e881c > 0)
        return;
    data_020e8818 = 0x266;
    func_0200af08(0, 0);
    data_020bea84 = 6;
}

/* 0x0203a628: set up the bottom screen's logo
 * @difftest @020bea84:32=int:0:6 stub=0x0200aad0 cases=40 */
void func_0203a628(void)
{
    Cnt cnt;
    s32 st;

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
    st = data_020bea84;
    data_020e881c = 0x5000;
    switch ((u32)st) {
    case 1:
        func_0200aad0(3, (u32)data_020bea88, 0, 0, 0, 0);
        Caption(4);
        break;
    case 2:
        func_0200aad0(3, (u32)data_020beaa0, 0, 0, 0, 0);
        Caption(0);
        break;
    case 5:
        func_0200aad0(3, (u32)data_020beab0, 0, 0, 0, 0);
        Caption(2);
        break;
    }
    func_0200b098(2);
    func_0200b098(1);
}

/* 0x0203a7fc: leave
 * @difftest cases=3 */
void func_0203a7fc(void)
{
    func_0200b098(0);
    func_0200b098(2);
    func_0200b098(1);
    func_0200b098(3);
}

/* 0x0203a830
 * @difftest cases=3 */
void func_0203a830(void)
{
}

/* 0x0203a834: show the next top-screen logo
 * @difftest @020beac4:32=int:0:5 stub=0x0200aad0 cases=30 */
void func_0203a834(void)
{
    func_0200afb0(0, 0);
    func_0200b168(3, data_020aaca0.v);
    switch ((u32)data_020beac4) {
    case 1:
        func_0200aad0(3, (u32)data_020beac8, 0, 0, 0, 0);
        data_020beac4 = 2;
        break;
    case 2:
        func_0200aad0(3, (u32)data_020beae0, 0, 0, 0, 0);
        data_020beac4 = 3;
        break;
    case 3:
        func_0200aad0(3, (u32)data_020beaf0, 0, 0, 0, 0);
        data_020beac4 = 0;
        break;
    }
    func_0200b098(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b0ac(3);
}
