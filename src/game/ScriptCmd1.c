/*
 * Camera zoom transition and the first script command handlers.
 * ARM9 main, 0x0200be9c - 0x0200c614 (25 functions).
 *
 * 0x0200be9c-0x0200c268: a zoom/brightness transition object (vtable
 * data_020bcd14) that interpolates camera scale, a BG layer scale and
 * other values over time. 0x0200c28c on: script command handlers that take
 * a command record {.., s8 group @4, u8 index @5, args @6..} and return 1;
 * they are called from the script interpreter at 0x0200c614.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcd14[];           /* zoom transition vtable */
extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcb94[];           /* RTTI: CActor */
extern u8 data_020bcef0[];           /* RTTI */
extern u8 data_020bd638;
extern u8 data_020def74[];

void *func_020062e0(void);
void *func_02009394(void);
void func_02009fec(s32 layer, s32 scale);
void *func_0200917c(s32 group, s32 index);
void func_020091c0(s32 group);
void func_0201237c(void *actor, s32 v);
void func_02018588(void *cmd);
void func_020185d0(void *cmd);
void func_020186ec(void);
void func_020186fc(void);
void func_0201cea4(s32 v);
void func_02042808(u32 v);
void func_0205f4b0(void *snd, u32 id);
void func_02068e24(void *p, u32 v);
void func_02070e84(void *m, u32 v);
void func_02070ec4(void *m);
void func_02070fc8(void *m);
void func_02090ad8(u32 reg, u32 a, u32 b, u32 c, u32 d);
void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */
void func_0207fe44(void *p);

/* this file's own functions, called before their definition */
void func_0200beb4(void *self, s32 *a, s32 *b);
s32 func_0200bec8(void *self, s32 a, s32 b, s32 t);
void func_0200bf10(u8 *z, s32 dt);
void func_0200c1a8(u8 *z);

/* virtual call with the function pointer in r1, as the original leaves it */
static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

static inline void SetCameraScale(s32 s)
{
    u8 *cam = func_020062e0();
    S32_AT(cam, 0x370) = s;
    S32_AT(cam, 0x374) = s;
    S32_AT(cam, 0x388) = s;
    S32_AT(cam, 0x39c) = s;
}

/* 0x0200be9c
 * @difftest ptr:0x24:4 */
s32 func_0200be9c(void *z) { return S32_AT(z, 0x20); }

/* 0x0200bea4
 * @difftest ptr:0x58:4 */
s32 func_0200bea4(void *z) { return S32_AT(z, 0x54); }

/* 0x0200beac
 * @difftest ptr:0x58:4 */
s32 func_0200beac(void *z) { return S32_AT(z, 0x50); }

/* 0x0200beb4: std::swap
 * @difftest u32 ptr:4:4 ptr:4:4
 * @difftest $V=ptr:4:4 u32 $V $V */
void func_0200beb4(void *self, s32 *a, s32 *b)
{
    s32 t = *a;
    *a = *b;
    *b = t;
}

/* 0x0200bec8: linear interpolation a..b at t (fx32)
 * @difftest u32 s32 s32 int:0:0x1001 */
s32 func_0200bec8(void *self, s32 a, s32 b, s32 t)
{
    return FxMul(a, 0x1000 - t) + FxMul(b, t);
}

/* 0x0200bf10: advance the transition by dt
 * @difftest $Z=ptr:0x58:4 @$Z+0x28:32=int:0:0x1000 @$Z+0x2c:32=int:0:0x2000 @$Z+0x14:32=int:0:0x1000 $Z int:0:0x400 */
