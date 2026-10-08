#ifndef SPYRO_GAME_H
#define SPYRO_GAME_H

/* Helpers shared by decompiled game code. */
#include "types.h"
#include "functions.h"
#include "data.h"

/* Field access by byte offset, for classes whose layout is not modelled yet. */
#define U32_AT(p, off) (*(u32 *)((u8 *)(p) + (off)))
#define S32_AT(p, off) (*(s32 *)((u8 *)(p) + (off)))
#define U16_AT(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define S16_AT(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define U8_AT(p, off)  (*(u8 *)((u8 *)(p) + (off)))
#define S8_AT(p, off)  (*(s8 *)((u8 *)(p) + (off)))
#define PTR_AT(p, off) (*(void **)((u8 *)(p) + (off)))

/* Bit-field helpers on u32 words (the original is C++ with bitfields). */
#define BITS(v, lo, n)  (((u32)(v) >> (lo)) & ((1u << (n)) - 1u))
#define SET_BITS(word, lo, n, val) \
    ((word) = ((word) & ~(((1u << (n)) - 1u) << (lo))) | (((u32)(val) & ((1u << (n)) - 1u)) << (lo)))

/* Shifts that are well defined on negative values (two's complement). */
#define SHL(x, n) ((s32)((u32)(x) << (n)))

/* 64-bit values in game structures are only 4-byte aligned: never let the
 * compiler access them as one doubleword. */
static inline void Put64(void *p, u64 v)
{
    ((u32 *)p)[0] = (u32)v;
    ((u32 *)p)[1] = (u32)(v >> 32);
}
static inline u64 Get64(const void *p)
{
    return ((const u32 *)p)[0] | ((u64)((const u32 *)p)[1] << 32);
}

/* fx32 multiply as the game's compiler emits it: 64-bit product, +0x800, >> 12 */
static inline fx32 FxMul(fx32 a, fx32 b)
{
    return (fx32)((u64)((s64)a * b + 0x800) >> 12);
}

/* fx32 radians -> 16-bit angle index (65536 per turn), rounded */
static inline u32 FxRadToIdx(fx32 rad)
{
    u64 prod = (u64)(s64)rad * 0x28BE60DB9391ull;   /* rad * 65536 / (2*pi), Q44 */
    return (((u32)(prod >> 32) + 0x800) << 4) >> 16;
}
#define FX_SIN_RAD(rad) (FX_SinCosTable_[(FxRadToIdx(rad) >> 4) * 2])
#define FX_COS_RAD(rad) (FX_SinCosTable_[(FxRadToIdx(rad) >> 4) * 2 + 1])

/* Call slot N (byte offset 4*N) of a C++ object's vtable. */
#define VCALL(obj, off, type) ((type)(*(void ***)(obj))[(off) / 4])

#endif
