/*
 * Script command handlers, part 6.
 * ARM9 main, 0x0200f244 - 0x0200fb74 (28 functions).
 *
 * Variable copies and comparisons, actor placement and path following,
 * camera/zoom waits, lighting presets, a second countdown timer and sound
 * wrappers. Command layout as in ScriptCmd3.c.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcb94[];           /* RTTI: CActor */
extern u8 data_020bcef0[];           /* RTTI */
extern const char data_020bcec0[];
extern u8 data_020d9f9c[];
extern u8 data_020da9ac;
extern s8 data_020da9b0;
extern s8 data_020da9f8;
extern s8 data_020da9fc;
extern u8 data_020daa00;
extern u32 data_020daa04;
extern u8 data_020daa0c[];
extern u8 data_020daa24[];           /* countdown timer object */

void *func_02009394(void);
u32 func_02008930(s32 i);
void *func_0200917c(s32 group, s32 index);
void func_020091c0(s32 group);
void func_0200ebcc(u32 v, u32 op, u32 ref, u32 snd_true, u32 snd_false, u32 grp_true, u32 grp_false);
fx32 func_02001ef4(u32 dir);
void func_02019640(u32 v);
void func_02019660(u32 v);
void func_02019b08(u32 a, u32 b);
void func_02019b7c(u32 v);
u32 func_02019b98(u32 v);
void func_0201b770(u32 a, u32 b, u32 c);
void func_0201f524(void *actor);
void func_0201f564(void *actor);
void func_0201f770(s32 v);
void func_0201f780(s32 v);
s32 func_0201f96c(void);
u32 func_020186ec(void);
void func_020201ec(void *cmd);
void func_0202023c(void *cmd);
void func_0202028c(u32 v);
void func_020202f0(u32 v);
void func_020203d0(void *cmd);
void func_02020440(u32 v);
void func_020204d8(u32 v);
void func_02020504(u32 v);
void func_02049c14(void *timer, u32 now);
void *func_02049c74(void *timer);
void func_02049c98(void *timer, u32 frames);
void *func_02049ca0(void *timer);
void func_0205f4b0(void *snd, u32 id);
void func_0207100c(void *m, u32 path, u32 v, u32 a, u32 b, u32 c, u32 last, u32 time);
void func_020a6cf0(void *obj, void *(*dtor)(void *), void *link);
void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */

#define LOOKUP(cmd) ((u8 *)func_0200917c((s8)(cmd)[4], (cmd)[5]))
#define AS_ACTOR(a) ((a) != NULL ? (u8 *)func_020a6efc(a, data_020bcb88, data_020bcb94, -1) : (a))
#define VAR(i) data_020d9f9c[i]

static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

static inline void Sound(u32 id) { func_0205f4b0(func_02009394(), id); }

static s32 RoundShl(s32 v, s32 sh)
{
    float f = (float)SHL(v, sh);
    return (s32)(v > 0 ? 0.5f + f : f - 0.5f);
}

static inline void LightPreset(void)
{
    u32 k;
    func_0201b770(2, 0x3f, 1);
    func_02019b08(0x1e, 0);
    k = func_02019b98(1);
    func_02019b7c(1);
    if (k == 0x24)
        func_02019b08(0x14, 1);
    else
        func_02019b08(k, 1);
}

/* 0x0200f244: variable copy
 * @difftest $C=ptr:8 @$C+4:16=int:0:0x240 @$C+6:16=int:0:0x240 $C */
s32 func_0200f244(u8 *cmd)
{
    VAR(U16_AT(cmd, 6)) = VAR(U16_AT(cmd, 4));
    return 1;
}

/* 0x0200f264: variable = value
 * @difftest $C=ptr:8 @$C+4:16=int:0:0x240 $C */
s32 func_0200f264(u8 *cmd)
{
    VAR(U16_AT(cmd, 4)) = cmd[6];
    return 1;
}

/* 0x0200f280: compare a variable with a constant
 * @difftest $C=ptr:12 @$C+4:16=int:0:0x240 @$C+6:8=int:0:7 @$C+10:8=pick:0,1,2 @$C+11:8=pick:0,1,2 $C cases=100 */
s32 func_0200f280(u8 *cmd)
{
    func_0200ebcc(VAR(U16_AT(cmd, 4)), cmd[6], cmd[7], cmd[8], cmd[9], cmd[10], cmd[11]);
    return 1;
}

/* 0x0200f2d4: compare two variables
 * @difftest $C=ptr:16 @$C+4:16=int:0:0x240 @$C+6:16=int:0:0x240 @$C+8:8=int:0:7 @$C+11:8=pick:0,1,2 @$C+12:8=pick:0,1,2 $C cases=100 */
s32 func_0200f2d4(u8 *cmd)
{
    func_0200ebcc(VAR(U16_AT(cmd, 4)), cmd[8], VAR(U16_AT(cmd, 6)), cmd[9], cmd[10], cmd[11], cmd[12]);
    return 1;
}

