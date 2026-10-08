/*
 * Background layers, part 2: loading tiles/palettes/maps from resources and
 * files, map writing, tile/screen clears, display mode set-up.
 * ARM9 main, 0x0200a2b4 - 0x0200b36c (33 functions).
 *
 * Source file "BG.cpp" (func_0200aad0). Layer layout: see BG1.c. Some
 * functions address a layer without the +8 of the screen header
 * (LAYER0), so their field offsets are 8 larger.
 *
 * BG data blobs: u16 flags (bit 0: 256-colour palette, bit 1: 16-colour
 * palette in bits 12-15, bit 2: raw VRAM block, bit 3: map follows, bit 4:
 * tiles follow, bits 5-6: tile compression, bit 7: 8bpp, bit 8: half-size map).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020da1ec[];
extern u8 data_020da55c[];                    /* palette animation state */
extern const u16 data_020bccd0, data_020bccd4, data_020bccd8, data_020bccdc;
extern const char data_020bcce0[];            /* "BG.cpp" */

void MIi_CpuClear16(u16 data, void *dest, u32 size);
void func_02009790(s32 handle, const void *src, u32 size, void *dst);
void func_02009838(u32 kind, const void *src, u32 size, void *dst);
void func_020098cc(u16 *dst, const u16 *src, u32 w, u32 h, s32 stride, u32 pal, u32 add);
void func_0200a250(void);
void func_0200bd58(const void *pal, u32 offset, u32 count);
void func_0201817c(u32 screen);
void func_020804ac(void *file, u32 id);
void func_0208f82c(u32 v);
void func_0208f848(u32 a, u32 b, u32 c);
void func_0208ff2c(u32 v);
void func_02090768(u32 v);
void func_020904e8(u32 v);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));
void func_020a6dcc(void *arr, u32 n, u32 size, void *(*ctor)(void *), void *(*dtor)(void *));

/* this file's own functions, called before their definition */
void func_0200a2b4(s32 layer, const u16 *map, u32 add, u32 pal, u32 x, u32 y);
u32 func_0200a704(s32 handle, u32 pal);
u16 *func_0200a77c(s32 layer, s32 handle, u32 tile, u32 pal);
u32 func_0200a934(const u8 *data, u32 pal);
u16 *func_0200a9a4(s32 layer, const u8 *data, u32 tile, u32 pal);
void func_0200ab94(u8 *map, u32 x, u32 y, s32 w, s32 h, u32 tile, u32 stride);
void func_0200ac78(u8 *map, u32 x, u32 y, s32 w, s32 h, u32 v);
void func_0200ad34(u8 *l, u32 tile, u32 pix);
void func_0200ae14(u8 *l, u32 v, u32 pal);
void func_0200aec8(s32 layer, u32 v, u32 pal);
void func_0200af2c(u32 bits);
void func_0200af6c(u32 bits);
void func_0200afb0(u32 mode, u32 flags);
u32 func_0200b068(void);
void func_0200b0c0(s32 layer, u32 cnt);
void func_0200b1a0(u32 screen);
void *func_0200b264(void *scr);
void *func_0200b290(void *l);
void *func_0200b2d4(void *l);

#define SCREEN(scr) (data_020da1ec + (scr) * 0x1b8)
#define LAYER(layer) (SCREEN(data_020df0fc) + 8 + (layer) * 0x6c)
#define LAYER0(layer) (SCREEN(data_020df0fc) + (layer) * 0x6c)
#define FLAGS(l) U32_AT(l, 0x18)
#define VRAM_BG(scr) (0x06000000u + ((scr) << 21))
#define CHAR_BASE(cnt) (BITS(cnt, 2, 3) << 14)
#define SCREEN_BASE(cnt) (BITS(cnt, 8, 5) << 11)

static inline u32 MinU(u32 a, u32 b) { return a >= b ? b : a; }
static inline u32 MaxK(u32 k, u32 v) { return k <= v ? v : k; }

