/*
 * Script command: progress-dependent hint dialogue.
 * ARM9 main, 0x0200c614 - 0x0200d8e0 (1 function).
 *
 * A large decision tree: depending on the command's character (cmd[4]),
 * how far the story has progressed (func_02013b84, func_02014384) and the
 * progress counters func_0204a80c(n), it opens one of many dialogue texts
 * with func_02011664(id, 1). It runs over several frames: the first call
 * opens a text (data_020da9c8 = 1, busy), later calls wait for the dialogue
 * box and may chain a second text; data_020da9d4 is cleared when no further
 * text follows. Returns nonzero once the command is finished.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern s8 data_020da9c8;             /* command busy */
extern s8 data_020da9d4;             /* a follow-up text may come */

u32 func_02013b84(void *obj);
s32 func_02014384(void *obj);
s32 func_0204a80c(u32 counter);
void func_02011664(u32 text, u32 v);
u32 func_02011b88(void);
s32 func_02011bc8(void);
s32 func_02011eac(void);

#define P(k) func_0204a80c(k)

static inline void Say(u32 id) { func_02011664(id, 1); }
static inline void SayEnd(u32 id)
{
    func_02011664(id, 1);
    data_020da9d4 = 0;
}

/* first frame: choose and open the hint text */
static void Begin(u32 who, u32 stage, s32 started)
{
    switch (who) {
    case 5:
        if (P(9) == 0) { SayEnd(0x13); return; }
        if (!started || stage == 1) { SayEnd(0x12); return; }
        if (stage >= 2 && P(8) < 2) { SayEnd(0x2f); return; }
        if (stage >= 3 && P(0xe) == 0) { SayEnd(0x14); return; }
        if (stage >= 4 && P(8) < 3) { SayEnd(0x2f); return; }
        if (stage >= 4 && P(0x10) == 0) { SayEnd(0x300); return; }
        if (stage >= 5 && P(3) == 0) { SayEnd(0x3dc); return; }
        if (P(0xe) == 0) { SayEnd(0x14); return; }
        if (P(0xb) < 1) return;
        if (P(0xb) == 1) { SayEnd(0x15); return; }
        if (P(0xb) == 2) { SayEnd(0x16); return; }
        SayEnd(0x17);
        return;

    case 3:
        if (P(2) == 0) { SayEnd(0x1b); return; }
        if (!started) { SayEnd(0x18); return; }
        if (stage >= 2 && P(8) < 2) SayEnd(0x33);
        else if (stage >= 3 && P(0xe) == 0) SayEnd(0x2e);
        else if (stage >= 4 && P(8) < 3) SayEnd(0x33);
        else if (stage >= 5 && P(3) == 0) SayEnd(0x1f);
        else if (stage >= 4 && P(0x10) == 0) SayEnd(0x301);
        else if (P(1) == 1) Say(0x19);
        else if (P(1) == 2) Say(0x1a);
        else if (P(2) == 1) Say(0x1c);
        else if (P(2) == 2) Say(0x1d);
        else if (P(2) == 3) Say(0x1e);
        else if (P(3) == 0) Say(0x1f);
        else if (P(3) == 1) Say(0x20);
        else if (P(3) == 2) Say(0x21);
        else SayEnd(0x23);
        data_020da9c8 = 1;
        return;

    case 4:
        if (!started) { SayEnd(0x34); return; }
        if (stage >= 2 && P(8) == 1) SayEnd(0x29);
        else if (stage >= 3 && P(0xe) == 0) SayEnd(0x2c);
        else if (stage >= 4 && P(8) == 2) SayEnd(0x38);
        else if (stage >= 4 && P(0x10) == 0) SayEnd(0x304);
        else if (stage >= 5 && P(3) == 0) SayEnd(0x3dd);
        else if (stage == 2 || stage == 4) SayEnd(0x34);
        else if (P(7) == 1) Say(0x2b5);
        else if (P(7) == 2) Say(0x2b6);
        else SayEnd(0x2d2);
        data_020da9c8 = 1;
        return;

    case 2:
        if (!started) { SayEnd(0x35); return; }
        if (stage >= 2 && P(8) < 2) SayEnd(0x30);
        else if (stage >= 3 && P(0xe) == 0) SayEnd(0x2a);
        else if (stage >= 4 && P(0x10) == 0) SayEnd(0x303);
        else if (stage >= 4 && P(8) < 3) SayEnd(0x30);
        else if (stage >= 5 && P(3) == 0) SayEnd(0x3de);
        else if (P(6) == 0) Say(0x2b7);
        else if (P(6) == 1) Say(0x2b8);
        else if (P(6) == 2) Say(0x2b9);
        else if (P(0xa) == 0) Say(0x2ba);
        else if (P(0xa) == 1) Say(0x2bb);
        else if (P(0xa) == 2) Say(0x2bc);
        else SayEnd(0x2d0);
        data_020da9c8 = 1;
        return;

    case 0:
        if (!started) { SayEnd(0x36); return; }
        if (stage >= 2 && P(8) < 2) SayEnd(0x31);
        else if (stage >= 3 && P(0xe) == 0) SayEnd(0x2b);
        else if (stage >= 4 && P(8) < 3) SayEnd(0x31);
        else if (stage >= 4 && P(0x10) == 0) SayEnd(0x302);
        else if (stage >= 5 && P(3) == 0) SayEnd(0x3e0);
        else if (P(4) == 1) Say(0x2bd);
        else if (P(4) == 2) Say(0x2be);
        else if (P(5) == 1) Say(0x2bf);
        else if (P(5) == 2) Say(0x2c0);
        else SayEnd(0x2d1);
        data_020da9c8 = 1;
        return;

    case 1:
        if (!started) { SayEnd(0x37); return; }
        if (stage >= 2 && P(8) < 2) SayEnd(0x32);
        else if (stage >= 3 && P(0xe) == 0) SayEnd(0x2d);
        else if (stage >= 4 && P(0xc) == 0) SayEnd(0x2c1);
        else if (stage >= 4 && P(8) < 3) SayEnd(0x32);
        else if (stage >= 5 && P(3) == 0) SayEnd(0x3df);
        else if (stage - 1 <= 1) SayEnd(0x37);
        else if (stage >= 4) {
            if (P(0xc) == 0) Say(0x2c1);
            else if (P(0xc) == 1) Say(0x2c2);
            else if (P(0xc) == 2) Say(0x2c3);
            else if (P(0xd) == 0) Say(0x2c4);
            else if (P(0xd) == 1) Say(0x2c5);
            else if (P(0xd) == 2) Say(0x2c6);
            else SayEnd(0x2d3);
        } else if (stage >= 3 && P(0xe) > 0) {
            if (P(0xd) == 0) Say(0x2c4);
            else if (P(0xd) == 1) Say(0x2c5);
            else if (P(0xd) == 2) Say(0x2c6);
        } else {
            SayEnd(0x37);
        }
        data_020da9c8 = 1;
        return;
    }
}

