/*
 * Scroll.cpp, part 1: the scrolling BG maps made of 4x4-tile blocks
 * ("scrollLoadMap", "scrollLoadMap4x4"), changing a block and redrawing it,
 * and projecting world positions to the screen.
 * ARM9 main, 0x0201b9c0 - 0x0201c428 (10 functions).
 *
 * Per screen, data_020e2058 + screen * 0x130:
 *   +0x08/+0x0c and +0x10/+0x14 the two visible windows (pixels),
 *   +0x30 scroll targets, +0xb4 a 256-entry fade table,
 *   +0x44 two layers of 0x50 bytes:
 *     +0x04 BGxCNT, +0x08 blocks loaded, +0x0c palette, +0x10/+0x14 map and
 *     block files, +0x18 map (u16 block per 32x32 px), +0x1c blocks
 *     (16 u16 tiles each, then 16 attribute bytes each), +0x2c/+0x30
 *     buffer sizes, +0x34 map bytes, +0x38/+0x3a map width/height in
 *     blocks, +0x3c/+0x3e in tiles, +0x40/+0x44 in pixels, +0x48 blocks,
 *     +0x4c their tile bytes.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const char data_020bdcec[];   /* "Scroll.cpp" */
extern const char data_020bdcf8[], data_020bdd28[], data_020bdd60[], data_020bdd74[],
    data_020bdda0[], data_020bddd8[];
extern u8 *data_020c3b60;            /* levels, 0x60 bytes each */
extern u8 data_020e2058[];

void func_0200fdb4(const u8 *src, void *dst);
u32 func_0200feec(const u8 *src);
void func_020129ac(s32 id);
u32 func_0201d4a0(u32 tile, u32 pal, u32 a, u32 layer);
u32 func_0201de40(u32 tile, u32 bank, u32 layer);

/* this file's own functions, called before their definition */
u16 *func_0201baa8(u32 x, u32 y, u32 layer);
void func_0201bd1c(u32 tile, u32 pal, u32 layer, u32 x, u32 y, u32 blk);
s32 func_0201c340(s32 y);

#define SCR() (data_020e2058 + data_020df0fc * 0x130)
#define LAYER(i) (SCR() + 0x44 + (i) * 0x50)

static s32 RoundShl(s32 v, s32 sh)
{
    float f = (float)SHL(v, sh);
    return (s32)(v > 0 ? 0.5f + f : f - 0.5f);
}

/* 0x0201b9c0: build the fade table */
void func_0201b9c0(void)
{
    s32 size[2], tmp[3];
    u32 i;

    size[0] = 0;
    size[1] = 0;
    if (PTR_AT(SCR(), 0xb4) == NULL)
        PTR_AT(SCR(), 0xb4) = func_0207ff70(0x200, data_020bdcec, 0xcd4);
    func_0201c428(tmp);
    size[1] = tmp[1];
    size[0] = tmp[0];
    (void)size;
    for (i = 0; i < 0x100; i++) {
        s32 v = 0x7fff - (s32)(i * 0x80);
        if (v < 0)
            v = 0;
        ((u16 *)PTR_AT(SCR(), 0xb4))[i] = v;
    }
}

/* 0x0201baa8: map entry of the block under a pixel position
 * @difftest int:0:0x800 int:0:0x800 int:0:1 @020df0fc:32=int:0:1 */
u16 *func_0201baa8(u32 x, u32 y, u32 layer)
{
    u8 *l = LAYER(layer);
    u32 bx = (x << 11) >> 16, by = (y << 11) >> 16;
    return (u16 *)((u8 *)PTR_AT(l, 0x18) + (((by * U16_AT(l, 0x38) + bx) << 16) >> 15));
}

/* 0x0201bb08: block under a pixel position
 * @difftest int:0:0x800 int:0:0x800 int:0:1 @020df0fc:32=int:0:1 */
u32 func_0201bb08(u32 x, u32 y, u32 layer) { return *func_0201baa8(x, y, layer); }

/* 0x0201bb24: change the block under a pixel position and redraw it where
 * it is visible */