/* does this layer use 16-bit (text) map entries in this BG mode */
static int IsTextLayer(u32 mode, s32 layer)
{
    if (mode == 0 || layer <= 1)
        return 1;
    if (mode == 1 && layer <= 2)
        return 1;
    if (mode == 2 && layer <= 1)
        return 1;
    if (mode == 3)
        return 1;
    if (mode == 4 && layer != 2)
        return 1;
    return (u16)(mode + 0xfffb) <= 1;
}

/* 0x0200a2b4: write a map {u16 w, h, entries} into the layer's map buffer at
 * (x, y), across the 32x32 screen blocks of the layer's size */
void func_0200a2b4(s32 layer, const u16 *map, u32 add, u32 pal, u32 x, u32 y)
{
    u8 *l = LAYER(layer);
    const u16 *src = map + 2;
    u8 *buf = PTR_AT(l, 0xc);
    u32 mode = func_0200b068();

    if (IsTextLayer(mode, layer)) {
        u32 w0, h0, w1, h1, size, w, ox, sx, oy, sy, avail;
        u8 *blk;

        w0 = MaxK(data_020bccd4, MinU((u16)(32 - x), map[0]));
        h0 = MaxK(data_020bccd0, MinU((u16)(32 - y), map[1]));
        func_020098cc((u16 *)(buf + (x + y * 32) * 2), src, w0, h0, map[0], pal, add);

        size = BITS(U16_AT(l, 0), 14, 2);
        blk = (u8 *)PTR_AT(l, 0xc) + 0x800;
        if (size == 1 || size == 3) {
            w = map[0];
            ox = w0 == 0 ? x - 32 : 0;
            sx = w0 == 0 ? x - 32 : 0;
            avail = w - w0;
            w1 = MinU(32 - sx, avail);
            func_020098cc((u16 *)(blk + (y * 32 + ox) * 2), src + w0, w1, h0, w, pal, add);
            blk = (u8 *)PTR_AT(l, 0xc) + 0x1000;
        }
        size = BITS(U16_AT(l, 0), 14, 2);
        if ((u16)(size + 0xfffe) <= 1) {
            w = map[0];
            oy = h0 == 0 ? y - 32 : 0;
            w0 = MaxK(data_020bccdc, MinU((u16)(32 - x), w));
            sy = h0 == 0 ? y - 32 : 0;
            avail = map[1] - h0;
            h1 = MinU(32 - sy, avail);
            func_020098cc((u16 *)(blk + (x + oy * 32) * 2), src + h0 * w, w0, h1, w, pal, add);
            blk = (u8 *)PTR_AT(l, 0xc) + 0x1800;
        }
        if (BITS(U16_AT(l, 0), 14, 2) == 3) {
            u32 hh = map[1];
            h1 = MaxK(data_020bccd8, MinU((u16)(32 - y), hh));
            w = map[0];
            oy = h1 == 0 ? y - 32 : 0;
            ox = w0 == 0 ? x - 32 : 0;
            sx = w0 == 0 ? x - 32 : 0;
            w1 = MinU(32 - sx, w - w0);
            sy = h1 == 0 ? y - 32 : 0;
            func_020098cc((u16 *)(blk + (ox + oy * 32) * 2), src + h1 * w + w0, w1,
                          MinU(32 - sy, hh - h1), w, pal, add);
        }
    } else {
        /* affine layer: 8-bit map entries */
        u32 stride = 16u << BITS(U16_AT(l, 0), 14, 2);
        u32 rows = map[1];
        u8 *p = buf + y * stride + x;
        const u8 *s = (const u8 *)src;
        if (rows != 0) {
            do {
                u32 n = map[0];
                vu16 *q = (vu16 *)((u32)p & ~1u);
                if (n != 0) {
                    do {
                        if ((u32)p & 1) {
                            *q = (u16)((*q & 0xff) | (add + (s[0] << 8)));
                        } else if (n == 1) {
                            *q = (u16)((*q & 0xff00) | (s[0] + add));
                        } else {
                            *q = (u16)((s[0] + add) | ((s[1] + add) << 8));
                            p++;
                            s++;
                            n--;
                        }
                        q++;
                        p++;
                        s++;
                    } while (--n != 0);
                }
                p += stride - map[0];
            } while (--rows != 0);
        }
    }
    FLAGS(l) |= 0x20;
}

