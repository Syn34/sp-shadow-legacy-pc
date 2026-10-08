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

/* ---- ground-shadow probe offsets (fx32) */
extern const fx32 data_020bc9f0, data_020bc9f8, data_020bc9fc, data_020bca08,
    data_020bca18, data_020bca20, data_020bca24, data_020bca28, data_020bca2c,
    data_020bca30, data_020bca44, data_020bca48, data_020bca50, data_020bca54;

#endif
