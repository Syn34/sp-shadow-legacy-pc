/*
 * The touch-screen glyph recogniser: strokes drawn on the touch screen
 * are matched against the glyph prototypes (data_020bb7ac) with the SDK's
 * pattern recognition library (PRC: func_0209b150 database size,
 * func_0209b078 database set-up, func_0209b06c / func_0209b6c0 buffer
 * sizes, func_0209afcc input pattern, func_0209b19c recognition).
 * ARM9 main, 0x02053a94 - 0x02053dfc (5 functions).
 *
 * Layout: the stroke base (func_020718c0 / func_020718bc, reset
 * func_0207188c) starts with the points {data, count, capacity} (u16 x, y
 * each; -1, -1 ends a stroke), +0x100c a stroke is open, +0x1010 the point
 * count, +0x1014 the prototype database, +0x105c the input pattern,
 * +0x109c its parameters, +0x10a8 / +0x10ac / +0x10b0 the buffers.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bb7ac[];           /* the glyph prototypes */
extern const char data_020c07e4[], data_020c0800[];   /* message, source file name */

void *func_0207ff70(u32 size, const char *file, s32 line);
void func_0207ff14(void *ptr, const char *file, s32 line);
void func_0209a344(void *self);
void func_0207188c(void *self);
void *func_020718bc(void *self);
void *func_020718c0(void *self);
u32 func_0209b150(void *protos, s32 kinds, u32 a, u32 b);
void func_0209b078(void *db, void *buf, void *protos, s32 kinds, u32 a, u32 b);
u32 func_0209b06c(u32 points, u32 strokes);
u32 func_0209b6c0(u32 points, u32 strokes, void *db, u32 a);
void func_0209afcc(void *pattern, void *buf, void *strokes, u32 points, u32 nstrokes, void *param);
void func_0209b19c(void **entry, s32 *score, u32 n, void *buf, void *pattern, void *db, s32 kinds, u32 a);

/* 0x02053a94: recognise what was drawn: the glyph's code (-1: none), its
 * score in *score
 * @difftest $S=zero:0x10b4 @$S+4:32=pick:0,1 @$S+0x1010:32=pick:0,1,2 $S ptr:4:4 stub=0x0209afcc,0x0209b19c,0x0207188c cases=6
 * @difftest $P=zero:0x10 $S=zero:0x10b4 @$S:32=$P @$S+4:32=1 @$S+8:32=pick:1,4 @$S+0x100c:8=pick:0,1 @$S+0x1010:32=2 $S ptr:4:4 stub=0x0209afcc,0x0209b19c,0x0207188c cases=10 */
s32 func_02053a94(u8 *self, s32 *score)
{
    void *e;
    s32 sc, code = -1;

    if (U32_AT(self, 4) == 0)
        return -1;
    if (U32_AT(self, 0x1010) < 2)
        return -1;
    if (S8_AT(self, 0x100c) == 0) {
        self[0x100c] = 1;
        if (U32_AT(self, 4) < U32_AT(self, 8)) {
            U16_AT(PTR_AT(self, 0), U32_AT(self, 4) * 4) = 0xffff;
            U16_AT(PTR_AT(self, 0), U32_AT(self, 4) * 4 + 2) = 0xffff;
            U32_AT(self, 4)++;
        }
    }
    func_0209afcc(self + 0x105c, PTR_AT(self, 0x10ac), self, 0x28, 0x14, self + 0x109c);
    func_0209b19c(&e, &sc, 2, PTR_AT(self, 0x10b0), self + 0x105c, self + 0x1014, -1, 0);
    if (e == NULL) {
        *score = 0;
        code = -1;
    } else {
        code = U16_AT(e, 8);
        *score = sc;
    }
    func_02004490(data_020c07e4, code, sc >> 12, (s32)((sc & 0xfff) * 1000) / 4096);
    func_0207188c(self);
    return code;
}

/* 0x02053c38: set the recogniser up
 * @difftest zero:0x10b4 stub=0x0209a344,0x0207188c,0x0209b150:0x40,0x0207ff70:0x1234,0x0209b078,0x0209b06c:0x40,0x0209b6c0:0x40 cases=3 */
void func_02053c38(u8 *self)
{
    func_0209a344(self);
    func_0207188c(self);
    PTR_AT(self, 0x10a8) = func_0207ff70(func_0209b150(data_020bb7ac, -1, 0, 0), data_020c0800, 0x3c);
    func_0209b078(self + 0x1014, PTR_AT(self, 0x10a8), data_020bb7ac, -1, 0, 0);
    U32_AT(self, 0x109c) = U32_AT(self, 0x104c);
    U32_AT(self, 0x10a0) = 1;
    U32_AT(self, 0x10a4) = 0x18;
    PTR_AT(self, 0x10ac) = func_0207ff70(func_0209b06c(0x28, 0x14), data_020c0800, 0x50);
    PTR_AT(self, 0x10b0) = func_0207ff70(func_0209b6c0(0x28, 0x14, self + 0x1014, 0), data_020c0800, 0x58);
}

/* 0x02053d1c: free the recogniser's buffers
 * @difftest ptr:0x10b4:4 stub=0x0207ff14 cases=3 */
void func_02053d1c(u8 *self)
{
    func_0207ff14(PTR_AT(self, 0x10a8), data_020c0800, 0x28);
    PTR_AT(self, 0x10a8) = NULL;
    func_0207ff14(PTR_AT(self, 0x10ac), data_020c0800, 0x2b);
    PTR_AT(self, 0x10ac) = NULL;
    func_0207ff14(PTR_AT(self, 0x10b0), data_020c0800, 0x2e);
    PTR_AT(self, 0x10b0) = NULL;
}

/* 0x02053d88: destructor
 * @difftest ptr:0x10b4:4 stub=0x0207ff14,0x020718bc cases=3 */
u8 *func_02053d88(u8 *self)
{
    func_0207ff14(PTR_AT(self, 0x10a8), data_020c0800, 0x21);
    func_0207ff14(PTR_AT(self, 0x10ac), data_020c0800, 0x22);
    func_0207ff14(PTR_AT(self, 0x10b0), data_020c0800, 0x23);
    func_020718bc(self);
    return self;
}

/* 0x02053de4: constructor
 * @difftest ptr:0x10b4:4 stub=0x020718c0 cases=3 */
u8 *func_02053de4(u8 *self)
{
    func_020718c0(self);
    return self;
}
