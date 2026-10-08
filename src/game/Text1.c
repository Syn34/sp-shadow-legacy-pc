/*
 * Text.cpp, part 1: the text language, a per-screen raster (VCount
 * interrupt) scheduler and the OBJ VRAM tile allocator.
 * ARM9 main, 0x02028a50 - 0x02029364 (22 functions).
 *
 * Everything per screen is indexed by data_020df0fc (0 main, 1 sub).
 *
 * Raster scheduler: data_020e2fd0 + scr * 0x268: +0x000 a new list is
 * pending, +0x004 index of the next entry, +0x008 the active list and
 * +0x138 the pending list, 0x13 entries of 0x10 bytes each:
 * {u16 line; u16 pad; u32 arg0; u32 arg1; void (*fn)(void *, void *)}.
 * The VCount handler runs an entry, then programs DISPSTAT for the next
 * one (a NULL `fn` wraps back to entry 0).
 *
 * OBJ VRAM allocator: one bit per 32-byte block (0x400 blocks, 32 KiB) in
 * data_020e34a0 + scr * 0x80 (in use); data_020e35a0 + scr * 0x80 is a
 * mask of the blocks to keep at the end of the frame (deferred frees
 * clear bits in it). Sizes are converted to blocks by >> 5 (`mode` != 0)
 * or >> 6.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define REG16(a) (*(vu16 *)(a))
#define REG_DISPSTAT 0x04000004

extern const char data_020bde84[];   /* "Text.cpp" */
extern u8 data_020b6524[];           /* s32 text resource per language */
extern u8 data_020b653c[];           /* default raster list (1 entry) */
extern u8 data_020e2bc8[];           /* current language */
extern u8 data_020e2bcc[];           /* thousands separator */
extern u8 data_020e2fd0[];
extern u8 data_020e2fd4[];
extern u8 data_020e2fd8[];
extern u8 data_020e34a0[];
extern u8 data_020e35a0[];
extern u8 data_020ebbb8[];

s32 func_02017c94(u8 *blob);
void func_02017fd8(u32 v);
void func_0201800c(void);
void func_02018018(void (*fn)(void));
void func_02018158(void);
s32 func_020198e0(s32 num, s32 den);
u32 func_0200fcdc(const void *src, void *dst);

#define SCR() data_020df0fc
#define RAS() (data_020e2fd0 + SCR() * 0x268)
#define USED() (data_020e34a0 + SCR() * 0x80)
#define KEEP() (data_020e35a0 + SCR() * 0x80)
#define OBJ_VRAM(t) ((u8 *)((SCR() << 21) + 0x06400000) + (t) * 32)

void func_02028b04(void);
void func_02028b6c(void);
void func_02028c44(void);
void func_02028c8c(const void *list, u32 n);
u32 func_02028e60(const u8 *map, s32 n, u32 pos);
void func_02028f64(u32 pos, u32 n);
void func_02029004(u32 pos, u32 n);
void func_020290a4(u32 pos, u32 n);
void func_02029128(s32 *res, u32 tile);

/* 0x02028a50: switch the text language
 * @difftest pick:0,1,2,3,4 cases=20 */
void func_02028a50(u32 lang)
{
    func_0207fe28(data_020bde84, 0x7b);
    func_020129ac(((s32 *)data_020b6524)[data_020e2bc8[0]]);
    func_02017c94(func_02012a64(((s32 *)data_020b6524)[lang]));
    data_020e2bc8[0] = lang;
    func_0207fe24();
    if (data_020e2bc8[0] == 1)
        data_020e2bcc[0] = ' ';
    else if (data_020e2bc8[0] == 3)
        data_020e2bcc[0] = '.';
    else
        data_020e2bcc[0] = ',';
}

/* 0x02028ae8: switch to the language of the settings
 * @difftest @020ebbc2:8=pick:0,1,2,3,4 cases=20 */
void func_02028ae8(void)
{
    func_02028a50(data_020ebbb8[0xa]);
}

/* 0x02028b00
 * @difftest cases=2 */
void func_02028b00(void)
{
}

/* 0x02028b04: clear the pending raster list
 * @difftest @020df0fc:32=int:0:2 cases=10 */
void func_02028b04(void)
{
    u8 *s = RAS();
    s32 i;

    for (i = 0; i < 0x13; i++) {
        u8 *e = s + i * 0x10;
        U16_AT(e, 0x138) = 0xffff;
        U16_AT(e, 0x13a) = 0;
        U32_AT(e, 0x13c) = 0;
        U32_AT(e, 0x140) = 0;
        U32_AT(e, 0x144) = 0;
    }
}

/* 0x02028b6c: VCount handler: run the next raster entry of both screens
 * @difftest @020df0fc:32=int:0:2 cases=10 */
