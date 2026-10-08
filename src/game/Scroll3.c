/*
 * Scroll.cpp, part 3: drawing and releasing whole rows and columns of the
 * scrolling map as the camera moves, the per-frame scroll update, the 3D
 * rear-plane (clear image) bitmap that layer 1 is drawn into, set-up
 * ("scrollInit") and the per-screen state's constructor and destructor.
 * ARM9 main, 0x0201d638 - 0x0201f004 (19 functions).
 *
 * Layer 0 is a 64x64-tile BG whose tiles come from the tile cache
 * (Scroll2.c). Layer 1 is drawn as direct colour into a 512x256 bitmap
 * (data_020e2058 + 0xbc, screen 0); 256x256 of it is copied, wrapping, into
 * VRAM bank A while it is mapped to the CPU, and shown as the 3D clear image
 * (REG_CLRIMAGE_OFFSET scrolls it).
 *
 * Per screen, data_020e2058 + screen * 0x130 (see Scroll1.c, Scroll2.c):
 *   +0x00/+0x04 wanted, +0x08/+0x0c drawn, +0x10/+0x14 previous camera
 *   position; +0x18..+0x24 the old/new positions for the bitmap copy;
 *   +0x30[2] {x, y} of a column (0) / row (1) to release next frame;
 *   +0x40 the camera may pass the right edge; +0x124 bit 2 bitmap copy
 *   pending.
 * data_020e1ee0 is the camera position (top-left corner, pixels).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define REG16(a) (*(vu16 *)(a))

extern const char data_020bdcec[];   /* "Scroll.cpp" */
extern s32 data_020e1ee0[2];         /* camera position */
extern u8 data_020e1edc[];           /* read-ahead slot (u32) */
extern u8 data_020e1fa8[];           /* read-ahead slots, 0x58 bytes each */
extern u8 data_020e2058[];

void func_02002164(void *self);      /* empty destructor (2D vector) */
void func_02005cb0(s32 *v);          /* 2D vector constructor */
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void func_020a6dcc(void *arr, u32 n, u32 size, void *(*ctor)(void *), void *(*dtor)(void *));   /* __construct_array */
void func_0200a044(s32 layer, s32 angle, s32 speed);
void func_0200a0b0(s32 layer, s32 x, s32 y);
void func_0200a188(s32 layer);
s32 func_0200bea4(void *z);
s32 func_0200beac(void *z);
void func_0201c45c(const s32 *pos);
void func_0201d308(u16 *dst, s32 tile, u32 x, u32 y, u32 pal, u32 layer);
u32 func_0201d4a0(u32 tile, u32 bank, u32 unused, u32 layer);
void func_0201f60c(s32 *pos);
void func_0207fe8c(u32 dma, void *dst, u32 value, u32 size, u32 width);   /* fill */
void func_0208fb8c(void);
void func_0208fbb4(void);
void func_0208fcdc(void);            /* map VRAM bank A to the CPU (LCDC) */
void func_0208fff8(u32 banks);       /* map VRAM banks as texture image */
void func_02090768(u32 v);
void func_02090da0(u32 colour, u32 alpha, u32 depth, u32 id, u32 fog);   /* G3X_SetClearColor */

void func_0201d638(u32 x, u32 y, u32 wrap);
void func_0201d8f0(u32 x, u32 y, u32 wrap);
void func_0201dbb0(u32 x, u32 y, u32 wrap);
void func_0201dd14(u32 x, u32 y, u32 wrap);
u32 func_0201de40(u32 tile, u32 bank, u32 layer);
void func_0201e0a8(s32 x, s32 y, u32 i);
void func_0201e0e8(void);
void func_0201ebac(void);
void *func_0201ef14(u8 *p);
void *func_0201ef58(void *p);
void *func_0201eff4(u8 *p);

#define SCR() (data_020e2058 + data_020df0fc * 0x130)
/* a constructor or destructor for __construct_array / __destroy_arr */
#define FN(f) ((void *(*)(void *))(void (*)(void))(f))
#define BG_MAP(scr, l) (SHL(scr, 21) + 0x06000000 + (BITS(U16_AT(l, 4), 8, 5) << 11))
#define ATTR(blocks, l, blk) ((blocks) + U32_AT(l, 0x4c) + (s32)((blk) << 5) / 2)

