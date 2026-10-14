/*
 * The story pages (comic cut-scenes): a picture and a caption per page on
 * the touch screen, turned with a touch or A, with their own music.
 * ARM9 main, 0x02040d1c - 0x02041498 (23 functions).
 *
 * The player object (vtable data_020bf170, a singleton: data_020ebb9c,
 * base class vtable data_020bf5b0): +4 the story (-1 none), +8 the page,
 * +0xc the music to resume, +0x10 the story's pages (data_020bf4e0[story]:
 * {count, {picture file, caption text} * count}), +0x14 the state.
 * Story 0xa leads to states 0x22 / 0x23, the others back to the game.
 *
 * Testing: with fake players and pages, the state switch, the music
 * player and the background loader stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT / colour value passed by value in a register */

extern Cnt data_020aaca0, data_020aaca4, data_020aaca8;
extern u8 data_020bf170[];           /* the player's vtable */
extern u8 *const data_020bf4e0[];    /* the stories' pages */
extern const char data_020bf598[];   /* "B_CutScene.bin" */
extern u8 data_020bf5b0[];           /* the singleton base's vtable */
extern u8 data_020e2308[];           /* sound state: +0xa the music playing */
extern void *data_020ebb9c;          /* the singleton */

void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
void func_0201b770(u32 a, u32 b, u32 c);
void func_0201b7f0(u32 screen);
void func_0201b808(u32 screen);
s32 func_0201b650(u32 screen);
void func_02019b08(u32 state, u32 screen);
void func_02019b40(u32 screen);
u32 func_02019b60(u32 screen);
void func_02019240(void);
void func_02019260(void);
void func_0204378c(u32 screen);
void func_020437c4(u32 screen);
void func_020202f0(u32 v);
const char *func_02028a20(u32 id);
u32 func_020135a8(u32 tile, s32 x, s32 y, s32 maxw, const u8 *str, u32 align);
void func_0207fe44(void *p);
u8 *func_02040cf8(u8 *map);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200af18(s32 layer);
void func_0200af08(s32 layer, u32 v);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_02013a5c(Cnt color);

s32 func_02040f48(u8 *self);
void func_02040f50(u8 *self);
s32 func_02040f64(u8 *self);
void func_02040f6c(u8 *self, s32 state);
s32 func_02040f74(u8 *self);
void func_02040d34(u8 *self);
void func_02040e38(u8 *self);
void func_02040ea0(u8 *self);
void func_02041328(u8 *self);
void func_02041408(u8 *self, void *inst);
void func_02041424(u8 *self);

#define PAGES(self) ((u8 *)PTR_AT(self, 0x10))

/* 0x02040d1c: the chest map's destructor (as a static object's)
 * @difftest $M=zero:0x10 $M cases=3 */
u8 *func_02040d1c(u8 *map)
{
    func_02040cf8(map);
    return map;
}

/* 0x02040d34: the touch screen's blank page
 * @difftest stub=0x0200aad0 cases=5 */
void func_02040d34(u8 *self)
{
    Cnt cnt;

    data_020df0fc = 1;
    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(3, data_020aaca4.v);
    func_0200af18(3);
    func_0200b0ac(3);
    func_0200aad0(3, (u32)data_020bf598, 0, 0, 0, 0);
    cnt = data_020aaca8;
    func_0200b168(0, cnt.v);
    func_0200b0ac(0);
    func_02013a5c(cnt);
    func_0200af08(0, 0);
    func_0201391c(1, 0, -1);
    func_0200b098(1);
    func_0200b098(2);
    data_020df0fc = 0;
}

/* 0x02040e38: the top screen during the pages
 * @difftest cases=5 */
