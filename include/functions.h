#ifndef SPYRO_FUNCTIONS_H
#define SPYRO_FUNCTIONS_H

/*
 * Prototypes for game functions called from decompiled code but not (yet)
 * decompiled themselves. Signatures are inferred from call sites; keep exactly
 * one declaration per function here so every caller agrees.
 */
#include "types.h"

/* ---- memory (x_memory.cpp) */
void *func_0207ff70(u32 size, const char *file, s32 line);
void *func_0207ff48(u32 size, const char *file, s32 line);
void func_0207ff14(void *ptr, const char *file, s32 line);

/* ---- graphics / text */
void func_0207fed8(s32 a, void *dest, const void *src, u32 size, s32 e);
void func_020139f8(void *a, void *b, u32 flag);
void func_020134b8(s32 a, s32 b, s32 c, s32 d, u32 *args, s32 f, s32 *g);
u32 func_020138a0(const char *str);
u32 func_02013868(const char *str);
void func_0201391c(s32 a, s32 b, s32 c);
void func_02029228(u32 a, u32 b, u32 c);
void func_0200bd20(s32 a, s32 b, s32 c);

/* ---- actors */
void func_02003e3c(void *obj, s32 v);
void *func_02005cd0(s32 v);
void func_020044a0(void *obj);
void func_0201ac94(void *obj, s32 v);
void func_02003b98(void *obj, s32 x, s32 y);
void func_02003890(void *obj, s32 *pos);
s32 func_0201a0a0(s32 *xz, void *obj);
void func_02028678(s32 value, char *buf);

/* ---- timing */
void *func_020062d0(void);
u64 func_020822c8(void *timer);
u64 func_02082290(void *timer, s32 v);

/* ---- fixed-point math (NitroSDK FX/MTX) */
void func_0208ea6c(void *mtx44);              /* MTX_Identity44 */
s32 func_0208eff8(s32 v);                     /* FX_Sqrt */
s32 func_0208f070(s32 num, s32 den);          /* FX_Div */

#endif
