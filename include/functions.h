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

void func_02002228(void *actor, void *anim);
void func_020036a4(void *actor, s32 v);
void func_020036c0(s32 *rect);
void func_020036c4(void *actor, const s32 *rect);
void func_020036e8(s32 *rect, const s32 *a, const s32 *b, const s32 *c, const s32 *d);
void func_02003720(s32 *rect);
u32 *func_02003738(void *actor);
void func_02003798(s32 *out, const s32 *in, const s32 *scale);
void func_020038d4(s32 *out, const s32 *in, s32 shift);
void *func_02003958(void *actor);
void func_020039d0(void *actor, s32 v);
void func_020039e8(void *actor, s32 v);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
void func_02003cbc(void *actor, s32 frame);
void func_02003ddc(void *actor, void *anim);
s32 func_02004490(const char *fmt, ...);
void func_0201f6c8(s32 *pos);
void func_0205047c(void *actor);
void func_020291ec(u32 a, u32 b, u32 c);
u16 func_02029284(s32 v, u32 b, u32 c);

/* ---- resources / files */
void *func_02012a64(s32 handle);              /* lock resource, return data */
void func_020129ac(s32 handle);               /* unlock resource */
void func_0207fe28(const char *file, s32 line);
void func_0207fe24(void);
void func_02080488(void *file, u32 id);       /* open */
u32 func_02080388(void *file);                /* size */
void func_02080418(void *dest, u32 size, s32 n, void *file);  /* read */
void func_02080458(void *file);               /* close */

/* ---- containers */
void func_020062f0(void *vec, void **value, u32 tag);   /* push_back (grow) */
void func_02006240(void *vec, void **pos, u32 tag);     /* erase */

/* ---- timing */
void *func_020062d0(void);
u64 func_020822c8(void *timer);
u64 func_02082290(void *timer, s32 v);

/* ---- fixed-point math (NitroSDK FX/MTX) */
void func_0208ea6c(void *mtx44);              /* MTX_Identity44 */
s32 func_0208eff8(s32 v);                     /* FX_Sqrt */
s32 func_0208f070(s32 num, s32 den);          /* FX_Div */

#endif
