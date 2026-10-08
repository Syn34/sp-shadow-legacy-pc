/*
 * SoundDS.cpp, part 4: the tracker-style music player's rows and effects:
 * reading a row of pattern data, new notes (with new-note actions and
 * duplicate checks), the volume column and the effect column on the first
 * tick and on every following tick, and moving through the order list.
 * ARM9 main, 0x020241b8 - 0x02026758 (8 functions).
 *
 * The player layout is described in SeqPlayer1.c; further fields used
 * here: +0x08 order list, +0x0c patterns, +0x14 read pointer into the
 * current pattern, +0x22 row, +0x26 tracks, +0x28 playing, +0x29 flags
 * (0x01 loop), +0x2a/+0x2e tick counter and speed, +0x2b order position,
 * +0x2d tempo, +0x30 order length, +0x34/+0x35 next row and order
 * position, +0x37 an effect parameter. Per track +0x00 note, +0x01
 * instrument, +0x02 volume column, +0x03/+0x04 effect and parameter,
 * +0x22 what the row set (0x01 note, 0x02 instrument, 0x04 volume
 * column, 0x08 effect; 0x10/0x20 effect/volume column active on later
 * ticks). data_020e2ba8 + 0 is the player callback (event, value, flag).
 *
 * Effect E70-E72 (past-note actions) in the original indexes a 3-byte
 * table with the whole effect byte and so reads the caller's stack; here
 * it uses the low nibble, as intended.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const u8 data_020b58d0[];     /* tone portamento speeds of the volume column */
extern const u8 data_020bde48[];     /* past-note actions */
extern u8 data_020bde54[];           /* u32 *: note rate factors */
extern u8 data_020e2314[];
extern u8 data_020e2614[];
extern u8 data_020e2b70[];
extern u8 data_020e2ba4[];
extern u8 data_020e2ba8[];

u32 func_02019910(u32 a, u32 b);
void func_02022ecc(u32 v);
u32 func_02023040(u32 rate, s32 bend);
void func_02023450(u32 t);
void func_02023574(u8 *vol, u32 x, u32 max);
u32 func_020235bc(u8 *vol, u32 x, u32 max);
void func_02023648(void);
void func_02023778(u32 bits);
void func_02023d60(u32 v, u32 mode);
u32 func_02027810(void);
void func_02027c94(u32 id, u32 v);
void func_02027cec(u32 id, u32 pos, u32 row);
void func_020281bc(u32 v);

void func_02025a08(u32 ti);
void func_02025d58(u32 ti);
void func_02025f30(u32 ti, s32 porta);
void func_02026714(u32 ev, u32 a, s32 b);

#define D data_020e2614
#define PLAYER() ((u8 *)PTR_AT(data_020e2ba4, 0))
#define VOICE(p, v) ((p) + 0x93c + (v) * 0x20)
#define TRACK(p, t) ((p) + 0x3c + (t) * 0x24)
#define MARK(e, b) ((e)[0x11] = ((e)[0x11] & ~3) | ((((e)[0x11] & 3) | (b)) & 3))

/* 0x020241b8: per-tick effects of every track (the effect column, then the
 * volume column)
 * @difftest $P=ptr:0xd40:4 @020e2ba4:32=$P $S=ptr:0x2400:4 @$P+0:32=$S $I=ptr:0x2000:4 @$P+4:32=$I $M=ptr:0x100 @$I+0x2c:32=$M @$I+0x30:32=$M @$I+0x34:32=0 @$I+0x38:32=0 @$I+0x3c:32=0 $O=ptr:0x100 @$P+8:32=$O @$P+0x26:8=1 @$P+0x3d:8=1 @$P+0x3c:8=int:0:0x80 @$P+0x44:8=pick:0,1,0xff @$P+0x41:8=int:0:0x41 @$P+0x9f5:8=int:0:16 @$P+0xa15:8=int:0:16 @$P+0x3f:8=int:0:0x1b @$P+0x40:8=u8 @$P+0x3e:8=u8 @$P+0x5e:8=pick:0x10,0x20,0x30 @04000006:16=0x8c cases=150 */
