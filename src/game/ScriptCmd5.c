/*
 * Script command handlers, part 5.
 * ARM9 main, 0x0200edb4 - 0x0200f244 (17 functions).
 *
 * Variable arithmetic on the saved byte variables (data_020d9f9c), a random
 * value command, a countdown timer command, object show/hide and actor
 * freezing. Command layout as in ScriptCmd3.c.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcb94[];           /* RTTI: CActor */
extern u8 data_020bd638;
extern u8 data_020d9f9c[];
extern u8 data_020da1cc[];
extern u8 data_020da9b8;
extern u8 data_020da9c0;
extern s8 data_020da9f4;
extern u32 data_020daa08;
extern u8 data_020daa18[];
extern u8 data_020daa38[];           /* countdown timer object */

void *func_02009394(void);
void *func_0200917c(s32 group, s32 index);
void func_020091c0(s32 group);
void func_02002218(void *actor, u32 v);
void func_0201b5a4(u32 a, u32 b);
void func_0201b5bc(u32 a, u32 b);
void func_0201b770(u32 a, u32 b, u32 c);
void func_0201b820(u32 mask, u32 v);
void func_02049c14(void *timer, u32 now);
void *func_02049c74(void *timer);
void func_02049c98(void *timer, u32 frames);
void *func_02049ca0(void *timer);
void func_0205f4b0(void *snd, u32 id);
u64 func_020a500c(u32 num, u32 den);           /* unsigned divide: r0 quotient, r1 remainder */
void func_020a6cf0(void *obj, void *(*dtor)(void *), void *link);   /* register static destructor */
void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */
int rand(void);

#define LOOKUP(cmd) ((u8 *)func_0200917c((s8)(cmd)[4], (cmd)[5]))
#define AS_ACTOR(a) ((a) != NULL ? (u8 *)func_020a6efc(a, data_020bcb88, data_020bcb94, -1) : (a))

static inline void Sound(u32 id) { func_0205f4b0(func_02009394(), id); }

/* 0x0200edb4
 * @difftest u32 */
s32 func_0200edb4(u8 *cmd) { return 1; }

/* 0x0200edbc: clear 16 flag bytes
 * @difftest u32 */
s32 func_0200edbc(u8 *cmd)
{
    u8 *p = data_020da1cc;
    s32 i = 0x230;
    do {
        i++;
        *p++ = 0;
    } while (i < 0x240);
    return 1;
}

/* 0x0200ede4: variable -= value
 * @difftest $C=ptr:8 @$C+4:16=int:0:0x240 $C */
s32 func_0200ede4(u8 *cmd)
{
    data_020d9f9c[U16_AT(cmd, 4)] -= cmd[6];
    return 1;
}

/* 0x0200ee08: variable += value
 * @difftest $C=ptr:8 @$C+4:16=int:0:0x240 $C */
s32 func_0200ee08(u8 *cmd)
{
    data_020d9f9c[U16_AT(cmd, 4)] += cmd[6];
    return 1;
}

/* 0x0200ee2c: variable = random value in [lo, hi]
 * @difftest $C=ptr:12 @$C+4:16=int:-5:20 @$C+6:16=int:-5:40 @$C+8:16=int:0:0x240 $C */
s32 func_0200ee2c(u8 *cmd)
{
    s32 hi = S16_AT(cmd, 6), lo = S16_AT(cmd, 4);
    u32 r = rand() & 0x7fff;
    u32 rem = (u32)(func_020a500c(r, hi - lo + 1) >> 32);
    data_020d9f9c[U16_AT(cmd, 8)] = lo + rem;
    return 1;
}

/* 0x0200ee78
 * @difftest u32 */
s32 func_0200ee78(u8 *cmd) { return 1; }

