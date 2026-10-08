/*
 * The game's sound interface: music and sound effects by number, volumes,
 * pausing, and the lock that keeps the sound update (run from an alarm)
 * from running while the game is talking to the sound system.
 * Also the sounds played directly on hardware channels (the channel timer
 * comes from the hardware divider).
 * ARM9 main, 0x0201ff6c - 0x02020b9c (34 functions).
 *
 * data_020e2308 (0xc bytes): +0x00 initialised, +0x04 a value from the
 * sound callback, +0x09 effect volume, +0x0a the music playing (0xff none),
 * +0x0b flags: 0x01 music paused, 0x02 music to restart, 0x04 callback
 * value set, 0x08 updates off, 0x10 fading, 0x20 sound off, 0x40 update
 * missed while locked, 0x80 locked.
 * data_020ebbb8 holds the saved options (+0x08/+0x09 volumes).
 * data_020e26c0 holds 0x38-byte direct sounds: +0x00 the samples, +0x04 and
 * +0x06 volumes, +0x05 format flags (0x02 16-bit, 0x08 ADPCM, 0x10 loop),
 * +0x07 pan, +0x0a hardware channel, +0x0b flags (bit 0 on a hardware
 * channel, bits 1-7 state), +0x0c length, +0x10/+0x14 loop start/end,
 * +0x18 sample rate. data_020e2614 + 0x58d is the master volume.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define REG16(a) (*(vu16 *)(a))
#define REG32(a) (*(vu32 *)(a))
#define REG_DIVCNT 0x04000280
#define REG_DIV_NUMER 0x04000290
#define REG_DIV_DENOM 0x04000298
#define REG_DIV_RESULT 0x040002a0

typedef struct {
    u32 w[3];
} SePan;                             /* 12-byte argument passed by value */

extern u8 data_020b5808[];           /* reverb settings (u32 each) */
extern u8 *data_020c3b50;            /* sound effects, 12 bytes each */
extern u8 *data_020c3b5c;            /* music, u32 each */
extern u8 data_020e2308[];
extern u8 data_020e2314[];           /* software channels, 0x18 bytes each */
extern u8 data_020e231c[];
extern u8 data_020e26c0[];           /* direct sounds, 0x38 bytes each */
extern u8 data_020e2614[];
extern u8 data_020ebbb8[];

u32 func_020211e8(s32 i);
void func_020213e0(s32 handle, s32 v);
u32 func_02021514(u32 h);
u32 func_02021540(u32 h);
void func_0202159c(void);
void func_02021708(void);
void func_02021890(u32 handle, s32 v);
s32 func_020219d0(u32 id, u32 a, SePan pan);
void func_02022640(u32 v);
void func_0202265c(u32 v);
void func_0202268c(u32 v);
void func_0202286c(void);
void func_02022a6c(void);
void func_02022bdc(void);
void func_02022d18(void);
void func_02022d94(void);
void func_02027918(u32 a, u32 b, u32 c, u32 d);
void func_02027ec4(u32 v);
void func_020281bc(u32 v);
void func_02028330(u32 v);
void func_02028438(u32 a, u32 id, u32 b);
void func_020285cc(void);
void func_02028628(u32 v);
s32 func_02095a50(u32 v);
s32 func_02095cd4(u32 v);
void func_020953d4(u32 ch, u32 fmt, const void *data, u32 loop, u32 start, u32 len, s32 vol,
                   u32 shift, u32 timer, s32 pan);   /* SND_SetupChannelPcm */
void func_0209542c(u32 mask, s32 pan);
void func_02095458(u32 mask, s32 vol, u32 shift);
void func_0209548c(u32 mask, u32 timer);
void func_02095930(u32 v);
void func_02021c1c(void *a, void *b, u32 len);
void func_02022218(void *a);
void func_0209820c(void *lock);
void func_02098278(void *lock);

void func_0201ff6c(void);
void func_0201ffd0(void);
void func_020200cc(s32 handle, s32 v);
void func_02020194(void);
void func_020204d8(u32 v);
void func_02020504(u32 v);
void func_02020530(u32 v);
void func_0202055c(s32 i);
void func_02020584(SePan *out);
void func_0202068c(void);
void func_020206c8(void);

#define S data_020e2308
#define FLAGS (S[0xb])

/* 0x0201ff6c: unlock, running the update that came in meanwhile
 * @difftest @020e2313:8=u8 @020e2620:32=u32 cases=40 */
