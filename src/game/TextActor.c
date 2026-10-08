/*
 * TextActor and actor ground probing.
 * ARM9 main, 0x02001930 - 0x020030d0 (25 functions).
 *
 * The first function references the string "TextActor.cpp" (allocation debug
 * info), which names the original source file of at least the start of this
 * range. The range also holds a small decompressor, 8-way direction helpers and
 * the code that probes the ground under an actor with a ring of 10 points.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"
#include <stdarg.h>

/* forward declarations within this file */
void func_02001930(void *text, u32 *args);
void func_02001b10(void *text, const char *str);
void *func_02001c28(u32 *args);
void func_02002168(void *actor);
u32 func_02002270(void *actor, const s32 *mtx44, const s32 *in, s32 *out3, s32 *out2);
void func_020022e8(s32 *out, const s32 *mtx44, const s32 *in);
void func_02002974(void *self);
void func_02002978(s32 *mtx44, const fx32 *angles);
void func_02002bbc(fx32 *out, const fx32 *x, const fx32 *y, const fx32 *z);
void func_02002bd8(fx32 *out, const fx32 *vec2, const fx32 *len);
void func_02002c90(void *actor);

/* 0x02001930: lay out text (printf-style args) and upload the glyphs to OBJ VRAM */
void func_02001930(void *text, u32 *args)
{
    s32 limit = 0x7fffffff;
    void *work, *pixels;

    func_02001b10(text, (const char *)args[0]);
    work = func_0207ff70(0x78, data_020bc9e0, 0x1b1);
    pixels = func_0207ff48((BITS(U32_AT(text, 0xa8), 13, 1) + 1) * 0x780, data_020bc9e0, 0x1b2);
    func_020139f8(work, pixels, BITS(U32_AT(text, 0xa8), 13, 1));
    func_020134b8(0, 0, 0, 0xf0, args, 0, &limit);

    if (U16_AT(text, 0xb2) <= 8) {
        SET_BITS(U32_AT(text, 0xa8), 30, 2, 1);
        func_0207fed8(3, (void *)(SHL(data_020df0fc, 21) + 0x6400000 + U16_AT(text, 0xde) * 0x20),
                      pixels, U16_AT(text, 0xb4) << 7, 0x10);
    } else {
        u8 *dst, *src;
        s32 row;

        SET_BITS(U32_AT(text, 0xa8), 30, 2, 2);
        dst = (u8 *)(SHL(data_020df0fc, 21) + 0x6400000 + U16_AT(text, 0xde) * 0x20);
        src = (u8 *)pixels;
        for (row = 0; row < (s32)U16_AT(text, 0xb4); row++) {
            s32 i;
            for (i = 4; i > 0; i--) {
                func_0207fed8(3, dst, src, 0x20, 0x10);
                func_0207fed8(3, dst + 0x80, src + 0x20, 0x20, 0x10);
                src += 0x40;
                dst += 0x20;
            }
            dst += 0x80;
        }
    }
    func_0207ff14(pixels, data_020bc9e0, 0x1e3);
    func_0207ff14(work, data_020bc9e0, 0x1e4);
}

/* 0x02001b10: measure the text and (re)allocate its OBJ VRAM if the size changed */
void func_02001b10(void *text, const char *str)
{
    u32 w = func_020138a0(str) & 0xffff;
    u32 h = func_02013868(str) & 0xffff;
    s32 tiles, rows;
    u32 size, old;

    U16_AT(text, 0xb0) = (u16)(w < data_020bc9d8 ? w : data_020bc9d8);
    U16_AT(text, 0xb2) = (u16)(h < data_020bc9dc ? h : data_020bc9dc);
    tiles = (s32)(w + 0x1f) / 32;
    rows = (s32)(h + 7) / 8;
    U16_AT(text, 0xb4) = (u16)tiles;
    size = (u16)(((u16)((u32)tiles << 2)) * (u16)rows << 6);
    old = U16_AT(text, 0xdc);
    if (size == old)
        return;
    if (old != 0)
        func_02029228(U16_AT(text, 0xde), old, BITS(U32_AT(text, 0xa8), 13, 1) & 0xff);
    U16_AT(text, 0xdc) = (u16)size;
    func_02003e3c(text, 0);
}