void func_0201bb24(u32 x, u32 y, u32 blk, u32 layer)
{
    u16 *p = func_0201baa8(x, y, layer), *oldt, *newt;
    u32 old = *p, row, col, pal;
    u8 *s = SCR(), *l;
    u16 cx = ((x << 11) >> 16) << 18 >> 16;
    u16 cy;
    u32 ax = S32_AT(s, 0x10) >> 3, ay = S32_AT(s, 0x14) >> 3;
    u32 bx = S32_AT(s, 0x08) >> 3, by = S32_AT(s, 0x0c) >> 3;

    l = s + 0x44 + layer * 0x50;
    cy = ((y << 11) >> 16) << 18 >> 16;
    oldt = (u16 *)((u8 *)PTR_AT(l, 0x1c) + ((old << 21) >> 16));
    pal = l[0xc];
    newt = (u16 *)((u8 *)PTR_AT(l, 0x1c) + blk * 32);
    for (row = 0; row < 4; row++) {
        for (col = 0; col < 4; col++) {
            if (cx >= ax && cx < ax + 0x21 && cy >= ay && cy < ay + 0x19) {
                func_0201de40(*oldt, pal, layer);
                func_0201bd1c(*newt, pal, layer, cx, cy, blk);
            } else if (cx >= bx && cx < bx + 0x21 && cy >= by && cy < by + 0x19) {
                func_0201bd1c(*newt, pal, layer, cx, cy, blk);
            }
            cx++;
            oldt++;
            newt++;
        }
        cx -= 4;
        cy++;
    }
    *p = blk;
}

/* 0x0201bd1c: write one tile of block `blk` into the BG map at tile (x, y) */
void func_0201bd1c(u32 tile, u32 pal, u32 layer, u32 x, u32 y, u32 blk)
{
    u8 *l = LAYER(layer);
    u16 b = blk;
    u32 base = SHL(data_020df0fc, 21) + 0x06000000 + (BITS(U16_AT(l, 4), 8, 5) << 11);
    u8 *attr = (u8 *)PTR_AT(l, 0x1c) + U32_AT(l, 0x4c) + (s32)(b << 5) / 2;
    u16 *row = (u16 *)(base + ((y & 0x3f) << 7));
    u32 cx = x & 0x3f;

    if (layer != 1)
        row[cx] = func_0201d4a0(tile, pal, 0, layer);
    row[cx] = row[cx] | (attr[(y & 3) * 4 + (x & 3)] << 10);
}

/* 0x0201bdd8: load the 4x4 blocks of a layer ("scrollLoadMap4x4") */
void func_0201bdd8(u32 layer, s32 file, u32 size)
{
    u8 *l, *src;
    func_02004490(data_020bdcf8, layer, file);
    l = LAYER(layer);
    if (PTR_AT(l, 0x14) == NULL) {
        PTR_AT(l, 0x14) = func_0207ff70(size, data_020bdcec, 0xb9e);
        U32_AT(l, 0x30) = size;
    }
    src = func_02012a64(file);
    func_02004490(data_020bdd28, func_0200feec(src), U32_AT(l, 0x30));
    func_0200fdb4(src, PTR_AT(l, 0x14));
    PTR_AT(l, 0x1c) = (u8 *)PTR_AT(l, 0x14) + 4;
    U32_AT(l, 0x48) = U32_AT(PTR_AT(l, 0x14), 0);
    U32_AT(l, 0x4c) = U32_AT(l, 0x48) << 5;
    func_02004490(data_020bdd60, U32_AT(l, 0x48));
    U32_AT(l, 8) = 1;
    func_020129ac(file);
}