/* 0x0200ee80: show or hide an object
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 @$C+6:8=pick:0,1 $C cases=60 */
s32 func_0200ee80(u8 *cmd)
{
    u8 *a = LOOKUP(cmd);
    if (a != NULL) {
        if (cmd[6] != 0) {
            U32_AT(a, 4) |= 8;
            U32_AT(a, 0x38) |= 4;
        } else {
            U32_AT(a, 4) &= ~8u;
            U32_AT(a, 0x38) &= ~4u;
        }
    }
    return 1;
}

/* 0x0200eef8: wait for a number of frames, then play a sound and spawn a group
 * @difftest @020da9f4:8=pick:0,1 @020daa48:32=int:-2:3 $C=ptr:8 @$C+4:16=pick:0,0,30 @$C+6:8=int:0:3 $C cases=60 */
s32 func_0200eef8(u8 *cmd)
{
    if (!(data_020daa08 & 1)) {
        func_02049ca0(data_020daa38);
        func_020a6cf0(data_020daa38, func_02049c74, data_020daa18);
        data_020daa08 |= 1;
    }
    if (data_020da9f4 == 0) {
        if (U16_AT(cmd, 4) == 0) {
            Sound(cmd[7]);
            func_020091c0((s8)cmd[6]);
            data_020da9f4 = 0;
        } else {
            data_020da9b8 = cmd[7];
            data_020da9c0 = cmd[6];
            func_02049c98(data_020daa38, U16_AT(cmd, 4));
            data_020da9f4 = 1;
        }
    } else {
        func_02049c14(data_020daa38, func_020822c0(func_020062d0()));
        if ((s8)(S32_AT(data_020daa38, 0x10) <= 0)) {
            Sound(data_020da9b8);
            func_020091c0((s8)data_020da9c0);
            data_020da9b8 = 0;
            data_020da9c0 = 0;
            data_020da9f4 = 0;
        }
    }
    return (s8)(data_020da9f4 == 0);
}

/* 0x0200f058
 * @difftest u32 */
s32 func_0200f058(u8 *cmd) { return 1; }

/* 0x0200f060
 * @difftest u32 */
s32 func_0200f060(u8 *cmd) { return 1; }

/* 0x0200f068
 * @difftest u32 */
s32 func_0200f068(u8 *cmd) { return 1; }

/* 0x0200f070
 * @difftest ptr:8 cases=20 */
s32 func_0200f070(u8 *cmd)
{
    func_0201b820((cmd[4] | (cmd[5] << 1) | (cmd[6] << 2) | (cmd[7] << 3)) & 0xff, 0x1f);
    return 1;
}

/* 0x0200f0b0
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200f0b0(u8 *cmd)
{
    u8 *a = AS_ACTOR(LOOKUP(cmd));
    func_02002218(a, cmd[6]);
    return 1;
}

/* 0x0200f0f8
 * @difftest ptr:8 cases=10 */
s32 func_0200f0f8(u8 *cmd)
{
    func_0201b5bc(0x3f, 1);
    return 1;
}

/* 0x0200f11c
 * @difftest ptr:8 cases=10 */
s32 func_0200f11c(u8 *cmd)
{
    func_0201b770(0, 0x3f, 1);
    func_0201b5a4(0x3f, 1);
    return 1;
}

/* 0x0200f150: freeze an actor
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200f150(u8 *cmd)
{
    u8 *a = AS_ACTOR(LOOKUP(cmd));
    if (a != NULL)
        U32_AT(a, 0x38) |= 0x800000;
    data_020bd638 = 1;
    return 1;
}

/* 0x0200f1b8: unfreeze an actor and stop it
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200f1b8(u8 *cmd)
{
    u8 *a = AS_ACTOR(LOOKUP(cmd));
    if (a != NULL) {
        U32_AT(a, 0x38) &= ~0x800000u;
        S32_AT(a, 0x70) = 0;
        S32_AT(a, 0x74) = 0;
        S32_AT(a, 0x5c) = S32_AT(a, 0x54);
        S32_AT(a, 0x60) = S32_AT(a, 0x58);
    }
    data_020bd638 = 0;
    return 1;
}