/* 0x0201d638: draw the 33 tiles of the map row at pixel row y, from pixel
 * column x (wrapped to the map width `wrap`)
 * @difftest int:0:0x600 int:0:0x200 pick:0x200,0x400,0x600 @020df0fc:32=0 cases=30 */
void func_0201d638(u32 x, u32 y, u32 wrap)
{
    u32 ty4, tx, by, bx2, vy, vx0, layer;

    while (x >= wrap)
        x -= wrap;
    ty4 = (y & 0x18) >> 1;
    tx = (x & 0x18) >> 3;
    by = y >> 5;
    bx2 = (x >> 5) << 1;
    vy = ((y & 0x1ff) >> 3) << 7;
    vx0 = (x & 0x1ff) >> 3;
    for (layer = 0; layer < 2; layer++) {
        u32 scr = data_020df0fc;
        u8 *l = data_020e2058 + scr * 0x130 + 0x44 + layer * 0x50;
        u8 *m, *blocks, *attr, *tiles;
        u16 *vram = NULL;
        u32 px, col, pal, i, vx = 0;

        if (U32_AT(l, 8) == 0)
            continue;
        m = (u8 *)PTR_AT(l, 0x18) + (by * U16_AT(l, 0x38)) * 2 + bx2;
        blocks = PTR_AT(l, 0x1c);
        px = x;
        col = tx;
        if (layer != 1) {
            vx = vx0;
            vram = (u16 *)(BG_MAP(scr, l) + vy);
        }
        pal = l[0xc];
        attr = ATTR(blocks, l, U16_AT(m, 0));
        tiles = blocks + (U16_AT(m, 0) << 5);
        for (i = 0; i < 0x21; i++) {
            u32 t = ((u16 *)tiles)[ty4 + col];
            if (layer == 1) {
                func_0201d308(PTR_AT(data_020e2058 + data_020df0fc * 0x130 + layer * 0x50, 0x6c), t,
                              px & 0x1f8, y & 0xf8, (attr[ty4 + col] >> 2) & 0xff, layer & 0xff);
            } else {
                u32 n = func_0201d4a0(t, pal, 0, layer);
                vram[vx] = n | (attr[ty4 + col] << 10);
                vx = (vx + 1) & 0x3f;
            }
            col++;
            px += 8;
            if (col >= 4) {
                m += 2;
                if (px >= wrap) {
                    px = 0;
                    m -= S16_AT(l, 0x38) * 2;
                }
                col -= 4;
                attr = ATTR(blocks, l, U16_AT(m, 0));
                tiles = blocks + (U16_AT(m, 0) << 5);
            }
        }
    }
}

/* 0x0201d8f0: draw the 25 tiles of the map column at pixel column x (wrapped
 * to the map width), from pixel row y
 * @difftest int:0:0x600 int:0:0x100 pick:0x200,0x400,0x600 @020df0fc:32=0 cases=30 */
void func_0201d8f0(u32 x, u32 y, u32 wrap)
{
    u32 tx, ty40, by, bx2, vy0, vx, layer;

    while (x >= wrap)
        x -= wrap;
    tx = (x & 0x18) >> 3;
    ty40 = (y & 0x18) >> 1;
    by = y >> 5;
    bx2 = (x >> 5) << 1;
    vy0 = ((y & 0x1ff) >> 3) << 7;
    vx = (x & 0x1ff) >> 3;
    for (layer = 0; layer < 2; layer++) {
        u32 scr = data_020df0fc;
        u8 *l = data_020e2058 + scr * 0x130 + 0x44 + layer * 0x50;
        u8 *m, *blocks, *attr, *tiles;
        u16 *vram = NULL;
        u32 py, r, pal = 0, i, mx = 0;

        if (U32_AT(l, 8) == 0)
            continue;
        m = (u8 *)PTR_AT(l, 0x18) + (by * U16_AT(l, 0x38)) * 2 + bx2;
        blocks = PTR_AT(l, 0x1c);
        attr = ATTR(blocks, l, U16_AT(m, 0));
        tiles = blocks + (U16_AT(m, 0) << 5);
        py = y;
        r = ty40;
        if (layer != 1) {
            vram = (u16 *)(BG_MAP(scr, l) + vy0);
            mx = vx;
        }
        if (layer != 1)
            pal = l[0xc];
        for (i = 0; i < 0x19; i++) {
            u32 t = ((u16 *)tiles)[tx + r];
            if (layer == 1) {
                func_0201d308(PTR_AT(data_020e2058 + data_020df0fc * 0x130 + layer * 0x50, 0x6c), t,
                              x & 0x1f8, py & 0xf8, (attr[tx + r] >> 2) & 0xff, layer & 0xff);
            } else {
                u32 n = func_0201d4a0(t, pal, 0, layer);
                vram[mx] = n | (attr[tx + r] << 10);
            }
            r += 4;
            py += 8;
            vram = (u16 *)(BG_MAP(data_020df0fc, l) + (((py & 0x1ff) >> 3) << 7));
            if (r >= 0x10) {
                r -= 0x10;
                m += U16_AT(l, 0x38) * 2;
                attr = ATTR(blocks, l, U16_AT(m, 0));
                tiles = blocks + (U16_AT(m, 0) << 5);
            }
        }
    }
}

