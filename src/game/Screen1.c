/*
 * Save memory layout (options chunk and three save slots), screen fades
 * ("screenFadeOut"/"screenFadeIn", master brightness), reading a file at
 * an offset, and the first scroll reset.
 * ARM9 main, 0x0201afd8 - 0x0201b9c0 (24 functions).
 *
 * Fades: data_020e1e9c + screen * 0x20: +0 fade type, +4 state (1 fading
 * in, 2 fading out), +8 parameter, +0xc brightness (-16..16).
 * Fade steps per type: data_020b5788 (in) and data_020b57c8 (out).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define REG16(a) (*(vu16 *)(a))

extern s32 (*data_020b5788[])(u32 param);   /* fade-in steps */
extern s32 (*data_020b57c8[])(u32 param);   /* fade-out steps */
extern const char data_020bda1c[], data_020bda50[], data_020bda58[], data_020bda70[],
    data_020bda9c[], data_020bdac8[], data_020bdaf0[], data_020bdb08[], data_020bdb30[],
    data_020bdb58[], data_020bdb80[], data_020bdba8[], data_020bdbd4[], data_020bdbfc[],
    data_020bdc24[], data_020bdc3c[], data_020bdc68[], data_020bdc94[], data_020bdca8[],
    data_020bdcc8[];
extern s8 data_020e1e78;             /* save layout ready */
extern u32 data_020e1e7c;            /* bytes for options */
extern u32 data_020e1e80;            /* chunks for options */
extern u32 data_020e1e84;            /* first save slot chunk */
extern u32 data_020e1e88;            /* save slot size */
extern u32 data_020e1e8c;            /* chunks per save slot */
extern u32 data_020e1e90;            /* options chunk */
extern u32 data_020e1e94;            /* save game data bytes */
extern u32 data_020e1e98;            /* save game options bytes */
extern u8 data_020e1e9c[];           /* fades */
extern u8 data_020e2058[];           /* scroll state, 0x130 per screen */

u32 func_02009548(void);
u32 func_02009550(void);
u32 func_02009558(void);
void func_020094f4(void);
void func_020803d0(void *file, u32 pos, u32 whence);
void func_0208f800(u32 reg, s32 level);   /* GX(S)_SetMasterBrightness */
void func_02090ad8(u32 reg, u32 a, u32 b, u32 c, u32 d);
u32 func_020979a8(void *file, void *dst, u32 len);
u64 func_020a500c(u32 num, u32 den);           /* unsigned divide */

/* this file's own functions, called before their definition */
u32 func_0201b290(u32 slot);
void func_0201b754(u32 screen, u32 param, u32 type);
void func_0201b808(u32 screen);
void func_0201b848(u32 which, u32 mask, s32 level);

#define FADE(scr) (data_020e1e9c + (scr) * 0x20)

/* 0x0201afd8: lay out the save memory and print it */
void func_0201afd8(void)
{
    u32 q;

    data_020e1e90 = 0;
    q = (u32)func_020a500c(0x20, func_02009558());
    data_020e1e80 = q;
    data_020e1e7c = q * func_02009558();
    data_020e1e98 = 0x20;
    data_020e1e84 = data_020e1e90 + q;
    data_020e1e8c = (func_02009550() - data_020e1e80) / 3;
    data_020e1e88 = data_020e1e8c * func_02009558();
    data_020e1e94 = data_020e1e88 - data_020e1e98;

    func_02004490(data_020bda1c);
    func_02004490(data_020bda50);
    func_02004490(data_020bda58);
    func_02004490(data_020bda50);
    func_02004490(data_020bda70, func_02009548());
    func_02004490(data_020bda9c, func_02009558());
    func_02004490(data_020bda50);
    func_02004490(data_020bdac8, func_02009550());
    func_02004490(data_020bda50);
    func_02004490(data_020bda1c);
    func_02004490(data_020bda50);
    func_02004490(data_020bdaf0);
    func_02004490(data_020bda50);
    func_02004490(data_020bdb08, data_020e1e7c);
    func_02004490(data_020bdb30, data_020e1e80);
    func_02004490(data_020bdb58, data_020e1e90);
    func_02004490(data_020bda50);
    func_02004490(data_020bdb80, 3);
    func_02004490(data_020bdba8, data_020e1e88);
    func_02004490(data_020bdbd4, data_020e1e8c);
    func_02004490(data_020bdbfc, data_020e1e84);
    func_02004490(data_020bda50);
    func_02004490(data_020bda1c);
    func_02004490(data_020bda50);
    func_02004490(data_020bdc24);
    func_02004490(data_020bda50);
    func_02004490(data_020bdc3c, data_020e1e98);
    func_02004490(data_020bdc68, data_020e1e94);
    func_02004490(data_020bda50);
    func_02004490(data_020bda1c);
    func_020094f4();
    data_020e1e78 = 1;
}