/* 0x02001c04: printf-style text update
 * @difftest zero:0x200 ram:0x020bca68 cases=20 */
void func_02001c04(void *text, ...)
{
    va_list ap;
    va_start(ap, text);
    func_02001930(text, *(u32 **)&ap);
    va_end(ap);
}

/* 0x02001c28: create a text actor */
void *func_02001c28(u32 *args)
{
    void *t = func_02005cd0(0);

    U32_AT(t, 0x4) |= 8;
    U32_AT(t, 0x110) |= 4;
    U32_AT(t, 0x38) |= 0x40000;
    U32_AT(t, 0x38) |= 0x20;
    U32_AT(t, 0x38) |= 0x40;
    U8_AT(t, 0xe0) = 1;
    U32_AT(t, 0xa8) &= ~0x2000u;
    U32_AT(t, 0xac) &= ~0xc00u;
    U32_AT(t, 0xac) &= ~0xf000u;
    U32_AT(t, 0xac) = (U32_AT(t, 0xac) & ~0x3ffu) | (U16_AT(t, 0xde) & 0x3ff);
    SET_BITS(U32_AT(t, 0xa8), 14, 2, 1);
    func_02001930(t, args);
    return t;
}

/* 0x02001d0c: decompress a byte stream to a halfword-aligned destination using
 * only 16-bit stores (safe for VRAM). Commands:
 *   0x80|d  : copy (b & 0x1f) + 2 bytes from distance ((d << 3) | b >> 5);
 *             a length of 2 means fill ((d << 3) | b >> 5) + 0x42 bytes instead
 *   0x40|n  : fill n + 3 bytes with the next byte
 *   1..0x3f : copy that many literal bytes
 *   0       : end; *outLen receives the number of bytes written. */
/* @difftest $PAD=zero:0x1000 bytes:200:64 ptr:0x40000:2 ptr:8:4 cases=500 */
void func_02001d0c(const u8 *src, u8 *dst, s32 *outLen)
{
    u8 *start = dst;
    u32 cmd = *src++;
    u32 pend = 0;   /* a low byte is waiting for its high byte */
    u32 cur = 0;    /* pending byte(s) */
    u32 n, v, k;

    while (cmd != 0) {
        if (cmd & 0x80) {
            u32 b = *src++;
            u32 len = (b & 0x1f) + 2;
            u32 dist = ((cmd & 0x7f) << 3) | ((b & 0xe0) >> 5);
            const u8 *ref;

            if (len == 2) {
                n = dist + 0x42;
                goto fill;
            }
            ref = dst + pend - dist;
            if ((u32)ref & 1) {
                v = *ref++;
                pend ^= 1;
                len--;
                if (pend) {
                    cur = v;
                } else {
                    cur |= v << 8;
                    *(u16 *)dst = (u16)cur;
                    dst += 2;
                }
            }
            n = len >> 1;
            if (!pend) {
                while (n--) {
                    *(u16 *)dst = *(const u16 *)ref;
                    dst += 2;
                    ref += 2;
                }
                if (len & 1) {
                    pend = 1;
                    cur = *ref;
                }
            } else {
                while (n--) {
                    u32 h = *(const u16 *)ref;
                    ref += 2;
                    cur |= h << 8;
                    *(u16 *)dst = (u16)cur;
                    dst += 2;
                    cur = h >> 8;
                }
                if (len & 1) {
                    cur |= (u32)*ref << 8;
                    pend = 0;
                    *(u16 *)dst = (u16)cur;
                    dst += 2;
                }
            }
        } else if (cmd & 0x40) {
            n = (cmd & 0x3f) + 3;
        fill:
            v = *src++;
            if (pend) {
                n--;
                cur |= v << 8;
                *(u16 *)dst = (u16)cur;
                dst += 2;
            }
            cur = v | (v << 8);
            for (k = n >> 1; k != 0; k--) {
                *(u16 *)dst = (u16)cur;
                dst += 2;
            }
            pend = n & 1;
            if (pend)
                cur &= 0xff;
        } else {
            u32 len = cmd;
            if ((u32)src & 1) {
                pend ^= 1;
                if (pend) {
                    cur = *src++;
                } else {
                    cur |= (u32)*src++ << 8;
                    *(u16 *)dst = (u16)cur;
                    dst += 2;
                }
                len--;
            }
            n = len >> 1;
            if (!pend) {
                while (n--) {
                    *(u16 *)dst = *(const u16 *)src;
                    dst += 2;
                    src += 2;
                }
                if (len & 1) {
                    pend = 1;
                    cur = *src++;
                }
            } else {
                while (n--) {
                    u32 h = *(const u16 *)src;
                    src += 2;
                    cur |= h << 8;
                    *(u16 *)dst = (u16)cur;
                    dst += 2;
                    cur = h >> 8;
                }
                if (len & 1) {
                    cur |= (u32)*src++ << 8;
                    pend = 0;
                    *(u16 *)dst = (u16)cur;
                    dst += 2;
                }
            }
        }
        cmd = *src++;
    }
    if (pend)
        *dst++ = (u8)cur;
    *outLen = dst - start;
}