/* 0x0201dbb0: release the 33 cached tiles of a map row (layer 0)
 * @difftest int:0:0x600 int:0:0x200 pick:0x200,0x400,0x600 @020df0fc:32=0 cases=30 */
void func_0201dbb0(u32 x, u32 y, u32 wrap)
{
    u32 tx, ty4, by, bx2, layer;

    while (x >= wrap)
        x -= wrap;
    tx = (x & 0x18) >> 3;
    ty4 = (y & 0x18) >> 1;
    by = y >> 5;
    bx2 = (x >> 5) << 1;
    for (layer = 0; layer < 2; layer++) {
        u8 *l, *m, *blocks, *tiles;
        u32 px, col, pal, i;

        if (layer == 1)
            continue;
        l = SCR() + 0x44 + layer * 0x50;
        if (U32_AT(l, 8) == 0)
            continue;
        blocks = PTR_AT(l, 0x1c);
        px = x;
        pal = l[0xc];
        m = (u8 *)PTR_AT(l, 0x18) + (by * U16_AT(l, 0x38)) * 2 + bx2;
        col = tx;
        tiles = blocks + (U16_AT(m, 0) << 5);
        for (i = 0; i < 0x21; i++) {
            func_0201de40(((u16 *)tiles)[ty4 + col], pal, layer);
            col++;
            px += 8;
            if (col >= 4) {
                m += 2;
                if (px >= wrap) {
                    px = 0;
                    m -= S16_AT(l, 0x38) * 2;
                }
                col -= 4;
                tiles = blocks + (U16_AT(m, 0) << 5);
            }
        }
    }
}

/* 0x0201dd14: release the 25 cached tiles of a map column (layer 0)
 * @difftest int:0:0x600 int:0:0x100 pick:0x200,0x400,0x600 @020df0fc:32=0 cases=30 */
void func_0201dd14(u32 x, u32 y, u32 wrap)
{
    u32 tx, ty40, by, bx2, layer;

    while (x >= wrap)
        x -= wrap;
    tx = (x & 0x18) >> 3;
    by = y >> 5;
    bx2 = (x >> 5) << 1;
    ty40 = (y & 0x18) >> 1;
    for (layer = 0; layer < 2; layer++) {
        u8 *l, *m, *tiles;
        u32 r, pal, i;

        if (layer == 1)
            continue;
        l = SCR() + 0x44 + layer * 0x50;
        if (U32_AT(l, 8) == 0)
            continue;
        m = (u8 *)PTR_AT(l, 0x18) + (by * U16_AT(l, 0x38)) * 2 + bx2;
        tiles = (u8 *)PTR_AT(l, 0x1c) + (U16_AT(m, 0) << 5);
        pal = l[0xc];
        r = ty40;
        for (i = 0; i < 0x19; i++) {
            func_0201de40(((u16 *)tiles)[tx + r], pal, layer);
            r += 4;
            if (r >= 0x10) {
                r -= 0x10;
                m += U16_AT(l, 0x38) * 2;
                tiles = (u8 *)PTR_AT(l, 0x1c) + (U16_AT(m, 0) << 5);
            }
        }
    }
}

/* 0x0201de40: release one use of the cached copy of source tile `tile`;
 * returns 1 if it was not cached
 * @difftest int:0:0x800 int:0:2 int:0:2 @020df0fc:32=0 */
