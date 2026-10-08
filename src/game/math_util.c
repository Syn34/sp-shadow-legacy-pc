/*
 * Game math helpers and small accessors.
 * ARM9 main, 0x02070810 - 0x02070b08 (23 functions).
 *
 * The fixed-point routines are the game's copies of NitroSDK FX helpers
 * (fx32 = 20.12 fixed point, angles in fx32 radians or 16-bit table indices).
 * The accessors belong to a game class whose name is not known yet; field
 * offsets are documented on each function.
 *
 * Names other than SDK ones are descriptive and were chosen for this project.
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "types.h"

extern const s16 FX_SinCosTable_[4096 * 2];   /* {sin, cos} pairs, 4.12 fixed */
extern const fx32 g_FxZero;                   /* 0          */
extern const fx32 g_FxTwoPi;                  /* 2*pi = 25736 */
extern const fx32 g_FxPi;                     /* pi   = 12868 */

extern void MI_Copy64B(const void *src, void *dest);
extern int rand(void);
extern void *func_020401d4(void *outIter, void *map, const void *key);

/* fx32 radians -> 16-bit angle index (65536 per turn), rounded */
static inline u32 RadToIdx(fx32 rad)
{
    u64 prod = (u64)(s64)rad * 0x28BE60DB9391ull;   /* rad * 65536 / (2*pi), Q44 */
    return (((u32)(prod >> 32) + 0x800) << 4) >> 16;
}

/* @difftest ptr:256:4 u32 */
void func_02070810(u8 *obj, s32 value) { *(s32 *)(obj + 0x98) = value; }

/* @difftest ptr:256:4 u32 */
void func_02070818(u8 *obj, s32 value) { *(s32 *)(obj + 0x94) = value; }

/* @difftest ptr:256:4 u32 */
void func_02070820(u8 *obj, s32 value) { *(s32 *)(obj + 0x90) = value; }

/* 0x02070828: cosine of an fx32 angle in radians
 * @difftest s32
 * @difftest int:-30000:30000 */
fx32 Math_CosRad(fx32 rad)
{
    return FX_SinCosTable_[(RadToIdx(rad) >> 4) * 2 + 1];
}

/* 0x0207087c: sine of an fx32 angle in radians
 * @difftest s32
 * @difftest int:-30000:30000 */
fx32 Math_SinRad(fx32 rad)
{
    return FX_SinCosTable_[(RadToIdx(rad) >> 4) * 2];
}

/* 0x020708c8: cosine of a 16-bit angle index
 * @difftest int:0:65536 */
fx32 Math_CosIdx(s32 idx)
{
    return FX_SinCosTable_[(idx >> 4) * 2 + 1];
}

/* 0x020708e8: sine of a 16-bit angle index
 * @difftest int:0:65536 */
fx32 Math_SinIdx(s32 idx)
{
    return FX_SinCosTable_[(idx >> 4) * 2];
}

/* 0x02070900: list head initialiser, *head = &node->next (node + 4)
 * @difftest ptr:8:4 u32 */
void func_02070900(void **head, u8 *node) { *head = node + 4; }

/* 0x0207090c: iterator copy
 * @difftest ptr:8:4 ptr:8:4 */
void func_0207090c(u32 *dest, const u32 *src) { *dest = *src; }

/* 0x02070918: std::map<...>::find wrapper (iterator returned through outIter)
 * @difftest ptr:8:4 zero:32 ptr:8:4 */
void *func_02070918(void *outIter, void *map, const void *key)
{
    return func_020401d4(outIter, map, key);
}

/* @difftest ptr:16:4 u32 */
void func_02070924(u8 *obj, s32 value) { *(s32 *)(obj + 0x8) = value; }

/* 0x0207092c: wrap an angle into [-pi, pi]
 * @difftest s32 cases=300
 * @difftest int:-200000:200000 */
fx32 Math_WrapAngleSigned(fx32 angle)
{
    fx32 hi = g_FxPi;
    fx32 lo = -hi;

    if (angle < lo) {
        fx32 step = g_FxTwoPi;
        do {
            angle += step;
        } while (angle < lo);
    }
    if (angle <= hi)
        return angle;
    {
        fx32 step = g_FxTwoPi;
        do {
            angle -= step;
        } while (angle > hi);
    }
    return angle;
}

/* 0x0207097c: wrap an angle into [0, 2*pi]
 * @difftest s32 cases=300
 * @difftest int:-200000:200000 */
fx32 Math_WrapAngle(fx32 angle)
{
    fx32 lo = g_FxZero;
    fx32 turn;

    if (angle < lo) {
        fx32 step = g_FxTwoPi;
        do {
            angle += step;
        } while (angle < lo);
    }
    turn = g_FxTwoPi;
    if (angle <= turn)
        return angle;
    do {
        angle -= turn;
    } while (angle > turn);
    return angle;
}

/* 0x020709c8: &array[index] for an object whose first field is a u32 array
 * @difftest ptr:8:4 int:-1000:1000 */
u32 *func_020709c8(u32 **obj, s32 index) { return *obj + index; }

/* 0x020709d4: set a vector at +0x10 and raise the "dirty" flag at +0x8c
 * @difftest ptr:256:4 ptr:16:4 */
void func_020709d4(u8 *obj, const s32 *vec)
{
    *(s32 *)(obj + 0x10) = vec[0];
    *(s32 *)(obj + 0x14) = vec[1];
    *(s32 *)(obj + 0x18) = vec[2];
    obj[0x8c] = 1;
}

/* 0x020709f8: 4x4 matrix assignment, returns dest
 * @difftest ptr:64:4 ptr:64:4 */
void *func_020709f8(void *dest, const void *src)
{
    MI_Copy64B(src, dest);
    return dest;
}

/* 0x02070a18: fixed-point multiply with rounding (FX_Mul)
 * @difftest s32 s32 */
fx32 Math_Mul(fx32 a, fx32 b)
{
    return (fx32)((u64)((s64)a * b + 0x800) >> 12);
}

/* 0x02070a34: 4x4 matrix copy
 * @difftest ptr:64:4 ptr:64:4 */
void func_02070a34(const void *src, void *dest) { MI_Copy64B(src, dest); }

/* 0x02070a40: set the vector at +0x90
 * @difftest ptr:256:4 ptr:16:4 */
void func_02070a40(u8 *obj, const s32 *vec)
{
    *(s32 *)(obj + 0x90) = vec[0];
    *(s32 *)(obj + 0x94) = vec[1];
    *(s32 *)(obj + 0x98) = vec[2];
}

/* @difftest ptr:256:4 u32 u32 */
void func_02070a5c(u8 *obj, s32 a, s32 b)
{
    *(s32 *)(obj + 0x9c) = a;
    *(s32 *)(obj + 0xa0) = b;
}

/* 0x02070a68: dot product of two fx32 3-vectors (each term rounded)
 * @difftest ptr:16:4 ptr:16:4 */
fx32 Math_Dot(const fx32 *a, const fx32 *b)
{
    return Math_Mul(a[0], b[0]) + Math_Mul(a[1], b[1]) + Math_Mul(a[2], b[2]);
}

/* @difftest ptr:16:4 u32 */
void func_02070adc(u8 *obj, s32 value) { *(s32 *)(obj + 0x4) = value; }

/* 0x02070ae4: 15-bit random number
 * @difftest cases=200 */
s32 Math_Rand15(void)
{
    return rand() & 0x7fff;
}