/* 0x0201bed4: load the map of a layer ("scrollLoadMap") */
void func_0201bed4(u32 layer, s32 file, u32 size)
{
    u8 *l, *src;
    func_02004490(data_020bdd74, layer, file);
    l = LAYER(layer);
    if (PTR_AT(l, 0x10) == NULL) {
        PTR_AT(l, 0x10) = func_0207ff70(size, data_020bdcec, 0xb4f);
        U32_AT(l, 0x2c) = size;
    }
    src = func_02012a64(file);
    func_02004490(data_020bdda0, func_0200feec(src), U32_AT(l, 0x2c));
    func_0200fdb4(src, PTR_AT(l, 0x10));
    PTR_AT(l, 0x18) = (u8 *)PTR_AT(l, 0x10) + 4;
    U16_AT(l, 0x38) = U8_AT(PTR_AT(l, 0x10), 0);
    U16_AT(l, 0x3a) = U8_AT(PTR_AT(l, 0x10), 2);
    U16_AT(l, 0x3c) = U16_AT(l, 0x38) << 2;
    U16_AT(l, 0x3e) = U16_AT(l, 0x3a) << 2;
    U32_AT(l, 0x34) = U16_AT(l, 0x3a) * (U16_AT(l, 0x38) << 1);
    U32_AT(l, 0x40) = U16_AT(l, 0x38) << 5;
    U32_AT(l, 0x44) = U16_AT(l, 0x3a) << 5;
    func_02004490(data_020bddd8, U16_AT(l, 0x38), U16_AT(l, 0x3a));
    func_020129ac(file);
}

/* project the x/y of pos towards the camera by the zoom */
static void Zoom(s32 *out, const s32 *pos, u8 *cam, s32 scale)
{
    s32 cx = S32_AT(cam, 0x28), cy = S32_AT(cam, 0x2c);
    out[0] = pos[0] - cx;
    out[1] = pos[1] - cy;
    out[0] = FxMul(out[0], scale);
    out[1] = FxMul(out[1], scale);
    out[0] = out[0] + cx;
    out[1] = out[1] + cy;
    out[2] = func_0201c340(out[1]);
}

/* 0x0201c010: screen position of a world position (x, y, height) */
void func_0201c010(s32 *out, const s32 *pos)
{
    s32 scale, sh[2], size[3], h, v;
    u8 *cam;

    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    scale = S32_AT(func_020062e0(), 0x370);
    cam = func_0201f490();
    Zoom(out, pos, cam, scale);
    func_02003b08(sh, (s32 *)(cam + 0x28), 4);
    h = pos[2] - (func_0201a0a0(sh, NULL) >> 4);
    func_0201c428(size);
    v = size[1] - (out[1] >> 12) + (FxMul(scale, h) >> 12);
    out[1] = RoundShl(v, 12);
}

/* 0x0201c17c: screen position of a point on the ground (pos is changed to
 * its >> 4 value) */
void func_0201c17c(s32 *out, s32 *pos)
{
    s32 scale = S32_AT(func_020062e0(), 0x370), sh[2], p[2], size[3], h, g, r;
    u8 *cam;

    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    cam = func_0201f490();
    Zoom(out, pos, cam, scale);
    func_02003b08(sh, pos, 4);
    pos[0] = sh[0];
    pos[1] = sh[1];
    p[0] = pos[0];
    p[1] = pos[1];
    g = func_0201a0a0(p, NULL);
    func_02003b08(sh, (s32 *)(cam + 0x28), 4);
    h = (g - func_0201a0a0(sh, NULL)) >> 4;
    func_0201c428(size);
    if (size[1] > 0) {
        float f;
        func_0201c428(size);
        f = (float)SHL(size[1], 12);
        r = (s32)(0.5f + f);
    } else {
        float f;
        func_0201c428(size);
        f = (float)SHL(size[1], 12);
        r = (s32)(f - 0.5f);
    }
    out[1] = FxMul(scale, h) + (r - out[1]);
}

/* 0x0201c340: screen y of a world y on the current level
 * @difftest s32 */
s32 func_0201c340(s32 y)
{
    s32 r;
    if (S32_AT(data_020c3b60 + (func_020436f0(data_020bf6a0)[0xf] >> 1) * 0x60, 0x38) > 0) {
        float f = (float)SHL(S32_AT(data_020c3b60 + (func_020436f0(data_020bf6a0)[0xf] >> 1) * 0x60, 0x38), 12);
        r = (s32)(0.5f + f);
    } else {
        float f = (float)SHL(S32_AT(data_020c3b60 + (func_020436f0(data_020bf6a0)[0xf] >> 1) * 0x60, 0x38), 12);
        r = (s32)(f - 0.5f);
    }
    return y - r + S32_AT(data_020df1bc, 0xc) - 0x1d000;
}
