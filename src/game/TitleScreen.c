/*
 * The title screens: the "touch screen" splash and the main title with its
 * twinkling sprites, plus the static sprite arrays' destructors.
 * ARM9 main, 0x0202e000 - 0x0202f330 (23 functions).
 *
 * Sprites are 0x24-byte objects (vtable data_020be13c; +0x04..+0x10
 * rectangle, +0x14 shown, +0x15 flag): four on the splash
 * (data_020e37e8) and ten on the title (data_020e3918, in pairs). Each
 * pair on the title is flipped by an animator (data_020e38b4, five of 0x14
 * bytes: {index, count, timer, -, frames}) after a random delay.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

typedef struct {
    s32 idx;
    s32 count;
    s32 timer;
    s32 pad;
    u8 *frames;
} Anim;

extern Cnt data_020aaca0, data_020aacac;
extern const u32 data_020be0d4[], data_020be0d8[], data_020be0dc[], data_020be0e0[],
    data_020be0e4[], data_020be0e8[], data_020be0ec[], data_020be0f0[],
    data_020be0f4[], data_020be0f8[], data_020be0fc[], data_020be100[],
    data_020be104[], data_020be108[], data_020be118[], data_020be11c[];
extern const u32 data_020be18c[], data_020be190[], data_020be194[], data_020be198[],
    data_020be19c[], data_020be1a0[], data_020be1a4[], data_020be1a8[],
    data_020be1ac[], data_020be1b0[], data_020be1b4[], data_020be1b8[],
    data_020be1bc[], data_020be1c0[], data_020be1c4[], data_020be1c8[],
    data_020be1cc[], data_020be1d0[], data_020be1d4[], data_020be1d8[],
    data_020be1dc[], data_020be1e0[], data_020be1e4[], data_020be1e8[],
    data_020be1ec[], data_020be1f0[], data_020be1f4[], data_020be1f8[],
    data_020be1fc[], data_020be200[], data_020be204[], data_020be208[],
    data_020be20c[], data_020be210[], data_020be214[], data_020be218[],
    data_020be21c[], data_020be220[], data_020be224[], data_020be228[];
extern s32 data_020be10c;            /* splash: frame */
extern s32 data_020be110;            /* splash: timer */
extern s32 data_020be114;            /* splash: direction */
extern const char data_020be120[];   /* "SpyroMain_TS.bin" */
extern u8 data_020be13c[];           /* sprite vtable */
extern const char data_020be22c[], data_020be240[], data_020be258[], data_020be270[],
    data_020be28c[], data_020be2a8[];   /* "SpyroMain_BS.bin" and translations */
extern u8 data_020e37e8[];
extern Anim data_020e38b4[5];
extern u8 data_020e3918[];
extern u8 data_020ebbb8[];

void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200af18(s32 layer);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void *func_02018208(void);
void func_02082fb8(void *obj, u32 a, u32 b);
void func_020202f0(u32 v);
void func_02020194(void);
void func_0207fe20(void);
void *func_02006acc(PtrVec *v);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void func_02074860(void *sprite);
void func_020748d0(void *sprite, u32 a, u32 tile, u32 b);
void func_02076cbc(void *p);
void func_02076ccc(void *p);
void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
void func_02019b08(u32 state, u32 screen);
void func_0203a028(void);
u64 func_020a4e00(s32 num, s32 den);   /* quotient | remainder << 32 */
void srand(u32 seed);
int rand(void);

void *func_0202e42c(u8 *s);

#define FN(f) ((void *(*)(void *))(void (*)(void))(f))
#define SPLASH(i) (data_020e37e8 + (i) * 0x24)
#define TITLE(i) (data_020e3918 + (i) * 0x24)

static void InitSprite(u8 *s, const u32 *a, const u32 *b, const u32 *c, const u32 *d, u32 tile)
{
    void (*show)(void *, u32, void *);

    U32_AT(s, 4) = *a;
    U32_AT(s, 8) = *b;
    U32_AT(s, 0xc) = *c;
    U32_AT(s, 0x10) = *d;
    show = VCALL(s, 0, void (*)(void *, u32, void *));
    show(s, 1, (void *)show);
    s[0x14] = 1;
    func_020748d0(s, 0, tile, 0x20);
}

