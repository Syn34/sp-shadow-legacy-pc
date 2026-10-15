/*
 * The particle emitter presets (see ParticleBase.c for the fields): 14
 * kinds of emitters, each with its velocity, scale range, spread,
 * texture, life, size, spin, spawn rate and alpha.
 * ARM9 main, 0x02057464 - 0x02058900 (1 function).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020b6c90,
    data_020b6cd4,
    data_020b6cdc,
    data_020c0a7c,
    data_020c0a80,
    data_020c0a84,
    data_020c0a88,
    data_020c0a8c,
    data_020c0a90,
    data_020c0a94,
    data_020c0a98,
    data_020c0a9c,
    data_020c0aa0,
    data_020c0aa4,
    data_020c0aa8,
    data_020c0aac,
    data_020c0ab0,
    data_020c0ab4,
    data_020c0ab8,
    data_020c0abc,
    data_020c0ac0,
    data_020c0ac4,
    data_020c0ac8,
    data_020c0acc,
    data_020c0ad0,
    data_020c0ad4,
    data_020c0ad8,
    data_020c0adc,
    data_020c0ae0,
    data_020c0ae4,
    data_020c0ae8,
    data_020c0aec,
    data_020c0af0,
    data_020c0af4,
    data_020c0af8,
    data_020c0afc,
    data_020c0b00,
    data_020c0b04,
    data_020c0b08,
    data_020c0b0c,
    data_020c0b10,
    data_020c0b14,
    data_020c0b18,
    data_020c0b1c,
    data_020c0b20,
    data_020c0b24,
    data_020c0b28,
    data_020c0b2c,
    data_020c0b30,
    data_020c0b34,
    data_020c0b38,
    data_020c0b3c,
    data_020c0b40,
    data_020c0b44,
    data_020c0b48,
    data_020c0b4c,
    data_020c0b50,
    data_020c0b54,
    data_020c0b58,
    data_020c0b5c,
    data_020c0b60,
    data_020c0b64,
    data_020c0b68,
    data_020c0b6c,
    data_020c0b70,
    data_020c0b74,
    data_020c0b78,
    data_020c0b7c,
    data_020c0b80,
    data_020c0b84,
    data_020c0b88,
    data_020c0b8c,
    data_020c0b90,
    data_020c0b94,
    data_020c0b98,
    data_020c0b9c,
    data_020c0ba0,
    data_020c0ba4,
    data_020c0ba8,
    data_020c0bac,
    data_020c0bb0,
    data_020c0bb4,
    data_020c0bb8,
    data_020c0bbc,
    data_020c0bc0,
    data_020c0bc4,
    data_020c0bc8,
    data_020c0bcc,
    data_020c0bd0,
    data_020c0bd4,
    data_020c0bd8,
    data_020c0bdc,
    data_020c0be0,
    data_020c0be4,
    data_020c0be8,
    data_020c0bec,
    data_020c0bf0,
    data_020c0bf4,
    data_020c0bf8,
    data_020c0bfc,
    data_020c0c00,
    data_020c0c04,
    data_020c0c08,
    data_020c0c0c,
    data_020c0c10,
    data_020c0c14,
    data_020c0c18;
extern const fx32 g_FxPi, g_FxPi_2;

s32 func_0208f070(s32 num, s32 den);
void func_020573c4(u8 *self, u32 id);
void func_0205bb20(u8 *p, u32 a, s32 life);

#define F(self, off) S32_AT(self, 0x3000 + (off))

/* fill in the fields every preset sets */
static void Preset(u8 *self, s32 ox, s32 oy, s32 oz, s32 life, s32 size, s32 spin,
                   const fx32 *v0, const fx32 *v1, const fx32 *v2, const fx32 *spread,
                   const fx32 *s0, const fx32 *s1, const fx32 *s2, const fx32 *s3,
                   u32 mode, u32 rate, s32 alpha)
{
    u32 n;
    s32 r;

    F(self, 0xbf8) = ox;
    F(self, 0xbfc) = oy;
    F(self, 0xc00) = oz;
    F(self, 0xc14) = life;
    F(self, 0xc18) = size;
    F(self, 0xc1c) = spin;
    F(self, 0xc20) = *v0;
    F(self, 0xc24) = *v1;
    F(self, 0xc28) = *v2;
    F(self, 0xc2c) = *spread;
    F(self, 0xc30) = *s0;
    F(self, 0xc34) = *s1;
    F(self, 0xc38) = *s2;
    F(self, 0xc3c) = *s3;
    F(self, 0xc44) = mode;
    F(self, 0xc0c) = rate;
    n = F(self, 0xc0c);
    r = (s32)(n != 0 ? 0.5f + (float)(n << 12) : (float)(n << 12) - 0.5f);
    F(self, 0xc08) = func_0208f070(0x1000, r);
    F(self, 0xc40) = alpha;
}