void func_0200bf10(u8 *z, s32 dt)
{
    s32 t, v;
    S32_AT(z, 0x28) += dt;
    t = FxMul(S32_AT(z, 0x28), S32_AT(z, 0x2c));
    S32_AT(z, 0x20) = func_0200bec8(z, S32_AT(z, 0xc), S32_AT(z, 0x10), t);
    S32_AT(z, 0x24) = func_0200bec8(z, S32_AT(z, 0x18), S32_AT(z, 0x1c), t);
    if (S32_AT(z, 0x24) != S32_AT(z, 0x18))
        func_0201cea4(S32_AT(z, 0x24));
    S32_AT(z, 0x50) = func_0200bec8(z, S32_AT(z, 0x40), S32_AT(z, 0x44), t);
    S32_AT(z, 0x54) = func_0200bec8(z, S32_AT(z, 0x48), S32_AT(z, 0x4c), t);
    v = func_0200bec8(z, S32_AT(z, 0x38), S32_AT(z, 0x3c), t);
    func_02009fec(2, (s16)((u32)SHL(v, 4) >> 16));
    SetCameraScale(S32_AT(z, 0x20));
    if (S32_AT(z, 0x28) < S32_AT(z, 0x14))
        return;
    S32_AT(z, 0x20) = S32_AT(z, 0x10);
    SetCameraScale(S32_AT(z, 0x10));
    func_0201cea4(S32_AT(z, 0x1c));
    func_02009fec(2, (s16)((u32)SHL(S32_AT(z, 0x3c), 4) >> 16));
    U8_AT(z, 6) = 1;
    S32_AT(z, 0x28) = 0;
    if (S32_AT(z, 0x1c) > S32_AT(z, 0x18))
        func_02090ad8(0x04000050, 1, 4, 0x1f, 0);
    S32_AT(z, 0x50) = S32_AT(z, 0x44);
    S32_AT(z, 0x54) = S32_AT(z, 0x4c);
    func_0200beb4(z, (s32 *)(z + 0xc), (s32 *)(z + 0x10));
    func_0200beb4(z, (s32 *)(z + 0x18), (s32 *)(z + 0x1c));
    func_0200beb4(z, (s32 *)(z + 0x30), (s32 *)(z + 0x34));
    func_0200beb4(z, (s32 *)(z + 0x38), (s32 *)(z + 0x3c));
    func_0200beb4(z, (s32 *)(z + 0x40), (s32 *)(z + 0x44));
    func_0200beb4(z, (s32 *)(z + 0x48), (s32 *)(z + 0x4c));
}

/* 0x0200c110: per-frame update of the transition */
void func_0200c110(u8 *z, s32 dt)
{
    if (U8_AT(z, 6) != 0) {
        U8_AT(z, 6) = 0;
        U8_AT(z, 5) = 0;
    }
    if (U8_AT(z, 5) == 0)
        return;
    func_0200bf10(z, dt);
}

/* 0x0200c150: reset the transition and apply its start values */
void func_0200c150(u8 *z)
{
    func_0200c1a8(z);
    SetCameraScale(S32_AT(z, 0xc));
    func_0201cea4(S32_AT(z, 0x18));
    func_02009fec(2, (s16)((u32)SHL(S32_AT(z, 0x38), 4) >> 16));
}

/* 0x0200c1a8: default transition values
 * @difftest ptr:0x58:4 */
void func_0200c1a8(u8 *z)
{
    U8_AT(z, 4) = 0;
    U8_AT(z, 5) = 0;
    S32_AT(z, 0x8) = 4;
    S32_AT(z, 0x14) = 0x666;
    S32_AT(z, 0x28) = 0;
    S32_AT(z, 0xc) = 0x1000;
    S32_AT(z, 0x20) = 0x1000;
    S32_AT(z, 0x10) = 0x1548;
    S32_AT(z, 0x18) = 0x1000;
    S32_AT(z, 0x1c) = 0x99a;
    U8_AT(z, 6) = 0;
    S32_AT(z, 0x30) = 0x1000;
    S32_AT(z, 0x34) = 0x1548;
    S32_AT(z, 0x38) = 0x100000;
    S32_AT(z, 0x3c) = 0xc0000;
    S32_AT(z, 0x40) = 0;
    S32_AT(z, 0x44) = 0x20000;
    S32_AT(z, 0x50) = 0;
    S32_AT(z, 0x48) = 0;
    S32_AT(z, 0x4c) = 0x17000;
    S32_AT(z, 0x54) = 0;
}

/* 0x0200c234: deleting destructor
 * @difftest zero:0x58 cases=2 */
void *func_0200c234(void *z)
{
    PTR_AT(z, 0) = data_020bcd14;
    func_0207fe44(z);
    return z;
}

/* 0x0200c258: destructor
 * @difftest ptr:4:4 */
void *func_0200c258(void *z)
{
    PTR_AT(z, 0) = data_020bcd14;
    return z;
}

/* 0x0200c268: constructor
 * @difftest ptr:0x58:4 */
void *func_0200c268(u8 *z)
{
    PTR_AT(z, 0) = data_020bcd14;
    func_0200c1a8(z);
    return z;
}