u32 func_0201de40(u32 tile, u32 bank, u32 layer)
{
    u8 *s = SCR();
    u16 *heads = ((u16 **)(s + 0xf0))[bank];
    u8 *pool = ((u8 **)(s + 0xe8))[bank];
    u32 key = tile & 0x7ff, i = heads[key], prev = 0xffff, k;

    while (i != 0xffff) {
        if (tile == U16_AT(pool + i * 6, 2))
            break;
        prev = i & 0xffff;
        i = U16_AT(pool + i * 6, 4);
    }
    if (i == 0xffff)
        return 1;
    S16_AT(pool, i * 6) -= 1;
    if (S16_AT(pool, i * 6) != 0)
        return 0;
    s = SCR();
    ((u16 *)(s + 0x126))[layer] -= 1;
    k = ((u16 *)(s + 0x120))[bank];
    ((u16 *)(s + 0x120))[bank] = k + 1;
    ((u16 **)(s + 0xf8))[bank][k] = i;
    if (prev == 0xffff)
        ((u16 **)(SCR() + 0xf0))[bank][key] = U16_AT(((u8 **)(SCR() + 0xe8))[bank] + i * 6, 4);
    else
        U16_AT(pool + prev * 6, 4) = U16_AT(((u8 **)(SCR() + 0xe8))[bank] + i * 6, 4);
    U16_AT(pool + i * 6, 2) = 0xffff;
    U16_AT(pool + i * 6, 4) = 0xffff;
    return 0;
}

/* 0x0201dfe8: release the column and row queued by the last scroll */
void func_0201dfe8(void)
{
    s32 size[2], i;

    func_0201c428(size);
    for (i = 0; i < 2; i++) {
        u8 *q = SCR() + i * 8;
        if (S32_AT(q, 0x30) == -1)
            continue;
        if (i == 0)
            func_0201dd14(S32_AT(q, 0x30), S32_AT(q, 0x34), size[0]);
        else
            func_0201dbb0(S32_AT(q, 0x30), S32_AT(q, 0x34), size[0]);
        S32_AT(SCR() + i * 8, 0x30) = -1;
        S32_AT(SCR() + i * 8, 0x34) = -1;
    }
    S32_AT(SCR(), 0x10) = S32_AT(SCR(), 0x08);
    S32_AT(SCR(), 0x14) = S32_AT(SCR(), 0x0c);
}

/* 0x0201e0a8: queue column (i = 0) or row (i = 1) at (x, y) for release
 * @difftest s32 s32 int:0:2 @020df0fc:32=int:0:2 */
void func_0201e0a8(s32 x, s32 y, u32 i)
{
    u32 off = data_020df0fc * 0x130;
    S32_AT(data_020e2058 + off + i * 8, 0x30) = x;
    S32_AT(data_020e2058 + off + i * 8, 0x34) = y;
}

/* 0x0201e0e8: copy what changed of the layer 1 bitmap into the clear image
 * (VRAM bank A), at scanline 140
 * @difftest $B=ptr:0x40000:2 @04000006:16=0x8c @020df0fc:32=0 @020e2114:32=$B @020e217c:16=pick:4,0,5 @020e2070:32=int:0:0x400 @020e2078:32=pick:0,8,0x100,0x1f8 @020e2074:32=int:0:0x200 @020e207c:32=pick:0,8,0xc0,0xf8 cases=12 */
