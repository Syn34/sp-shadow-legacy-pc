#ifndef SPYRO_FUNCTIONS_H
#define SPYRO_FUNCTIONS_H

/*
 * Prototypes for game functions called from decompiled code but not (yet)
 * decompiled themselves. Signatures are inferred from call sites; keep exactly
 * one declaration per function here so every caller agrees.
 */
#include "types.h"

struct PtrVec;

/* ---- memory (x_memory.cpp) */
void *func_0207ff70(u32 size, const char *file, s32 line);
void *func_0207ff48(u32 size, const char *file, s32 line);
void func_0207ff14(void *ptr, const char *file, s32 line);

/* ---- SDK */
void MI_CpuCopy8(const void *src, void *dest, u32 size);
void MI_CpuFill8(void *dest, u8 data, u32 size);

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
void func_02003c74(void *actor, s32 x, s32 y);   /* place at an fx32 position */
void func_02003890(void *obj, s32 *pos);
s32 func_0201a0a0(s32 *xz, void *obj);
void func_02028678(s32 value, char *buf);

void func_02002164(void *self);               /* empty destructor */
void func_020022e4(void *self);               /* empty destructor */
void func_02002228(void *actor, void *anim);
void func_020030d0(u8 *actor);                /* advance the sprite animation */
void func_020036a4(void *actor, s32 v);
void func_020036c0(s32 *rect);
void func_020036c4(void *actor, const s32 *rect);
void func_020036e8(s32 *rect, const s32 *a, const s32 *b, const s32 *c, const s32 *d);
void func_02003720(s32 *rect);
void func_02003740(void *actor);              /* release the palette/VRAM slot */
u32 *func_02003738(void *actor);
void func_02003798(s32 *out, const s32 *in, const s32 *scale);
void *func_020037ec(void *actor);             /* pixels of the current frame (locks the resource) */
void func_0200384c(void *actor, u32 *size);   /* size of the current frame */
void func_020038d4(s32 *out, const s32 *in, s32 shift);
void *func_02003958(void *actor);
void func_020039d0(void *actor, s32 v);
void func_020039e8(void *actor, s32 v);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
void func_02003cbc(void *actor, s32 frame);
void func_020041e0(void *dst, const void *src);   /* copy 8 bytes */
void func_020040f4(u8 *dst, const u8 *src);       /* copy camera/scene setup */
void func_02003be8(void *actor);                  /* reset position from spawn point */
void func_020042b0(void *item, struct PtrVec *list, struct PtrVec *other);  /* move between lists */
void func_02003ddc(void *actor, void *anim);
s32 func_02004490(const char *fmt, ...);
void func_0201f6c8(s32 *pos);
void func_0205047c(void *actor);
void func_020291ec(u32 a, u32 b, u32 c);
u16 func_02029284(s32 v, u32 b, u32 c);

void func_0200809c(void *actor);
void func_02008104(u32 shape, u32 size, s32 *dim);   /* OBJ shape/size -> pixels */
void func_02008130(s32 a, s32 b);
void func_0200c110(u8 *zoom, s32 dt);
s32 func_0200be9c(void *obj);
void *func_02016f24(void *obj);
void func_02017180(void *obj);
void func_02018ba4(void);
void func_0201a390(u32 oam);
void func_0201a7c8(u32 oam, u32 *attr);
void func_0201aa88(void *actor);
void func_0201c428(s32 *out);
void func_0201f004(void);
void *func_0201f490(void);                    /* camera */
void func_02007708(void *actor);
void func_02028f04(void);
void func_0202926c(void *actor, void *pixels);
u8 *func_020436f0(u32 id);
void func_020504b8(void *actor);
void func_02050680(void *actor);
s32 func_02052f10(void *model);
u32 func_020822c0(void *timer);

/* ---- per-screen actor vectors (index data_020df0fc) */
struct PtrVec *func_020061a0(void);           /* actor pool (0x114-byte elements) */
struct PtrVec *func_020061c0(void);
struct PtrVec *func_020061e0(void);
struct PtrVec *func_02006200(void);           /* active actors */
struct PtrVec *func_02006220(void);           /* actors to draw */
void *func_020062a0(void);
void *func_020062b0(void);
void *func_020062c0(void);

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
void **func_02006240(struct PtrVec *v, void **pos, u32 tag);   /* erase */

/* ---- timing */
void *func_020062d0(void);
u64 func_020822c8(void *timer);
u64 func_02082290(void *timer, s32 v);

/* ---- fixed-point math (NitroSDK FX/MTX) */
void func_0208ea6c(void *mtx44);              /* MTX_Identity44 */
s32 func_0208eff8(s32 v);                     /* FX_Sqrt */
s32 func_0208f070(s32 num, s32 den);          /* FX_Div */

#endif