/* 0x0200a704: load the palette of a BG resource; returns its size
 * @difftest $R=ptr:0x220:2 @$R:16=pick:0,1,2,0x3002,0x4 $R u8 */
u32 func_0200a704(s32 handle, u32 pal)
{
    u16 *res = func_02012a64(handle);
    u32 f = *res++, r;
    if (f & 1) {
        func_0200bd58(res, 0, 0x100);
        r = 0x200;
    } else if (f & 2) {
        func_0200bd58(res, (pal + (f >> 12)) << 4, 0x10);
        r = 0x20;
    } else {
        r = 0;
    }
    func_020129ac(handle);
    return r;
}

/* skip the map header {u16 w, h, entries} of a BG blob */
static inline const u8 *SkipMap(const u8 *p, u32 flags)
{
    s32 n = U16_AT(p, 0) * U16_AT(p, 2);
    if (n & 1)
        n++;
    if (BITS(flags, 8, 1))
        n /= 2;
    return p + n * 2 + 4;
}

/* destination of a BG blob's tiles */
static inline void *TileDest(u8 *l, u32 flags, u32 tile)
{
    u8 *base = (u8 *)VRAM_BG(data_020df0fc) + CHAR_BASE(U16_AT(l, 0));
    return BITS(flags, 7, 1) ? base + tile * 64 : base + tile * 32;
}

/* 0x0200a77c: load palette, VRAM block and tiles of a BG resource; returns its map
 * @difftest $R=ptr:0x400:2 @$R:16=pick:0x10,0x18,0x98,0x118,0x0 @$R+2:16=int:1:8 @$R+4:16=int:1:8 pick:0,1,2,3 $R int:0:64 int:0:16 */
u16 *func_0200a77c(s32 layer, s32 handle, u32 tile, u32 pal)
{
    u16 *ret = NULL;
    u8 *res = func_02012a64(handle);
    u8 *l = LAYER(layer);
    const u8 *p = res + 2;

    p += func_0200a704((s32)res, pal);
    if (BITS(U16_AT(res, 0), 2, 1)) {
        u32 n = U16_AT(p, 0);
        p += 2;
        MI_CpuCopy8(p, (void *)VRAM_BG(data_020df0fc), n);
    }
    if (BITS(U16_AT(res, 0), 4, 1)) {
        void *dst;
        if (BITS(U16_AT(res, 0), 3, 1)) {
            ret = (u16 *)p;
            p = SkipMap(p, U16_AT(res, 0));
        }
        dst = TileDest(l, U16_AT(res, 0), tile);
        func_02009790((s32)res, p + 2, U16_AT(p, 0), dst);
    }
    func_020129ac(handle);
    return ret;
}

/* 0x0200a8bc: load a BG resource and write its map at (x, y)
 * @difftest $R=ptr:0x400:2 @$R:16=pick:0x10,0x18,0x98,0x118,0x0 @$R+2:16=int:1:8 @$R+4:16=int:1:8 pick:0,1 $R int:0:64 int:0:16 int:0:40 int:0:40 */
u16 *func_0200a8bc(s32 layer, s32 handle, u32 tile, u32 pal, u32 x, u32 y)
{
    u8 *res = func_02012a64(handle);
    u16 *map = func_0200a77c(layer, (s32)res, tile, pal);
    if (map != NULL)
        func_0200a2b4(layer, map, tile, pal, x, y);
    func_020129ac(handle);
    return map;
}