/* 0x0201b254
 * @difftest */
s32 func_0201b254(void) { return data_020e1e78; }

/* 0x0201b264
 * @difftest */
u32 func_0201b264(void) { return data_020e1e94; }

/* 0x0201b274: byte offset of save slot `slot`
 * @difftest int:0:3 */
u32 func_0201b274(u32 slot) { return func_0201b290(slot) * func_02009558(); }

/* 0x0201b290: first chunk of save slot `slot`
 * @difftest int:0:3 */
u32 func_0201b290(u32 slot) { return data_020e1e8c * slot + data_020e1e84; }

/* 0x0201b2b0: one step brighter (to white)
 * @difftest @020df0fc:32=int:0:1 @020e1ea8:32=int:-20:20 @020e1ec8:32=int:-20:20 u32 */
s32 func_0201b2b0(u32 mask)
{
    u32 s = data_020df0fc;
    if (S32_AT(FADE(s), 0xc) >= 0x10)
        return 0;
    S32_AT(FADE(s), 0xc) += 4;
    if (S32_AT(FADE(data_020df0fc), 0xc) > 0x10)
        S32_AT(FADE(data_020df0fc), 0xc) = 0x10;
    func_0201b848(data_020df0fc != 0 ? 1 : 0, mask, S32_AT(FADE(s), 0xc));
    return 1;
}

/* 0x0201b348: one step back from white
 * @difftest @020df0fc:32=int:0:1 @020e1ea8:32=int:-20:20 @020e1ec8:32=int:-20:20 u32 */
s32 func_0201b348(u32 mask)
{
    u32 s = data_020df0fc;
    if (S32_AT(FADE(s), 0xc) == 0)
        return 0;
    S32_AT(FADE(s), 0xc) -= 4;
    if (S32_AT(FADE(data_020df0fc), 0xc) < 0)
        S32_AT(FADE(data_020df0fc), 0xc) = 0;
    func_0201b848(data_020df0fc != 0 ? 1 : 0, mask, S32_AT(FADE(s), 0xc));
    return 1;
}

/* 0x0201b3e0: one step darker (to black)
 * @difftest @020df0fc:32=int:0:1 @020e1ea8:32=int:-20:20 @020e1ec8:32=int:-20:20 u32 */
s32 func_0201b3e0(u32 mask)
{
    u32 s = data_020df0fc;
    if (S32_AT(FADE(s), 0xc) <= -0x10)
        return 0;
    S32_AT(FADE(s), 0xc) -= 4;
    if (S32_AT(FADE(data_020df0fc), 0xc) < -0x10)
        S32_AT(FADE(data_020df0fc), 0xc) = -0x10;
    func_0201b848(data_020df0fc != 0 ? 1 : 0, mask, S32_AT(FADE(s), 0xc));
    return 1;
}

/* 0x0201b47c: one step back from black
 * @difftest @020df0fc:32=int:0:1 @020e1ea8:32=int:-20:20 @020e1ec8:32=int:-20:20 u32 */
s32 func_0201b47c(u32 mask)
{
    u32 s = data_020df0fc;
    if (S32_AT(FADE(s), 0xc) == 0)
        return 0;
    S32_AT(FADE(s), 0xc) += 4;
    if (S32_AT(FADE(data_020df0fc), 0xc) > 0)
        S32_AT(FADE(data_020df0fc), 0xc) = 0;
    func_0201b848(data_020df0fc != 0 ? 1 : 0, mask, S32_AT(FADE(s), 0xc));
    return 1;
}

/* 0x0201b514
 * @difftest u32 */
s32 func_0201b514(u32 param) { return 0; }

/* 0x0201b51c: start fading out a screen ("screenFadeOut")
 * @difftest u32 int:0:3 int:0:1 */
void func_0201b51c(u32 param, u32 mode, u32 screen)
{
    func_02004490(data_020bdc94);
    if (mode == 1) {
        func_0201b754(screen, param, 1);
        S32_AT(FADE(screen), 4) = 2;
    } else if (mode == 2) {
        func_0201b754(screen, param, 2);
        S32_AT(FADE(screen), 4) = 2;
    }
}

/* 0x0201b5a4: fade out the current screen
 * @difftest u32 int:0:3 @020df0fc:32=int:0:1 */
void func_0201b5a4(u32 param, u32 mode) { func_0201b51c(param, mode, data_020df0fc); }

