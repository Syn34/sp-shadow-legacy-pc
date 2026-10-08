/*
 * Scroll.cpp, part 2: the scrolling map's palettes (loading, the tinted copy
 * made from it, brightness) and the tile cache that streams 8-bit tiles from
 * the map's tile file into BG VRAM.
 * ARM9 main, 0x0201c428 - 0x0201d638 (11 functions).
 *
 * Per screen, data_020e2058 + screen * 0x130 (see Scroll1.c), and further:
 *   +0x00/+0x04 scroll position, +0x64/+0x68 the palette (0x1000 colours)
 *   and its tinted copy, +0x84/+0x88 the map size in pixels,
 *   +0xe4 the palette that is shown, +0xe8[2] cache entries (6 bytes:
 *   s16 users, u16 source tile, u16 next), +0xf0[2] hash heads by source
 *   tile & 0x7ff, +0xf8[2] stacks of free entries, +0x100 / +0x104,
 *   +0x118 the tile file, +0x120[2] free entries left, +0x126[2] tiles in
 *   use per layer.
 * data_020e1f0c is the open tile file and data_020e1fa8 holds two 0x58-byte
 * slots of tiles read one frame ahead for a 512-pixel wide direct-colour
 * bitmap:
 *   +0x00 drawn, +0x04 bitmap, +0x08 tile, +0x0c x, +0x10 y,
 *   +0x14 palette, +0x15 layer, +0x16 the 64 tile bytes.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

extern const char data_020bdcec[];   /* "Scroll.cpp" */
extern const s32 data_020bdcbc, data_020bdcc0, data_020bdcc4;   /* colour limits */
extern u8 data_020e1edc[];           /* read-ahead slot (u32) */
extern u8 data_020e1f0c[];           /* the tile file (FSFile) */
extern u8 data_020e1fa8[];           /* read-ahead slots, 0x58 bytes each */
extern u8 data_020e2058[];

void func_0200bd38(const void *src, u32 slot, u32 offset, u32 count);
void func_020022e8(s32 *out, const s32 *mtx44, const s32 *in);
s32 func_0201b930(void *file, void *dst, u32 offset, u32 size);
void func_0201b9c0(void);
void func_02097a0c(void *file);      /* FS_WaitAsync */

void func_0201c668(u32 layer);
void func_0201cc50(const s32 *hsv, s32 *rgb);
void func_0201d280(u16 *dst, const u8 *src, u32 tile, u32 x, u32 y, u32 pal, u32 layer);

#define SCR() (data_020e2058 + data_020df0fc * 0x130)
#define LAYER(i) (SCR() + 0x44 + (i) * 0x50)
#define SLOT(i) (data_020e1fa8 + (i) * 0x58)
#define CUR_SLOT (*(s32 *)data_020e1edc)

/* 0x0201c428: map size in pixels
 * @difftest ptr:8:4 @020df0fc:32=int:0:2 */
void func_0201c428(s32 *out)
{
    u8 *s = SCR() + 0x84;
    out[0] = S32_AT(s, 0);
    out[1] = S32_AT(s, 4);
}

/* 0x0201c45c: set the scroll position
 * @difftest ptr:8:4 @020df0fc:32=int:0:2 */
void func_0201c45c(const s32 *pos)
{
    u32 off = data_020df0fc * 0x130;
    S32_AT(data_020e2058 + off, 0) = pos[0];
    S32_AT(data_020e2058 + off, 4) = pos[1];
}