/* 0x0200f32c
 * @difftest u32 */
s32 func_0200f32c(u8 *cmd) { return 1; }

/* 0x0200f334
 * @difftest u32 */
s32 func_0200f334(u8 *cmd) { return 1; }

/* 0x0200f33c: place an actor at (x, y) facing direction cmd[12]
 * @difftest $C=ptr:16 @$C+8:8=int:0:2 @$C+9:8=int:0:4 @$C+12:8=int:0:8 $C cases=100 */
s32 func_0200f33c(u8 *cmd)
{
    u8 *a0 = func_0200917c((s8)cmd[8], cmd[9]), *a, *m;
    s32 p[2], t[2];
    a = AS_ACTOR(a0);
    if (a == NULL)
        return 1;
    S32_AT(a, 0x54) = RoundShl(S16_AT(cmd, 4), 16);
    S32_AT(a, 0x58) = RoundShl(S16_AT(cmd, 6), 16);
    S32_AT(a, 0x5c) = S32_AT(a, 0x54);
    S32_AT(a, 0x60) = S32_AT(a, 0x58);
    p[0] = S32_AT(a, 0x5c);
    p[1] = S32_AT(a, 0x60);
    S32_AT(a, 0x6c) = func_0201a0a0(p, a);
    func_020038d4(t, (s32 *)(a + 0x54), 4);
    S32_AT(a, 0x28) = t[0];
    S32_AT(a, 0x2c) = t[1];
    S32_AT(a, 0x24) = S32_AT(a, 0x6c);
    U8_AT(a, 0x10d) = cmd[12];
    m = PTR_AT(a, 0x44);
    if (m != NULL) {
        fx32 ang = func_02001ef4(U8_AT(a, 0x10d));
        s32 z = S32_AT(m, 0x18), x = S32_AT(m, 0x10);
        S32_AT(m, 0x10) = x;
        S32_AT(m, 0x14) = ang;
        S32_AT(m, 0x18) = z;
        U8_AT(m, 0x8c) = 1;
    }
    if (a0 == func_0201f490())
        func_0201f524(a0);
    return 1;
}

/* 0x0200f4b0: make an object's model follow a path
 * @difftest $C=ptr:16 @$C+4:8=int:0:2 @$C+5:8=int:0:4 @$C+8:8=int:0:3 $C cases=100 */
s32 func_0200f4b0(u8 *cmd)
{
    u8 *a = LOOKUP(cmd);
    if (a != NULL) {
        u32 v = cmd[9], n;
        void *m;
        if (S32_AT(a, 0xc) == 0x11) {
            u8 *b = a;
            if (a != NULL)
                b = func_020a6efc(a, data_020bcb88, data_020bcef0, -1);
            v = b[0x7c];
        }
        m = VCallR1(a, 0x38);
        n = (func_02008930(cmd[8]) - 1) & 0xff;
        func_0207100c(m, cmd[8], v, cmd[7], cmd[0xa], cmd[0xd], n, cmd[0xb] * 0x1e);
    }
    if (a != NULL)
        a = func_020a6efc(a, data_020bcb88, data_020bcb94, -1);
    if (a != NULL)
        U8_AT(a, 0x10c) = cmd[0xc];
    return 1;
}

/* 0x0200f5a8
 * @difftest u32 */
s32 func_0200f5a8(u8 *cmd) { return 1; }

/* 0x0200f5b0: start a camera move and wait for it
 * @difftest @020da9f8:8=pick:0,1 $C=ptr:8 @$C+4:16=pick:0,1,8,40 $C cases=60 */
s32 func_0200f5b0(u8 *cmd)
{
    if (data_020da9f8 == 0) {
        if (U16_AT(cmd, 4) != 0) {
            u32 v;
            data_020da9f8 = 1;
            v = U16_AT(cmd, 4);
            func_0201f780(RoundShl(v, 12));
        }
    } else if (func_0201f96c() != 4) {
        data_020da9f8 = 0;
    }
    return (s8)(data_020da9f8 == 0);
}

/* 0x0200f65c: set the light preset and colours
 * @difftest ptr:8 cases=20 */
s32 func_0200f65c(u8 *cmd)
{
    *(u16 *)data_020deebc = S16_AT(cmd, 4);
    LightPreset();
    func_02019660(cmd[4]);
    func_02019640(cmd[7]);
    return 1;
}

/* 0x0200f6ec
 * @difftest ptr:8 cases=20 */
s32 func_0200f6ec(u8 *cmd)
{
    *(u16 *)data_020deebc = 0;
    func_02004490(data_020bcec0, cmd[4], ((u8 *)data_020deebc)[2]);
    LightPreset();
    func_02019660(cmd[4]);
    func_02019640(cmd[5]);
    return 1;
}

