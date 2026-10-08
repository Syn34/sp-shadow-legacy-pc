#ifndef SPYRO_DATA_H
#define SPYRO_DATA_H

/* Game data referenced from decompiled code (names from config/arm9/symbols.txt). */
#include "types.h"

/* ---- fx32 angle constants (radians, 20.12) */
extern const fx32 g_FxZero;      /* 0      */
extern const fx32 g_FxPi;        /* pi     */
extern const fx32 g_FxTwoPi;     /* 2 pi   */
extern const fx32 g_FxPi_4;      /* pi/4   */
extern const fx32 g_FxPi_2;      /* pi/2   */
extern const fx32 g_Fx3Pi_4;     /* 3pi/4  */
extern const fx32 g_Fx5Pi_4;     /* 5pi/4  */
extern const fx32 g_Fx3Pi_2;     /* 3pi/2  */
extern const fx32 g_Fx7Pi_4;     /* 7pi/4  */
extern const s16 FX_SinCosTable_[4096 * 2];

/* ---- TextActor.cpp */
extern const char data_020bc9e0[];   /* "TextActor.cpp" */
extern const u16 data_020bc9d8;      /* max text width (240) */
extern const u16 data_020bc9dc;      /* max text height (16) */
extern u32 data_020df0fc;
extern char data_020bca68[];         /* number text buffer */
extern const char data_020bca74[];
extern u32 data_020deebc[];

/* ---- Actor.cpp */
extern const char data_020bca78[];   /* "Actor.cpp" */
extern const char data_020bca84[];   /* "actorKillAll::pActor->killFunction()\n" */
extern const char data_020bcaac[];   /* actorKillAll debug message */
extern const char data_020bcad0[];   /* actorKillAll debug message */
extern u8 *data_020c3b64;            /* table of 0x2c-byte records */
extern u8 *data_020c3b68;            /* table of 12-byte {?, first, last} index ranges */

/* std::vector<Actor *>-like container: {data, size, capacity} */
typedef struct PtrVec {
    void **items;
    s32 count;
    s32 cap;
} PtrVec;
extern PtrVec data_020d9ea0[];       /* active actors, per screen */
extern PtrVec data_020d9f18[];       /* free actors, per screen */
extern PtrVec data_020d9f30[];       /* actors drawn as sprites this frame, per screen */
extern u8 *data_020c3b60;            /* table of 0x60-byte records */
extern u8 *data_020df0b4;
extern s8 data_020df17c;
extern u8 data_020df1bc[];
extern u8 data_020df598[];           /* per screen 0x13ec bytes; +0x13e9: OAM manager id */
extern u8 data_020e1f50[];
extern u32 data_020bf6a0;

/* ---- ground-shadow probe offsets (fx32) */
extern const fx32 data_020bc9f0, data_020bc9f8, data_020bc9fc, data_020bca08,
    data_020bca18, data_020bca20, data_020bca24, data_020bca28, data_020bca2c,
    data_020bca30, data_020bca44, data_020bca48, data_020bca50, data_020bca54;

#endif