void func_02028b6c(void)
{
    s32 k;
    u16 v;
    u8 *s;

    for (k = 0; k < 2; k++) {
        u8 *e;
        s = RAS();
        e = s + 8 + U32_AT(s, 4) * 0x10;
        ((void (*)(void *, void *))PTR_AT(e, 0xc))(e + 4, e + 8);
        s = RAS();
        U32_AT(s, 4)++;
        if (U32_AT(s + U32_AT(s, 4) * 0x10, 0x14) == 0)
            U32_AT(s, 4) = 0;
        func_02018158();
    }
    REG16(REG_DISPSTAT) &= ~0x20;
    v = REG16(REG_DISPSTAT);
    REG16(REG_DISPSTAT) = v | (U16_AT(data_020e2fd8 + SCR() * 0x268, U32_AT(data_020e2fd4 + SCR() * 0x268, 0) * 0x10) << 7);
}

/* 0x02028c44: restart the active list at entry 0 */
void func_02028c44(void)
{
    u32 o = SCR() * 0x268;

    U32_AT(data_020e2fd4 + o, 0) = 0;
    func_02017fd8(U16_AT(data_020e2fd8 + o, U32_AT(data_020e2fd4 + o, 0) * 0x10));
}

/* 0x02028c8c: queue a new raster list of `n` entries (waits for the
 * previous one to be taken) */
void func_02028c8c(const void *list, u32 n)
{
    while (U32_AT(RAS(), 0) != 0)
        func_0201800c();
    func_02028b04();
    MI_CpuCopy8(list, RAS() + 0x138, n << 4);
    U32_AT(RAS(), 0) = 1;
}

/* 0x02028d24: take the pending raster list (VBlank)
 * @difftest @020df0fc:32=int:0:2 @020e2fd0:32=pick:0,1 @020e3238:32=pick:0,1 cases=20 */
void func_02028d24(void)
{
    u8 *s = RAS();

    if (U32_AT(s, 0) == 0)
        return;
    MI_CpuCopy8(s + 0x138, s + 8, 0x130);
    U32_AT(RAS(), 0) = 0;
    U32_AT(data_020e2fd4 + SCR() * 0x268, 0) = 0;
}

/* 0x02028da4: reset to the default raster list */
void func_02028da4(void)
{
    func_02028c8c(data_020b653c, 1);
    func_0201800c();
}

/* 0x02028dcc: initialise the raster scheduler of both screens */
void func_02028dcc(void)
{
    s32 k;

    for (k = 0; k < 2; k++) {
        U32_AT(RAS(), 4) = 0;
        func_02028b04();
        func_02028c8c(data_020b653c, 1);
        MI_CpuCopy8(RAS() + 0x138, RAS() + 8, 0x130);
        func_02018158();
    }
    func_02018018(func_02028b6c);
    func_02028c44();
}

/* 0x02028e60: find `n` free blocks from `pos` (0xffff: none). A whole
 * free byte counts as 8 blocks and a full byte is skipped as 8, even when
 * `pos` is not at a byte boundary.
 * @difftest ptr:0x80 int:0:0x40 int:0:0x400
 * @difftest $M=zero:0x80 @$M+0:32=u32 @$M+0x10:32=u32 @$M+0x14:32=u32 @$M+0x40:32=0xffffffff @$M+0x7c:32=u32 $M int:-2:0x80 int:0:0x400 */
u32 func_02028e60(const u8 *map, s32 n, u32 pos)
{
    s32 left = 1;
    u32 start = 0;

    while (pos + n <= 0x400 && left > 0) {
        start = pos;
        left = n;
        while (left > 0) {
            u8 b = map[pos >> 3];
            u32 bit;
            if (b == 0xff) {
                pos += 8;
                goto next;
            }
            if (b == 0) {
                pos += 8;
                left -= 8;
                continue;
            }
            bit = pos & 7;
            for (;;) {
                u32 m = 1 << bit;
                pos++;
                if (b & m)
                    goto next;
                bit++;
                left--;
                if (bit >= 8 || left <= 0)
                    break;
            }
        }
    next:;
    }
    return left <= 0 ? start : 0xffff;
}

/* 0x02028f04: end of frame: drop the blocks freed this frame
 * @difftest @020df0fc:32=int:0:2 cases=10 */
void func_02028f04(void)
{
    u32 *used = (u32 *)USED();
    u32 *keep = (u32 *)KEEP();
    u32 i;

    for (i = 0; i < 0x20; i++) {
        used[i] &= keep[i];
        keep[i] = 0xffffffff;
    }
}

/* 0x02028f64: free `n` blocks at `pos` at the end of the frame
 * @difftest @020df0fc:32=int:0:2 pick:0xffff,0,3,8,0x3f0 int:0:0x11 */
void func_02028f64(u32 pos, u32 n)
{
    u8 *map;
    u32 end;

    if (pos == 0xffff)
        return;
    end = pos + n;
    if (pos >= end)
        return;
    map = KEEP();
    do {
        u32 idx = pos >> 3, bit = pos & 7, m;
        if (bit == 0 && end - pos >= 8) {
            m = 0xff;
            pos += 8;
        } else {
            m = 1 << bit;
            pos++;
        }
        map[idx] &= ~m;
    } while (pos < end);
}

