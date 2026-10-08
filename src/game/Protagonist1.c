/*
 * CProtagonist, part 1: the hero's saved stats (health, breath energy,
 * experience, gems, level), temporary max-health/energy boosts and a debug
 * height read-out.
 * ARM9 main, 0x02013b78 - 0x020145bc (26 functions).
 *
 * The stats live in the save record func_020436f0(data_020bf6a0):
 *   +0x04 s16 breath energy, +0x06 u16 health, +0x08 experience,
 *   +0x0f bits 1-7 current level, +0x11 gems (u8), +0x12 breath level.
 * The hero object (data_020deebc) has the boosts: +0x55/+0x56 energy boost
 * timer/amount, +0x57/+0x58 health boost timer/amount.
 * Changing health or energy makes the three HUD bars (func_02011f34)
 * flash.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const char data_020bd09c[];   /* "+%d XP" */
extern const char data_020bd0a4[], data_020bd0b4[], data_020bd0d0[];
extern u8 data_020def74[];           /* boost timers */

void func_02005b98(s32 *out, const void *src);
s32 func_02011bc8(void);
u8 *func_02011f34(void);
s32 func_02043700(void);             /* max health */
s32 func_02043708(void);             /* max breath energy */
s32 func_02043710(void);
s32 func_02043748(void);
s32 func_0204380c(void);             /* number of breath levels */
s32 func_0205a364(void *obj);
s32 func_0205a3ec(void *obj, u32 id);
s32 func_0205a744(void *obj, void *item);
void func_02076b44(void *obj, const char *text, u32 fbits);
int func_020804f4(char *buf, const char *fmt, ...);   /* sprintf */
int rand(void);

/* this file's own functions, called before their definition */
u32 func_02013b84(void *self);
u32 func_02013cf8(void *self);
s32 func_02013e90(u8 *self);
s32 func_02014004(void *self);
s32 func_02014094(u8 *self);
void func_020141a8(u8 *self);
void func_020141b8(u8 *self);
void func_020141c8(u8 *self);
void func_02014214(u8 *self);
s32 func_020145ac(s32 a, s32 b, const char *fmt, ...);

#define STATS() func_020436f0(data_020bf6a0)
#define HERO_CAMERA() PTR_AT(func_02016f24(data_020deebc), 0xac)

/* make a HUD bar flash */
static inline void FlashBar(u8 *bar)
{
    bar[0] = 1;
    if ((s8)bar[0] == 0)
        U32_AT(bar, 0x24) = 0;
    U32_AT(bar, 0x24) = 0x5000;
}

static void FlashHud(void)
{
    u8 *hud = func_02011f34();
    if (func_02011bc8() != 0)
        return;
    FlashBar(hud + 0xe0);
    FlashBar(hud + 0x20c);
    FlashBar(hud + 0x354);
}

/* 0x02013b78
 * @difftest ptr:12:4 u32 */
void func_02013b78(u8 *p, u32 v)
{
    U32_AT(p, 4) = v;
    U32_AT(p, 8) = v;
}

/* 0x02013b84: breath level
 * @difftest u32 */
u32 func_02013b84(void *self) { return STATS()[0x12]; }

/* 0x02013bac: set the breath level
 * @difftest u32 int:0:10 */
void func_02013bac(void *self, u32 v)
{
    v &= 0xff;
    if ((s32)v >= func_0204380c())
        v = (func_0204380c() - 1) & 0xff;
    STATS()[0x12] = v;
}

/* 0x02013be8: gain experience and show "+n XP" on the HUD
 * @difftest @020daa54:8=pick:0,1 u32 int:-50:500 cases=40 */
void func_02013be8(void *self, s32 xp)
{
    char buf[12];
    u8 *stats = STATS(), *hud;
    U32_AT(stats, 8) += xp;
    func_020804f4(buf, data_020bd09c, xp);
    hud = func_02011f34();
    if (func_02011bc8() != 0)
        return;
    func_02076b44(hud + 0x7c4, buf, 0x46400000);   /* 12800.0f */
    func_02076b44(hud + 0x768, NULL, 0);
}