void func_020241b8(void)
{
    s32 i;

    for (i = PLAYER()[0x26] - 1; i >= 0; i--) {
        u8 *p = PLAYER(), *t = TRACK(p, i), *e;
        u32 fx;

        e = t[8] != 0xff ? VOICE(p, t[8]) : NULL;
        if (t[0x22] & 0x10) {
            if (e != NULL) {
                switch (t[3]) {
                case 5:
                    U32_AT(e, 0) = func_02023040(U32_AT(e, 0), -(t[4] << 2));
                    if (U32_AT(e, 0) < 0x20)
                        func_02023d60(t[8], 0);
                    MARK(e, 1);
                    break;
                case 6:
                    U32_AT(e, 0) = func_02023040(U32_AT(e, 0), t[4] << 2);
                    if (U32_AT(e, 0) > 0x1fffe000)
                        func_02023d60(t[8], 0);
                    MARK(e, 1);
                    break;
                case 9:
                    e[0x10] -= 1;
                    if (e[0x10] == 0) {
                        e[0x10] = (t[4] & 0xf) | 0x80;
                        MARK(e, 2);
                    } else if (e[0x10] == 0x80) {
                        e[0x10] = t[4] >> 4;
                        MARK(e, 2);
                    }
                    break;
                case 10: {
                    const u32 *tbl;
                    switch (e[0x10]) {
                    case 0:
                        e[0x10] = 1;
                        tbl = PTR_AT(data_020bde54, 0);
                        U32_AT(e, 0) = func_02019910(U32_AT((u8 *)PTR_AT(PLAYER(), 0) + e[9] * 0x24, 0x14),
                                                     tbl[e[8] + (t[4] & 0xf)]);
                        break;
                    case 1:
                        e[0x10] = 2;
                        tbl = PTR_AT(data_020bde54, 0);
                        U32_AT(e, 0) = func_02019910(U32_AT((u8 *)PTR_AT(PLAYER(), 0) + e[9] * 0x24, 0x14),
                                                     tbl[e[8] + (t[4] >> 4)]);
                        break;
                    case 2:
                        e[0x10] = 0;
                        tbl = PTR_AT(data_020bde54, 0);
                        U32_AT(e, 0) = func_02019910(U32_AT((u8 *)PTR_AT(PLAYER(), 0) + e[9] * 0x24, 0x14),
                                                     tbl[e[8]]);
                        break;
                    }
                    MARK(e, 1);
                    break;
                }
                case 0x11:
                    e[0x10] -= 1;
                    if (e[0x10] != 0)
                        break;
                    e[0x10] = t[4] & 0xf;
                    func_02025d58(i & 0xff);
                    fx = t[4] >> 4;
                    if (fx < 8) {
                        if (fx != 0) {
                            if (fx <= 5)
                                t[5] -= (1 << (fx - 1)) & 0xff;
                            else if (fx != 6)
                                t[5] >>= 1;
                            else
                                t[5] = (t[5] * 0xaaaa) >> 16;
                            if (t[5] > 0x40)
                                t[5] = 0;
                        }
                    } else if (fx != 8) {
                        if (fx <= 0xd) {
                            /* the original shifts by fx - 0xd: nothing for 9..0xc */
                            u32 sh = (fx - 0xd) & 0xff;
                            t[5] += sh < 32 ? (1u << sh) & 0xff : 0;
                        } else if (fx != 0xe) {
                            t[5] <<= 1;
                        } else {
                            t[5] += (t[5] >> 1) & 0xff;
                        }
                        if (t[5] > 0x40)
                            t[5] = 0x40;
                    }
                    e[6] = t[5];
                    MARK(e, 3);
                    break;
                case 0x12:
                case 0x19:
                    e[0x10] += 1;
                    MARK(e, 2);
                    break;
                case 0x13:
                    if ((t[4] >> 4) == 0xc && PLAYER()[0x2a] != PLAYER()[0x2e]) {
                        e[0x10] -= 1;
                        if (e[0x10] == 0) {
                            t[0x22] &= ~0x10;
                            func_02023d60(t[8], 0);
                        }
                    }
                    break;
                case 0x14: {
                    u32 x = t[4];
                    u8 *q = PLAYER();
                    if (x < 0x10)
                        func_02027c94(q[0x3b], (u16)(q[0x2d] + x));
                    else
                        func_02027c94(q[0x3b], (u16)(q[0x2d] - (x - 0x10)));
                    break;
                }
                case 0x17:
                    func_02023574(p + 0x2c, t[4], 0x80);
                    func_02023778(2);
                    break;
                }
                if (t[3] == 7 || t[3] == 0xc)
                    func_02023450(i & 0xff);
                if (t[3] == 8 || t[3] == 0x15 || t[3] == 0xb) {
                    e[0xf] += t[0x14];
                    MARK(e, 1);
                }
            }
            switch (t[3]) {
            case 4:
            case 0xb:
            case 0xc:
                func_02023574(t + 5, t[0xb], 0x40);
                if (e != NULL) {
                    e[6] = t[5];
                    MARK(e, 2);
                }
                break;
            case 0xe:
                func_02023574(t + 6, t[4], 0x40);
                if (e != NULL)
                    MARK(e, 2);
                break;
            case 0x10:
                func_02023574(t + 7, t[4], 0x40);
                if (e != NULL) {
                    e[7] = t[7];
                    MARK(e, 2);
                }
                break;
            case 0x13:
                if ((t[4] >> 4) == 0xd && PLAYER()[0x2a] != PLAYER()[0x2e]) {
                    t[4] -= 1;
                    if (!(t[4] & 0xf)) {
                        u32 vc;
                        t[0x22] &= ~0x10;
                        vc = ((t[0x22] & 0x20) && t[2] >= 0xc1 && t[2] <= 0xca) ? 1 : 0;
                        func_02025f30(i & 0xff, vc);
                        func_02025a08(i & 0xff);
                    }
                }
                break;
            case 0x1a:
                func_02026714(4, t[4], 0);
                break;
            }
        }
        if (t[0x22] & 0x20) {
            u32 x = t[2];
            if (x <= 0x5e) {
                t[5] += x - 0x55;
                if (t[5] > 0x40)
                    t[5] = 0x40;
                if (e != NULL) {
                    e[6] = t[5];
                    MARK(e, 2);
                }
            } else if (x <= 0x68) {
                t[5] -= x - 0x5f;
                if (t[5] > 0x40)
                    t[5] = 0;
                if (e != NULL) {
                    e[6] = t[5];
                    MARK(e, 2);
                }
            } else if (e != NULL) {
                if (x <= 0x72) {
                    MARK(e, 1);
                    U32_AT(e, 0) = func_02023040(U32_AT(e, 0), -(t[0x10] << 2));
                    if (U32_AT(e, 0) < 0x20)
                        func_02023d60(t[8], 0);
                } else if (x <= 0x7c) {
                    MARK(e, 1);
                    U32_AT(e, 0) = func_02023040(U32_AT(e, 0), t[0x10] << 2);
                    if (U32_AT(e, 0) > 0x1fffe000)
                        func_02023d60(t[8], 0);
                } else if (x <= 0xca) {
                    func_02023450(i & 0xff);
                } else if (x <= 0xd4) {
                    MARK(e, 1);
                    e[0xf] += t[0x14];
                }
            }
        }
    }
}

