/*
 * Script command handlers, part 4.
 * ARM9 main, 0x0200e4d4 - 0x0200edb4 (23 functions).
 *
 * Progress counters (func_0204a80c/func_0204a81c), conditional sounds and
 * group spawns, actor property commands and a few multi-frame dialogue
 * commands. Command layout as in ScriptCmd3.c.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcb94[];           /* RTTI: CActor */
extern s8 data_020da9d0;
extern s8 data_020da9ec;
extern u8 data_020da9b4;
extern u8 data_020da9bc;
extern u8 data_020def18[];

void *func_02009394(void);
u32 func_020089c0(s32 i);
s32 func_020089dc(s32 i);
void *func_02008950(s32 i, s32 k);
void *func_0200917c(s32 group, s32 index);
void func_020091c0(s32 group);
void *func_0200fcc0(void);
void func_02011664(u32 text, u32 v);
s32 func_02011bc8(void);
u32 func_02013b84(void *obj);
s32 func_02014384(void *obj);
void func_020185fc(u32 on);
u32 func_020186ec(void);
void func_020186fc(void);
void func_02019b08(u32 a, u32 b);
u32 func_02019b98(u32 v);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02020440(u32 v);
s32 func_02040f48(void *obj);
void func_02041340(void *obj, u32 v);
void func_02042480(u32 v);
void func_02042808(u32 v);
void func_020437c4(u32 screen);
s32 func_0204a80c(u32 counter);
void func_0204a81c(u32 counter, u32 v);
void func_0205f4b0(void *snd, u32 id);
void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */

/* this file's own functions, called before their definition */
void func_0200ebcc(u32 v, u32 op, u32 ref, u32 snd_true, u32 snd_false, u32 grp_true, u32 grp_false);

#define LOOKUP(cmd) ((u8 *)func_0200917c((s8)(cmd)[4], (cmd)[5]))
#define AS_ACTOR(a) ((a) != NULL ? (u8 *)func_020a6efc(a, data_020bcb88, data_020bcb94, -1) : (a))

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

/* 0x0200e4d4: set a progress counter
 * @difftest $C=ptr:8 @$C+4:8=int:0:16 $C cases=40 */
s32 func_0200e4d4(u8 *cmd)
{
    func_0204a81c(cmd[4] + 1, cmd[5]);
    return 1;
}

/* 0x0200e4fc: set a progress counter to 1
 * @difftest $C=ptr:8 @$C+4:8=int:0:16 $C cases=40 */
s32 func_0200e4fc(u8 *cmd)
{
    func_0204a81c(cmd[4] + 1, 1);
    return 1;
}

/* 0x0200e524: play cmd[5] while a counter is still low, else cmd[6]
 * @difftest $C=ptr:8 @$C+4:8=int:0:12 $C */
s32 func_0200e524(u8 *cmd)
{
    u32 low = 0;
    s32 v = func_0204a80c(cmd[4] + 1);
    switch ((u32)(cmd[4] + 1)) {
    case 9:
        if (v < 1)
            low = 1;
        break;
    case 1:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 10:
        if (v < 3)
            low = 1;
        break;
    case 2:
    case 3:
        if (v < 4)
            low = 1;
        break;
    }
    if (func_02014384(data_020deebc) != 0 && low)
        Sound(cmd[5]);
    else
        Sound(cmd[6]);
    return 1;
}

/* 0x0200e5e8
 * @difftest ptr:8 cases=40 */
s32 func_0200e5e8(u8 *cmd)
{
    if (func_02014384(data_020deebc) != 0)
        Sound(cmd[4]);
    else
        Sound(cmd[5]);
    return 1;
}

/* 0x0200e62c: compare a progress counter
 * @difftest $C=ptr:12 @$C+4:8=int:0:16 @$C+5:8=int:0:7 @$C+6:8=int:0:4 $C cases=100 */
s32 func_0200e62c(u8 *cmd)
{
    func_0200ebcc(func_0204a80c(cmd[4] + 1) & 0xff, cmd[5], cmd[6], cmd[7], cmd[8], 0, 0);
    return 1;
}