/* 0x02001ef4: 8-way direction -> angle (fx32 radians) */
fx32 func_02001ef4(u32 dir)
{
    switch (dir) {
    case 0: return g_FxPi;
    case 1: return g_Fx3Pi_4;
    case 2: return g_FxPi_2;
    case 3: return g_FxPi_4;
    case 4: return g_FxZero;
    case 5: return -g_FxPi_4;
    case 6: return -g_FxPi_2;
    case 7: return -g_Fx3Pi_4;
    default: return g_FxZero;
    }
}

/* 0x02001fac: pop up a number (e.g. damage or pickup count) above an actor
 * @difftest zero:0x200 int:-50:50 cases=20 */
void func_02001fac(void *actor, s32 value)
{
    char *buf = data_020bca68;
    s32 pos[2];
    void *t;
    u64 now, delay;

    if (PTR_AT(actor, 0xec) == NULL) {
        func_02002168(actor);
        t = PTR_AT(actor, 0xec);
        SET_BITS(U32_AT(t, 0x38), 5, 1, BITS(U32_AT(actor, 0x38), 5, 1));
        t = PTR_AT(actor, 0xec);
        SET_BITS(U32_AT(t, 0x38), 6, 1, BITS(U32_AT(actor, 0x38), 6, 1));
    }
    if (PTR_AT(actor, 0xec) == NULL)
        return;

    pos[0] = 0;
    pos[1] = 0;
    func_02028678(value, buf);
    if ((void *)data_020deebc[1] == actor) {
        if (value > 0)
            func_0201391c(3, 4, 0);
        else
            func_0201391c(3, 2, 0);
    } else {
        func_0201391c(3, 0, 0);
    }
    func_02001c04(PTR_AT(actor, 0xec), buf);
    U32_AT(PTR_AT(actor, 0xec), 0x4) |= 8;
    U32_AT(PTR_AT(actor, 0xec), 0x38) |= 0x2000;
    U32_AT(PTR_AT(actor, 0xec), 0x110) |= 4;
    S32_AT(actor, 0xf8) = -0x1e000;
    func_02003890(actor, pos);
    pos[1] = 0xc0 - pos[1];
    t = PTR_AT(actor, 0xec);
    func_02003b98(t, pos[0] - (U16_AT(t, 0xb0) >> 1), pos[1] + S32_AT(actor, 0xf8));
    now = func_020822c8(func_020062d0());
    delay = func_02082290(func_020062d0(), 2);
    Put64((u8 *)actor + 0xf0, now + delay);
}

