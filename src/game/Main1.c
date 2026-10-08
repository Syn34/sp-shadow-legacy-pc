/*
 * NitroMain: the compressed string table, the VBlank interrupt handler
 * (OAM double buffering, per-screen updates, frame counters), frame
 * pacing, the current screen, and leaving a map ("mapExitNormal").
 * ARM9 main, 0x02017b58 - 0x020186fc (31 functions).
 *
 * data_020df0b4 points to the frame state (data_020df0bc):
 *   +0x00 VBlank callback, +0x08 second callback, +0x1c VBlank counter,
 *   +0x20 counter at the last frame, +0x2c/+0x30/+0x34 per-second counters,
 *   +0x38 "screen updated" flag per screen.
 * data_020df598 holds two 0x13ec-byte OAM buffers per screen
 *   (+0 current, +4 previous, +8 and +0x808 the two 0x400-byte copies).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define REG16(a) (*(vu16 *)(a))
#define REG32(a) (*(vu32 *)(a))

extern const char data_020bd428[], data_020bd43c[], data_020bd44c[];
extern const char data_020bd648[], data_020bd658[], data_020bd684[], data_020bd6a8[],
    data_020bd6c8[], data_020bd6f0[];
extern u8 data_020bd638;
extern u32 data_020bd644;
extern u8 *data_020c3b60;            /* levels, 0x60 bytes each */
extern u8 data_020def18[];
extern u8 *data_020df0a4;            /* string Huffman tree */
extern u8 *data_020df0a8;            /* string table */
extern u32 *data_020df0ac;           /* string offsets */
extern u32 data_020df0b0;            /* OAM buffer being shown */
extern u8 *data_020df0b4;            /* frame state */
extern s32 data_020df0b8;            /* VBlank nesting */
extern u8 data_020df0bc[];
extern u32 data_020df0f8;            /* VBlanks per frame */
extern s8 data_020df108;
extern s8 data_020df110;
extern s8 data_020df118;
extern s8 data_020df11c;
extern s8 data_020df120;
extern s8 data_020df124;
extern u32 data_020df128;
extern u32 data_020df12c;            /* map mode */
extern s8 data_020df17c;
extern u8 data_020df598[];           /* OAM buffers */
extern u8 data_020e1f50[];
extern void *data_020f6f40;
extern u8 data_027c0000[];           /* DTCM */

void VBlankIntrWait(void);
void func_020043c8(struct PtrVec *list);
void func_02004864(void);
void func_020099fc(void);
void func_0200b2fc(void);
void func_0200b88c(void);
void func_0200b93c(void);
void *func_02009394(void);
void func_0201146c(void);
u8 *func_02011f34(void);
void func_020121c0(void);
void func_02013b78(u8 *p, u32 v);
void func_02018bc8(void);
void func_02018d8c(void);
void func_02019260(void);
void *func_02019670(void);
s32 func_02019b7c(u32 screen);
void func_02019bf8(void);
void func_0201a190(void);
void func_0201a5c4(void);
void func_0201d19c(void);
void func_0201d248(void);
void func_0201f998(void);
void func_02020108(void);
void func_02020194(void);
void func_02020390(void);
void func_020205c0(void);
void func_02028d24(void);
void func_0203fc9c(void);
void func_02042480(u32 v);
void func_0204378c(u32 screen);
s32 func_020437fc(void);
void func_0205f4ec(void *snd);
void func_0205fb28(void *snd);
void func_02072f2c(void *hud, u32 v);
void func_02072ff0(void *hud);
void func_02082e10(void *obj, u32 a, u32 b);
void func_02082f64(void *obj);
void func_02082fb8(void *obj, u32 a, u32 b);
void func_02083044(void *obj, u32 a, u32 b, u32 c);
void func_0208d6d8(void);
void func_0208f8bc(void);
void func_0208f960(u32 v);
void func_02091cc0(u32 mask);        /* OS_EnableIrqMask */
void func_02091e50(u32 mask, void (*fn)(void));   /* OS_SetIrqFunction */

/* this file's own functions, called before their definition */
void func_02017ccc(void);
void func_02017cd0(void);
void func_02017fcc(void);
void func_0201817c(u32 screen);
void *func_02018208(void);
void func_02018218(void);
void func_02018044(void (*fn)(void));
void func_020185fc(u32 mode);
void func_020186fc(u32 mode);

static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

/* 0x02017b58
 * @difftest */
void func_02017b58(void) {}