/* 0x0200a934: load the palette of a BG blob in memory; returns its size */
u32 func_0200a934(const u8 *data, u32 pal)
{
    u32 f = U16_AT(data, 0);
    data += 2;
    if (f & 1) {
        func_0200bd58(data, 0, 0x100);
        return 0x200;
    }
    if (!(f & 2))
        return 0;
    func_0200bd58(data, (pal + (f >> 12)) << 4, 0x10);
    return 0x20;
}

/* 0x0200a9a4: load palette, VRAM block and tiles of a BG blob in memory */
u16 *func_0200a9a4(s32 layer, const u8 *data, u32 tile, u32 pal)
{
    u8 *l = LAYER(layer);
    const u8 *p = data + 2;
    u16 *ret = NULL;

    p += func_0200a934(data, pal);
    if (BITS(U16_AT(data, 0), 2, 1)) {
        u32 n = U16_AT(p, 0);
        p += 2;
        MI_CpuCopy8(p, (void *)VRAM_BG(data_020df0fc), n);
    }
    if (BITS(U16_AT(data, 0), 4, 1)) {
        void *dst;
        if (BITS(U16_AT(data, 0), 3, 1)) {
            ret = (u16 *)p;
            p = SkipMap(p, U16_AT(data, 0));
        }
        dst = TileDest(l, U16_AT(data, 0), tile);
        func_02009838(BITS(U16_AT(data, 0), 5, 2), p + 2, U16_AT(p, 0), dst);
    }
    return ret;
}

/* 0x0200aad0: load a BG file and write its map at (x, y) */
u16 *func_0200aad0(s32 layer, u32 file_id, u32 tile, u32 pal, u32 x, u32 y)
{
    u32 file[0x48 / 4];
    u8 *buf = NULL;
    s32 n;
    u16 *map;

    func_020804ac(file, file_id);
    n = func_02080388(file);
    if (n > 0) {
        buf = func_0207ff70(n, data_020bcce0, 0x6d4);
        func_02080418(buf, n, 1, file);
    }
    func_02080458(file);
    map = func_0200a9a4(layer, buf, tile, pal);
    if (map != NULL)
        func_0200a2b4(layer, map, tile, pal, x, y);
    func_0207ff14(buf, data_020bcce0, 0x6e7);
    return map;
}

/* 0x0200ab94: write consecutive tile numbers into a w*h block of a 32-wide map
 * @difftest ptr:0x800:2 int:0:8 int:0:8 int:0:8 int:0:8 u16 int:0:32 */
void func_0200ab94(u8 *map, u32 x, u32 y, s32 w, s32 h, u32 tile, u32 stride)
{
    s32 row, k;
    u32 t = tile, skip = (u16)stride - w;
    if (h <= 0)
        return;
    for (row = 0; row < h; row++) {
        u16 *d = (u16 *)(map + (y + row) * 64 + x * 2);
        for (k = 0; k < w; k++) {
            *d++ = t;
            t = (u16)(t + 1);
        }
        t = (u16)(t + skip);
    }
}

/* 0x0200ac20: write consecutive tile numbers into a layer's map
 * @difftest pick:0,1,2,3 int:0:8 int:0:8 int:0:8 int:0:8 u16 int:0:32 */
void func_0200ac20(s32 layer, u32 x, u32 y, s32 w, s32 h, u32 tile, u32 stride)
{
    func_0200ab94(PTR_AT(LAYER0(layer), 0x18), x, y, w, h, (u16)tile, (u16)stride);
}

/* 0x0200ac78: fill a w*h block of a 32-wide map
 * @difftest ptr:0x800:2 int:0:8 int:0:8 int:0:8 int:0:8 u16 */
void func_0200ac78(u8 *map, u32 x, u32 y, s32 w, s32 h, u32 v)
{
    s32 row, k;
    if (h <= 0)
        return;
    for (row = 0; row < h; row++) {
        u16 *d = (u16 *)(map + (y + row) * 64 + x * 2);
        for (k = 0; k < w; k++)
            *d++ = v;
    }
}

/* 0x0200ace4: fill a block of a layer's map
 * @difftest pick:0,1,2,3 int:0:8 int:0:8 int:0:8 int:0:8 u16 */
