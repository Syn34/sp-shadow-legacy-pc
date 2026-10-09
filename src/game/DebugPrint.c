/*
 * The touch-screen debug text page: while it is shown, the game's print
 * hook (func_02080524) collects lines that are drawn on BG0 each frame.
 * ARM9 main, 0x0203a02c - 0x0203a22c (7 functions).
 *
 * Lines are 0x48-byte entries at data_020e8150 ({x, y, text[0x40]}, 0x18
 * of them, count data_020e8140; the count is not checked).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT / colour value passed by value in a register */

extern Cnt data_020b6810;
extern u32 data_020e8140;
extern u8 data_020e8150[];

void func_02080524(void (*hook)(const char *, s32, s32));
u32 func_02080be8(const char *s);                               /* strlen */
char *func_02080bb8(char *dst, const char *src, u32 n);         /* strncpy */
u32 func_0201341c(u32 tile, s32 x, s32 y, const u8 *str, u32 align);
void func_02013a5c(Cnt color);
void func_0203a028(void);
void func_0200af18(s32 layer);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */

void func_0203a178(const char *s, s32 x, s32 y);
void *func_0203a218(void *p);

#define LINE(i) (data_020e8150 + (i) * 0x48)

/* 0x0203a02c: leave the page
 * @difftest cases=5 */
void func_0203a02c(void)
{
    func_0200b098(0);
    func_0200b098(2);
    func_0200b098(1);
    func_0200b098(3);
    func_02080524(NULL);
}

/* 0x0203a068: draw the lines collected this frame
 * @difftest @020e8140:32=int:0:3 @020e8150:32=int:0:200 @020e8154:32=int:0:150 @020e8158:32=0x41414141 @020e815c:32=0 @020e8198:32=int:0:200 @020e819c:32=int:0:150 @020e81a0:32=0x42 @020e81e0:32=int:0:200 @020e81e4:32=int:0:150 @020e81e8:32=0x4343 cases=20 */
void func_0203a068(void)
{
    u32 t, i;

    func_0200af18(0);
    func_02013a5c(data_020b6810);
    func_0201391c(1, 0, -1);
    t = 1;
    for (i = 0; i < data_020e8140; i++)
        t = func_0201341c(t, S32_AT(LINE(i), 0), S32_AT(LINE(i), 4), LINE(i) + 8, 0);
    data_020e8140 = 0;
    func_0203a028();
}

/* 0x0203a110: set up the page
 * @difftest cases=5 */
void func_0203a110(void)
{
    func_0200b168(0, data_020b6810.v);
    func_0200af18(0);
    func_02080524(func_0203a178);
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
}

/* 0x0203a178: the print hook: add line `s` at column x, row y
 * @difftest $S=zero:0x80 @$S+0:32=0x6c6c6548 @$S+4:32=0x6f $S int:0:256 int:0:16 @020e8140:32=int:0:23 cases=40
 * @difftest $S=ptr:0x80:4 @$S+0x7c:32=0 $S int:0:256 int:0:16 @020e8140:32=int:0:23 cases=20 */
void func_0203a178(const char *s, s32 x, s32 y)
{
    u32 len = func_02080be8(s);
    u32 n = data_020e8140;
    u8 *e;

    if ((s32)len >= 0x3f)
        len = 0x3f;
    e = LINE(n);
    data_020e8140 = n + 1;
    func_02080bb8((char *)e + 8, s, len);
    e[8 + len] = 0;
    S32_AT(e, 0) = x;
    S32_AT(e, 4) = y * 0xc;
}

/* 0x0203a1f4: destroy the lines
 * @difftest cases=3 */
void func_0203a1f4(void)
{
    func_020a6d58(data_020e8150, 0x18, 0x48, func_0203a218);
}

/* 0x0203a218: a line's destructor
 * @difftest ptr:0x48 cases=3 */
void *func_0203a218(void *p)
{
    return p;
}

/* 0x0203a21c: clear a position
 * @difftest ptr:8:4 cases=3 */
void func_0203a21c(s32 *p)
{
    p[0] = 0;
    p[1] = 0;
}