/* 0x02017b5c: decode string `index` into out (at most maxlen bytes);
 * 0 done, 1 no table, 2 buffer full
 * @difftest $D=ptr:0x40 $O=ptr:8:4 @$O+0:32=pick:0,1,2 @$O+4:32=pick:0,3 $T=ptr:0x10:2 @$T+0:16=pick:0x41,0x101,0 @$T+2:16=pick:0x42,0x101,0xf1 @$T+4:16=pick:0x43,0,0x44 @$T+6:16=pick:0x45,0x100,0 @020df0a8:32=$D @020df0ac:32=$O @020df0a4:32=$T int:0:1 ptr:0x48 int:1:0x40 cases=200
 * @difftest @020df0a8:32=pick:0 u32 ptr:8 u32 cases=5 */
s32 func_02017b5c(u32 index, u8 *out, u32 maxlen)
{
    const u8 *p;
    u32 bits, bit = 0, n = 0, code = 0, two = 0;

    if (data_020df0a8 == NULL)
        return 1;
    p = data_020df0a8 + data_020df0ac[index];
    bits = *p++;
    do {
        const u8 *tree = data_020df0a4;
        u32 node = 0x100, c;
        do {
            if ((bits >> bit) & 1)
                node = U16_AT(tree, node * 4 - 0x3fe);
            else
                node = U16_AT(tree, node * 4 - 0x400);
            bit++;
            if (bit == 8) {
                bit = 0;
                bits = *p++;
            }
        } while (node >= 0x100);
        c = node & 0xff;
        out[n++] = c;
        if (c >= 0xf0 && two == 0) {
            two = 1;
            code = c << 8;
        } else if (two != 0) {
            code |= node & 0xff;
            two = 0;
        } else {
            code = node & 0xff;
        }
        if (out[n - 1] != 0 && n == maxlen)
            return 2;
    } while (code != 0);
    return 0;
}

/* 0x02017c94: use a string table
 * @difftest $B=ptr:0x20:4 @$B+0:32=int:4:0x18 $B */
s32 func_02017c94(u8 *blob)
{
    data_020df0a8 = blob;
    data_020df0a4 = blob + 4;
    data_020df0ac = (u32 *)(blob + U32_AT(blob, 0));
    return 0;
}

/* 0x02017ccc: default callback
 * @difftest */
void func_02017ccc(void) {}

static inline u8 *Oam(u32 scr) { return data_020df598 + scr * 0x13ec; }

/* 0x02017cd0: VBlank interrupt handler */
void func_02017cd0(void)
{
    s32 old = data_020df0b8;
    data_020df0b8 = old + 1;
    if (old == 0) {
        u32 saved = data_020df0fc, scr;
        void (*cb)(void);
        s32 i;

        data_020df0b0 ^= 1;
        for (scr = 0; scr < 2; scr++) {
            u8 *s;
            func_0201817c(scr);
            if (data_020df0b4[data_020df0fc + 0x38] != 0) {
                func_0201a5c4();
                s = Oam(data_020df0fc);
                if (PTR_AT(s, 4) == s + 8)
                    func_0207fed8(0, (void *)(0x07000000 + (data_020df0fc << 10)),
                                  s + 0x808 + (data_020df0b0 << 10), 0x400, 0x20);
                else
                    func_0207fed8(0, (void *)(0x07000000 + (data_020df0fc << 10)),
                                  s + 8 + (data_020df0b0 << 10), 0x400, 0x20);
                Oam(data_020df0fc)[0x13e9] = 0;
                func_0200b88c();
                s = Oam(data_020df0fc);
                PTR_AT(s, 4) = PTR_AT(s, 0);
                if (PTR_AT(s, 0) == s + 8)
                    PTR_AT(Oam(data_020df0fc), 0) = Oam(data_020df0fc) + 0x808;
                else
                    PTR_AT(Oam(data_020df0fc), 0) = Oam(data_020df0fc) + 8;
            } else {
                s = Oam(data_020df0fc);
                if (PTR_AT(s, 4) == s + 8)
                    func_0207fed8(0, (void *)(0x07000000 + (data_020df0fc << 10)),
                                  s + 8 + (data_020df0b0 << 10), 0x400, 0x20);
                else
                    func_0207fed8(0, (void *)(0x07000000 + (data_020df0fc << 10)),
                                  s + 0x808 + (data_020df0b0 << 10), 0x400, 0x20);
            }
        }
        cb = PTR_AT(data_020df0b4, 0);
        if (cb != NULL)
            cb();
        for (i = 0; i < 2; i++) {
            u8 *st;
            func_02028d24();
            func_0201817c(i);
            func_020099fc();
            func_0200b93c();
            st = data_020df0b4 + data_020df0fc;
            if (st[0x38] != 0)
                st[0x38] = 0;
        }
        U32_AT(data_020df0b4, 0x1c) += 1;
        U32_AT(data_020df0b4, 0x2c) += 1;
        if (U32_AT(data_020df0b4, 0x2c) >= 0x3c) {
            U32_AT(data_020df0b4, 0x34) = U32_AT(data_020df0b4, 0x30);
            U32_AT(data_020df0b4, 0x30) = 0;
            U32_AT(data_020df0b4, 0x2c) = U32_AT(data_020df0b4, 0x30);
        }
        func_0208d6d8();
        func_0201817c(saved);
    }
    data_020df0b8 -= 1;
    U32_AT(data_027c0000, 0x3ff8) |= 1;   /* OS_SetIrqCheckFlag(VBLANK) */
    (void)REG16(0x04000208);
    REG16(0x04000208) = 1;
    func_020205c0();
}