void func_0201e0e8(void)
{
    u8 *s = SCR();
    u16 f = U16_AT(s, 0x124);
    u32 sx = 0, dx = 0, sy = 0, dy = 0, hx = 0, hdx = 0, ex, ey;
    s32 doX = 0, doY = 0, a, b;
    s16 nx = 0, ny = 0;

    if (!(f & 4))
        return;
    U16_AT(s, 0x124) = f & ~4;
    a = S32_AT(s, 0x20);
    b = S32_AT(s, 0x18);
    if (a != b) {
        doX = 1;
        if (a < b) {
            sx = a & 0x1ff;
            ex = b & 0x1ff;
            dx = a & 0xff;
        } else {
            sx = (b + 0x100) & 0x1ff;
            ex = (a + 0x100) & 0x1ff;
            dx = (b + 0x100) & 0xff;
        }
        nx = (s16)(ex - sx);
        if (nx >= 0x200)
            nx = (s16)(nx - 0x200);
        if (nx <= 0)
            nx = (s16)(nx + 0x200);
    }
    s = SCR();
    a = S32_AT(s, 0x24);
    b = S32_AT(s, 0x1c);
    if (a != b) {
        s32 cx = S32_AT(s, 0x20);
        hx = cx & 0x1ff;
        hdx = cx & 0xff;
        doY = 1;
        if (a < b) {
            sy = a & 0xff;
            ey = b & 0xff;
            dy = a & 0xff;
        } else {
            sy = (b + 0xc0) & 0xff;
            ey = (a + 0xc0) & 0xff;
            dy = (b + 0xc0) & 0xff;
        }
        ny = (s16)(ey - sy);
        if (ny >= 0x100)
            ny = (s16)(ny - 0x100);
        if (ny <= 0)
            ny = (s16)(ny + 0x100);
    }
    if (doX != 1 && doY != 1)
        return;
    while (REG16(0x04000006) != 0x8c)
        ;
    func_0208fcdc();
    if (doX == 1) {
        const u16 *src = PTR_AT(data_020e2058, 0xbc);
        u16 *dst = (u16 *)0x06800000;
        u32 row, j;
        for (row = 0; row < 0x100; row++) {
            for (j = 0; j < (u32)nx; j++)
                dst[(dx + j) & 0xff] = src[(sx + j) & 0x1ff];
            src += 0x200;
            dst += 0x100;
        }
    }
    if (doY == 1) {
        const u16 *src = (const u16 *)PTR_AT(data_020e2058, 0xbc) + sy * 0x200;
        u16 *dst = (u16 *)(0x06800000 + (dy << 9));
        u32 k, j;
        for (k = 0; k < (u32)ny; k++) {
            if (dy + k > 0xff) {
                dst = (u16 *)0x06800000;
                dy = 0;
            }
            if (sy + k > 0xff) {
                src = PTR_AT(data_020e2058, 0xbc);
                sy = 0;
            }
            for (j = 0; j < 0x100; j++)
                dst[(hdx + j) & 0xff] = src[(hx + j) & 0x1ff];
            src += 0x200;
            dst += 0x100;
        }
    }
    func_0208fff8(1);
}

/* 0x0201e470: per-frame scroll: follow the camera, draw the column and row
 * that come into view (unless `force`, used when everything is redrawn)
 * @difftest pick:0,1 @04000006:16=0x8c @020df0fc:32=0 @020e2060:32=int:0:0x300 @020e2064:32=int:0:0x200 cases=20 */
void func_0201e470(u32 force)
{
    s32 size[2], *cam = data_020e1ee0, x, y;
    u32 layer;

    func_0201c428(size);
    func_0201f60c(cam);
    x = cam[0] - 0x80;
    y = cam[1] - 0x60;
    cam[1] = y;
    cam[0] = x;
    if (x < 0)
        cam[0] = 0;
    else if (x >= size[0] - 0x100 && S32_AT(SCR(), 0x40) == 0)
        cam[0] = size[0] - 0x101;
    y = cam[1];
    if (y < 0)
        cam[1] = 0;
    else if (y >= size[1] - 0xc0)
        cam[1] = size[1] - 0xc1;
    S32_AT(SCR(), 0) = cam[0];
    S32_AT(SCR(), 4) = cam[1];

    if (force == 0) {
        s32 old, cur;
        func_0201dfe8();
        old = S32_AT(SCR(), 0x08);
        cur = cam[0];
        if ((cur >> 3) != (old >> 3)) {
            if (old > cur) {
                if (old - cur > 8)
                    cam[0] = old - 8;
                func_0201d8f0(cam[0], S32_AT(SCR(), 0x0c), size[0]);
                func_0201e0a8(cam[0] + 0x108, S32_AT(SCR(), 0x0c), 0);
            } else {
                if (cur - old > 8)
                    cam[0] = old + 8;
                func_0201d8f0(cam[0] + 0x100, S32_AT(SCR(), 0x0c), size[0]);
                func_0201e0a8(cam[0] - 8, S32_AT(SCR(), 0x0c), 0);
            }
        }
        old = S32_AT(SCR(), 0x0c);
        cur = cam[1];
        if ((cur >> 3) != (old >> 3)) {
            if (old > cur) {
                if (old - cur > 8)
                    cam[1] = old - 8;
                func_0201d638(cam[0], cam[1], size[0]);
                func_0201e0a8(cam[0], cam[1] + 0xc8, 1);
            } else {
                if (cur - old > 8)
                    cam[1] = old + 8;
                func_0201d638(cam[0], cam[1] + 0xc0, size[0]);
                func_0201e0a8(cam[0], cam[1] - 8, 1);
            }
        }
    }

    func_0201d308(NULL, -1, 0, 0, 0, 0);
    S32_AT(SCR(), 0x08) = cam[0];
    S32_AT(SCR(), 0x0c) = cam[1];
    for (layer = 0; layer < 2; layer++) {
        s32 ox, oy;
        if (S32_AT(SCR() + layer * 0x50, 0x4c) == 0 || layer == 1)
            continue;
        func_0200a044(S32_AT(SCR() + layer * 0x50, 0x44), 0, 0);
        ox = func_0200beac(data_020e1f50);
        oy = func_0200bea4(data_020e1f50);
        func_0200a0b0(S32_AT(SCR() + layer * 0x50, 0x44), cam[0] + (ox >> 12), cam[1] + (oy >> 12));
    }
    S32_AT(SCR(), 0x18) = S32_AT(SCR(), 0x10);
    S32_AT(SCR(), 0x1c) = S32_AT(SCR(), 0x14);
    S32_AT(SCR(), 0x20) = S32_AT(SCR(), 0x08);
    S32_AT(SCR(), 0x24) = S32_AT(SCR(), 0x0c);
    S32_AT(SCR(), 0x10) = S32_AT(SCR(), 0x08);
    S32_AT(SCR(), 0x14) = S32_AT(SCR(), 0x0c);
    func_0201e0e8();
    REG16(0x04000356) = (cam[0] & 0xff) | (SHL(cam[1], 8) & 0xff00);
    U16_AT(SCR(), 0x124) |= 4;
}