/* 0x0201c494: load the palette of level `lvl` and open its tile file */
void func_0201c494(Cnt cnt, Cnt b, Cnt c, u32 lvl, u32 d, u32 e)
{
    const u16 *pal = func_02012a64(S32_AT(data_020c3b60 + lvl * 0x60, 0x24));
    u32 i;

    func_0200bd38(pal, 2, 0, 0x1000);
    if (PTR_AT(SCR(), 0x64) == NULL)
        PTR_AT(SCR(), 0x64) = func_0207ff70(0x2000, data_020bdcec, 0xa6c);
    if (PTR_AT(SCR(), 0x68) == NULL)
        PTR_AT(SCR(), 0x68) = func_0207ff70(0x2000, data_020bdcec, 0xa71);
    for (i = 0; i < 0x1000; i++) {
        ((u16 *)PTR_AT(SCR(), 0x64))[i] = pal[i];
        ((u16 *)PTR_AT(SCR(), 0x68))[i] = pal[i];
    }
    func_0201c668(0);
    func_020129ac(S32_AT(data_020c3b60 + lvl * 0x60, 0x24));
    func_0201b9c0();
    U16_AT(SCR(), 0x48) = cnt.v;
    U32_AT(SCR(), 0x118) = U32_AT(data_020c3b60 + lvl * 0x60, 0x20);
    func_02080488(data_020e1f0c, U32_AT(SCR(), 0x118));
    U32_AT(SCR(), 0x104) = U32_AT(SCR(), 0x100);
    U32_AT(SCR(), 0x08) = 0;
    U32_AT(SCR(), 0x0c) = 0;
}

/* a 5-bit colour component as fx32, rounded the way the game does it */
static s32 CompU(u32 v)
{
    float f = (float)(v << 12);
    return (s32)(v != 0 ? 0.5f + f : f - 0.5f);
}

static s32 CompS(s32 v)
{
    float f = (float)SHL(v, 12);
    return (s32)(v > 0 ? 0.5f + f : f - 0.5f);
}

/* an fx32 colour component (0..1) back to 5 bits */
static s32 To5(s32 v)
{
    s32 c = FxMul(v, 0x1f000) >> 12;
    if (c < 0)
        c = 0;
    if (c > 0x1f)
        c = 0x1f;
    return c;
}

static u32 Pack(s32 r, s32 g, s32 b)
{
    r = To5(r);
    g = To5(g);
    b = To5(b);
    return (r & 0x1f) | ((g << 5) & 0x3e0) | ((b << 10) & 0x7c00);
}

/* 0x0201c668: make the tinted copy of a layer's palette (a purple tint of
 * the luminance mixed with 20% of the colour, then a contrast stretch)
 * @difftest $S=ptr:0x2000:2 $D=ptr:0x2000:2 @020df0fc:32=0 @020e20bc:32=$S @020e20c0:32=$D pick:0 cases=3 */
void func_0201c668(u32 layer)
{
    s32 inv31, k, m[16], hsv[3], out[3], tmp[3], r, g, b;
    s32 i;

    inv31 = func_0208f070(0x1000, 0x1f000);
    func_0208ea6c(m);
    k = func_0208f070(0xff000, 0xaa000);
    m[0] = k;
    m[5] = k;
    m[10] = k;
    m[12] = FxMul(func_0208f070(-0x1e000, 0xff000), k);
    m[13] = FxMul(func_0208f070(-0x1e000, 0xff000), k);
    m[14] = FxMul(func_0208f070(-0x1e000, 0xff000), k);
    hsv[0] = 0x118000;
    hsv[1] = 0x548;
    hsv[2] = 0;
    for (i = 0; i < 0x1000; i++) {
        u32 c = ((u16 *)PTR_AT(LAYER(layer), 0x20))[i];
        r = FxMul(CompU(c & 0x1f), inv31);
        g = FxMul(CompU((c & 0x3e0) >> 5), inv31);
        b = FxMul(CompU((c & 0x7c00) >> 10), inv31);
        hsv[2] = FxMul(b, 0x150) + (FxMul(r, 0x4f0) + FxMul(g, 0x9c0));
        func_0201cc50(hsv, out);
        out[0] = FxMul(out[0], 0xccd) + FxMul(r, 0x333);
        out[1] = FxMul(out[1], 0xccd) + FxMul(g, 0x333);
        out[2] = FxMul(out[2], 0xccd) + FxMul(b, 0x333);
        func_020022e8(tmp, m, out);
        ((u16 *)PTR_AT(LAYER(layer), 0x24))[i] = Pack(tmp[0], tmp[1], tmp[2]);
    }
}