/* 0x02017fcc */
void func_02017fcc(void) { func_02004864(); }

/* 0x02017fd8: enable the V-count interrupt at line v */
void func_02017fd8(u32 v)
{
    func_02091cc0(4);
    func_0208f960(v);
    REG16(0x04000004) |= 0x20;
}

/* 0x0201800c */
void func_0201800c(void) { VBlankIntrWait(); }

/* 0x02018018: set the second callback (NULL: none)
 * @difftest $S=ptr:0x40:4 @020df0b4:32=$S pick:0,0x02001000 */
void func_02018018(void (*fn)(void))
{
    if (fn == NULL)
        PTR_AT(data_020df0b4, 8) = (void *)func_02017ccc;
    else
        PTR_AT(data_020df0b4, 8) = (void *)fn;
}

/* 0x02018044: set the VBlank callback (NULL: none)
 * @difftest $S=ptr:0x40:4 @020df0b4:32=$S pick:0,0x02001000 */
void func_02018044(void (*fn)(void))
{
    if (fn == NULL)
        PTR_AT(data_020df0b4, 0) = (void *)func_02017ccc;
    else
        PTR_AT(data_020df0b4, 0) = (void *)fn;
}

/* 0x02018070: install the VBlank interrupt */
void func_02018070(void)
{
    func_02091e50(1, func_02017cd0);
    func_02018044(func_02017fcc);
    func_02091cc0(1);
    (void)REG16(0x04000208);
    REG16(0x04000208) = 1;
    REG16(0x04000004) = 8;
}

/* 0x020180cc
 * @difftest */
void func_020180cc(void) { data_020df0b4 = data_020df0bc; }

/* 0x020180e4: debug output, then switch the sub display on */
void func_020180e4(void)
{
    func_02004490(data_020bd428, U32_AT(data_020df0b4, 0x1c));
    func_02004490(data_020bd43c, REG32(0x04000000));
    func_02004490(data_020bd44c, REG16(0x04000004));
    func_0208f8bc();
    REG32(0x04001000) |= 0x10000;
}

/* 0x02018158: switch to the other screen
 * @difftest @020df0fc:32=int:0:2 */
void func_02018158(void)
{
    if (data_020df0fc == 0)
        data_020df0fc = 1;
    else
        data_020df0fc = 0;
}

/* 0x0201817c: set the current screen
 * @difftest int:0:1 */
void func_0201817c(u32 screen) { data_020df0fc = screen; }

/* 0x0201818c: wait for the end of the frame */
void func_0201818c(void)
{
    u32 saved = data_020df0fc, now;
    func_02019bf8();
    do {
        VBlankIntrWait();
        now = U32_AT(data_020df0b4, 0x1c);
    } while (now - U32_AT(data_020df0b4, 0x20) < data_020df0f8);
    U32_AT(data_020df0b4, 0x20) = now;
    func_0201817c(saved);
}

/* 0x020181e4: is the main screen current
 * @difftest @020df0fc:32=int:0:2 */
s32 func_020181e4(void) { return (s8)(data_020df0fc == 0); }

/* 0x02018208
 * @difftest */
void *func_02018208(void) { return data_020f6f40; }