/* 0x0200c28c: script: load an object group
 * @difftest $C=ptr:8 @$C+4:8=int:0:3 @$C+5:8=pick:0,1 $C cases=20 */
s32 func_0200c28c(u8 *cmd)
{
    if (cmd[5] != 0)
        func_020091c0((s8)cmd[4]);
    else
        func_02042808(cmd[4]);
    func_020186ec();
    func_020186fc();
    return 1;
}

/* 0x0200c2cc: script: set a global byte
 * @difftest ptr:8 */
s32 func_0200c2cc(u8 *cmd)
{
    data_020bd638 = cmd[4];
    return 1;
}

/* 0x0200c2e4
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200c2e4(u8 *cmd)
{
    u8 *a = func_0200917c((s8)cmd[4], cmd[5]);
    if (a != NULL) {
        func_02070ec4(VCallR1(a, 0x38));
        if (S32_AT(a, 0xc) == 0x11) {
            if (a != NULL)
                a = func_020a6efc(a, data_020bcb88, data_020bcef0, -1);
            U8_AT(a, 0x7c) = 0;
        }
    }
    return 1;
}

/* 0x0200c358
 * @difftest ptr:0x20 cases=60 */
s32 func_0200c358(void *cmd)
{
    func_02018588(cmd);
    return 1;
}

/* 0x0200c374
 * @difftest ptr:0x20 cases=60 */
s32 func_0200c374(void *cmd)
{
    func_020185d0(cmd);
    return 1;
}

/* 0x0200c390: script: add to a counter
 * @difftest ptr:8 */
s32 func_0200c390(u8 *cmd)
{
    data_020def74[0x12e] += S16_AT(cmd, 4);
    return 1;
}

/* 0x0200c3b0: script: play one of two sounds depending on the counter
 * @difftest ptr:8 cases=60 */
s32 func_0200c3b0(u8 *cmd)
{
    if (data_020def74[0x12e] >= S16_AT(cmd, 4))
        func_0205f4b0(func_02009394(), cmd[6]);
    else
        func_0205f4b0(func_02009394(), cmd[7]);
    return 1;
}

/* 0x0200c3f8
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200c3f8(u8 *cmd)
{
    u8 *a = func_0200917c((s8)cmd[4], cmd[5]);
    if (a != NULL) {
        if (a != NULL)
            a = func_020a6efc(a, data_020bcb88, data_020bcb94, -1);
        if (a != NULL && VCallR1(a, 0x38) != NULL) {
            S32_AT(a, 0xfc) = cmd[6];
            func_02070e84(VCallR1(a, 0x38), cmd[6]);
        }
    }
    return 1;
}

/* 0x0200c48c
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200c48c(u8 *cmd)
{
    u8 *a = func_0200917c((s8)cmd[4], cmd[5]);
    if (a != NULL) {
        u32 v = cmd[6];
        func_02068e24((u8 *)VCallR1(a, 0x38) + 0x9c, v);
    }
    return 1;
}

/* 0x0200c4d0
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200c4d0(u8 *cmd)
{
    u8 *a = func_0200917c((s8)cmd[4], cmd[5]);
    if (a != NULL)
        func_02070fc8(VCallR1(a, 0x38));
    if (a != NULL)
        a = func_020a6efc(a, data_020bcb88, data_020bcb94, -1);
    if (a != NULL)
        U8_AT(a, 0x10c) = 0;
    return 1;
}

/* 0x0200c53c
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200c53c(u8 *cmd)
{
    u8 *a = func_0200917c((s8)cmd[4], cmd[5]);
    if (a != NULL)
        func_0201237c(a, (s8)(cmd[6] != 0));
    return 1;
}

/* 0x0200c580: script: set or clear an object's +0x38 bit 18
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200c580(u8 *cmd)
{
    u8 *a = func_0200917c((s8)cmd[4], cmd[5]);
    if (a != NULL)
        SET_BITS(U32_AT(a, 0x38), 18, 1, (s8)(cmd[6] != 0));
    return 1;
}

/* 0x0200c5d8: script: call an object's virtual slot 6
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200c5d8(u8 *cmd)
{
    u8 *a = func_0200917c((s8)cmd[4], cmd[5]);
    if (a != NULL)
        VCallR1(a, 0x18);
    return 1;
}