/* 0x02013c84: experience
 * @difftest u32 */
u32 func_02013c84(void *self) { return U32_AT(STATS(), 8); }

/* 0x02013cac: is this a breath item
 * @difftest $I=ptr:0x1c:4 @$I+0x18:32=pick:0,0x80,0x100,0x200,0x400,0x800,0x1000 $I */
s32 func_02013cac(u8 *item)
{
    u32 v = U32_AT(item, 0x18);
    return (s8)(v == 0x200 || v == 0x400 || v == 0x800 || v == 0x80);
}

/* 0x02013cf8: health
 * @difftest u32 */
u32 func_02013cf8(void *self) { return U16_AT(STATS(), 6); }

/* 0x02013d20: set health
 * @difftest u32 int:-20:200 */
void func_02013d20(void *self, s32 v)
{
    if (v > func_02043700())
        v = func_02043700();
    else if (v < 0)
        v = 0;
    U16_AT(STATS(), 6) = v;
}

/* 0x02013d64: add to health (damage is ignored while invulnerable)
 * @difftest @020deedc:32=pick:0,1 @020daa54:8=pick:0,1 u32 int:-30:30 cases=80 */
void func_02013d64(u8 *self, s32 delta)
{
    s32 v;
    if ((s8)BITS(data_020deebc[0x20 / 4], 0, 1) && delta < 0)
        return;
    v = delta + func_02013cf8(self);
    if (v > func_02013e90(self))
        v = func_02013e90(self);
    if (v > func_02043700())
        v = func_02043700();
    else if (v < 0)
        v = 0;
    U16_AT(STATS(), 6) = v;
    FlashHud();
}

/* 0x02013e90: current max health (with the boost)
 * @difftest $H=ptr:0x60:4 $H */
s32 func_02013e90(u8 *self)
{
    s32 v = func_02043710() + self[0x58];
    if (v > func_02043700())
        v = func_02043700();
    return v;
}

/* 0x02013ec4: add to breath energy
 * @difftest @020daa54:8=pick:0,1 $H=ptr:0x60:4 @$H+0x20:32=pick:0,1 $H int:-30:30 cases=80 */
void func_02013ec4(u8 *self, s32 delta)
{
    s32 v;
    if ((U32_AT(self, 0x20) & 1) && delta < 0)
        return;
    v = delta + func_02014004(self);
    if (v > func_02014094(self))
        v = func_02014094(self);
    if (v > func_02043708())
        v = func_02043708();
    else if (v < 0)
        v = 0;
    S16_AT(STATS(), 4) = v;
    S32_AT(HERO_CAMERA(), 0xc) = S16_AT(STATS(), 4);
    FlashHud();
}

/* 0x02014004: breath energy
 * @difftest u32 */
s32 func_02014004(void *self) { return S16_AT(STATS(), 4); }

/* 0x0201402c: set breath energy
 * @difftest u32 int:-20:200 */
void func_0201402c(void *self, s32 v)
{
    if (v > func_02043708())
        v = func_02043708();
    else if (v < 0)
        v = 0;
    S16_AT(STATS(), 4) = v;
    S32_AT(HERO_CAMERA(), 0xc) = S16_AT(STATS(), 4);
}

/* 0x02014094: current max breath energy (with the boost)
 * @difftest $H=ptr:0x60:4 $H */
s32 func_02014094(u8 *self)
{
    s32 v = func_02043748() + self[0x56];
    if (v > func_02043708())
        v = func_02043708();
    return v;
}

/* 0x020140c8: count down the health boost
 * @difftest $H=ptr:0x60:4 @$H+0x57:8=pick:1,2,0x84 $H cases=60 */
void func_020140c8(u8 *self)
{
    if (func_0205a3ec(data_020def74, 0x10) > -1) {
        self[0x57]--;
        if (self[0x57] != 0)
            return;
        func_0205a364(data_020def74);
        self[0x57] = 0x84;
    } else {
        func_020141a8(self);
    }
}