/* 0x0201e890: reset the tile cache and draw the whole view */
void func_0201e890(void)
{
    s32 size[2];
    u32 i, y;

    func_0201c428(size);
    MI_CpuFill8(PTR_AT(SCR(), 0xe8), 0xff, 0x1800);
    MI_CpuFill8(PTR_AT(SCR(), 0xec), 0xff, 0x1800);
    MI_CpuFill8(PTR_AT(SCR(), 0xf0), 0xff, 0x1000);
    MI_CpuFill8(PTR_AT(SCR(), 0xf4), 0xff, 0x1000);
    U16_AT(SCR(), 0x120) = 0x400;
    U16_AT(SCR(), 0x122) = 0x400;
    for (i = 0; i < 0x400; i++)
        ((u16 *)PTR_AT(SCR(), 0xf8))[i] = 0x3ff - i;
    for (i = 0; i < 0x400; i++)
        ((u16 *)PTR_AT(SCR(), 0xfc))[i] = 0x3ff - i;
    for (i = 0, y = 0; i < 0x19; i++, y += 8)
        func_0201d638(S32_AT(SCR(), 0x08), S32_AT(SCR(), 0x0c) + y, size[0]);
    func_0201d308(NULL, -1, 0, 0, 0, 0);
    S32_AT(SCR(), 0x20) = S32_AT(SCR(), 0x08);
    S32_AT(SCR(), 0x24) = S32_AT(SCR(), 0x0c);
    S32_AT(SCR(), 0x18) = S32_AT(SCR(), 0x08) + 0x100;
    S32_AT(SCR(), 0x1c) = S32_AT(SCR(), 0x0c) + 0xc0;
    func_0201e0e8();
    S32_AT(SCR(), 0x10) = S32_AT(SCR(), 0x08);
    S32_AT(SCR(), 0x14) = S32_AT(SCR(), 0x0c);
    S32_AT(SCR(), 0x18) = S32_AT(SCR(), 0x10);
    S32_AT(SCR(), 0x1c) = S32_AT(SCR(), 0x14);
    S32_AT(SCR(), 0x20) = S32_AT(SCR(), 0x08);
    S32_AT(SCR(), 0x24) = S32_AT(SCR(), 0x0c);
    U16_AT(SCR(), 0x126) = 0;
    U16_AT(SCR(), 0x128) = 0;
    U16_AT(SCR(), 0x12a) = 0;
    U16_AT(SCR(), 0x12c) = 0;
}

/* 0x0201eb6c: turn the clear image off (plain clear colour)
 * @difftest cases=5 */
void func_0201eb6c(void)
{
    func_0208fb8c();
    func_0208fff8(0);
    func_02090da0(0x7fff, 1, 0x7fff, 0x3f, 0);
}