/* 0x02002164: empty destructor (2D vector)
 * @difftest u32 */
void func_02002164(void *self) { (void)self; }

/* 0x02002168: create the actor's number-popup text actor
 * @difftest zero:0x200 cases=20 */
void func_02002168(void *actor)
{
    u32 args[2];
    void *t;

    if (PTR_AT(actor, 0xec) != NULL) {
        func_020044a0(PTR_AT(actor, 0xec));
        PTR_AT(actor, 0xec) = NULL;
    }
    args[0] = (u32)data_020bca74;
    func_0201391c(1, 0, 0);
    PTR_AT(actor, 0xec) = func_02001c28(args);
    U32_AT(PTR_AT(actor, 0xec), 0x4) |= 8;
    U32_AT(PTR_AT(actor, 0xec), 0x38) |= 0x2000;
    U32_AT(PTR_AT(actor, 0xec), 0x110) |= 4;
    func_0201ac94(PTR_AT(actor, 0xec), 0);
    t = PTR_AT(actor, 0xec);
    func_02003b98(t, 0x100, 0xc0);
}

/* 0x02002218
 * @difftest ptr:0x200:4 u8 */
void func_02002218(void *actor, u32 value)
{
    if (value != U8_AT(actor, 0x10e))
        U8_AT(actor, 0x10e) = (u8)value;
}

/* 0x02002228 */
void func_02002228(void *actor, void *arg)
{
    s32 id = S32_AT(arg, 0x8);
    if (id == -1)
        return;
    func_0200bd20(id, BITS(U32_AT(actor, 0xac), 12, 4) << 4, 0x10);
}

/* 0x02002270: transform a point and look up the ground height at its x/z */
u32 func_02002270(void *actor, const s32 *mtx44, const s32 *in, s32 *out3, s32 *out2)
{
    s32 tmp[3];
    s32 xz[2];

    func_020022e8(tmp, mtx44, in);
    out3[0] = tmp[0];
    out3[1] = tmp[1];
    out3[2] = tmp[2];
    out2[0] = SHL(out3[0], 4);
    out2[1] = SHL(out3[2], 4);
    xz[0] = out2[0];
    xz[1] = out2[1];
    return func_0201a0a0(xz, actor);
}

/* 0x020022e4: empty destructor (3D vector)
 * @difftest u32 */
void func_020022e4(void *self) { (void)self; }

/* 0x020022e8: out = in * M (4x4 fx32 matrix, row vector) */
void func_020022e8(s32 *out, const s32 *m, const s32 *in)
{
    s32 x = in[0], y = in[1], z = in[2];
    s64 t;

    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    t = (s64)y * m[4] + (s64)x * m[0] + (s64)z * m[8];
    out[0] = m[12] + (s32)((u64)t >> 12);
    t = (s64)y * m[5] + (s64)x * m[1] + (s64)z * m[9];
    out[1] = m[13] + (s32)((u64)t >> 12);
    t = (s64)y * m[6] + (s64)x * m[2] + (s64)z * m[10];
    out[2] = m[14] + (s32)((u64)t >> 12);
}

/* 0x02002394: ground heights under a ring of 10 points around the actor,
 * at its current position (heightsA) and at its next position (heightsB) */
