/*
 * The touch-screen menu (spells, part 3): setting it up.
 * ARM9 main, 0x02030b38 - 0x02031834 (3 functions).
 *
 * Owned spells get their icon (five 0x34-byte sprites from data_020e3b24)
 * and allow casting runes (data_020e3a88); the eleven result lines
 * (0x78-byte text entries from data_020e3c28, the first one last) are laid
 * out from rectangles in data and hidden until a rune is read.
 *
 * Testing: func_02030b38 runs with the background loader func_0200aad0
 * and the text layout func_020107a8 stubbed (both read files from the
 * card).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

extern Cnt data_020aacac;
extern const char data_020b654c[], data_020b6560[];
extern const char data_020be3c8[];   /* "B_Spells_BKG.bin" */
extern const s32 data_020be2c0[], data_020be2c4[], data_020be2c8[], data_020be2cc[],
    data_020be2d4[], data_020be2d8[], data_020be2dc[], data_020be2e0[],
    data_020be2e4[], data_020be2e8[], data_020be2ec[], data_020be2f0[],
    data_020be2f4[], data_020be2f8[], data_020be2fc[], data_020be300[],
    data_020be304[], data_020be308[], data_020be30c[], data_020be310[],
    data_020be314[], data_020be318[], data_020be31c[], data_020be320[],
    data_020be324[], data_020be328[], data_020be32c[], data_020be330[],
    data_020be334[], data_020be338[], data_020be33c[], data_020be340[],
    data_020be344[], data_020be348[], data_020be34c[], data_020be350[],
    data_020be354[], data_020be358[], data_020be35c[], data_020be360[],
    data_020be364[], data_020be368[], data_020be36c[], data_020be370[],
    data_020be374[], data_020be378[], data_020be37c[], data_020be380[],
    data_020be384[], data_020be388[], data_020be38c[], data_020be390[],
    data_020be394[], data_020be398[], data_020be39c[], data_020be3a0[],
    data_020be3a4[], data_020be3a8[], data_020be3ac[], data_020be3b0[],
    data_020be3b4[], data_020be3b8[], data_020be3c0[], data_020be3c4[];
extern s32 data_020be3bc;
extern u8 data_020e3a80;
extern u32 data_020e3a84;
extern u8 data_020e3a88;
extern u8 data_020e3a8c;
extern u8 data_020e3a90;
extern s32 data_020e3a98;
extern u8 data_020e3b24[];
extern u8 data_020e3c28[];
extern u8 data_020e3ca0[];
extern PtrVec data_020e4150;
extern s32 data_020e6054;

void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200af18(s32 layer);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
s32 func_0204a868(u32 counter);
void func_02074c14(void *sprite, u32 a, u32 tile, u32 tile2, u32 b, s32 c);
void func_020108f4(void *text, const char *s);
void func_020107a8(void *obj);
void func_02053c38(PtrVec *strokes);
void func_02076cbc(void *p);
void func_02074e4c(void *p);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void *func_02072130(void *p);
extern u8 data_020be3e4[];

#define FN(f) ((void *(*)(void *))(void (*)(void))(f))

#define ICON(i) (data_020e3b24 + (i) * 0x34)
#define ENTRY(i) (data_020e3ca0 + (i) * 0x78)

/* show the icon of spell `spell` if it is owned */
static void Icon(u32 spell, u8 *s, const s32 *a, const s32 *b, const s32 *c, const s32 *d, u32 tile)
{
    void (*show)(void *, u32, void *);

    if (func_0204a868(spell) == 0)
        return;
    S32_AT(s, 4) = *a;
    S32_AT(s, 8) = *b;
    S32_AT(s, 0xc) = *c;
    S32_AT(s, 0x10) = *d;
    show = VCALL(s, 0, void (*)(void *, u32, void *));
    show(s, 1, (void *)show);
    s[0x14] = 1;
    func_02074c14(s, 0, tile, tile, 0x20, -1);
    data_020e3a88 = 1;
}