/* 0x0201ebac: turn the clear image on and clear it and the bitmap to white */
void func_0201ebac(void)
{
    func_0208fbb4();
    func_0208fff8(1);
    func_0208fcdc();
    func_0207fe8c(3, (void *)0x06800000, 0x7fff, 0x20000, 0x10);
    func_0208fff8(1);
    func_0207fe8c(3, PTR_AT(SCR(), 0xbc), 0x7fff, 0x40000, 0x10);
    func_0200a188(0);
    func_02090768(2);
}

/* 0x0201ec40: set up scrolling for the current screen ("scrollInit") */
void func_0201ec40(void)
{
    s32 pos[2];
    u32 i;

    pos[0] = 0;
    pos[1] = 0;
    *(s32 *)data_020e1edc = 0;
    for (i = 0; i < 2; i++)
        U32_AT(data_020e1fa8 + i * 0x58, 0) = 1;
    func_0201c45c(pos);
    U8_AT(SCR(), 0x50) = 0;
    U32_AT(SCR(), 0x44) = 2;
    U32_AT(SCR(), 0x94) = 0;
    if (PTR_AT(SCR(), 0xbc) == NULL)
        PTR_AT(SCR(), 0xbc) = func_0207ff70(0x40000, data_020bdcec, 0x245);
    if (PTR_AT(SCR(), 0xe8) == NULL)
        PTR_AT(SCR(), 0xe8) = func_0207ff70(0x1800, data_020bdcec, 0x24b);
    if (PTR_AT(SCR(), 0xec) == NULL)
        PTR_AT(SCR(), 0xec) = func_0207ff70(0x1800, data_020bdcec, 0x250);
    if (PTR_AT(SCR(), 0xf8) == NULL)
        PTR_AT(SCR(), 0xf8) = func_0207ff70(0x800, data_020bdcec, 0x255);
    if (PTR_AT(SCR(), 0xfc) == NULL)
        PTR_AT(SCR(), 0xfc) = func_0207ff70(0x800, data_020bdcec, 0x25a);
    if (PTR_AT(SCR(), 0xf0) == NULL)
        PTR_AT(SCR(), 0xf0) = func_0207ff70(0x1000, data_020bdcec, 0x25f);
    if (PTR_AT(SCR(), 0xf4) == NULL)
        PTR_AT(SCR(), 0xf4) = func_0207ff70(0x1000, data_020bdcec, 0x264);
    if (PTR_AT(SCR(), 0xe4) == NULL)
        PTR_AT(SCR(), 0xe4) = func_0207ff70(0x2000, data_020bdcec, 0x269);
    func_0201ebac();
    U16_AT(SCR(), 0x124) &= ~4;
}

/* 0x0201eeec: empty (called once at start-up)
 * @difftest cases=2 */
void func_0201eeec(void) {}

/* 0x0201eef0: destroy the per-screen scroll state (registered at start-up)
 * @difftest cases=5 */
void func_0201eef0(void)
{
    func_020a6d58(data_020e2058, 2, 0x130, FN(func_0201ef14));
}

/* 0x0201ef14: destructor of one screen's scroll state
 * @difftest ptr:0x130:4 */
void *func_0201ef14(u8 *p)
{
    func_020a6d58(p + 0x44, 2, 0x50, func_0201ef58);
    func_020a6d58(p + 0x30, 2, 8, FN(func_02002164));
    return p;
}

/* 0x0201ef58: destructor of a scroll layer (empty)
 * @difftest u32 */
void *func_0201ef58(void *p) { return p; }

/* 0x0201ef5c: constructor of one screen's scroll state
 * @difftest ptr:0x130:4 */
void *func_0201ef5c(u8 *p)
{
    u32 i;
    for (i = 0; i < 12; i++)
        U32_AT(p, i * 4) = 0;
    func_020a6dcc(p + 0x30, 2, 8, FN(func_02005cb0), FN(func_02002164));
    func_020a6dcc(p + 0x44, 2, 0x50, FN(func_0201eff4), func_0201ef58);
    return p;
}

/* 0x0201eff4: constructor of a scroll layer
 * @difftest ptr:0x50:4 */
void *func_0201eff4(u8 *p)
{
    U32_AT(p, 0x40) = 0;
    U32_AT(p, 0x44) = 0;
    return p;
}