/* later frames: when the first text has closed, open its follow-up */
static void Continue(u32 who, u32 stage, u32 shown)
{
    switch (who) {
    case 3:
        if (shown - 0x19 <= 1) {
            if (P(2) == 1) Say(0x1c);
            else if (P(2) == 2) Say(0x1d);
            else if (P(2) == 3) Say(0x1e);
            data_020da9c8 = 1;
            return;
        }
        if (shown - 0x1b <= 3) {
            if (stage >= 5) {
                if (P(3) == 0) Say(0x1f);
                else if (P(3) == 1) Say(0x20);
                else if (P(3) == 2) Say(0x21);
                else SayEnd(0x23);
            } else {
                SayEnd(0x2f9);
            }
            data_020da9c8 = 1;
            return;
        }
        SayEnd(0x2fa);
        return;

    case 4:
        if (stage == 2 || stage == 4 || (shown != 0x29 && shown != 0x38)) {
            SayEnd(0x2fd);
            return;
        }
        if (P(7) == 1) Say(0x2b5);
        else if (P(7) == 2) Say(0x2b6);
        else SayEnd(0x2d2);
        data_020da9c8 = 1;
        return;

    case 2:
        if (shown - 0x2b7 <= 2) {
            if (P(0xa) == 0) Say(0x2ba);
            else if (P(0xa) == 1) Say(0x2bb);
            else if (P(0xa) == 2) Say(0x2bc);
            else SayEnd(0x2d0);
            data_020da9c8 = 1;
            return;
        }
        SayEnd(0x2fe);
        return;

    case 0: {
        u32 k = shown - 0x2bd;
        if (k <= 1 && P(5) > 0) {
            if (P(5) == 1) Say(0x2bf);
            else if (P(5) == 2) Say(0x2c0);
            else SayEnd(0x2d1);
            data_020da9c8 = 1;
            return;
        }
        if (k <= 1 && P(5) == 0) {
            SayEnd(0x39c);
            return;
        }
        SayEnd(0x2ff);
        return;
    }

    case 1:
        if (shown - 0x2c2 <= 1) {
            if (P(0xd) == 0) Say(0x2c4);
            else if (P(0xd) == 1) Say(0x2c5);
            else if (P(0xd) == 2) Say(0x2c6);
            else SayEnd(0x2d3);
            data_020da9c8 = 1;
            return;
        }
        SayEnd(0x2fc);
        return;
    }
}

/* 0x0200c614
 * @difftest $C=ptr:8 @$C+4:8=int:0:7 @020da9c8:8=pick:0,0,1 @020da9d4:8=pick:0,1 $C
 * @difftest $C=ptr:8 @$C+4:8=int:0:7 @020da9c8:8=pick:0,0,1 @020da9d4:8=pick:0,1 @020ebc51:8=int:0:2 @020ebc52:8=int:0:6 @020ebce5:8=int:0:4 @020ebce6:8=int:0:4 @020ebce7:8=int:0:4 @020ebce8:8=int:0:4 @020ebce9:8=int:0:4 @020ebcea:8=int:0:4 @020ebceb:8=int:0:4 @020ebcec:8=int:0:4 @020ebced:8=int:0:4 @020ebcee:8=int:0:4 @020ebcef:8=int:0:4 @020ebcf0:8=int:0:4 @020ebcf1:8=int:0:4 @020ebcf2:8=int:0:4 @020ebcf3:8=int:0:4 @020ebcf4:8=int:0:4 $C cases=1500 */
s32 func_0200c614(u8 *cmd)
{
    u32 stage = func_02013b84(data_020deebc) + 1;
    s32 started = (s8)(func_02014384(data_020deebc) != 0);

    if (data_020da9c8 == 0) {
        data_020da9c8 = 1;
        data_020da9d4 = 1;
        Begin(cmd[4], stage, started);
    } else if (func_02011bc8() != 0 || data_020da9d4 == 0) {
        if (func_02011bc8() == 0)
            data_020da9c8 = 0;
    } else if (func_02011eac() != 0) {
        if (func_02011eac() == 1)
            data_020da9d4 = 0;
    } else {
        u32 shown = func_02011b88();
        data_020da9c8 = 1;
        Continue(cmd[4], stage, shown);
    }
    return (s8)(data_020da9c8 == 0);
}