void func_02002394(void *actor, u32 *heightsA, u32 *heightsB, fx32 depth, fx32 width)
{
    s32 pts[10][3];
    s32 mtx[16];
    fx32 dir[2], unit[2], angle, len;
    fx32 euler[3];
    s32 out3[3], out2[2];
    fx32 hw = FxMul(width, 0x800);
    fx32 nhw = FxMul(-width, 0x800);
    s32 i;

    pts[0][0] = width; pts[0][1] = data_020bca2c; pts[0][2] = depth;
    pts[1][0] = width; pts[1][1] = data_020bca18; pts[1][2] = data_020bc9f8;
    pts[2][0] = width; pts[2][1] = data_020bc9fc; pts[2][2] = -depth;
    pts[3][0] = hw;    pts[3][1] = data_020bca44; pts[3][2] = depth;
    pts[4][0] = hw;    pts[4][1] = data_020bca28; pts[4][2] = -depth;
    pts[5][0] = nhw;   pts[5][1] = data_020bca20; pts[5][2] = depth;
    pts[6][0] = nhw;   pts[6][1] = data_020bca50; pts[6][2] = -depth;
    pts[7][0] = -width; pts[7][1] = data_020bca08; pts[7][2] = depth;
    pts[8][0] = -width; pts[8][1] = data_020bc9f0; pts[8][2] = data_020bca24;
    pts[9][0] = -width; pts[9][1] = data_020bca54; pts[9][2] = -depth;

    func_0208ea6c(mtx);
    dir[0] = 0;
    dir[1] = 0;
    switch (U8_AT(actor, 0x10d)) {
    case 0: angle = g_FxPi_2; break;
    case 1: angle = g_FxPi_4; break;
    case 2: angle = g_FxZero; break;
    case 3: angle = g_Fx7Pi_4; break;
    case 4: angle = g_Fx3Pi_2; break;
    case 5: angle = g_Fx5Pi_4; break;
    case 6: angle = g_FxPi; break;
    case 7: angle = g_Fx3Pi_4; break;
    default: angle = 0; break;  /* original leaves this uninitialised */
    }
    len = func_0208eff8(FxMul(dir[0], dir[0]) + FxMul(dir[1], dir[1]));
    if (len <= 1) {
        dir[1] = 0;
        dir[0] = 0;
    } else {
        func_02002bd8(unit, dir, &len);
        dir[0] = unit[0];
        dir[1] = unit[1];
    }
    func_02002bbc(euler, &g_FxZero, &angle, &g_FxZero);
    func_02002978(mtx, euler);
    func_02002974(euler);

    mtx[12] = S32_AT(actor, 0x54) >> 4;
    mtx[13] = data_020bca30;
    mtx[14] = S32_AT(actor, 0x58) >> 4;
    out3[0] = out3[1] = out3[2] = 0;
    out2[0] = out2[1] = 0;
    for (i = 0; i < 10; i++)
        heightsA[i] = func_02002270(actor, mtx, pts[i], out3, out2);

    mtx[12] = S32_AT(actor, 0x5c) >> 4;
    mtx[13] = data_020bca48;
    mtx[14] = S32_AT(actor, 0x60) >> 4;
    for (i = 0; i < 10; i++)
        heightsB[i] = func_02002270(actor, mtx, pts[i], out3, out2);
}

/* 0x02002974 */
void func_02002974(void *self) { (void)self; }

static inline s32 SinOf(fx32 rad, s32 cos)
{
    u64 prod = (u64)(s64)rad * 0x28BE60DB9391ull;
    u32 idx = ((((u32)(prod >> 32) + 0x800) << 4) >> 16) >> 4;
    return FX_SinCosTable_[idx * 2 + cos];
}

/* 0x02002978: write the rotation part of a 4x4 matrix from Euler angles */
void func_02002978(s32 *m, const fx32 *angles)
{
    s32 sx = SinOf(angles[0], 0), cx = SinOf(angles[0], 1);
    s32 sy = SinOf(angles[1], 0), cy = SinOf(angles[1], 1);
    s32 sz = SinOf(angles[2], 0), cz = SinOf(angles[2], 1);
    fx32 sxcz = FxMul(sx, cz);
    fx32 sxsz = FxMul(sx, sz);

    m[0] = FxMul(cy, cz) + FxMul(sy, sxsz);
    m[1] = FxMul(cx, sz);
    m[2] = FxMul(cy, sxsz) - FxMul(sy, cz);
    m[4] = FxMul(sy, sxcz) - FxMul(sz, cy);
    m[5] = FxMul(cx, cz);
    m[6] = FxMul(sy, sz) + FxMul(sxcz, cy);
    m[8] = FxMul(cx, sy);
    m[9] = -sx;
    m[10] = FxMul(cx, cy);
}