/* 0x0201cc50: hue (degrees), saturation, value to RGB (all fx32)
 * @difftest $H=ptr:12:4 @$H+0:32=int:0:0x168000 @$H+4:32=int:0:0x1000 @$H+8:32=int:0:0x1000 $H ptr:12:4 */
void func_0201cc50(const s32 *hsv, s32 *rgb)
{
    s32 t[3], inv60, h;

    t[0] = 0;
    t[1] = 0;
    t[2] = 0;
    inv60 = func_0208f070(0x1000, 0x3c000);
    h = hsv[0];

    if (h < 0x78000) {
        t[0] = FxMul(0x78000 - h, inv60);
        t[1] = FxMul(hsv[0], inv60);
        t[2] = 0;
    } else if (h < 0xf0000) {
        t[0] = 0;
        t[1] = FxMul(0xf0000 - hsv[0], inv60);
        t[2] = FxMul(hsv[0] - 0x78000, inv60);
    } else {
        t[0] = FxMul(h - 0xf0000, inv60);
        t[1] = 0;
        t[2] = FxMul(0x168000 - hsv[0], inv60);
    }
    if (t[0] >= data_020bdcc0)
        t[0] = data_020bdcc0;
    if (t[1] >= data_020bdcc4)
        t[1] = data_020bdcc4;
    if (t[2] >= data_020bdcbc)
        t[2] = data_020bdcbc;
    rgb[0] = FxMul(0x1000 - hsv[1] + FxMul(hsv[1], t[0]), hsv[2]);
    rgb[1] = FxMul(0x1000 - hsv[1] + FxMul(hsv[1], t[1]), hsv[2]);
    rgb[2] = FxMul(0x1000 - hsv[1] + FxMul(hsv[1], t[2]), hsv[2]);
}

/* 0x0201cea4: show the palette at brightness v (fx32, 1.0 = normal)
 * @difftest $S=ptr:0x2000:2 $D=ptr:0x2000:2 @020df0fc:32=0 @020e20bc:32=$S @020e20c0:32=$S @020e213c:32=$D int:0:0x1400 cases=4 */
void func_0201cea4(s32 v)
{
    s32 inv31 = func_0208f070(0x1000, 0x1f000), scale;
    u16 *dst = PTR_AT(SCR(), 0xe4);
    const u16 *src;
    s32 i;

    if ((s8)(func_020436f0(data_020bf6a0)[0xf] & 1))
        src = PTR_AT(SCR(), 0x68);
    else
        src = PTR_AT(SCR(), 0x64);
    scale = FxMul(v, inv31);
    for (i = 0; i < 0x1000; i++) {
        u32 c = src[i];
        s32 r = FxMul(CompS(c & 0x1f), scale);
        s32 g = FxMul(CompS((c & 0x3e0) >> 5), scale);
        s32 b = FxMul(CompS((c & 0x7c00) >> 10), scale);
        dst[i] = Pack(r, g, b);
    }
}

/* 0x0201d19c: show the palette (or its tinted copy) at normal brightness
 * @difftest $S=ptr:0x2000:2 $T=ptr:0x2000:2 $D=ptr:0x2000:2 @020df0fc:32=0 @020e20bc:32=$S @020e20c0:32=$T @020e213c:32=$D cases=4 */
void func_0201d19c(void)
{
    u16 *dst = PTR_AT(SCR(), 0xe4);
    const u16 *src;
    s32 i;

    if ((s8)(func_020436f0(data_020bf6a0)[0xf] & 1))
        src = PTR_AT(SCR(), 0x68);
    else
        src = PTR_AT(SCR(), 0x64);
    for (i = 0; i < 0x1000; i++)
        dst[i] = src[i];
}

/* 0x0201d248: load the shown palette into the BG extended palettes */
void func_0201d248(void)
{
    func_0200bd38(PTR_AT(SCR(), 0xe4), 2, 0, 0x1000);
}

/* 0x0201d280: draw an 8x8 tile of 8-bit colour indices into a 512-pixel wide
 * direct-colour bitmap, through palette `pal` of a layer
 * @difftest $P=ptr:0x2000:2 @020df0fc:32=0 @020e20bc:32=$P ptr:0x4800:2 ptr:0x40 u32 int:0:8 int:0:8 int:0:16 pick:0 cases=40 */