/* fetch an effect parameter, or reuse the last one when it is 0 */
#define MEMORY(t, slot) do { if ((t)[4] == 0) (t)[4] = (t)[slot]; else (t)[slot] = (t)[4]; } while (0)

/* 0x02024c0c: first-tick effects of track `ti`
 * @difftest $P=ptr:0xd40:4 @020e2ba4:32=$P $S=ptr:0x2400:4 @$P+0:32=$S $I=ptr:0x2000:4 @$P+4:32=$I $M=ptr:0x100 @$I+0x2c:32=$M @$I+0x30:32=$M @$I+0x34:32=0 @$I+0x38:32=0 @$I+0x3c:32=0 $O=ptr:0x100 @$P+8:32=$O @$P+0x26:8=1 @$P+0x3d:8=1 @$P+0x3c:8=int:0:0x80 @$P+0x44:8=pick:0,1,0xff @$P+0x41:8=int:0:0x41 @$P+0x9f5:8=int:0:16 @$P+0xa15:8=int:0:16 @$P+0x3f:8=int:0:0x1b @$P+0x40:8=u8 @$P+0x5e:8=pick:8,9,0x28 pick:0 cases=150 */
void func_02024c0c(u32 ti)
{
    u8 *p = PLAYER(), *t = TRACK(p, ti), *e;
    u32 x, r;

    e = t[8] != 0xff ? VOICE(p, t[8]) : NULL;
    if (!(t[0x22] & 8))
        return;
    if (t[3] == 4 || (u8)(t[3] - 0xb) <= 1) {
        MEMORY(t, 0xb);
        if (t[4] != 0) {
            r = func_020235bc(t + 5, t[4], 0x40);
            t[0x22] |= r;
            if (e != NULL) {
                e[6] = t[5];
                MARK(e, 2);
            }
        }
    }
    switch (t[3]) {
    case 1:
        if (t[4] == 0)
            return;
        PLAYER()[0x2e] = t[4];
        p = PLAYER();
        p[0x2a] = p[0x2e];
        return;
    case 2:
        PLAYER()[0x35] = t[4];
        PLAYER()[0x34] = 0;
        U16_AT(PLAYER(), 0x22) = 0;
        return;
    case 3:
        PLAYER()[0x34] = t[4];
        U16_AT(PLAYER(), 0x22) = 0;
        return;
    case 5:
        MEMORY(t, 0x10);
        x = t[4];
        if ((x >> 4) == 0xf || (x >> 4) == 0xe) {
            if (e == NULL)
                return;
            U32_AT(e, 0) = func_02023040(U32_AT(e, 0), (x >> 4) == 0xf ? -(s32)((x & 0xf) << 2) : -(s32)(x & 0xf));
            if (U32_AT(e, 0) < 0x20)
                func_02023d60(t[8], 0);
            MARK(e, 1);
            return;
        }
        t[0x22] |= 0x10;
        return;
    case 6:
        MEMORY(t, 0x10);
        x = t[4];
        if ((x >> 4) == 0xf || (x >> 4) == 0xe) {
            if (e == NULL)
                return;
            U32_AT(e, 0) = func_02023040(U32_AT(e, 0), (x >> 4) == 0xf ? (s32)((x & 0xf) << 2) : (s32)(x & 0xf));
            if (U32_AT(e, 0) > 0x1fffe000)
                func_02023d60(t[8], 0);
            MARK(e, 1);
            return;
        }
        t[0x22] |= 0x10;
        return;
    case 7:
        t[0x22] |= 0x10;
        MEMORY(t, 0x11);
        /* fall through */
    case 0xc: {
        u32 ins = t[1], note;
        u8 *in;
        if (ins == 0)
            return;
        note = t[0];
        if (note >= 0x78)
            return;
        p = PLAYER();
        in = (u8 *)PTR_AT(p, 4) + ins * 0x20;
        U32_AT(t, 0xc) = func_02019910(
            U32_AT((u8 *)PTR_AT(p, 0) + ((u8 *)PTR_AT(in, 0x10))[note] * 0x24, 0x14),
            ((u32 *)PTR_AT(data_020bde54, 0))[((u8 *)PTR_AT(in, 0xc))[note]]);
        return;
    }
    case 8:
        t[0x22] |= 0x10;
        if (t[4] & 0xf)
            t[0x15] = (t[4] & 0xf) << 2;
        if (t[4] & 0xf0)
            t[0x14] = (t[4] >> 4) << 2;
        return;
    case 9:
        t[0x22] |= 0x10;
        if (e != NULL)
            e[0x10] = 0;
        return;
    case 10:
        t[0x22] |= 0x10;
        MEMORY(t, 0x1c);
        if (e == NULL)
            return;
        e[0x10] = (PLAYER()[0x2f] & 0x10) ? 0 : 2;
        return;
    case 0xb:
        t[0x22] |= 0x10;
        return;
    case 0xd:
        x = t[4];
        if (x > 0x40)
            x = 0x40;
        t[6] = x;
        if (e != NULL)
            MARK(e, 2);
        return;
    case 0xe:
        MEMORY(t, 0x18);
        if (t[4] == 0)
            return;
        r = func_020235bc(t + 6, t[4], 0x40);
        t[0x22] |= r;
        if (e != NULL)
            MARK(e, 2);
        return;
    case 0xf: {
        u8 *c;
        MEMORY(t, 0x12);
        if (!(t[0x22] & 1))
            return;
        if (t[8] == 0xff)
            return;
        c = data_020e2314 + t[8] * 0x18;
        U32_AT(c, 0) = U32_AT(c, 0) + (U16_AT(t, 0x12) << 8);
        if (U32_AT(c, 0) < U32_AT(c, 0x10))
            return;
        if (U32_AT(c, 0x14) == 0)
            U32_AT(c, 0) = 0;
        else
            U32_AT(c, 0) = U32_AT(c, 0x10) - U32_AT(c, 0x14);
        return;
    }
    case 0x10:
        MEMORY(t, 0x19);
        if (t[4] == 0)
            return;
        r = func_020235bc(t + 7, t[4], 0x40);
        t[0x22] |= r;
        if (e == NULL)
            return;
        e[7] = t[7];
        MARK(e, 2);
        return;
    case 0x11:
        t[0x22] = 0x10;
        MEMORY(t, 0x20);
        if (t[4] == 0)
            return;
        if (e != NULL)
            e[0x10] = t[4] & 0xf;
        return;
    case 0x12:
        t[0x22] = 0x10;
        if (t[4] & 0xf)
            t[0x16] = (t[0x16] & 0xf0) | (t[4] & 0xf);
        if (t[4] & 0xf0)
            t[0x16] = (t[0x16] & 0xf) | (t[4] & 0xf0);
        return;
    case 0x13:
        MEMORY(t, 0x1a);
        x = t[4];
        switch (x >> 4) {
        case 3:
            if ((s32)(x & 0xf) < 4)
                t[0x21] = (t[0x21] & ~3) | (x & 3);
            return;
        case 4:
            if ((s32)(x & 0xf) < 4)
                t[0x21] = (t[0x21] & ~0xc) | ((x & 3) << 2);
            return;
        case 5:
            if ((s32)(x & 0xf) < 4)
                t[0x21] = (t[0x21] & ~0x30) | ((x & 3) << 4);
            return;
        case 6:
            PLAYER()[0x37] = x & 0xf;
            return;
        case 7:
            switch (x & 0xf) {
            case 0:
            case 1:
            case 2: {
                u8 modes[3];
                s32 j;
                modes[0] = data_020bde48[0];
                modes[1] = data_020bde48[1];
                modes[2] = data_020bde48[2];
                for (j = D[0x10] - 1; j >= 0; j--) {
                    if (ti == PLAYER()[0x946 + j * 0x20])
                        func_02023d60(j & 0xff, modes[t[4] & 0xf]);
                }
                return;
            }
            case 3:
            case 4:
            case 5:
            case 6:
                if (e == NULL)
                    return;
                e[0x11] = (e[0x11] & ~0xc) | ((((x & 0xf) - 3) & 3) << 2);
                return;
            case 7:
                if (e != NULL)
                    e[0xb] &= ~8;
                return;
            case 8:
                if (e == NULL)
                    return;
                if (PTR_AT((u8 *)PTR_AT(PLAYER(), 4) + t[1] * 0x20, 0x14) != NULL)
                    e[0xb] |= 8;
                return;
            }
            return;
        case 8:
            t[7] = (x & 0xf) << 2;
            if (e == NULL)
                return;
            e[7] = t[7];
            MARK(e, 2);
            return;
        case 0xa:
            t[0x13] = x & 0xf;
            return;
        case 0xb:
            if ((x & 0xf) == 0) {
                p = PLAYER();
                t[9] = *(u8 *)((void **)PTR_AT(p, 0xc))[((u8 *)PTR_AT(p, 8))[p[0x2b]]] - U16_AT(p, 0x22);
                return;
            }
            if (t[0xa] == 0) {
                t[0xa] = (x & 0xf) + 1;
                return;
            }
            t[0xa] -= 1;
            if (t[0xa] != 0)
                return;
            PLAYER()[0x34] = t[9];
            p = PLAYER();
            p[0x35] = p[0x2b];
            U16_AT(PLAYER(), 0x22) = 0;
            return;
        case 0xc:
            if (e == NULL)
                return;
            e[0x10] = x & 0xf;
            if (e[0x10] == 0) {
                t[4] |= t[0x1f] & 0xf;
                e[0x10] = t[0x1f] & 0xf;
            } else {
                t[0x1f] = (t[0x1f] & ~0xf) | (e[0x10] & 0xf);
            }
            t[0x22] |= 0x10;
            return;
        }
        return;
    case 0x14:
        MEMORY(t, 0x1d);
        if (t[4] < 0x20) {
            t[0x22] |= 0x10;
            return;
        }
        func_02027c94(PLAYER()[0x3b], t[4]);
        return;
    case 0x15:
        t[0x22] |= 0x10;
        if (t[4] & 0xf)
            t[0x15] = (t[4] & 0xf) << 2;
        if (t[4] & 0xf0)
            t[0x14] = t[4] >> 4;
        return;
    case 0x17:
        MEMORY(t, 0x1b);
        r = func_020235bc(PLAYER() + 0x2c, t[4], 0x80);
        t[0x22] |= r;
        func_02023778(2);
        return;
    case 0x18:
        t[7] = (t[4] * 0x10101u) >> 18;
        if (e == NULL)
            return;
        e[7] = t[7];
        MARK(e, 2);
        return;
    case 0x1a:
        t[0x22] |= 0x10;
        func_02026714(4, t[4], 1);
        return;
    }
}