/* 0x02002bbc: vector from three components */
void func_02002bbc(fx32 *out, const fx32 *x, const fx32 *y, const fx32 *z)
{
    out[0] = *x;
    out[1] = *y;
    out[2] = *z;
}

/* 0x02002bd8: 2D vector divided by a length */
void func_02002bd8(fx32 *out, const fx32 *vec2, const fx32 *len)
{
    fx32 y = func_0208f070(vec2[1], *len);
    out[0] = func_0208f070(vec2[0], *len);
    out[1] = y;
}

/* 0x02002c1c */
BOOL func_02002c1c(void *actor, s32 a, s32 b)
{
    s32 d = (s32)((u32)a - (u32)b);
    s32 e;
    if (d < 0 || d > 0x80000)
        return FALSE;
    e = (s32)((u32)a - (u32)S32_AT(actor, 0x6c));
    if (e <= 0 || e > 0x80000)
        return FALSE;
    return TRUE;
}

/* 0x02002c50 */
BOOL func_02002c50(void *actor, s32 a, s32 b)
{
    s32 d = (s32)((u32)a - (u32)b);
    (void)actor;
    if (d < 0 || d > 0x80000)
        return FALSE;
    return TRUE;
}

/* 0x02002c6c */
BOOL func_02002c6c(s32 h, s32 y, s32 step)
{
    if (h < y)
        return FALSE;
    return (s32)((u32)h - (u32)y) <= -step;
}

/* 0x02002c90: settle the actor on the ground (height at +0x6c, fall speed +0x78) */
void func_02002c90(void *actor)
{
    s32 xz[2], xz2[2];
    s32 heightsA[10], heightsB[10];
    s32 h, fall, y, best, bestIdx, i;
    BOOL blocked;

    S32_AT(actor, 0x6c) = S32_AT(actor, 0x24);
    xz[0] = S32_AT(actor, 0x54);
    xz[1] = S32_AT(actor, 0x58);
    func_0201a0a0(xz, actor);
    xz2[0] = S32_AT(actor, 0x5c);
    xz2[1] = S32_AT(actor, 0x60);
    h = func_0201a0a0(xz2, actor);
    func_02002394(actor, (u32 *)heightsA, (u32 *)heightsB, 0x4000, 0xb000);

    blocked = FALSE;
    if (func_02002c50(actor, heightsB[0], heightsA[0]) &&
        func_02002c50(actor, heightsB[1], heightsA[1]) &&
        func_02002c50(actor, heightsB[2], heightsA[2]))
        blocked = TRUE;
    if (blocked) {
        if (func_02002c1c(actor, heightsB[0], heightsA[0]) ||
            func_02002c1c(actor, heightsB[1], heightsA[1]) ||
            func_02002c1c(actor, heightsB[2], heightsA[2])) {
            S32_AT(actor, 0x6c) += 0x30000;
            return;
        }
    }

    fall = S32_AT(actor, 0x78) - 0x5000;
    if (fall < -0x40000)
        fall = -0x40000;
    y = S32_AT(actor, 0x6c) + fall;
    best = 0;
    bestIdx = -1;
    for (i = 0; i < 10; i++) {
        if (func_02002c6c(heightsA[i], y, fall) && heightsA[i] > best) {
            best = heightsA[i];
            bestIdx = i;
        }
    }
    if (bestIdx > -1) {
        S32_AT(actor, 0x6c) = heightsA[bestIdx];
        y = heightsA[bestIdx];
        S32_AT(actor, 0x78) = 0;
    } else {
        if ((s8)BITS(U32_AT(actor, 0x38), 17, 1) == 0 && y < h) {
            y = h;
            fall = 0;
        }
        S32_AT(actor, 0x6c) = y;
        S32_AT(actor, 0x78) = fall;
    }

    U32_AT(actor, 0x38) &= ~0x8000u;
    for (i = 0; i < 7; i++) {
        s32 v = heightsB[i];
        if (v > y || v == 0) {
            U32_AT(actor, 0x38) |= 0x8000;
            S32_AT(actor, 0x5c) = S32_AT(actor, 0x54);
            S32_AT(actor, 0x60) = S32_AT(actor, 0x58);
        }
    }
}