void func_0201d280(u16 *dst, const u8 *src, u32 tile, u32 x, u32 y, u32 pal, u32 layer)
{
    const u16 *colours = (const u16 *)PTR_AT(LAYER(layer & 0xff), 0x20) + (pal & 0xff) * 0x100;
    u32 row, col;

    dst += x + SHL(y, 9);
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++)
            *dst++ = colours[*src++];
        dst += 0x1f8;
    }
}

/* 0x0201d308: start reading a tile into this frame's slot (tile -1: none)
 * and draw the tile read in the previous frame
 * @difftest $P=ptr:0x2000:2 @020df0fc:32=0 @020e20bc:32=$P @020e1edc:32=int:0:2 @020e1fa8:32=int:0:2 @020e2000:32=int:0:2 ptr:0x4800:2 pick:-1 int:0:8 int:0:8 int:0:16 pick:0 */
void func_0201d308(u16 *dst, s32 tile, u32 x, u32 y, u32 pal, u32 layer)
{
    s32 cur = CUR_SLOT, prev = cur - 1;
    u8 *e;

    if (prev < 0)
        prev = 1;
    if (tile != -1) {
        U32_AT(SLOT(cur), 0) = 0;
        PTR_AT(SLOT(cur), 4) = dst;
        S32_AT(SLOT(cur), 8) = tile;
        U32_AT(SLOT(cur), 0xc) = x;
        U32_AT(SLOT(cur), 0x10) = y;
        U8_AT(SLOT(cur), 0x14) = pal;
        U8_AT(SLOT(cur), 0x15) = layer;
        if (U32_AT(data_020e1f0c, 0xc) & 1)
            func_02097a0c(data_020e1f0c);
        func_0201b930(data_020e1f0c, SLOT(CUR_SLOT) + 0x16, SHL(tile, 6), 0x40);
    } else if (U32_AT(data_020e1f0c, 0xc) & 1) {
        func_02097a0c(data_020e1f0c);
    }
    e = SLOT(prev);
    if (U32_AT(e, 0) == 0) {
        func_0201d280(PTR_AT(e, 4), e + 0x16, U32_AT(e, 8), U32_AT(e, 0xc), U32_AT(e, 0x10),
                      e[0x14], e[0x15]);
        U32_AT(e, 0) = 1;
    }
    if (++CUR_SLOT >= 2)
        CUR_SLOT = 0;
}

/* 0x0201d4a0: the BG tile that holds source tile `tile` in bank `bank`,
 * reading it from the tile file into VRAM when it is not cached yet */
u32 func_0201d4a0(u32 tile, u32 bank, u32 unused, u32 layer)
{
    u8 *s = SCR();
    u16 *heads = ((u16 **)(s + 0xf0))[bank];
    u8 *pool = ((u8 **)(s + 0xe8))[bank];
    u8 *l = s + 0x44 + layer * 0x50;
    u32 key = tile & 0x7ff, i = heads[key], prev = 0xffff, n;

    while (i != 0xffff) {
        u8 *e = pool + i * 6;
        if (tile == U16_AT(e, 2)) {
            S16_AT(e, 0) += 1;
            return i;
        }
        prev = i & 0xffff;
        i = U16_AT(e, 4);
    }
    ((u16 *)(s + 0x126))[layer] += 1;
    ((u16 *)(s + 0x120))[bank] -= 1;
    n = ((u16 **)(s + 0xf8))[bank][((u16 *)(s + 0x120))[bank]];
    if (prev != 0xffff)
        U16_AT(pool + prev * 6, 4) = n;
    else
        ((u16 **)(s + 0xf0))[bank][key] = n;
    U16_AT(pool + n * 6, 2) = tile;
    U16_AT(pool + n * 6, 0) = 1;
    func_0201b930(data_020e1f0c,
                  (void *)(SHL(data_020df0fc, 21) + 0x06000000 + (BITS(U16_AT(l, 4), 2, 3) << 14)
                           + SHL(n + SHL(bank, 10), 6)),
                  SHL(tile, 6), 0x40);
    return n;
}