/* 0x02025a08: the volume column on the first tick of a row
 * @difftest $P=ptr:0xd40:4 @020e2ba4:32=$P $S=ptr:0x2400:4 @$P+0:32=$S $I=ptr:0x2000:4 @$P+4:32=$I $M=ptr:0x100 @$I+0x2c:32=$M @$I+0x30:32=$M @$I+0x34:32=0 @$I+0x38:32=0 @$I+0x3c:32=0 $O=ptr:0x100 @$P+8:32=$O @$P+0x26:8=1 @$P+0x3d:8=1 @$P+0x3c:8=int:0:0x80 @$P+0x44:8=pick:0,1,0xff @$P+0x41:8=int:0:0x41 @$P+0x9f5:8=int:0:16 @$P+0xa15:8=int:0:16 @$P+0x3e:8=u8 @$P+0x5e:8=pick:4,5,0x25 pick:0 cases=100 */
void func_02025a08(u32 ti)
{
    u8 *p = PLAYER(), *t = TRACK(p, ti), *e;
    u32 x, f = t[0x22];

    e = t[8] != 0xff ? VOICE(p, t[8]) : NULL;
    if (!(f & 4))
        return;
    x = t[2];
    if (x <= 0x40) {
        t[5] = x;
    } else if (x <= 0x4a) {
        t[5] += x - 0x41;
        if (t[5] > 0x40)
            t[5] = 0x40;
    } else if (x <= 0x54) {
        s32 d = x - 0x4b;
        if ((s32)t[5] > d)
            t[5] -= d;
        else
            t[5] = 0;
    } else if (x <= 0x68) {
        t[0x22] |= 0x20;
        return;
    } else if (x <= 0x72) {
        if (x != 0x69)
            t[0x10] = (x - 0x69) << 2;
        t[0x22] |= 0x20;
        return;
    } else if (x <= 0x7c) {
        if (x != 0x73)
            t[0x10] = (x - 0x73) << 2;
        t[0x22] |= 0x20;
        return;
    } else if (x <= 0xc0) {
        t[7] = x - 0x80;
        if (e == NULL)
            return;
        e[7] = t[7];
        MARK(e, 2);
        return;
    } else if (x <= 0xca) {
        u8 *in;
        if (t[1] == 0)
            return;
        if (t[0] >= 0x78)
            return;
        if (!(f & 1))
            return;
        if (x != 0xc1)
            t[0x11] = data_020b58d0[x - 0xc1];
        p = PLAYER();
        in = (u8 *)PTR_AT(p, 4) + t[1] * 0x20;
        U32_AT(t, 0xc) = func_02019910(
            U32_AT((u8 *)PTR_AT(p, 0) + ((u8 *)PTR_AT(in, 0x10))[t[0]] * 0x24, 0x14),
            ((u32 *)PTR_AT(data_020bde54, 0))[((u8 *)PTR_AT(in, 0xc))[t[0]]]);
        t[0x22] |= 0x20;
        return;
    } else {
        if (x <= 0xd4) {
            if (x != 0xcb)
                t[0x14] = (x - 0xcb) << 2;
            t[0x22] |= 0x20;
        }
        return;
    }
    if (e == NULL)
        return;
    e[6] = t[5];
    MARK(e, 2);
}