/* 0x0200e67c: compare the story stage
 * @difftest $C=ptr:12 @$C+5:8=int:0:7 @$C+4:8=int:0:6 $C cases=100 */
s32 func_0200e67c(u8 *cmd)
{
    func_0200ebcc((func_02013b84(data_020deebc) + 1) & 0xff, cmd[5], cmd[4], cmd[6], cmd[7], 0, 0);
    return 1;
}

/* 0x0200e6d0
 * @difftest $C=ptr:8 @$C+4:8=int:0:5 $C cases=20 */
s32 func_0200e6d0(u8 *cmd)
{
    func_0201b770(2, 0x3f, 1);
    switch (cmd[4]) {
    case 0:
        func_02019b08(5, data_020df0fc);
        break;
    case 1:
        func_02019b08(6, data_020df0fc);
        break;
    case 2:
        func_02019b08(7, data_020df0fc);
        break;
    case 3:
        func_02019b08(7, data_020df0fc);
        break;
    }
    func_02019b08(func_02019b98(1), 1);
    return 1;
}

/* 0x0200e778
 * @difftest $C=ptr:8 @$C+4:8=pick:0,1 $C cases=20 */
s32 func_0200e778(u8 *cmd)
{
    u32 on;
    func_02020440(0x47);
    on = cmd[4] != 0;
    if (on != func_020186ec())
        func_020185fc(on);
    return 1;
}

/* 0x0200e7b8
 * @difftest u32 */
s32 func_0200e7b8(u8 *cmd) { return 1; }

/* 0x0200e7c0
 * @difftest ptr:8 cases=10 */
s32 func_0200e7c0(u8 *cmd)
{
    func_02042480(0);
    return 1;
}

/* 0x0200e7e0
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200e7e0(u8 *cmd)
{
    u8 *a = AS_ACTOR(LOOKUP(cmd));
    U8_AT(a, 0x10d) = cmd[6];
    return 1;
}

/* 0x0200e828: set or move an actor's height
 * @difftest $C=ptr:12 @$C+4:8=int:0:2 @$C+5:8=int:0:4 @$C+8:8=pick:0,1 $C cases=60 */
s32 func_0200e828(u8 *cmd)
{
    u8 *a = AS_ACTOR(LOOKUP(cmd));
    if (cmd[8] != 0)
        S32_AT(a, 0x24) += RoundShl(S16_AT(cmd, 6), 16);
    else
        S32_AT(a, 0x24) = RoundShl(S16_AT(cmd, 6), 16);
    return 1;
}

/* 0x0200e904
 * @difftest $C=ptr:12 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200e904(u8 *cmd)
{
    u8 *a = AS_ACTOR(LOOKUP(cmd));
    if (a != NULL) {
        U8_AT(a, 0x108) = cmd[6];
        U8_AT(a, 0x109) = cmd[7];
        U8_AT(a, 0x10a) = cmd[9];
        U8_AT(a, 0x10b) = 1;
    }
    return 1;
}

/* 0x0200e96c: start an event and wait until it ends
 * @difftest @020da9d0:8=pick:0,1 $C=ptr:8 @$C+4:16=int:0:16 $C cases=40 */
s32 func_0200e96c(u8 *cmd)
{
    if (data_020da9d0 == 0) {
        func_02041340(func_0200fcc0(), U16_AT(cmd, 4));
        data_020da9d0 = 1;
    } else if ((s8)(func_02040f48(func_0200fcc0()) != -1) == 0) {
        data_020da9d0 = 0;
    }
    return (s8)(data_020da9d0 == 0);
}

/* 0x0200e9f4: stop Spyro and show a text; when it closes, play a sound and
 * spawn a group
 * @difftest @020da9ec:8=pick:0,1 $C=ptr:8 @$C+6:8=int:0:3 $C cases=40 */
