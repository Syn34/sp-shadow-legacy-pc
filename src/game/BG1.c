/*
 * Background layers, part 1: per-frame scroll/rotation update, hardware
 * register commit, tile pixel plotting and map copies.
 * ARM9 main, 0x02009548 - 0x0200a2b4 (20 functions).
 *
 * Probably the start of "BG.cpp" (referenced at 0x0200aad0). Per screen
 * there is a 0x1b8-byte block at data_020da1ec: +0 dirty flag, +4 DISPCNT,
 * then 4 layers of 0x6c bytes from +8. Layer fields (relative to the layer):
 *   +0x00 BGxCNT, +0x08 map source, +0x10 map destination (VRAM),
 *   +0x14 -> tile base, +0x18 flags, +0x1c/+0x20 scroll (x, y; 16.16),
 *   +0x24/+0x28 current position, +0x2c/+0x30 velocity,
 *   +0x34/+0x38 target, +0x3c frames to target, +0x3e scale (with bounce
 *   +0x40 speed, +0x42 min, +0x44 max), +0x46 angle, +0x48 angular speed,
 *   +0x4a/+0x4c centre, +0x4e..+0x54 affine matrix, +0x58/+0x5c reference
 *   point, +0x60..+0x6a wave scroll.
 * Flags: 1 move to target, 2 rotate, 4 bounce scale, 8 matrix dirty,
 * 0x10/0x20 copy map, 0x40 wave, 0x80 lock scroll, 0x100 affine regs dirty,
 * 0x200 BGxCNT dirty, 0x400 scroll regs dirty.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020da1ec[];
extern const s32 data_020df168[2];
extern void (*data_020daa50)(const void *src, void *dst, u32 arg);   /* decompressor */

s32 func_020a4e00(s32 num, s32 den);        /* signed division */
void func_020951f4(const void *src, void *dst);   /* Thumb */
void func_02095044(const void *src, void *dst);
void func_02019864(s32 *out, const s32 *a, const s32 *b);
s32 func_02019910(s32 a, s32 b);
s32 func_0201993c(s32 a, s32 b);
s32 func_02019954(u32 ang);
s32 func_02019968(u32 ang);
s32 func_020199a0(u32 ang);
s32 func_020199c0(u32 ang);

/* this file's own functions, called before their definition */
void func_02009560(s32 layer, u32 color, u32 x, u32 y);
void func_020096d4(s32 layer, u32 color, u32 x, u32 y);
void func_02009748(u32 x, u32 y, u8 *tile, u32 color);
void func_02009944(s32 layer);
void func_0200a1bc(s32 layer);

#define SCREEN(scr) (data_020da1ec + (scr) * 0x1b8)
#define LAYER(layer) (SCREEN(data_020df0fc) + 8 + (layer) * 0x6c)
#define FLAGS(l) U32_AT(l, 0x18)
#define REG16(a) (*(vu16 *)(a))
#define REG32(a) (*(vu32 *)(a))

/* 0x02009548
 * @difftest */
u32 func_02009548(void) { return 0x2000; }

/* 0x02009550
 * @difftest */
u32 func_02009550(void) { return 0x100; }

/* 0x02009558
 * @difftest */
u32 func_02009558(void) { return 0x20; }

/* 0x02009560: plot a 5-pixel "plus"
 * @difftest @020df0fc:32=0 $B=zero:4 @020da200:32=$B pick:0 u8 int:1:255 int:1:191 */
void func_02009560(s32 layer, u32 color, u32 x, u32 y)
{
    func_020096d4(layer, color, x, y);
    func_020096d4(layer, color, x, y - 1);
    func_020096d4(layer, color, x, y + 1);
    func_020096d4(layer, color, x - 1, y);
    func_020096d4(layer, color, x + 1, y);
}

/* 0x020095d8: draw a thick line
 * @difftest @020df0fc:32=0 $B=zero:4 @020da200:32=$B pick:0 u8 int:1:200 int:1:180 int:1:200 int:1:180 cases=60 */
void func_020095d8(s32 layer, u32 color, s32 x0, s32 y0, s32 x1, s32 y1)
{
    s32 dx = (s16)(x1 - x0), dy = (s16)(y1 - y0), n, ady, sx, sy, fx, fy, i;

    n = (s16)(dx < 0 ? -dx : dx);
    ady = (s16)(dy < 0 ? -dy : dy);
    if (ady > n)
        n = ady;
    if (n <= 0)
        return;
    sx = func_0208f070(SHL(dx, 12), SHL(n, 12));
    sy = func_0208f070(SHL(dy, 12), SHL(n, 12));
    fx = SHL(x0, 12);
    fy = SHL(y0, 12);
    for (i = 0; i <= n; i++) {
        func_02009560(layer, color, fx >> 12, fy >> 12);
        fx += sx;
        fy += sy;
    }
}