/* lay out a result line at (x, y, w, h) with text `s`, hidden */
static void Line(u8 *e, s32 x, s32 y, s32 w, s32 h, const char *s)
{
    s32 r[4];
    s32 bottom = y + h, right = x + w;

    r[0] = x;
    r[1] = y;
    r[2] = w;
    r[3] = h;
    func_020036e8((s32 *)e, &r[1], &r[0], &bottom, &right);
    func_020108f4(e + 0x14, s);
    func_020107a8(e);
    if (PTR_AT(e, 0x10) != NULL)
        func_02003cbc(PTR_AT(e, 0x10), 0);
    if (PTR_AT(e, 0x10) != NULL)
        U32_AT(PTR_AT(e, 0x10), 4) &= ~8u;
}

/* 0x02030b38: set up the spells menu
 * @difftest stub=0x0200aad0,0x020107a8 cases=5 */
void func_02030b38(void)
{
    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020aacac.v);
    func_0200af18(0);
    func_0200aad0(0, (u32)data_020be3c8, 0, 0, 0, 0);
    Icon(0x10, ICON(0), data_020be320, data_020be38c, data_020be388, data_020be384, 0x301);
    Icon(0xe, ICON(1), data_020be380, data_020be37c, data_020be378, data_020be374, 0x306);
    Icon(0xf, ICON(2), data_020be370, data_020be36c, data_020be368, data_020be348, 0x30b);
    Icon(0x11, ICON(3), data_020be360, data_020be324, data_020be2f4, data_020be354, 0x310);
    Icon(0x12, ICON(4), data_020be350, data_020be2e4, data_020be2c8, data_020be2f0, 0x316);
    if (func_0204a868(0x13) != 0)
        data_020e3a88 = 1;
    Line(ENTRY(0), *data_020be2f8, *data_020be2cc, *data_020be2c4, *data_020be3c4, data_020b654c);
    Line(ENTRY(1), *data_020be3c0, *data_020be330, *data_020be32c, *data_020be3b0, data_020b654c);
    Line(ENTRY(2), *data_020be3ac, *data_020be3a4, *data_020be3a0, *data_020be398, data_020b654c);
    Line(ENTRY(3), *data_020be31c, *data_020be318, *data_020be314, *data_020be310, data_020b654c);
    Line(ENTRY(4), *data_020be30c, *data_020be308, *data_020be364, *data_020be358, data_020b654c);
    Line(ENTRY(5), *data_020be2fc, *data_020be3b4, *data_020be338, *data_020be2ec, data_020b654c);
    Line(ENTRY(6), *data_020be3b8, *data_020be2e8, *data_020be3a8, *data_020be39c, data_020b654c);
    Line(ENTRY(7), *data_020be390, *data_020be2dc, *data_020be2d8, *data_020be304, data_020b654c);
    Line(ENTRY(8), *data_020be35c, *data_020be34c, *data_020be33c, *data_020be328, data_020b654c);
    Line(ENTRY(9), *data_020be394, *data_020be2c0, *data_020be2d4, *data_020be334, data_020b654c);
    Line(data_020e3c28, *data_020be2e0, *data_020be300, *data_020be344, *data_020be340, data_020b6560);
    func_02053c38(&data_020e4150);
    data_020e3a80 = 0;
    data_020e3a90 = 0;
    data_020e3a8c = 0;
    data_020e3a84 = 0;
    data_020e3a98 = 0;
    data_020e6054 = 0;
    data_020be3bc = -1;
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
}

/* 0x020317e4: destructor of the menu's text sprites
 * @difftest zero:0x40 cases=5 */
void *func_020317e4(u8 *p)
{
    PTR_AT(p, 0) = data_020be3e4;
    func_02076cbc(p);
    func_02074e4c(p + 0x18);
    return p;
}

/* 0x02031810: destroy the result lines
 * @difftest cases=3 */
void func_02031810(void)
{
    func_020a6d58(data_020e3ca0, 10, 0x78, FN(func_02072130));
}