/* set every particle up for a preset */
static void InitParticles(u8 *self, u32 a)
{
    u32 i;

    for (i = 0; i < 0x3c; i++)
        func_0205bb20(self + 0xb4 + i * 0xfc, a, F(self, 0xc14));
}

/* 0x02057464: use preset `kind`
 * @difftest zero:0x3d00 int:0:16 stub=0x020573c4,0x0205bb20 cases=48 */
void func_02057464(u8 *self, u32 kind)
{
    F(self, 0xc64) = kind;
    switch ((u32)F(self, 0xc64)) {
    case 0:
        Preset(self, 0x0, 0x0, 0x0, 0xb33, 0x1000, 0x0,
               &data_020c0c18, &data_020c0c14, &data_020c0c10,
               &g_FxZero, &data_020c0c0c, &data_020c0c08, &data_020c0c04, &data_020c0c00,
               0, 0x1e, 0x1f);
        break;
    case 1:
        Preset(self, 0x0, 0x0, 0x6000, 0xb33, 0x2000, 0x32000,
               &data_020c0bfc, &data_020c0bf8, &data_020c0bf4,
               &data_020b6cdc, &data_020c0bf0, &data_020c0bec, &data_020c0be8, &data_020c0be4,
               0, 0x1e, 0x18);
        F(self, 0xbcc) = 0xc00;
        InitParticles(self, 0x9a);
        break;
    case 2:
        Preset(self, 0x0, 0x0, 0x6000, 0xb33, 0x2000, 0x32000,
               &data_020c0ba8, &data_020c0ba4, &data_020c0ba0,
               &data_020b6cdc, &data_020c0b9c, &data_020c0b98, &data_020c0b94, &data_020c0b90,
               0, 0x1e, 0x18);
        InitParticles(self, 0x9c);
        break;
    case 3:
        func_020573c4(self, 0x924);
        Preset(self, 0x0, 0x0, 0x0, 0x800, 0x8000, 0x14000,
               &data_020c0b70, &data_020c0ad8, &data_020c0b68,
               &data_020b6c90, &data_020c0b64, &data_020c0a80, &data_020c0b5c, &data_020c0b58,
               1, 0xa, 0xf);
        break;
    case 4:
        func_020573c4(self, 0x947);
        Preset(self, 0x0, 0x0, 0x0, 0x333, 0x2000, 0x1000,
               &data_020c0b54, &data_020c0b50, &data_020c0b4c,
               &data_020b6c90, &data_020c0b48, &data_020c0b44, &data_020c0b40, &data_020c0b3c,
               0, 0x1, 0x1e);
        break;
    case 5:
        func_020573c4(self, 0x26d);
        Preset(self, 0x0, 0x0, 0x0, 0x800, 0xa000, 0x1000,
               &data_020c0b38, &data_020c0b34, &data_020c0b30,
               &data_020b6c90, &data_020c0b2c, &data_020c0b28, &data_020c0b24, &data_020c0b20,
               1, 0x1, 0x1e);
        break;
    case 6:
        {
            s32 y = S32_AT(self, 0x14), z = S32_AT(self, 0x18);

            S32_AT(self, 0x10) = -g_FxPi_2;
            S32_AT(self, 0x14) = y;
            S32_AT(self, 0x18) = z;
            self[0x8c] = 1;
        }
        Preset(self, 0x0, 0x0, 0x6000, 0xb33, 0x2000, 0x32000,
               &data_020c0bc4, &data_020c0bc0, &data_020c0bbc,
               &data_020b6cd4, &data_020c0bb8, &data_020c0bb4, &data_020c0bb0, &data_020c0bac,
               1, 0x1e, 0x18);
        InitParticles(self, 0x9a);
        break;
    case 7:
        func_020573c4(self, 0x45d);
        F(self, 0xbcc) = 0x1000;
        Preset(self, 0x0, 0x0, 0x0, 0x2000, 0x2000, 0x1000,
               &data_020c0b1c, &data_020c0b18, &data_020c0b14,
               &data_020b6c90, &data_020c0b10, &data_020c0b0c, &data_020c0b08, &data_020c0b04,
               1, 0x2, 0x1e);
        break;
    case 8:
        func_020573c4(self, 0x5d5);
        F(self, 0xbcc) = 0x1000;
        Preset(self, 0x0, 0x19000, 0x19000, 0x800, 0x2000, 0x1000,
               &data_020c0b00, &data_020c0afc, &data_020c0af8,
               &g_FxZero, &data_020c0af4, &data_020c0af0, &data_020c0aec, &data_020c0ae8,
               1, 0x1, 0x20);
        F(self, 0xc10) = 1;
        break;
    case 9:
        func_020573c4(self, 0x5d5);
        F(self, 0xbcc) = 0x1000;
        Preset(self, 0x6000, 0x19000, 0x19000, 0x800, 0x1000, 0x32000,
               &data_020c0ae4, &data_020c0b6c, &data_020c0adc,
               &g_FxPi, &data_020c0a7c, &data_020c0ad4, &data_020c0ad0, &data_020c0acc,
               1, 0xc, 0x20);
        F(self, 0xc10) = 6;
        self[0x3c60] = 1;
        break;
    case 10:
        Preset(self, 0x0, 0x5000, 0x6000, 0xb33, 0x1000, 0xb4000,
               &data_020c0be0, &data_020c0bdc, &data_020c0bd8,
               &data_020b6cdc, &data_020c0bd4, &data_020c0bd0, &data_020c0bcc, &data_020c0bc8,
               0, 0x1e, 0x18);
        InitParticles(self, 0x9a);
        break;
    case 11:
        Preset(self, 0x0, 0x5000, 0x6000, 0xb33, 0x1000, 0xb4000,
               &data_020c0b8c, &data_020c0b88, &data_020c0b84,
               &data_020b6cdc, &data_020c0b80, &data_020c0b7c, &data_020c0b78, &data_020c0b74,
               0, 0x1e, 0x18);
        InitParticles(self, 0x9c);
        break;
    case 12:
        func_020573c4(self, 0x47a);
        Preset(self, 0x0, 0x0, 0x0, 0xa000, 0x1000, 0x5000,
               &data_020c0ac8, &data_020c0ac4, &data_020c0ac0,
               &g_FxZero, &data_020c0abc, &data_020c0ab8, &data_020c0ab4, &data_020c0ab0,
               1, 0xc, 0x20);
        F(self, 0xbcc) = 0x1000;
        F(self, 0xc48) = 1;
        F(self, 0xc4c) = data_020c0aac;
        F(self, 0xc50) = data_020c0aa8;
        F(self, 0xc54) = data_020c0aa4;
        break;
    case 13:
        func_020573c4(self, 0x8cf);
        Preset(self, 0x0, 0x64000, 0x0, 0xa000, 0x1000, 0x5000,
               &data_020c0aa0, &data_020c0a9c, &data_020c0ae0,
               &g_FxZero, &data_020c0b60, &data_020c0a98, &data_020c0a94, &data_020c0a90,
               1, 0xc, 0x20);
        F(self, 0xbcc) = 0x1000;
        F(self, 0xc48) = 1;
        F(self, 0xc4c) = data_020c0a8c;
        F(self, 0xc50) = data_020c0a88;
        F(self, 0xc54) = data_020c0a84;
        break;
    }
}