void func_02040e38(u8 *self)
{
    func_0200afb0(0, 0);
    func_0200b168(3, data_020aaca0.v);
    func_0200af18(3);
    func_0200b098(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b0ac(3);
}

/* 0x02040ea0: show the current page
 * @difftest $T=zero:0x20 @$T+0:32=2 @$T+8:32=int:0:0x40 @$T+0x10:32=int:0:0x40 $S=zero:0x18 @$S+0x10:32=$T @$S+8:32=int:0:1 $S stub=0x0200aad0 cases=10 */
void func_02040ea0(u8 *self)
{
    func_0200af18(3);
    func_0200aad0(3, U32_AT(PAGES(self) + func_02040f64(self) * 8, 4), 0, 0, 0, 0);
    data_020df0fc = 1;
    func_0200af08(0, 0);
    func_020135a8(1, 0x80, 0x20, 0xe0,
                  (const u8 *)func_02028a20(U32_AT(PAGES(self) + func_02040f64(self) * 8, 8)), 1);
    data_020df0fc = 0;
}

/* 0x02040f48: the story
 * @difftest ptr:0x18 cases=3 */
s32 func_02040f48(u8 *self)
{
    return S32_AT(self, 4);
}

/* 0x02040f50: next page
 * @difftest ptr:0x18 cases=3 */
void func_02040f50(u8 *self)
{
    S32_AT(self, 8)++;
}

/* 0x02040f64: the page
 * @difftest ptr:0x18 cases=3 */
s32 func_02040f64(u8 *self)
{
    return S32_AT(self, 8);
}

/* 0x02040f6c: set the state
 * @difftest ptr:0x18 int:0:7 cases=3 */
void func_02040f6c(u8 *self, s32 state)
{
    S32_AT(self, 0x14) = state;
}

/* 0x02040f74: the state
 * @difftest ptr:0x18 cases=5 */
s32 func_02040f74(u8 *self)
{
    return (s8)S32_AT(self, 0x14);
}

/* 0x02040f84: the story pages' per-frame update
 * @difftest $T=zero:0x20 @$T+0:32=2 @$T+8:32=int:0:0x40 @$T+0x10:32=int:0:0x40 $S=zero:0x18 @$S+4:32=pick:-1,0,0xa @$S+8:32=int:0:2 @$S+0xc:32=int:0:4 @$S+0x10:32=$T @$S+0x14:32=int:-1:8 $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x5c:32=pick:0,1 @020e2312:8=pick:0,3,0xff $S stub=0x02019b08,0x02019b40,0x020202f0,0x0200aad0,0x02019240,0x02019260 cases=200 */
void func_02040f84(u8 *self)
{
    switch ((u32)func_02040f74(self)) {
    case 0:
        if (func_02040f48(self) == -1)
            return;
        func_02040f6c(self, 1);
        func_0201b770(2, 0x3f, 1);
        func_0201b7f0(0);
        func_020437c4(data_020df0fc);
        S32_AT(self, 0xc) = data_020e2308[0xa];
        if (S32_AT(self, 0xc) == 0xff)
            S32_AT(self, 0xc) = 0;
        func_020202f0(0x27);
        func_02019b08(0x24, 1);
        return;
    case 1:
        if (func_0201b650(0) != 0 || func_0201b650(1) != 0)
            return;
        func_02019260();
        func_02040e38(self);
        func_02040d34(self);
        func_02040ea0(self);
        func_02040f6c(self, 2);
        func_0201b770(2, 0x3f, 1);
        func_0201b808(0);
        return;
    case 2:
        if (func_0201b650(0) != 0)
            return;
        if (func_02081d34(func_02011f44(), 0xc) == 0 && func_02081d34(func_02011f44(), 0xa) == 0)
            return;
        func_02040f50(self);
        if (func_02040f64(self) < S32_AT(PAGES(self), 0)) {
            func_0201b770(2, 0x3f, 1);
            func_0201b7f0(0);
            func_0201b7f0(1);
            func_02040f6c(self, 3);
            return;
        }
        if (S32_AT(self, 4) == 0xa) {
            func_02041328(self);
            func_0204378c(data_020df0fc);
            func_02040f6c(self, 0);
            func_02019b08(0x22, 0);
            func_02019b08(0x23, 1);
            return;
        }
        func_02040f6c(self, 5);
        func_0201b770(2, 0x3f, 1);
        func_0201b7f0(0);
        if (func_02019b60(data_020df0fc) == 9)
            return;
        func_02019b40(1);
        return;
    case 3:
        if (func_0201b650(0) != 0 || func_0201b650(1) != 0)
            return;
        func_0201b770(2, 0x3f, 1);
        func_0201b808(0);
        func_0201b808(1);
        func_02040ea0(self);
        func_02040f6c(self, 4);
        return;
    case 4:
        if (func_0201b650(0) != 0 || func_0201b650(1) != 0)
            return;
        func_02040f6c(self, 2);
        return;
    case 5:
        if (func_0201b650(0) != 0 || func_0201b650(1) != 0)
            return;
        func_02019240();
        func_02040d34(self);
        func_0201b770(2, 0x3f, 1);
        func_0201b808(0);
        func_02040f6c(self, 6);
        return;
    case 6:
        if (func_0201b650(data_020df0fc) != 0)
            return;
        func_02041328(self);
        func_0204378c(data_020df0fc);
        func_020202f0(S32_AT(self, 0xc));
        func_02040f6c(self, 0);
        return;
    }
}

/* 0x02041328: no story
 * @difftest ptr:0x18 cases=3 */
void func_02041328(u8 *self)
{
    S32_AT(self, 4) = -1;
    S32_AT(self, 8) = 0;
    PTR_AT(self, 0x10) = NULL;
}

/* 0x02041340: start story `id`
 * @difftest ptr:0x18 int:0:0xa cases=11 */
void func_02041340(u8 *self, s32 id)
{
    S32_AT(self, 4) = id;
    S32_AT(self, 8) = 0;
    PTR_AT(self, 0x10) = data_020bf4e0[id];
}

/* 0x02041360: the player's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_02041360(u8 *self)
{
    PTR_AT(self, 0) = data_020bf170;
    func_02041424(self);
    func_0207fe44(self);
    return self;
}

/* 0x0204138c: the player's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_0204138c(u8 *self)
{
    PTR_AT(self, 0) = data_020bf170;
    func_02041424(self);
    return self;
}

/* 0x020413b0: the player's constructor
 * @difftest zero:0x18 cases=3 */
u8 *func_020413b0(u8 *self)
{
    func_02041408(self, self);
    PTR_AT(self, 0) = data_020bf170;
    S32_AT(self, 4) = -1;
    S32_AT(self, 8) = 0;
    PTR_AT(self, 0x10) = NULL;
    func_02040f6c(self, 0);
    S32_AT(self, 0xc) = 0;
    return self;
}

/* 0x020413fc
 * @difftest cases=3 */
void func_020413fc(void)
{
}

/* 0x02041400
 * @difftest cases=3 */
void func_02041400(void)
{
}

/* 0x02041404
 * @difftest cases=3 */
void func_02041404(void)
{
}

/* 0x02041408: the singleton base's constructor
 * @difftest zero:0x18 ptr:0x18 cases=3 */
void func_02041408(u8 *self, void *inst)
{
    PTR_AT(self, 0) = data_020bf5b0;
    data_020ebb9c = inst;
}

/* 0x02041424: the singleton base's destructor
 * @difftest zero:0x18 cases=3 */
void func_02041424(u8 *self)
{
    PTR_AT(self, 0) = data_020bf5b0;
    data_020ebb9c = NULL;
}

/* 0x02041444: the singleton base's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_02041444(u8 *self)
{
    PTR_AT(self, 0) = data_020bf5b0;
    data_020ebb9c = NULL;
    func_0207fe44(self);
    return self;
}

/* 0x02041478: the singleton base's destructor (another copy)
 * @difftest zero:0x18 cases=3 */
void func_02041478(u8 *self)
{
    PTR_AT(self, 0) = data_020bf5b0;
    data_020ebb9c = NULL;
}