/* advance an animator by `dt`; on expiry flip to its next sprite and wait
 * 0x800..0x5000 again */
static void AnimStep(Anim *a, s32 dt)
{
    a->timer -= dt;
    if (a->timer > 0)
        return;
    a->idx = a->idx + 1;
    a->idx = (s32)(func_020a4e00(a->idx, a->count) >> 32);
    func_02074860(a->frames + a->idx * 0x24);
    a->timer = (rand() & 0x7fff) % 0x4801 + 0x800;
}

/* 0x0202e000: destroy a vector, returning it
 * @difftest zero:12 cases=5 */
PtrVec *func_0202e000(PtrVec *v)
{
    func_02006acc(v);
    return v;
}

/* 0x0202e018: leave the splash screen
 * @difftest cases=3 */
void func_0202e018(void)
{
    func_0200b098(0);
    func_0200b098(2);
    func_0200b098(1);
    func_0200b098(3);
    func_02020194();
}

/* 0x0202e050: per-frame update of the splash: the four sprites play
 * forwards and backwards, pausing a random time at the start */
void func_0202e050(void)
{
    s32 t = data_020be110 - (s32)func_020822c0(func_020062d0());

    data_020be110 = t;
    if (t > 0)
        return;
    data_020be10c += data_020be114;
    if (data_020be10c == 4) {
        data_020be10c = 3;
        data_020be114 = -1;
        data_020be110 = 0x400;
    } else if (data_020be10c == -1) {
        data_020be10c = 0;
        data_020be114 = 1;
        data_020be110 = (rand() & 0x7fff) % 0x2801 + 0x1800;
    }
    func_02074860(SPLASH(data_020be10c));
}

/* 0x0202e148: set up the splash screen */
void func_0202e148(void)
{
    func_0200afb0(0, 0);
    func_0200b168(0, data_020aaca0.v);
    func_0200af18(0);
    func_0200aad0(0, (u32)data_020be120, 0, 0, 0, 0);
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
    InitSprite(SPLASH(0), data_020be0d4, data_020be100, data_020be0dc, data_020be0f8, 0x300);
    InitSprite(SPLASH(1), data_020be11c, data_020be118, data_020be0ec, data_020be108, 0x308);
    InitSprite(SPLASH(2), data_020be104, data_020be0fc, data_020be0f4, data_020be0f0, 0x310);
    InitSprite(SPLASH(3), data_020be0e4, data_020be0e0, data_020be0e8, data_020be0d8, 0x318);
    func_020202f0(1);
}

/* 0x0202e400
 * @difftest ptr:0x24 u8 cases=10 */
void func_0202e400(u8 *s, u8 v)
{
    s[0x15] = v;
}

/* 0x0202e408: destroy the splash sprites
 * @difftest cases=3 */
void func_0202e408(void)
{
    func_020a6d58(data_020e37e8, 4, 0x24, FN(func_0202e42c));
}

/* 0x0202e42c: sprite destructor
 * @difftest zero:0x24 cases=5 */
void *func_0202e42c(u8 *s)
{
    PTR_AT(s, 0) = data_020be13c;
    func_02076cbc(s);
    return s;
}

/* 0x0202e450: sprite constructor
 * @difftest zero:0x24 cases=5 */
void *func_0202e450(u8 *s)
{
    func_02076ccc(s);
    PTR_AT(s, 0) = data_020be13c;
    return s;
}

/* 0x0202e474
 * @difftest ptr:0x24 cases=10 */
s32 func_0202e474(u8 *s)
{
    return (s8)s[0x15];
}

/* 0x0202e47c: hide all background layers
 * @difftest cases=3 */
void func_0202e47c(void)
{
    func_0200b098(0);
    func_0200b098(2);
    func_0200b098(1);
    func_0200b098(3);
}

/* 0x0202e4b0: per-frame update of the title: any key re-seeds the random
 * numbers and starts the game; the sprite pairs twinkle */