/* 0x02002ecc: move with ground probing, retrying along each axis when blocked */
void func_02002ecc(void *actor)
{
    s32 vx, vz, y, fall, n;

    func_02002c90(actor);
    vx = S32_AT(actor, 0x70);
    vz = S32_AT(actor, 0x74);
    y = S32_AT(actor, 0x6c);
    fall = S32_AT(actor, 0x78);
    for (n = 2; n > 0; n--) {
        s8 stuck = S32_AT(actor, 0x5c) == S32_AT(actor, 0x54) &&
                   S32_AT(actor, 0x60) == S32_AT(actor, 0x58);
        if (!stuck)
            break;
        if (S32_AT(actor, 0x70) == 0 && S32_AT(actor, 0x74) == 0)
            break;
        if (n == 2) {
            S32_AT(actor, 0x70) = vx;
            S32_AT(actor, 0x74) = 0;
        } else {
            S32_AT(actor, 0x70) = 0;
            S32_AT(actor, 0x74) = vz;
        }
        S32_AT(actor, 0x5c) = S32_AT(actor, 0x54) + S32_AT(actor, 0x70);
        S32_AT(actor, 0x60) = S32_AT(actor, 0x58) + S32_AT(actor, 0x74);
        func_02002c90(actor);
    }
    S32_AT(actor, 0x6c) = y;
    S32_AT(actor, 0x78) = fall;
}

/* 0x02002fa4: simple (single point) ground settle */
void func_02002fa4(void *actor)
{
    s32 xz[2], xz2[2];
    s32 h0, h1, fall, y;

    S32_AT(actor, 0x6c) = S32_AT(actor, 0x24);
    xz[0] = S32_AT(actor, 0x54);
    xz[1] = S32_AT(actor, 0x58);
    h0 = func_0201a0a0(xz, actor);
    xz2[0] = S32_AT(actor, 0x5c);
    xz2[1] = S32_AT(actor, 0x60);
    h1 = func_0201a0a0(xz2, actor);

    {
        s32 d = (s32)((u32)h1 - (u32)h0);
        if (d >= 0 && d <= 0x80000 && S32_AT(actor, 0x24) < h1) {
            S32_AT(actor, 0x6c) += 0x30000;
            return;
        }
    }

    fall = S32_AT(actor, 0x78) - 0x5000;
    if (fall < -0x40000)
        fall = -0x40000;
    y = S32_AT(actor, 0x6c) + fall;
    if (h0 >= y && (s32)((u32)h0 - (u32)y) <= -fall) {
        S32_AT(actor, 0x6c) = h0;
        S32_AT(actor, 0x78) = 0;
        y = h0;
    } else {
        S32_AT(actor, 0x6c) = y;
        S32_AT(actor, 0x78) = fall;
    }

    if (h1 > y || h1 == 0) {
        U32_AT(actor, 0x38) |= 0x8000;
        S32_AT(actor, 0x5c) = S32_AT(actor, 0x54);
        S32_AT(actor, 0x60) = S32_AT(actor, 0x58);
    } else {
        U32_AT(actor, 0x38) &= ~0x8000u;
    }
}