/* 0x0200f790: make the camera follow an object
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200f790(u8 *cmd)
{
    u8 *a = LOOKUP(cmd);
    if (a != NULL)
        func_0201f524(a);
    return 1;
}

/* 0x0200f7c4: move the camera to an object and wait
 * @difftest @020da9fc:8=pick:0,1 $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 @$C+6:16=int:-8:8 $C cases=60 */
s32 func_0200f7c4(u8 *cmd)
{
    if (data_020da9fc == 0) {
        u8 *a = LOOKUP(cmd);
        if (a != NULL) {
            s32 s = S16_AT(cmd, 6);
            u32 n = (u32)(s32)(s16)(s < 0 ? -s : s);
            float f;
            if (n > 5)
                n = 5;
            func_0201f564(a);
            f = (float)(n << 12);
            func_0201f770((s32)(n != 0 ? 0.5f + f : f - 0.5f));
            data_020da9fc = 1;
        }
    } else if (func_0201f96c() != 1) {
        data_020da9fc = 0;
    }
    return (s8)(data_020da9fc == 0);
}

/* 0x0200f894: call an object's virtual slot 5
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200f894(u8 *cmd)
{
    u8 *a = LOOKUP(cmd);
    if (a != NULL)
        VCallR1(a, 0x14);
    return 1;
}

/* 0x0200f8d0
 * @difftest u32 */
void *func_0200f8d0(void *cmd) { return cmd; }

/* 0x0200f8d4: set the level variable
 * @difftest ptr:8 */
s32 func_0200f8d4(u8 *cmd)
{
    data_020d9f9c[0] = cmd[4];
    return 1;
}

/* 0x0200f8ec: wait cmd[4] seconds, then play a sound and spawn a group
 * @difftest @020da9b0:8=pick:0,1 @020daa34:32=int:-2:3 $C=ptr:8 @$C+4:16=pick:0,0,2 @$C+6:8=int:0:3 $C cases=60 */
s32 func_0200f8ec(u8 *cmd)
{
    if (!(data_020daa04 & 1)) {
        func_02049ca0(data_020daa24);
        func_020a6cf0(data_020daa24, func_02049c74, data_020daa0c);
        data_020daa04 |= 1;
    }
    if (data_020da9b0 == 0) {
        if (U16_AT(cmd, 4) == 0) {
            Sound(cmd[7]);
            func_020091c0((s8)cmd[6]);
            data_020da9b0 = 0;
        } else {
            data_020da9ac = cmd[7];
            data_020daa00 = cmd[6];
            func_02049c98(data_020daa24, (u16)(U16_AT(cmd, 4) * 0x1e));
            data_020da9b0 = 1;
        }
    } else {
        func_02049c14(data_020daa24, func_020822c0(func_020062d0()));
        if ((s8)(S32_AT(data_020daa24, 0x10) <= 0)) {
            Sound(data_020da9ac);
            func_020091c0((s8)data_020daa00);
            data_020da9ac = 0;
            data_020daa00 = 0;
            data_020da9b0 = 0;
        }
    }
    return (s8)(data_020da9b0 == 0);
}

/* 0x0200fa5c
 * @difftest ptr:8 cases=10 */
s32 func_0200fa5c(u8 *cmd)
{
    func_02020504(cmd[4]);
    return 1;
}

/* 0x0200fa7c
 * @difftest ptr:8 cases=10 */
s32 func_0200fa7c(u8 *cmd)
{
    func_020203d0(cmd);
    return 1;
}

/* 0x0200fa98
 * @difftest ptr:8 cases=10 */
s32 func_0200fa98(u8 *cmd)
{
    func_020204d8(cmd[4]);
    return 1;
}

/* 0x0200fab8
 * @difftest ptr:8 cases=10 */
s32 func_0200fab8(u8 *cmd)
{
    func_02020440(cmd[4]);
    return 1;
}

/* 0x0200fad8
 * @difftest ptr:8 cases=10 */
s32 func_0200fad8(u8 *cmd)
{
    func_0202028c(cmd[4]);
    return 1;
}

/* 0x0200faf8
 * @difftest $C=ptr:8 @$C+4:8=pick:7,3 $C cases=10 */
s32 func_0200faf8(u8 *cmd)
{
    if (cmd[4] == 7 && func_020186ec() != 1)
        func_020202f0(cmd[4]);
    return 1;
}

/* 0x0200fb2c
 * @difftest ptr:8 cases=10 */
s32 func_0200fb2c(u8 *cmd)
{
    func_020201ec(cmd);
    return 1;
}

/* 0x0200fb48
 * @difftest ptr:8 cases=10 */
s32 func_0200fb48(u8 *cmd)
{
    func_0202023c(cmd);
    return 1;
}

/* 0x0200fb64
 * @difftest u32 */
s32 func_0200fb64(u8 *cmd) { return 1; }

/* 0x0200fb6c
 * @difftest u32 */
s32 func_0200fb6c(u8 *cmd) { return 1; }