void func_0202e4b0(void)
{
    s32 dt;
    s32 i;

    if (func_02081d34(func_02011f44(), 0xc) != 0) {
        srand((u32)func_020822c8(func_020062d0()));
        func_02019b08(9, 1);
    }
    dt = func_020822c0(func_020062d0());
    for (i = 0; i < 5; i++)
        AnimStep(&data_020e38b4[i], dt);
    func_0203a028();
}

/* the title's animators start on a random sprite of their pair */
static void AnimStart(Anim *a, u8 *frames, u32 m)
{
    a->count = 2;
    a->frames = frames;
    if (((rand() & 0x7fff) % m & 1) == 1) {
        a->timer = 0;
        AnimStep(a, 0x1000);
    }
}

/* 0x0202e75c: set up the title screen */
void func_0202e75c(void)
{
    const char *bg;

    func_02082fb8(func_02018208(), 8, 0x200010);
    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    srand((u32)func_020822c8(func_020062d0()));
    func_0200b168(0, data_020aacac.v);
    func_0200af18(0);
    switch (data_020ebbb8[0xa]) {
    case 1:
        bg = data_020be240;
        break;
    case 2:
        bg = data_020be258;
        break;
    case 3:
        bg = data_020be270;
        break;
    case 4:
        bg = data_020be28c;
        break;
    case 5:
        bg = data_020be2a8;
        break;
    default:
        bg = data_020be22c;
        break;
    }
    func_0200aad0(0, (u32)bg, 0, 0, 0, 0);
    InitSprite(TITLE(0), data_020be20c, data_020be208, data_020be204, data_020be200, 0x320);
    InitSprite(TITLE(1), data_020be1fc, data_020be1f8, data_020be1f4, data_020be1f0, 0x3a0);
    InitSprite(TITLE(2), data_020be1b4, data_020be1e8, data_020be1a8, data_020be1cc, 0x325);
    InitSprite(TITLE(3), data_020be1dc, data_020be1ac, data_020be1d8, data_020be198, 0x3a5);
    InitSprite(TITLE(4), data_020be190, data_020be224, data_020be21c, data_020be214, 0x32b);
    InitSprite(TITLE(5), data_020be210, data_020be1c4, data_020be1c0, data_020be1bc, 0x3ab);
    InitSprite(TITLE(6), data_020be1ec, data_020be1e4, data_020be1d0, data_020be18c, 0x32f);
    InitSprite(TITLE(7), data_020be19c, data_020be228, data_020be1c8, data_020be1a4, 0x3af);
    InitSprite(TITLE(8), data_020be1b8, data_020be1e0, data_020be1b0, data_020be220, 0x336);
    InitSprite(TITLE(9), data_020be1a0, data_020be1d4, data_020be218, data_020be194, 0x3b6);
    AnimStart(&data_020e38b4[0], TITLE(0), 0x24);
    AnimStart(&data_020e38b4[1], TITLE(2), 0x16);
    AnimStart(&data_020e38b4[2], TITLE(4), 0x58);
    AnimStart(&data_020e38b4[3], TITLE(6), 0x39);
    AnimStart(&data_020e38b4[4], TITLE(8), 0x5d);
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
    func_0207fe20();
}

/* 0x0202f27c..0x0202f30c: destroy the title's sprite pairs
 * @difftest cases=3 */
void func_0202f27c(void)
{
    func_020a6d58(TITLE(8), 2, 0x24, FN(func_0202e42c));
}

/* @difftest cases=3 */
void func_0202f2a0(void)
{
    func_020a6d58(TITLE(6), 2, 0x24, FN(func_0202e42c));
}

/* @difftest cases=3 */
void func_0202f2c4(void)
{
    func_020a6d58(TITLE(4), 2, 0x24, FN(func_0202e42c));
}

/* @difftest cases=3 */
void func_0202f2e8(void)
{
    func_020a6d58(TITLE(2), 2, 0x24, FN(func_0202e42c));
}

/* @difftest cases=3 */
void func_0202f30c(void)
{
    func_020a6d58(TITLE(0), 2, 0x24, FN(func_0202e42c));
}