/* 0x02025d58: start the note of track `ti` on its voice */
void func_02025d58(u32 ti)
{
    u8 *p = PLAYER(), *t = TRACK(p, ti), *ins = PTR_AT(p, 4), *in, *smp, *e, *c, *s;
    u32 inst = t[1], note = t[0], v = t[8];

    in = ins + inst * 0x20;
    smp = (u8 *)PTR_AT(p, 0) + ((u8 *)PTR_AT(in, 0x10))[note] * 0x24;
    e = VOICE(p, v);
    e[4] = note;
    e[5] = t[1];
    e[6] = t[5];
    e[7] = t[7];
    e[8] = ((u8 *)PTR_AT(in, 0xc))[t[0]];
    e[9] = ((u8 *)PTR_AT(in, 0x10))[t[0]];
    e[0xa] = ti;
    e[0x11] = (e[0x11] & ~0xc) | ((ins[inst * 0x20] & 3) << 2);
    e[0x11] = (e[0x11] & ~3) | 3;
    U32_AT(e, 0) = ((U32_AT(smp, 0x14) >> 1) * (((u32 *)PTR_AT(data_020bde54, 0))[e[8]] >> 5)) >> 10;
    p = PLAYER();
    s = (u8 *)PTR_AT(p, 0) + e[9] * 0x24;
    c = data_020e2314 + t[8] * 0x18;
    U32_AT(c, 0) = U32_AT(s, 0);
    U32_AT(c, 4) = 0;
    if (s[5] & 0x20) {
        U32_AT(c, 0x10) = U32_AT(s, 0) + U32_AT(s, 0x1c);
        U32_AT(c, 0x14) = U32_AT(s, 0x1c) - U32_AT(s, 0x18);
    } else if (!(s[5] & 0x10)) {
        U32_AT(c, 0x10) = U32_AT(s, 0) + U32_AT(s, 8);
        U32_AT(c, 0x14) = 0;
    } else {
        U32_AT(c, 0x10) = U32_AT(s, 0) + U32_AT(s, 0x10);
        U32_AT(c, 0x14) = U32_AT(s, 0x10) - U32_AT(s, 0xc);
    }
}