void func_0200ace4(s32 layer, u32 x, u32 y, s32 w, s32 h, u32 v)
{
    func_0200ac78(PTR_AT(LAYER0(layer), 0x18), x, y, w, h, (u16)v);
}

/* 0x0200ad34: fill one tile of a layer with a colour */
void func_0200ad34(u8 *l, u32 tile, u32 pix)
{
    u32 cnt = U16_AT(l, 0), bpp8 = BITS(cnt, 7, 1), tsize = (bpp8 + 1) << 5;
    u8 *dst = (u8 *)VRAM_BG(data_020df0fc) + CHAR_BASE(cnt) + tsize * tile;
    u16 v;
    if (!bpp8) {
        u32 c = pix & 0xf;
        v = (u16)((c << 12) + (c << 8) + (c << 4) + c);
    } else {
        u32 c = pix & 0xff;
        v = (u16)(c + (c << 8));
    }
    MIi_CpuClear16(v, dst, tsize);
}

/* 0x0200add4: fill one tile of a layer with a colour
 * @difftest @020df0fc:32=0 pick:0,1,2,3 int:0:64 u8 */
void func_0200add4(s32 layer, u32 tile, u32 pix)
{
    func_0200ad34(LAYER(layer), tile, pix);
}

/* 0x0200ae14: clear a layer: fill tile `v` with colour `pal`, fill the map with it */
void func_0200ae14(u8 *l, u32 v, u32 pal)
{
    u32 size;
    func_0200ad34(l, v, pal);
    if (!BITS(U16_AT(l, 0), 7, 1))
        v = (u16)(v | ((pal >> 4) << 12));
    switch (BITS(U16_AT(l, 0), 14, 2)) {
    case 0:
        size = 0x800;
        break;
    case 1:
    case 2:
        size = 0x1000;
        break;
    case 3:
        size = 0x2000;
        break;
    default:
        size = 0;
        break;
    }
    MIi_CpuClear16(v, (u8 *)VRAM_BG(data_020df0fc) + SCREEN_BASE(U16_AT(l, 0)), size);
}

/* 0x0200aec8: clear a layer
 * @difftest @020df0fc:32=0 pick:0,1,2,3 int:0:64 u8 */
void func_0200aec8(s32 layer, u32 v, u32 pal)
{
    func_0200ae14(LAYER(layer), v, pal);
}

/* 0x0200af08
 * @difftest @020df0fc:32=0 pick:0,1,2,3 int:0:64 */
void func_0200af08(s32 layer, u32 v) { func_0200aec8(layer, v, 0); }

/* 0x0200af18
 * @difftest @020df0fc:32=0 pick:0,1,2,3 */
void func_0200af18(s32 layer) { func_0200aec8(layer, 0, 0); }

/* 0x0200af2c: set DISPCNT bits
 * @difftest u32 */
void func_0200af2c(u32 bits)
{
    u8 *g = SCREEN(data_020df0fc);
    U32_AT(g, 4) |= bits;
    U32_AT(g, 0) |= 1;
}

/* 0x0200af6c: clear DISPCNT bits
 * @difftest u32 */
void func_0200af6c(u32 bits)
{
    u8 *g = SCREEN(data_020df0fc);
    U32_AT(g, 4) &= ~bits;
    U32_AT(g, 0) |= 1;
}

/* 0x0200afb0: set the display mode of the current screen */
void func_0200afb0(u32 mode, u32 flags)
{
    func_0200a250();
    U32_AT(SCREEN(data_020df0fc), 4) = mode | 0x10010;
    if (flags & 1) {
        U32_AT(SCREEN(data_020df0fc), 4) |= 0x40000000;
        func_020904e8(0x40);
    }
    if (flags & 2)
        U32_AT(SCREEN(data_020df0fc), 4) |= 8;
    U32_AT(SCREEN(data_020df0fc), 0) |= 1;
}

/* 0x0200b068: BG mode of the current screen
 * @difftest */