/* 0x02029004: free `n` blocks at `pos` now
 * @difftest @020df0fc:32=int:0:2 pick:0xffff,0,3,8,0x3f0 int:0:0x11 */
void func_02029004(u32 pos, u32 n)
{
    u8 *map;
    u32 end;

    if (pos == 0xffff)
        return;
    end = pos + n;
    if (pos >= end)
        return;
    map = USED();
    do {
        u32 idx = pos >> 3, bit = pos & 7, m;
        if (bit == 0 && end - pos >= 8) {
            m = 0xff;
            pos += 8;
        } else {
            m = 1 << bit;
            pos++;
        }
        map[idx] &= ~m;
    } while (pos < end);
}

/* 0x020290a4: mark `n` blocks at `pos` used
 * @difftest @020df0fc:32=int:0:2 pick:0,3,8,0x3f0 int:0:0x11 */
void func_020290a4(u32 pos, u32 n)
{
    u8 *map;
    u32 end = pos + n;

    if (pos >= end)
        return;
    map = USED();
    do {
        u32 idx = pos >> 3, bit = pos & 7, m;
        if (bit == 0 && end - pos >= 8) {
            m = 0xff;
            pos += 8;
        } else {
            m = 1 << bit;
            pos++;
        }
        map[idx] |= m;
    } while (pos < end);
}

/* 0x02029128: decompress the frames of an animation resource into OBJ
 * VRAM at block `tile`: res[1] is the frame table ({u16 count at +6; u16
 * offsets at +0xc} -> {u8 w, h at +2/+3; u16 data offset at +4}), res[0]
 * the graphics
 * @difftest $G=ptr:0x200:4 @$G+0:32=0x4000 $T=zero:0x40 @$T+6:16=int:0:3 @$T+0xc:16=4 @$T+0xe:16=0xc @$T+0x12:8=int:1:8 @$T+0x13:8=int:1:8 @$T+0x1a:8=int:1:8 @$T+0x1b:8=int:1:8 $X=ptr:8:4 @$X+0:32=$G @$X+4:32=$T @020df0fc:32=int:0:2 $X int:0:0x100 cases=60 */
void func_02029128(s32 *res, u32 tile)
{
    u8 *tab = func_02012a64(res[1]);
    u32 off = 0;
    u32 i;

    for (i = 0; i < U16_AT(tab, 6); i++) {
        u8 *f = tab + 0xc + U16_AT(tab, 0xc + i * 2);
        u8 *dst = OBJ_VRAM(tile) + off;
        off += func_020198e0(f[2] * f[3], 2);
        func_0200fcdc((u8 *)func_02012a64(res[0]) + U16_AT(f, 4), dst);
        func_020129ac(res[0]);
    }
    func_020129ac(res[1]);
}

/* 0x020291ec: free `size` bytes at block `pos` at the end of the frame
 * @difftest @020df0fc:32=int:0:2 pick:0xffff,0,8,0x100 pick:0,0x40,0x200,0x800 pick:0,1 */
void func_020291ec(u32 pos, u32 size, u32 mode)
{
    if (size == 0)
        return;
    func_02028f64(pos, (u16)(mode != 0 ? size >> 5 : size >> 6));
}

/* 0x02029228: free `size` bytes at block `pos` now
 * @difftest @020df0fc:32=int:0:2 pick:0xffff,0,8,0x100 pick:0,0x40,0x200,0x800 pick:0,1 */
void func_02029228(u32 pos, u32 size, u32 mode)
{
    func_02029004(pos, (u16)(mode != 0 ? size >> 5 : size >> 6));
}

/* 0x02029248: decompress `src` into OBJ VRAM at block `tile` */
u32 func_02029248(const void *src, u32 tile)
{
    return func_0200fcdc(src, OBJ_VRAM(tile));
}

/* 0x0202926c: decompress `src` into the blocks of sprite `obj` */
u32 func_0202926c(void *obj, const void *src)
{
    return func_02029248(src, U16_AT(obj, 0xde));
}

/* 0x02029284: allocate `size` bytes of OBJ VRAM (and fill them from the
 * animation resource `res`, if any); returns the block or 0xffff
 * @difftest @020df0fc:32=int:0:2 pick:0 pick:0,0x40,0x200,0x800,0x8000 pick:0,1 */
u16 func_02029284(s32 res, u32 size, u32 mode)
{
    u32 n = mode == 0 ? size >> 6 : size >> 5;
    u32 pos = func_02028e60(USED(), n, 0);

    if (pos == 0xffff)
        return pos;
    func_020290a4((u16)pos, (u16)n);
    if (res != 0)
        func_02029128((s32 *)res, (u16)pos);
    return pos;
}

/* 0x0202930c: free all OBJ VRAM of the screen
 * @difftest @020df0fc:32=int:0:2 cases=10 */
void func_0202930c(void)
{
    MI_CpuFill8(USED(), 0, 0x80);
    MI_CpuFill8(KEEP(), 0xff, 0x80);
}