/* 0x02025f30: the note of a new row on track `ti` (porta: a tone
 * portamento, which keeps the playing voice) */
void func_02025f30(u32 ti, s32 porta)
{
    u8 *p = PLAYER(), *t = TRACK(p, ti), *e = NULL, *in;
    u32 v = t[8], k;
    s32 j;

    if (v != 0xff)
        e = VOICE(p, v);
    in = (u8 *)PTR_AT(p, 4) + t[1] * 0x20;
    if (t[0x22] & 1) {
        u32 note = t[0];
        if (note == 0x7e) {
            func_02023d60(v, 0);
            t[8] = 0xff;
            return;
        }
        if (note == 0x7f) {
            func_02023d60(v, 0x12);
            return;
        }
        if (note < 0x78 && porta == 0) {
            if (e != NULL)
                func_02023d60(v, (e[0x11] >> 2) & 3);
            if (t[6] != 0) {
                switch (in[1]) {
                case 1: {
                    u32 rel = ((u8 *)PTR_AT(in, 0xc))[t[0]];
                    for (j = D[0x10] - 1; j >= 0; j--) {
                        u8 *q = PLAYER() + j * 0x20;
                        if (ti == q[0x946] && rel == q[0x944])
                            func_02023d60(j & 0xff, in[2]);
                    }
                    break;
                }
                case 2: {
                    u32 si = ((u8 *)PTR_AT(in, 0x10))[t[0]];
                    for (j = D[0x10] - 1; j >= 0; j--) {
                        u8 *q = PLAYER() + j * 0x20;
                        if (ti == q[0x946] && si == q[0x945])
                            func_02023d60(j & 0xff, in[2]);
                    }
                    break;
                }
                case 3:
                    for (j = D[0x10] - 1; j >= 0; j--) {
                        u8 *q = PLAYER() + j * 0x20;
                        if (ti == q[0x946] && t[1] == q[0x941])
                            func_02023d60(j & 0xff, in[2]);
                    }
                    break;
                }
                t[8] = func_02027810();
                if (t[8] != 0xff)
                    data_020e2b70[t[8]] = PLAYER()[0x3b];
            } else {
                t[8] = 0xff;
            }
            e = t[8] == 0xff ? NULL : VOICE(PLAYER(), t[8]);
        }
    }
    if (!(t[0x22] & 3))
        return;
    if (t[1] == 0)
        return;
    t[5] = ((u8 *)PTR_AT(PLAYER(), 0) + ((u8 *)PTR_AT(in, 0x10))[t[0]] * 0x24)[6];
    if (t[0] < 0x78 && !(in[9] & 0x80))
        t[7] = in[9];
    if (t[8] == 0xff)
        return;
    if (porta == 0) {
        func_02022ecc(t[8]);
        func_02025d58(ti);
        for (k = 0; k < 3; k++) {
            u8 *env = PTR_AT(in, 0x14 + k * 4);
            if (env != NULL && (env[8] & 1))
                e[0xb] |= 8 << k;
        }
        return;
    }
    e[6] = t[5];
    e[7] = t[7];
    MARK(e, 2);
}