void func_0201ff6c(void)
{
    if (!(FLAGS & 0x80))
        return;
    if ((FLAGS & 0x40) && !(U32_AT(data_020e2614, 0xc) & 8))
        func_0202286c();
    FLAGS &= ~0x80;
}

/* 0x0201ffd0: lock
 * @difftest @020e2313:8=u8 */
void func_0201ffd0(void) { FLAGS |= 0x80; }

/* 0x0201ffe8: callback from the sound system
 * @difftest pick:0,1,4,2 pick:0,1,2,3 pick:0,1 */
void func_0201ffe8(s32 kind, s32 v, s32 w)
{
    switch (kind) {
    case 0:
    case 1:
        switch (v) {
        case 0:
            FLAGS |= 2;
            break;
        case 1:
            FLAGS |= 0x10;
            func_02027918(0, 0xff, 0x80, 4);
            break;
        case 2:
            func_02027918(0, 0xff, 0x80, 4);
            break;
        }
        break;
    case 4:
        if (w == 1) {
            u8 f = FLAGS;
            S32_AT(S, 4) = v;
            FLAGS = f | 4;
        }
        break;
    }
}

/* 0x020200cc: (locked) func_020213e0 */
void func_020200cc(s32 handle, s32 v)
{
    func_0201ffd0();
    func_020213e0(handle, v);
    func_0201ff6c();
}

/* 0x020200fc: whether a direct-sound handle is still valid
 * @difftest pick:-1,0,1,0x102,0x305 */
u32 func_020200fc(u32 h) { return func_02021540(h); }

/* 0x02020108: pause all 16 sound effect players */
void func_02020108(void)
{
    s32 i;
    func_0201ffd0();
    for (i = 15; i >= 0; i--)
        func_02021890(func_020211e8(i), 1);
    func_0201ff6c();
}

/* 0x02020144: pause the music
 * @difftest @020e2313:8=pick:0,0x20,0x80,0xc0 cases=20 */
void func_02020144(void)
{
    if (FLAGS & 0x20)
        return;
    func_0201ffd0();
    func_02027ec4(1);
    FLAGS &= ~1;
    func_0201ff6c();
}

/* 0x02020194: stop the music */
void func_02020194(void)
{
    if (FLAGS & 0x20)
        return;
    func_0201ffd0();
    func_02027ec4(4);
    {
        u8 f = FLAGS;
        S[0xa] = 0xff;
        FLAGS = f & ~1;
    }
    func_0201ff6c();
}

/* 0x020201ec
 * @difftest @020e2313:8=pick:0,0x20,0x80,0xc0 cases=20 */
void func_020201ec(void)
{
    if (FLAGS & 0x20)
        return;
    func_0201ffd0();
    func_020281bc(0);
    FLAGS &= ~1;
    func_0201ff6c();
}

/* 0x0202023c
 * @difftest @020e2313:8=pick:0,0x20,0x80,0xc0 cases=20 */
void func_0202023c(void)
{
    if (FLAGS & 0x20)
        return;
    func_0201ffd0();
    func_02028330(0);
    FLAGS |= 1;
    func_0201ff6c();
}

/* 0x0202028c: play music `i` (at the saved volume)
 * @difftest int:0:6 @020e2313:8=pick:0,0x20,0x80 cases=20 */
void func_0202028c(s32 i)
{
    if (FLAGS & 0x20)
        return;
    func_020206c8();
    func_020204d8(data_020ebbb8[9]);
    func_02028438(1, ((u32 *)data_020c3b5c)[i] & 0xffff, 0);
    func_0202068c();
}

/* 0x020202f0: change the music to `i` (0: stop) unless it is playing */
void func_020202f0(u32 i)
{
    if (FLAGS & 0x20)
        return;
    if (i == 0) {
        func_02020194();
        return;
    }
    if (i == S[0xa] && !(FLAGS & 2))
        return;
    func_020206c8();
    func_02028438(0, ((u32 *)data_020c3b5c)[i] & 0xffff, 1);
    func_0202068c();
    FLAGS &= ~2;
    if (!(FLAGS & 1))
        S[0xa] = i;
}

/* 0x02020390
 * @difftest @020e2313:8=pick:0,0x80,0xc0 cases=20 */
void func_02020390(void)
{
    func_0201ffd0();
    func_0202159c();
    func_0201ff6c();
}

/* 0x020203b0
 * @difftest @020e2313:8=pick:0,0x80,0xc0 cases=20 */
void func_020203b0(void)
{
    func_0201ffd0();
    func_02021708();
    func_0201ff6c();
}