u32 func_0200b068(void) { return (u16)(U32_AT(SCREEN(data_020df0fc), 4) & 7); }

/* 0x0200b098: hide a layer
 * @difftest int:0:4 */
void func_0200b098(s32 layer) { func_0200af6c(0x100u << layer); }

/* 0x0200b0ac: show a layer
 * @difftest int:0:4 */
void func_0200b0ac(s32 layer) { func_0200af2c(0x100u << layer); }

/* 0x0200b0c0: set a layer's BGxCNT and derive its VRAM addresses
 * @difftest int:0:4 u16 */
void func_0200b0c0(s32 layer, u32 cnt)
{
    u8 *l = LAYER(layer);
    cnt = (u16)cnt;
    U16_AT(l, 0) = cnt;
    PTR_AT(l, 0x10) = (void *)(VRAM_BG(data_020df0fc) + SCREEN_BASE(cnt));
    PTR_AT(l, 0xc) = PTR_AT(l, 0x10);
    PTR_AT(l, 0x14) = (void *)(VRAM_BG(data_020df0fc) + CHAR_BASE(cnt));
    FLAGS(l) |= 0x200;
}

/* 0x0200b168: set up and show a layer
 * @difftest int:0:4 u16 */
void func_0200b168(s32 layer, u32 cnt)
{
    func_0200b0c0(layer, (u16)cnt);
    func_0200af2c(0x100u << layer);
}

/* 0x0200b1a0: initialise the display of a screen */
void func_0200b1a0(u32 screen)
{
    u32 saved = data_020df0fc;
    if (screen == 0) {
        func_02090768(1);
        func_0208f848(1, 0, 0);
        func_0201817c(0);
        func_0200afb0(0, 0);
    } else {
        func_0208ff2c(4);
        func_0208f82c(0);
        func_0201817c(1);
        func_0200afb0(0, 0);
    }
    func_0201817c(saved);
}

/* 0x0200b21c: initialise both screens */
void func_0200b21c(void)
{
    func_0200b1a0(0);
    func_0200b1a0(1);
}

/* 0x0200b240: static destructor of the screen blocks
 * @difftest cases=2 */
void func_0200b240(void) { func_020a6d58(data_020da1ec, 2, 0x1b8, func_0200b264); }

/* 0x0200b264: screen block destructor
 * @difftest ptr:0x1b8:4 cases=20 */
void *func_0200b264(void *scr)
{
    func_020a6d58((u8 *)scr + 8, 4, 0x6c, func_0200b290);
    return scr;
}

/* 0x0200b290: layer destructor (empty)
 * @difftest u32 */
void *func_0200b290(void *l) { return l; }

/* 0x0200b294: screen block constructor
 * @difftest ptr:0x1b8:4 cases=20 */
void *func_0200b294(void *scr)
{
    func_020a6dcc((u8 *)scr + 8, 4, 0x6c, func_0200b2d4, func_0200b290);
    return scr;
}

/* 0x0200b2d4: layer constructor
 * @difftest ptr:0x40:4 */
void *func_0200b2d4(void *l)
{
    u32 i;
    for (i = 0x1c; i <= 0x38; i += 4)
        U32_AT(l, i) = 0;
    return l;
}

/* 0x0200b2fc: step the palette animation
 * @difftest $A=ptr:0x80:4 @$A+1:8=int:1:4 @$A+0xe:16=int:0:3 @020da55c:8=pick:3,7,1 @020da55e:8=int:0:3 @020da55f:8=u8 @020da564:32=$A */
void func_0200b2fc(void)
{
    u8 *s = data_020da55c, *anim;
    u32 cur;
    if (s[0] != 3)
        return;
    cur = s[2];
    anim = PTR_AT(s, 8);
    s[3]++;
    if (s[3] < U16_AT(anim + cur * 8, 0xe))
        return;
    s[2] = cur + 1;
    s[3] = 0;
    if (s[2] >= anim[1])
        s[2] = 0;
    s[0] |= 4;
}