/* 0x0202627c: read the next row of the current pattern
 * @difftest $P=ptr:0xd40:4 @020e2ba4:32=$P $S=ptr:0x2400:4 @$P+0:32=$S $I=ptr:0x2000:4 @$P+4:32=$I $M=ptr:0x100 @$I+0x2c:32=$M @$I+0x30:32=$M @$I+0x34:32=0 @$I+0x38:32=0 @$I+0x3c:32=0 $O=ptr:0x100 @$P+8:32=$O @$P+0x26:8=1 @$P+0x3d:8=1 @$P+0x3c:8=int:0:0x80 @$P+0x44:8=pick:0,1,0xff @$P+0x41:8=int:0:0x41 @$P+0x9f5:8=int:0:16 @$P+0xa15:8=int:0:16 $R=zero:8 @$R+0:8=0x80 @$R+1:8=int:0:0x78 @$R+2:8=pick:0,1 @$P+0x14:32=$R cases=60
 * @difftest $P=ptr:0xd40:4 @020e2ba4:32=$P $S=ptr:0x2400:4 @$P+0:32=$S $I=ptr:0x2000:4 @$P+4:32=$I $M=ptr:0x100 @$I+0x2c:32=$M @$I+0x30:32=$M @$I+0x34:32=0 @$I+0x38:32=0 @$I+0x3c:32=0 $O=ptr:0x100 @$P+8:32=$O @$P+0x26:8=1 @$P+0x3d:8=1 @$P+0x3c:8=int:0:0x80 @$P+0x44:8=pick:0,1,0xff @$P+0x41:8=int:0:0x41 @$P+0x9f5:8=int:0:16 @$P+0xa15:8=int:0:16 $R=zero:8 @$R+0:8=0x40 @$R+1:8=u8 @$P+0x14:32=$R cases=60
 * @difftest $P=ptr:0xd40:4 @020e2ba4:32=$P $S=ptr:0x2400:4 @$P+0:32=$S $I=ptr:0x2000:4 @$P+4:32=$I $M=ptr:0x100 @$I+0x2c:32=$M @$I+0x30:32=$M @$I+0x34:32=0 @$I+0x38:32=0 @$I+0x3c:32=0 $O=ptr:0x100 @$P+8:32=$O @$P+0x26:8=1 @$P+0x3d:8=1 @$P+0x3c:8=int:0:0x80 @$P+0x44:8=pick:0,1,0xff @$P+0x41:8=int:0:0x41 @$P+0x9f5:8=int:0:16 @$P+0xa15:8=int:0:16 $R=zero:8 @$R+0:8=0xc0 @$R+1:8=int:0:0x20 @$R+2:8=u8 @$P+0x14:32=$R cases=60
 * @difftest $P=ptr:0xd40:4 @020e2ba4:32=$P $S=ptr:0x2400:4 @$P+0:32=$S $I=ptr:0x2000:4 @$P+4:32=$I $M=ptr:0x100 @$I+0x2c:32=$M @$I+0x30:32=$M @$I+0x34:32=0 @$I+0x38:32=0 @$I+0x3c:32=0 $O=ptr:0x100 @$P+8:32=$O @$P+0x26:8=1 @$P+0x3d:8=1 @$P+0x3c:8=int:0:0x80 @$P+0x44:8=pick:0,1,0xff @$P+0x41:8=int:0:0x41 @$P+0x9f5:8=int:0:16 @$P+0xa15:8=int:0:16 $R=zero:8 @$R+0:8=0xc0 @$R+1:8=pick:0xa0,0xb3,0x87,0x93 @$R+2:8=u8 @$R+3:8=int:0:0x78 @$P+0x14:32=$R cases=60 */