s32 func_0200e9f4(u8 *cmd)
{
    if (data_020da9ec == 0) {
        u32 scr;
        void *o;
        u8 *p;
        data_020da9b4 = cmd[7];
        scr = data_020df0fc;
        data_020da9bc = cmd[6];
        func_020437c4(scr);
        o = PTR_AT(data_020def18, 8);
        VCALL(o, 0xc, void (*)(void *, s32))(o, 0);
        S32_AT(data_020deebc[1], 0x70) = 0;
        S32_AT(data_020deebc[1], 0x74) = 0;
        p = (u8 *)data_020deebc[1];
        S32_AT(p, 0x5c) = S32_AT(p, 0x54);
        S32_AT(p, 0x60) = S32_AT(p, 0x58);
        func_02011664(U16_AT(cmd, 4), 1);
        data_020da9ec = 1;
    } else if (func_02011bc8() == 0) {
        Sound(data_020da9b4);
        func_020091c0((s8)data_020da9bc);
        data_020da9b4 = 0;
        data_020da9bc = 0;
        data_020da9ec = 0;
    }
    return (s8)(data_020da9ec == 0);
}

/* 0x0200eb14
 * @difftest u32 */
s32 func_0200eb14(u8 *cmd) { return 1; }

/* 0x0200eb1c
 * @difftest u32 */
s32 func_0200eb1c(u8 *cmd) { return 1; }

/* 0x0200eb24: call virtual slot 13 on every object of a group
 * @difftest $C=ptr:8 @$C+4:8=int:0:3 $C cases=20 */
s32 func_0200eb24(u8 *cmd)
{
    u32 g = cmd[4];
    s32 gs, n, k;
    if (func_020089dc(g))
        return 1;
    gs = (s8)g;
    n = func_020089c0(gs);
    for (k = 0; k < n; k++) {
        u8 *a = func_0200917c(gs, (s16)k);
        if (a != NULL)
            VCallR1(a, 0x34);
    }
    return 1;
}

/* 0x0200eb98
 * @difftest u32 */
void *func_0200eb98(void *cmd) { return cmd; }

/* 0x0200eb9c: play a sound and spawn a group
 * @difftest $C=ptr:8 @$C+4:8=int:0:3 $C cases=20 */
s32 func_0200eb9c(u8 *cmd)
{
    Sound(cmd[5]);
    func_020091c0((s8)cmd[4]);
    func_020186ec();
    func_020186fc();
    return 1;
}

/* 0x0200ebcc: compare v with ref (op: == != > >= < <=); on success play
 * snd_true and spawn grp_true, else play snd_false and show grp_false
 * @difftest u8 int:0:7 u8 u8 u8 pick:0,1,2 pick:0,1,2 cases=100 */
void func_0200ebcc(u32 v, u32 op, u32 ref, u32 snd_true, u32 snd_false, u32 grp_true, u32 grp_false)
{
    u32 ok;
    switch (op) {
    case 0: ok = v == ref; break;
    case 1: ok = v != ref; break;
    case 2: ok = v > ref; break;
    case 3: ok = v >= ref; break;
    case 4: ok = v < ref; break;
    case 5: ok = v <= ref; break;
    default: ok = 0; break;
    }
    if (ok) {
        Sound(snd_true);
        if (func_020089dc((u8)grp_true))
            return;
        func_02042808((u8)grp_true);
        func_020186ec();
        func_020186fc();
        return;
    }
    Sound((u8)snd_false);
    if (func_020089dc((u8)grp_false))
        return;
    {
        s32 gs = (s8)grp_false, n = func_020089c0(gs), k;
        for (k = 0; k < n; k++) {
            u8 *a = func_02008950(gs, (s16)k);
            if (a != NULL) {
                U32_AT(a, 0x38) |= 4;
                U32_AT(a, 4) |= 8;
            }
        }
    }
}

/* 0x0200ed28: set an actor's sprite priority
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200ed28(u8 *cmd)
{
    u8 *a = AS_ACTOR(LOOKUP(cmd));
    if (a != NULL)
        U32_AT(a, 0xac) = (U32_AT(a, 0xac) & ~0xc00u) | ((cmd[6] & 3) << 10);
    return 1;
}

/* 0x0200ed8c: play a sound and spawn a group
 * @difftest $C=ptr:8 @$C+4:8=int:0:3 $C cases=20 */
s32 func_0200ed8c(u8 *cmd)
{
    Sound(cmd[5]);
    func_020091c0((s8)cmd[4]);
    return 1;
}