/* 0x020203d0: resume all 16 sound effect players
 * @difftest @020e2313:8=pick:0,0x80,0xc0 cases=20 */
void func_020203d0(void)
{
    s32 i;
    func_0201ffd0();
    for (i = 15; i >= 0; i--)
        func_02021890(func_020211e8(i), 0);
    func_0201ff6c();
}

/* 0x0202040c: resume one sound effect player (-1: none) */
void func_0202040c(s32 handle)
{
    if (handle == -1)
        return;
    func_0201ffd0();
    func_02021890(handle, 0);
    func_0201ff6c();
}

/* 0x02020440: play sound effect `i`; returns its player */
s32 func_02020440(s32 i)
{
    SePan pan, tmp;
    s32 h;

    func_02020584(&pan);
    tmp = pan;
    h = func_020219d0(U32_AT(data_020c3b50 + i * 0xc, 0) & 0xffff, 1, tmp);
    if (S32_AT(data_020c3b50 + i * 0xc, 4) != 0)
        func_020200cc(h, S32_AT(data_020c3b50 + i * 0xc, 4));
    return h;
}

/* 0x020204cc: the sample id of a direct sound
 * @difftest pick:-1,0,1,0x102,0x305 */
u32 func_020204cc(u32 h) { return func_02021514(h); }

/* 0x020204d8: set the music volume */
void func_020204d8(u32 v)
{
    func_0201ffd0();
    func_02022640(v);
    func_0201ff6c();
    data_020ebbb8[9] = v;
}

/* 0x02020504: set the sound effect volume */
void func_02020504(u32 v)
{
    func_0201ffd0();
    func_0202265c(v);
    func_0201ff6c();
    data_020ebbb8[8] = v;
}

/* 0x02020530 */
void func_02020530(u32 v)
{
    func_0201ffd0();
    func_0202268c(v);
    func_0201ff6c();
    S[9] = v;
}

/* 0x0202055c: choose reverb setting `i` */
void func_0202055c(s32 i)
{
    func_0201ffd0();
    func_02028628(((u32 *)data_020b5808)[i]);
    func_0201ff6c();
}

/* 0x02020584: default effect position (centre)
 * @difftest ptr:12:4 */
void func_02020584(SePan *out)
{
    SePan p;
    MI_CpuFill8(&p, 0, 0xc);
    *(u8 *)&p = 7;
    *out = p;
}

/* 0x020205c0: sound update (run from an alarm): wait for the sound
 * system, then update unless locked */
void func_020205c0(void)
{
    u32 lock[0x20 / 4];

    while (func_02095cd4(0) != 0)
        ;
    func_02095a50(0);
    if (FLAGS & 8)
        return;
    if (FLAGS & 0x80) {
        FLAGS |= 0x40;
        return;
    }
    FLAGS &= ~0x40;
    func_02098278(lock);
    func_0202286c();
    func_0209820c(lock);
}

/* 0x0202064c */
void func_0202064c(void)
{
    if (FLAGS & 8)
        return;
    func_0201ffd0();
    func_020285cc();
    func_02022a6c();
    func_0201ff6c();
}

/* 0x0202068c */
void func_0202068c(void)
{
    if (S32_AT(S, 0) != 1)
        return;
    func_0201ffd0();
    func_02022bdc();
    func_0201ff6c();
}

/* 0x020206c8 */
void func_020206c8(void)
{
    if (S32_AT(S, 0) != 1)
        return;
    func_0201ffd0();
    func_02022d18();
    func_0201ff6c();
}

/* 0x02020704: initialise sound */
void func_02020704(void)
{
    MI_CpuFill8(S, 0, 0xc);
    S[9] = 0x20;
    func_02022d94();
    func_02020530(S[9]);
    func_02020504(0x7f);
    func_020204d8(0x7f);
    S[0xa] = 0xff;
    FLAGS = 0;
    func_0202055c(0);
    S32_AT(S, 0) = 1;
    func_0202068c();
}

/* 0x02020780: apply the sample rate of direct sound `i`
 * @difftest $T=ptr:0x38:4 @$T+0xb:8=pick:0,1,2 @$T+0xa:8=int:0:8 @$T+0x18:32=int:0x1000:0x10000 @020e26c0:32=0 pick:-1,0 */