/* 0x02014138: count down the energy boost
 * @difftest $H=ptr:0x60:4 @$H+0x55:8=pick:1,2,0x84 $H cases=60 */
void func_02014138(u8 *self)
{
    if (func_0205a3ec(data_020def74, 0xf) > -1) {
        self[0x55]--;
        if (self[0x55] != 0)
            return;
        func_0205a364(data_020def74);
        self[0x55] = 0x84;
    } else {
        func_020141b8(self);
    }
}

/* 0x020141a8: end the health boost
 * @difftest ptr:0x60:4 */
void func_020141a8(u8 *self)
{
    self[0x58] = 0;
    self[0x57] = 0;
}

/* 0x020141b8: end the energy boost
 * @difftest ptr:0x60:4 */
void func_020141b8(u8 *self)
{
    self[0x56] = 0;
    self[0x55] = 0;
}

/* 0x020141c8: start a health boost
 * @difftest ptr:0x60:4 */
void func_020141c8(u8 *self)
{
    u32 r;
    self[0x57] = 0x84;
    r = rand() & 0x7fff;
    self[0x58] = ((r & 3) + 1) * (func_02013b84(self) + 1);
}

/* 0x02014214: start an energy boost
 * @difftest ptr:0x60:4 */
void func_02014214(u8 *self)
{
    u32 r;
    self[0x55] = 0x84;
    r = rand() & 0x7fff;
    self[0x56] = ((r & 3) + 1) * (func_02013b84(self) + 1);
}

/* 0x02014260: use an item; boost potions start a boost
 * @difftest $I=ptr:0x20:4 @$I+8:32=pick:0x59,0x5a,3 $I cases=60 */
s32 func_02014260(u8 *item)
{
    s32 r = func_0205a744(data_020def74, item);
    if (r != 0) {
        if (U32_AT(item, 8) == 0x59)
            func_02014214((u8 *)data_020deebc);
        if (U32_AT(item, 8) == 0x5a)
            func_020141c8((u8 *)data_020deebc);
    }
    return r;
}

/* 0x020142c0: add gems
 * @difftest u32 int:0:300 */
void func_020142c0(void *self, u32 n)
{
    u8 old;
    if (n == 0)
        return;
    old = STATS()[0x11];
    STATS()[0x11] = n + old;
}

/* 0x0201430c: spend gems
 * @difftest u32 int:0:300 */
void func_0201430c(void *self, u32 n)
{
    u8 old;
    if (n > STATS()[0x11]) {
        STATS()[0x11] = 0;
        return;
    }
    old = STATS()[0x11];
    STATS()[0x11] = old - n;
}

/* 0x02014384: gems
 * @difftest u32 */
u32 func_02014384(void *self) { return STATS()[0x11]; }

static inline s32 LevelHeight(void)
{
    return S32_AT(data_020c3b60 + (STATS()[0xf] >> 1) * 0x60, 0x38);
}

static inline s32 RoundFx(s32 v)
{
    float f = (float)SHL(v, 12);
    return (s32)(v > 0 ? 0.5f + f : f - 0.5f);
}

/* 0x020143ac: debug read-out of the hero's height on the level height map */
void func_020143ac(void)
{
    s32 pos[3], y, scale, v;
    s64 prod;
    func_02005b98(pos, data_020df1bc);
    y = (S32_AT(func_02016f24(data_020deebc), 0xc) - pos[2] + 0x1d000) >> 12;
    scale = func_0208f070(0xff000, RoundFx(LevelHeight()));
    func_020145ac(0, 5, data_020bd0a4, LevelHeight());
    func_020145ac(0, 6, data_020bd0b4, y + LevelHeight());
    v = RoundFx(y);
    prod = (s64)v * scale + 0x800;
    func_020145ac(0, 7, data_020bd0d0, ((s32)(prod >> 12) >> 12) + 0xff);
}

/* 0x020145ac: debug text output (does nothing in this build)
 * @difftest u32 u32 u32 u32 */
s32 func_020145ac(s32 a, s32 b, const char *fmt, ...)
{
    return 0;
}