void func_0202627c(void)
{
    s32 i;
    u8 *rd;

    for (i = PLAYER()[0x26] - 1; i >= 0; i--)
        PLAYER()[i * 0x24 + 0x5e] = 0;
    rd = PTR_AT(PLAYER(), 0x14);
    while (*rd != 0) {
        u32 b, ti, note = 0xff, ins = 0xff, vol = 0xff, fx = 0xff, par = 0xff, delayed = 0;
        u8 *t;

        PTR_AT(PLAYER(), 0x14) = rd + 1;
        b = *rd;
        ti = b & 0x3f;
        t = TRACK(PLAYER(), ti);
        switch (b >> 6) {
        case 1:
            rd = PTR_AT(PLAYER(), 0x14);
            PTR_AT(PLAYER(), 0x14) = rd + 1;
            vol = *rd;
            break;
        case 2: {
            u32 x;
            rd = PTR_AT(PLAYER(), 0x14);
            PTR_AT(PLAYER(), 0x14) = rd + 1;
            note = *rd;
            x = *(u8 *)PTR_AT(PLAYER(), 0x14);
            if (x != 0)
                ins = x;
            PTR_AT(PLAYER(), 0x14) = (u8 *)PTR_AT(PLAYER(), 0x14) + 1;
            if (note & 0x80) {
                note &= 0x7f;
                rd = PTR_AT(PLAYER(), 0x14);
                PTR_AT(PLAYER(), 0x14) = rd + 1;
                vol = *rd;
            }
            break;
        }
        case 3: {
            u32 f;
            rd = PTR_AT(PLAYER(), 0x14);
            PTR_AT(PLAYER(), 0x14) = rd + 1;
            f = *rd;
            rd = PTR_AT(PLAYER(), 0x14);
            PTR_AT(PLAYER(), 0x14) = rd + 1;
            par = *rd;
            if (f & 0x20) {
                rd = PTR_AT(PLAYER(), 0x14);
                PTR_AT(PLAYER(), 0x14) = rd + 1;
                note = *rd;
            }
            if (f & 0x40) {
                rd = PTR_AT(PLAYER(), 0x14);
                PTR_AT(PLAYER(), 0x14) = rd + 1;
                ins = *rd;
            }
            if (f & 0x80) {
                rd = PTR_AT(PLAYER(), 0x14);
                PTR_AT(PLAYER(), 0x14) = rd + 1;
                vol = *rd;
            }
            fx = f & 0x1f;
            break;
        }
        }
        if (ins != 0xff) {
            t[1] = ins;
            t[0x22] |= 2;
        }
        if (note != 0xff) {
            t[0] = note;
            t[0x22] |= 1;
        }
        if (vol != 0xff) {
            t[2] = vol;
            t[0x22] |= 4;
        }
        if (fx != 0xff) {
            t[3] = fx;
            t[4] = par;
            t[0x22] |= 8;
        }
        if ((t[0x22] & 8) && t[3] == 0x13 && (t[4] >> 4) == 0xd) {
            if ((t[4] & 0xf) == 0)
                t[4] |= t[0x1f] >> 4;
            else
                t[0x1f] = (t[0x1f] & ~0xf0) | ((t[4] & 0xf) << 4);
            if (t[4] & 0xf) {
                t[0x22] |= 0x10;
                delayed = 1;
            }
        }
        if (!delayed) {
            func_02025f30(ti & 0xff, (fx == 7 || (vol >= 0xc1 && vol <= 0xca)) ? 1 : 0);
            func_02025a08(ti & 0xff);
            func_02024c0c(ti & 0xff);
        }
        rd = PTR_AT(PLAYER(), 0x14);
    }
    PTR_AT(PLAYER(), 0x14) = rd + 1;
}

/* 0x020265c8: go to the next entry of the order list (skipping 0xfe
 * markers, looping or stopping at the end); 1 when the song ended
 * @difftest $P=ptr:0xd40:4 @020e2ba4:32=$P $S=ptr:0x2400:4 @$P+0:32=$S $I=ptr:0x2000:4 @$P+4:32=$I $M=ptr:0x100 @$I+0x2c:32=$M @$I+0x30:32=$M @$I+0x34:32=0 @$I+0x38:32=0 @$I+0x3c:32=0 $O=ptr:0x100 @$P+8:32=$O @$P+0x26:8=1 @$P+0x3d:8=1 @$P+0x3c:8=int:0:0x80 @$P+0x44:8=pick:0,1,0xff @$P+0x41:8=int:0:0x41 @$P+0x9f5:8=int:0:16 @$P+0xa15:8=int:0:16 @$P+0x35:8=int:0:8 @$P+0x30:8=int:0:8 @$P+0x29:8=pick:0,1 @$P+0x3b:8=pick:0,2 @$O+0:8=pick:0xfe,0xff,1 @$O+1:8=pick:0xfe,0xff,2 @$O+2:8=pick:0xfe,0xff,3 cases=100 */
u32 func_020265c8(void)
{
    u8 *p = PLAYER();
    u32 len, pos;

    p[0x2b] = p[0x35];
    for (;;) {
        p = PLAYER();
        len = p[0x30];
        pos = p[0x2b];
        if (pos < len && ((u8 *)PTR_AT(p, 8))[pos] == 0xfe) {
            p[0x2b] = pos + 1;
            func_02026714(5, 0, 1);
            continue;
        }
        break;
    }
    if (pos >= len || ((u8 *)PTR_AT(p, 8))[pos] == 0xff) {
        if (p[0x29] & 1) {
            p[0x2b] = 0;
            func_02026714(1, 0, 1);
        } else {
            func_02023648();
            PLAYER()[0x28] = 0;
            func_02026714(0, PLAYER()[0x3b], 1);
            if (PLAYER()[0x3b] == 1) {
                func_020281bc(0);
                PTR_AT(data_020e2ba4, 0) = NULL;
            }
            return 1;
        }
    }
    p = PLAYER();
    func_02027cec(p[0x3b], p[0x2b], p[0x34]);
    p = PLAYER();
    p[0x35] = p[0x2b] + 1;
    PLAYER()[0x34] = 0;
    return 0;
}

/* 0x02026714: call the player's callback (the player being run is kept) */
void func_02026714(u32 ev, u32 a, s32 b)
{
    void (*cb)(u32, u32, s32) = PTR_AT(data_020e2ba8, 0);
    u32 save;

    if (cb == NULL)
        return;
    save = U32_AT(data_020e2ba4, 0);
    cb(ev, a, (s8)b);
    U32_AT(data_020e2ba4, 0) = save;
}