void func_02020780(u32 i)
{
    u8 *c = data_020e26c0 + (i & 0xff) * 0x38;

    if (i == (u32)-1)
        return;
    if (U32_AT(c, 0) == 0)
        return;
    if ((u8)c[0xb] >> 1)
        return;
    if ((c[0xb] & 1) == 1) {
        s32 rate;
        func_02095930(1);
        rate = S32_AT(c, 0x18);
        REG16(REG_DIVCNT) = 0;
        REG32(REG_DIV_NUMER) = 0xffb0ff;
        REG32(REG_DIV_DENOM) = rate;
        REG32(REG_DIV_DENOM + 4) = 0;
        while (REG16(REG_DIVCNT) & 0x8000)
            ;
        func_0209548c(1 << c[0xa], REG32(REG_DIV_RESULT));
    } else {
        s32 rate = S32_AT(c, 0x18);
        u32 den = U16_AT(data_020e2614, 0x12);
        REG16(REG_DIVCNT) = 2;
        REG32(REG_DIV_NUMER) = (u32)rate << 16;
        REG32(REG_DIV_NUMER + 4) = ((u32)(rate >> 31) << 16) | ((u32)rate >> 16);
        REG32(REG_DIV_DENOM) = den;
        REG32(REG_DIV_DENOM + 4) = 0;
        while (REG16(REG_DIVCNT) & 0x8000)
            ;
        U32_AT(data_020e231c, c[0xa] * 0x18) = REG32(REG_DIV_RESULT);
    }
}

/* 0x020208c8: apply the volume and pan of direct sound `i`
 * @difftest pick:-1,0,1,2,3 */
void func_020208c8(u32 i)
{
    u8 *c = data_020e26c0 + (i & 0xff) * 0x38;
    s32 vol;

    if (i == (u32)-1)
        return;
    if (U32_AT(c, 0) == 0)
        return;
    if ((u8)c[0xb] >> 1)
        return;
    vol = data_020e2614[0x58d] * (c[4] * c[6]);
    if ((c[0xb] & 1) == 1) {
        s32 pan = (c[7] * 0x7f) >> 6;
        func_02095930(2);
        func_02095458(1 << c[0xa], vol >> 12, 0);
        func_0209542c(1 << c[0xa], pan);
    } else {
        u8 *h = data_020e2314 + c[0xa] * 0x18;
        S16_AT(h, 0xc) = (vol * (0x40 - c[7])) >> 10;
        S16_AT(h, 0xe) = (vol * c[7]) >> 10;
    }
}

/* 0x020209cc: start a direct sound on its hardware channel */
void func_020209cc(u8 *w)
{
    u32 f = w[5], len, loop, rep, fmt, end;

    if (f & 0x10) {
        len = U32_AT(w, 0x14);
        loop = U32_AT(w, 0x10);
        rep = 1;
    } else {
        len = U32_AT(w, 0xc);
        rep = 2;
        loop = 0;
    }
    if (f & 8) {
        end = (len + 7) >> 3;
        loop >>= 3;
        fmt = 2;
    } else if (!(f & 2)) {
        end = (len + 3) >> 2;
        loop >>= 2;
        fmt = 0;
    } else {
        end = (len + 1) >> 1;
        loop >>= 1;
        fmt = 1;
    }
    func_02095930(1);
    {
        u32 rate = U32_AT(w, 0x18);
        REG16(REG_DIVCNT) = 0;
        REG32(REG_DIV_NUMER) = 0xffb0ff;
        REG32(REG_DIV_DENOM) = rate;
        REG32(REG_DIV_DENOM + 4) = 0;
    }
    while (REG16(REG_DIVCNT) & 0x8000)
        ;
    {
        s32 vol = data_020e2614[0x58d] * (w[6] * w[4]);
        u32 timer = REG32(REG_DIV_RESULT);
        func_020953d4(w[0xa], fmt, PTR_AT(w, 0), rep, loop, end - loop, vol >> 12, 0, timer,
                      (w[7] * 0x7f) >> 6);
    }
}

/* 0x02020afc: start a direct sound on a software channel (pan.b0 7: none) */
void func_02020afc(u8 *w, SePan pan)
{
    u32 a[3], s[12];

    if ((u8)pan.w[0] == 7)
        return;
    *(u8 *)a = pan.w[0];
    a[1] = pan.w[1];
    a[2] = pan.w[2];
    s[11] = (w[5] & 2) ? 1 : 0;
    s[1] = 0;
    s[0] = U32_AT(w, 0);
    s[2] = U32_AT(w, 0xc);
    MI_CpuFill8(&s[3], 0, 0x20);
    func_02022218(a);
    func_02021c1c(a, s, U32_AT(w, 0xc));
}