/* 0x02018218: leave the map ("mapExitNormal") */
void func_02018218(void)
{
    if (data_020df118 == 0 && func_02019b7c(data_020df0fc) != 0x25)
        func_02042480(1);
    func_0205fb28(func_02009394());
    func_02072ff0(func_02011f34());
    VCallR1(func_020062b0(), 0xc);
    VCallR1((u8 *)func_020062c0() + 4, 0x10);
    VCallR1((u8 *)func_02019670() + 4, 0x10);
    VCallR1(func_020062a0(), 0xc);
    func_0203fc9c();
    func_02004490(data_020bd648);
    func_0205f4ec(func_02009394());
    func_02004490(data_020bd658);
    func_0201a190();
    func_02004490(data_020bd684);
    func_02020108();
    func_02004490(data_020bd6a8);
    func_0201146c();
    func_020121c0();
    U32_AT(PTR_AT(data_020deebc, 4), 0x110) &= ~0x80u;
    func_020043c8(func_02006200());
    func_02004490(data_020bd6c8);
    func_020043c8(func_020061e0());
    func_02004490(data_020bd6f0);
    func_0201f998();
    func_02013b78((u8 *)data_020deebc, 0);
    func_02013b78(data_020def18, 0);
    data_020df17c = 0;
}

/* 0x02018370: the current level's start position
 * @difftest ptr:8:4 */
void func_02018370(s32 *out)
{
    out[0] = S32_AT(data_020c3b60 + (func_020436f0(data_020bf6a0)[0xf] >> 1) * 0x60, 0x30);
    out[1] = S32_AT(data_020c3b60 + (func_020436f0(data_020bf6a0)[0xf] >> 1) * 0x60, 0x34);
}

/* 0x020183e8 */
void func_020183e8(void)
{
    u32 saved = data_020df0fc, i;
    for (i = 0; i < 2; i++) {
        data_020df0fc = i;
        if ((s8)data_020e1f50[5] != 0 && i == 0)
            func_0201d248();
    }
    data_020df0fc = saved;
    func_02004864();
    func_0200b2fc();
}

/* 0x0201844c
 * @difftest */
void func_0201844c(void) { data_020df124 = 1; }

/* 0x02018460: leave the map for another one */
void func_02018460(void)
{
    data_020df120 = 1;
    data_020bd638 = 1;
    func_02020194();
    if (func_020437fc() != 0) {
        func_02072f2c(func_02011f34(), 0);
        func_0204378c(data_020df0fc);
        func_02020390();
    }
    func_02019260();
    func_02018218();
    func_02082e10(func_02018208(), 0, 0);
    func_02082f64(func_02018208());
}

/* 0x020184e0: store the next level in the save, then start it */
void func_020184e0(void)
{
    u32 lvl = data_020df128;
    u8 *st = func_020436f0(data_020bf6a0), b;
    st[0xf] = (st[0xf] & 1) | ((lvl & 0xff & 0x7f) << 1);
    b = data_020df108;
    func_020436f0(data_020bf6a0)[0x10] = b;
    data_020df120 = 0;
    data_020bd638 = 1;
    func_02082fb8(func_02018208(), 0x100, 0x10);
    func_02083044(func_02018208(), 8, 0x20, 0x200);
    func_02018bc8();
}

/* 0x02018588: apply a pending map mode change
 * @difftest @020bd644:32=pick:0,1 @020df12c:32=pick:0,1 cases=40 */
void func_02018588(void)
{
    if (data_020bd644 != data_020df12c)
        func_020185fc(data_020bd644);
    data_020df11c = 0;
}

/* 0x020185d0: remember the map mode
 * @difftest */
void func_020185d0(void)
{
    data_020df11c = 1;
    data_020bd644 = data_020df12c;
}

/* 0x020185fc: change the map mode
 * @difftest @020df12c:32=pick:0,1 pick:0,1,2 cases=40 */
void func_020185fc(u32 mode)
{
    u8 *st;
    if (mode == data_020df12c)
        return;
    if (mode == 1) {
        st = func_020436f0(data_020bf6a0);
        st[0xf] = (st[0xf] & ~1) | 1;
        data_020df12c = 1;
    } else {
        st = func_020436f0(data_020bf6a0);
        st[0xf] &= ~1;
        data_020df12c = 0;
    }
    func_0201d19c();
    func_0201d248();
    func_020186fc(data_020df12c);
    func_02018d8c();
    data_020df110 = 1;
}

/* 0x020186b8
 * @difftest */
void func_020186b8(void) { data_020df110 = 0; }

/* 0x020186cc
 * @difftest */
s32 func_020186cc(void) { return data_020df110; }

/* 0x020186dc
 * @difftest u32 */
void func_020186dc(u32 v) { data_020df12c = v; }

/* 0x020186ec: map mode
 * @difftest */
u32 func_020186ec(void) { return data_020df12c; }