/* 0x0201b5bc: start fading in the current screen ("screenFadeIn")
 * @difftest u32 int:0:3 @020df0fc:32=int:0:1 */
void func_0201b5bc(u32 param, u32 mode)
{
    func_02004490(data_020bdca8);
    if (mode == 1) {
        func_0201b754(data_020df0fc, param, 1);
        func_0201b808(data_020df0fc);
    } else if (mode == 2) {
        func_0201b754(data_020df0fc, param, 2);
        func_0201b808(data_020df0fc);
    }
}

/* 0x0201b650: is a screen fading
 * @difftest int:0:1 */
s32 func_0201b650(u32 screen) { return (s8)(S32_AT(FADE(screen), 4) != 0); }

/* 0x0201b678: run one fade step; 0 when done */
s32 func_0201b678(void)
{
    u32 s = data_020df0fc;
    u8 *f = FADE(s);
    u32 st = U32_AT(f, 4);
    s32 r = 0;

    if (st & 1) {
        r = data_020b5788[U32_AT(FADE(s), 0)](U32_AT(f, 8));
        if (r == 0 && data_020df0fc == 0)
            func_02090ad8(0x04000050, 1, 4, 0x1f, 0);
    } else if (st & 2) {
        r = data_020b57c8[U32_AT(FADE(s), 0)](U32_AT(f, 8));
    }
    if (r == 0)
        S32_AT(FADE(data_020df0fc), 4) = 0;
    return r;
}

/* 0x0201b754
 * @difftest int:0:1 u32 int:0:3 */
void func_0201b754(u32 screen, u32 param, u32 type)
{
    U32_AT(FADE(screen), 8) = param;
    U32_AT(FADE(screen), 0) = type;
}

/* 0x0201b770: set the fade of screen 0, 1 or both (2)
 * @difftest int:0:3 u32 int:0:3 */
void func_0201b770(u32 which, u32 param, u32 type)
{
    switch (which) {
    case 0:
        func_0201b754(0, param, type);
        break;
    case 1:
        func_0201b754(1, param, type);
        break;
    case 2:
        func_0201b754(0, param, type);
        func_0201b754(1, param, type);
        break;
    }
}

/* 0x0201b7f0
 * @difftest int:0:1 */
void func_0201b7f0(u32 screen) { S32_AT(FADE(screen), 4) = 2; }

/* 0x0201b808
 * @difftest int:0:1 */
void func_0201b808(u32 screen) { S32_AT(FADE(screen), 4) = 1; }

/* 0x0201b820: set the blend control of the current screen
 * @difftest u8 u8 @020df0fc:32=int:0:1 */
void func_0201b820(u32 a, u32 b)
{
    REG16(0x04000050 + (data_020df0fc << 12)) = a | 0x40 | (b << 8);
}

/* 0x0201b848: set the brightness of screen 0, 1 or both (2)
 * @difftest int:0:3 u32 int:-16:16 */
void func_0201b848(u32 which, u32 mask, s32 level)
{
    switch (which) {
    case 0:
        S32_AT(FADE(0), 0xc) = level;
        func_0208f800(0x0400006c, level);
        break;
    case 1:
        S32_AT(FADE(1), 0xc) = level;
        func_0208f800(0x0400106c, level);
        break;
    case 2:
        S32_AT(FADE(0), 0xc) = level;
        S32_AT(FADE(1), 0xc) = level;
        func_0208f800(0x0400006c, level);
        func_0208f800(0x0400106c, level);
        break;
    }
}

/* 0x0201b8e0: both screens black, no fade running
 * @difftest */
void func_0201b8e0(void)
{
    func_0201b770(2, 0x3f, 1);
    S32_AT(FADE(0), 4) = 0;
    S32_AT(FADE(1), 4) = 0;
    S32_AT(FADE(0), 0xc) = -0x10;
    S32_AT(FADE(1), 0xc) = -0x10;
    func_0201b848(2, 0x3f, -0x10);
}

/* 0x0201b930: read `size` bytes at `offset` of a file */
s32 func_0201b930(void *file, void *dst, u32 offset, u32 size)
{
    func_020803d0(file, offset, 0);
    if (func_020979a8(file, dst, size) >= size)
        return 1;
    func_02004490(data_020bdcc8);
    return 0;
}

/* 0x0201b984: reset the current screen's scroll targets
 * @difftest @020df0fc:32=int:0:1 */
void func_0201b984(void)
{
    u8 *s = data_020e2058 + data_020df0fc * 0x130;
    u32 i;
    for (i = 0; i < 2; i++)
        S32_AT(s + i * 8, 0x30) = -1;
}