/* 0x020096d4: plot one pixel into the layer's 8bpp tiles (sub-screen VRAM)
 * @difftest @020df0fc:32=0 $B=zero:4 @020da200:32=$B pick:0 u8 int:0:256 int:0:192 */
void func_020096d4(s32 layer, u32 color, u32 x, u32 y)
{
    u32 scr = data_020df0fc;
    u8 *l = data_020da1ec + scr * 0x1b8 + layer * 0x6c;
    u32 tile = *(u16 *)PTR_AT(l, 0x14) + (x >> 3) + ((y >> 3) << 5);
    func_02009748(x & 7, y & 7, (u8 *)(0x0602c000 + (scr << 21)) + tile * 64, color);
}

/* 0x02009748: set one pixel of an 8bpp tile (VRAM: 16-bit access)
 * @difftest int:0:8 int:0:8 vram:64:2 u8
 * @difftest int:0:8 int:0:8 ptr:64:2 u32 */
void func_02009748(u32 x, u32 y, u8 *tile, u32 color)
{
    vu16 *p = (vu16 *)(tile + ((x >> 1) + (y << 2)) * 2);
    u16 v = *p;
    if (x & 1)
        v = (u16)((v & ~0xff00u) | (color << 8));
    else
        v = (u16)((v & ~0xffu) | color);
    *p = v;
}

/* 0x02009790: unpack a resource's data (raw, LZ, Huffman or custom)
 * @difftest $R=ptr:4:2 @$R:16=pick:0,0x20,0x40,0x60 $R ptr:0x40 int:0:0x40 ptr:0x40 cases=100 */
void func_02009790(s32 handle, const void *src, u32 size, void *dst)
{
    u8 *res = func_02012a64(handle);
    switch (BITS(U16_AT(res, 0), 5, 2)) {
    case 0:
        MI_CpuCopy8(src, dst, size);
        break;
    case 1:
        func_020951f4(src, dst);
        break;
    case 2:
        func_02095044(src, dst);
        break;
    case 3:
        data_020daa50(src, dst, 0);
        break;
    }
    func_020129ac(handle);
}

/* 0x02009838: unpack data of a given compression kind
 * @difftest pick:0 ptr:0x40 int:0:0x40 ptr:0x40 */
void func_02009838(u32 kind, const void *src, u32 size, void *dst)
{
    switch (kind) {
    case 0:
        MI_CpuCopy8(src, dst, size);
        break;
    case 1:
        func_020951f4(src, dst);
        break;
    case 2:
        func_02095044(src, dst);
        break;
    case 3:
        data_020daa50(src, dst, 0);
        break;
    }
}

/* 0x020098cc: copy a w*h block of map entries into a 32-wide map, adding
 * a palette and tile offset
 * @difftest ptr:0x800:2 ptr:0x800:2 int:0:16 int:0:16 int:16:32 int:0:16 u16 */
void func_020098cc(u16 *dst, const u16 *src, u32 w, u32 h, s32 stride, u32 pal, u32 add)
{
    u32 palbits = pal << 12;
    if (h == 0)
        return;
    do {
        u32 k;
        for (k = w; k != 0; k--)
            *dst++ = *src++ + palbits + add;
        dst += 32 - w;
        src += stride - w;
    } while (--h != 0);
}

/* 0x02009944: advance the wave scroll of a layer
 * @difftest @020df0fc:32=0 @020da258:16=s16 @020da25a:16=s16 pick:0 */
void func_02009944(s32 layer)
{
    u8 *l = LAYER(layer);
    if (S16_AT(l, 0x64) != 0) {
        s32 s;
        S16_AT(l, 0x60) += S16_AT(l, 0x64);
        s = func_02019954((S16_AT(l, 0x60) >> 8) & 0xff);
        S32_AT(l, 0x1c) += S16_AT(l, 0x68) * s;
    }
    if (S16_AT(l, 0x66) != 0) {
        s32 c;
        S16_AT(l, 0x62) += S16_AT(l, 0x66);
        c = func_02019968((S16_AT(l, 0x62) >> 8) & 0xff);
        S32_AT(l, 0x20) += S16_AT(l, 0x6a) * c;
    }
}

/* 0x020099fc: write the dirty layer state to the hardware (VBlank) */
void func_020099fc(void)
{
    u32 i;
    for (i = 0; i < 4; i++) {
        u32 scr = data_020df0fc;
        u8 *l = SCREEN(scr) + 8 + i * 0x6c;
        if (!(FLAGS(l) & 0x710))
            continue;
        if ((FLAGS(l) & 0x500) && U8_AT(data_020df0b4 + scr, 0x38) != 0) {
            if (FLAGS(l) & 0x100) {
                if (i == 2) {
                    REG16(0x04000020 + (scr << 12)) = S16_AT(l, 0x4e);
                    REG16(0x04000022 + (scr << 12)) = S16_AT(l, 0x50);
                    REG16(0x04000024 + (scr << 12)) = S16_AT(l, 0x52);
                    REG16(0x04000026 + (scr << 12)) = S16_AT(l, 0x54);
                    REG32(0x04000028 + (scr << 12)) = U32_AT(l, 0x58) & ~0xf0000000u;
                    REG32(0x0400002c + (scr << 12)) = U32_AT(l, 0x5c) & ~0xf0000000u;
                } else if (i == 3) {
                    REG16(0x04000030 + (scr << 12)) = S16_AT(l, 0x4e);
                    REG16(0x04000032 + (scr << 12)) = S16_AT(l, 0x50);
                    REG16(0x04000034 + (scr << 12)) = S16_AT(l, 0x52);
                    REG16(0x04000036 + (scr << 12)) = S16_AT(l, 0x54);
                    REG32(0x04000038 + (scr << 12)) = U32_AT(l, 0x58) & ~0xf0000000u;
                    REG32(0x0400003c + (scr << 12)) = U32_AT(l, 0x5c) & ~0xf0000000u;
                }
            }
            if ((FLAGS(l) & 0x400) && !(FLAGS(l) & 0x80)) {
                u32 v = (U32_AT(l, 0x20) & 0xffff0000u) | (u16)(S32_AT(l, 0x1c) >> 16);
                REG32(0x04000010 + (scr << 12) + i * 4) = v;
            }
            FLAGS(l) &= ~0x500u;
        }
        if (FLAGS(l) & 0x200) {
            FLAGS(l) &= ~0x200u;
            REG16(0x04000008 + (scr << 12) + i * 2) = U16_AT(l, 0);
        }
        if ((FLAGS(l) & 0x10) && (FLAGS(l) & 0x20)) {
            FLAGS(l) &= ~0x20u;
            MI_CpuCopy8(PTR_AT(l, 8), PTR_AT(l, 0x10), 0x800);
        }
    }
    {
        u32 scr = data_020df0fc;
        u8 *g = SCREEN(scr);
        u32 f = U32_AT(g, 0);
        if (f & 1) {
            U32_AT(g, 0) = f & ~1u;
            REG32(0x04000000 + (scr << 12)) = U32_AT(g, 4);
        }
    }
}

/* 0x02009cd4: per-frame update of the four layers of the current screen */
void func_02009cd4(void)
{
    u32 i;
    for (i = 0; i < 4; i++) {
        u8 *l = LAYER(i);
        if (!(FLAGS(l) & 0x4f))
            continue;
        if (FLAGS(l) & 1) {
            u32 n = U16_AT(l, 0x3c);
            if (n == 0) {
                func_02019864((s32 *)(l + 0x24), (s32 *)(l + 0x2c), (s32 *)(l + 0x24));
            } else {
                if (n == 1) {
                    S32_AT(l, 0x24) = S32_AT(l, 0x34);
                    S32_AT(l, 0x28) = S32_AT(l, 0x38);
                    FLAGS(l) &= ~1u;
                } else {
                    S32_AT(l, 0x24) += func_020a4e00(S32_AT(l, 0x34) - S32_AT(l, 0x24), n);
                    S32_AT(l, 0x28) += func_020a4e00(S32_AT(l, 0x38) - S32_AT(l, 0x28), U16_AT(l, 0x3c));
                }
                U16_AT(l, 0x3c)--;
            }
            FLAGS(l) |= 0x400;
        }
        S32_AT(l, 0x1c) = S32_AT(l, 0x24);
        S32_AT(l, 0x20) = S32_AT(l, 0x28);
        if (FLAGS(l) & 0x40) {
            func_02009944(i);
            FLAGS(l) |= 0x400;
        }
        if (FLAGS(l) & 4) {
            S16_AT(l, 0x3e) += S16_AT(l, 0x40);
            if (S16_AT(l, 0x40) > 0) {
                if (S16_AT(l, 0x3e) >= S16_AT(l, 0x44)) {
                    S16_AT(l, 0x3e) = S16_AT(l, 0x44);
                    if (S16_AT(l, 0x42) != S16_AT(l, 0x44))
                        S16_AT(l, 0x40) = -S16_AT(l, 0x40);
                }
            } else if (S16_AT(l, 0x40) < 0) {
                if (S16_AT(l, 0x3e) <= S16_AT(l, 0x42)) {
                    S16_AT(l, 0x3e) = S16_AT(l, 0x42);
                    if (S16_AT(l, 0x42) != S16_AT(l, 0x44))
                        S16_AT(l, 0x40) = -S16_AT(l, 0x40);
                }
            }
            FLAGS(l) |= 8;
        }
        if (FLAGS(l) & 2) {
            S16_AT(l, 0x46) += S16_AT(l, 0x48);
            FLAGS(l) |= 8;
        }
        if (FLAGS(l) & 8) {
            u32 ang = (S16_AT(l, 0x46) >> 8) & 0xff;
            s32 sc = (s16)U16_AT(l, 0x3e), cx, cy;
            S16_AT(l, 0x4e) = func_0201993c(func_020199a0(ang), sc);
            S16_AT(l, 0x50) = func_0201993c(func_020199c0(ang), sc);
            S16_AT(l, 0x52) = func_0201993c((s16)-func_020199c0(ang), sc);
            S16_AT(l, 0x54) = func_0201993c(func_020199a0(ang), sc);
            cx = -0x800000 - SHL(S16_AT(l, 0x4a), 16);
            cy = -0x600000 - SHL(S16_AT(l, 0x4c), 16);
            func_02019910(SHL(S16_AT(l, 0x50), 8), cy);
            func_02019910(SHL(S16_AT(l, 0x4e), 8), cx);
            func_02019910(SHL(S16_AT(l, 0x54), 8), cy);
            func_02019910(SHL(S16_AT(l, 0x52), 8), cx);
            FLAGS(l) &= ~8u;
            FLAGS(l) |= 0x100;
        }
    }
}

/* 0x02009fec: set a layer's scale
 * @difftest int:0:4 s16 */
void func_02009fec(s32 layer, s32 scale)
{
    u8 *l = LAYER(layer);
    S16_AT(l, 0x3e) = scale;
    FLAGS(l) |= 8;
}

/* 0x0200a044: set a layer's angle and rotation speed
 * @difftest int:0:4 s16 s16
 * @difftest int:0:4 s16 pick:0 */
void func_0200a044(s32 layer, s32 angle, s32 speed)
{
    u8 *l = LAYER(layer);
    S16_AT(l, 0x46) = angle;
    S16_AT(l, 0x48) = speed;
    FLAGS(l) |= speed != 0 ? 2 : 8;
}

/* 0x0200a0b0: set a layer's rotation centre
 * @difftest int:0:4 s32 s32 */
void func_0200a0b0(s32 layer, s32 x, s32 y)
{
    u8 *l = LAYER(layer);
    S32_AT(l, 0x58) = SHL(x, 8);
    S32_AT(l, 0x5c) = SHL(y, 8);
}

/* 0x0200a100: set a layer's scroll position
 * @difftest int:0:4 s16 s16 */
void func_0200a100(s32 layer, s32 x, s32 y)
{
    u8 *l = LAYER(layer);
    s32 fx = SHL(x, 16), fy = SHL(y, 16);
    S32_AT(l, 0x24) = fx;
    S32_AT(l, 0x28) = fy;
    S32_AT(l, 0x1c) = fx;
    S32_AT(l, 0x20) = fy;
    FLAGS(l) |= 0x400;
}

/* 0x0200a188: lock a layer's scroll registers
 * @difftest int:0:4 */
void func_0200a188(s32 layer) { FLAGS(LAYER(layer)) |= 0x80; }

/* 0x0200a1bc: reset a layer
 * @difftest int:0:4 */
void func_0200a1bc(s32 layer)
{
    u8 *l = LAYER(layer);
    FLAGS(l) = 0;
    S32_AT(l, 0x2c) = data_020df168[0];
    S32_AT(l, 0x30) = data_020df168[1];
    S32_AT(l, 0x24) = data_020df168[0];
    S32_AT(l, 0x28) = data_020df168[1];
    S32_AT(l, 0x1c) = data_020df168[0];
    S32_AT(l, 0x20) = data_020df168[1];
    FLAGS(l) = 0x600;
    if (layer < 2)
        return;
    S16_AT(l, 0x3e) = 0x100;
    FLAGS(l) |= 8;
}

/* 0x0200a250: reset all layers of the current screen
 * @difftest */
void func_0200a250(void)
{
    u32 i;
    u8 *g;
    for (i = 0; i < 4; i++)
        func_0200a1bc(i);
    g = SCREEN(data_020df0fc);
    U32_AT(g, 4) &= 0xffff00f8;
    U32_AT(g, 0) = 1;
}
