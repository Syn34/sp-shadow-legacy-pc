/*
 * The load-game screen (touch screen): three save slots, a delete toggle
 * with yes / no confirmation, and starting the game from a slot.
 * ARM9 main, 0x020366a4 - 0x02037fc0 (11 functions).
 *
 * Buttons are 0x78-byte text entries ({top, left, bottom, right}, +0x10
 * text actor, +0x14 text settings, +0x28 flag): data_020e701c the three
 * slots, data_020e6eb4 the delete toggle (+0x28 set while deleting),
 * data_020e6fa4 the message line, data_020e6dc4 / data_020e6e3c yes / no
 * and data_020e6f2c the cancel button. They slide 4 pixels a frame between
 * the positions in data_020e6c70..data_020e6cd8.
 * data_020e6c5c is the screen's state (func_02037f8c), data_020e6c68 the
 * state to go to after a wait (10, until the time data_020e6c90) or a fade
 * (11); data_020e6c60 / data_020e6c64 the chosen slot and its button.
 *
 * Testing: the set-ups run with the background loader func_0200aad0, the
 * text loader func_020107a8 and the save-header reader func_02043780
 * stubbed (they read from the card or backup memory); the state machine
 * with a faked pad and the save-slot functions, the state switch, text
 * changes (func_02072128) and sound effects (func_02020440, which can
 * stream from the card) stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT / colour value passed by value in a register */

extern Cnt data_020aaca4, data_020b66fc;
extern const u8 data_020b6700[], data_020b6714[], data_020b6728[], data_020b673c[],
    data_020b6750[];                 /* text settings */
extern const s32 data_020b6764, data_020b6768;
extern const s32 data_020be888;
extern u32 data_020be8d0;            /* next free text tile */
extern const char data_020be8d8[];   /* "SpyroMain_Load_BS.bin" */
extern const char data_020be8f0[];   /* "%d-%d-0%d" */
extern const char data_020be8fc[], data_020be908[], data_020be914[], data_020be920[];
                                     /* "%d:0%d:0%d" "%d:0%d:%d" "%d:%d:0%d" "%d:%d:%d" */
extern s32 data_020e6c5c;
extern s32 data_020e6c60;
extern u8 *data_020e6c64;
extern s32 data_020e6c68;
extern s32 data_020e6c6c;
extern s32 data_020e6c70[2], data_020e6c78[2], data_020e6c80[2], data_020e6c88[2];
extern u32 data_020e6c90[2];         /* u64, only 4-byte aligned accesses */
extern s32 data_020e6c98[2], data_020e6ca0[2], data_020e6ca8[2], data_020e6cb8[2],
    data_020e6cc0[2], data_020e6cc8[2], data_020e6cd0[2], data_020e6cd8[2];
extern u8 data_020e6dc4[], data_020e6e3c[], data_020e6eb4[], data_020e6f2c[], data_020e6fa4[];
extern u8 data_020e701c[];

void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_0203a028(void);
void func_02019b08(u32 state, u32 screen);
void func_0201b770(u32 a, u32 b, u32 c);
void func_0201b7f0(u32 screen);
void func_0201b808(u32 screen);
s32 func_0201b650(u32 screen);
void func_02019640(u32 v);
void func_02019660(u32 v);
s32 func_02020440(u32 v);
s32 func_0204297c(s32 slot);
void func_0204244c(s32 slot);
void func_02042b6c(s32 slot, s32 a);
s32 func_02043680(s32 slot);
void func_02043780(void);
void func_0203ee8c(s32 a);
s32 func_02071db4(void *b);
void func_02071dac(void *b, u32 on);
void func_02071e5c(void *b);
void func_02072128(void *b, u32 text);
u32 func_02072078(void *b, u32 tile);
u32 func_02071f10(void *b, u32 tile);
void func_02071eb0(void *b, const char *s);
void func_02071e94(void *b, const char *s);
void func_020108f4(void *text, const u8 *s);
void func_020107a8(void *b);
s32 func_020804f4(char *dst, const char *fmt, ...);              /* sprintf */
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200af18(s32 layer);
void func_0200af08(s32 layer, u32 v);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_02013a5c(Cnt color);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void *func_02072130(void *p);

void func_020371d4(void);
void func_02037470(void);
void func_020374e8(void);
void func_02037638(void);
void func_02037894(void);
void func_02037e8c(void);
void func_02037f8c(s32 state);

#define FN(f) ((void *(*)(void *))(void (*)(void))(f))
#define SLOT(i) (data_020e701c + (i) * 0x78)
#define TEXT(b) PTR_AT(b, 0x10)

static void Touch(s32 *p)
{
    func_0202f6dc(p, func_02011f44());
}

static s32 In(const s32 *p, const u8 *b)
{
    s32 in = 0;

    if (p[0] > S32_AT(b, 4) && p[0] < S32_AT(b, 0xc) && p[1] < S32_AT(b, 8) && p[1] > S32_AT(b, 0))
        in = 1;
    return in;
}

/* move a button to (x, y), keeping its size */
static void MoveTo(u8 *b, s32 x, s32 y)
{
    s32 h = S32_AT(b, 8) - S32_AT(b, 0);
    s32 w = S32_AT(b, 0xc) - S32_AT(b, 4);

    S32_AT(b, 0) = y;
    S32_AT(b, 4) = x;
    S32_AT(b, 8) = y + h;
    S32_AT(b, 0xc) = x + w;
    if (TEXT(b) != NULL)
        func_02003b98(TEXT(b), x, y);
}

static void ShiftY(u8 *b, s32 d)
{
    S32_AT(b, 0) += d;
    S32_AT(b, 8) += d;
    if (TEXT(b) != NULL)
        func_02003b98(TEXT(b), S32_AT(b, 4), S32_AT(b, 0));
}

static void ShiftX(u8 *b, s32 d)
{
    S32_AT(b, 4) += d;
    S32_AT(b, 0xc) += d;
    if (TEXT(b) != NULL)
        func_02003b98(TEXT(b), S32_AT(b, 4), S32_AT(b, 0));
}

static void Free(u8 *b)
{
    if (TEXT(b) != NULL) {
        func_020044a0(TEXT(b));
        TEXT(b) = NULL;
    }
}

static void FadeOut(void)
{
    func_0201b770(1, 0x3f, 1);
    func_0201b7f0(data_020df0fc);
}

static void FadeIn(void)
{
    func_0201b770(1, 0x3f, 1);
    func_0201b808(data_020df0fc);
}

/* wait for the delay data_020be888 (halved) from now */
static void SetDeadline(s32 half)
{
    u64 t = func_020822c8(func_020062d0());
    u64 d = func_02082290(func_020062d0(), data_020be888);

    Put64(data_020e6c90, t + (half ? d >> 1 : d));
}

/* 0x020366a4: free the screen's texts
 * @difftest cases=5 */
void func_020366a4(void)
{
    s32 i;

    Free(data_020e6f2c);
    Free(data_020e6e3c);
    Free(data_020e6dc4);
    Free(data_020e6fa4);
    Free(data_020e6eb4);
    for (i = 2; i >= 0; i--)
        Free(SLOT(i));
    func_0200b098(0);
    func_0200b098(2);
    func_0200b098(1);
    func_0200b098(3);
}

/* 0x020367b4: the load screen's per-frame update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e6c5c:32=int:0:15 @020e6c60:32=int:0:2 @020e6c64:32=pick:0,0x020e701c @020e6c68:32=int:0:14 @020e6c90:32=pick:0,0xffffffff @020e6c94:32=pick:0,0xffffffff @020e6dc4:32=int:0:192 @020e6dc8:32=int:0:128 @020e6dcc:32=int:0:192 @020e6dd0:32=int:128:256 @020e6e3c:32=int:0:192 @020e6e40:32=int:0:128 @020e6e44:32=int:0:192 @020e6e48:32=int:128:256 @020e6f2c:32=int:0:192 @020e6f30:32=int:0:256 @020e6f34:32=int:0:192 @020e6f38:32=int:0:256 @020e6eb4:32=int:0:60 @020e6eb8:32=int:0:100 @020e6ebc:32=int:60:192 @020e6ec0:32=int:100:256 @020e6edc:32=pick:0,1 @020e701c:32=int:0:60 @020e7020:32=int:0:128 @020e7024:32=int:60:192 @020e7028:32=int:128:256 stub=0x0204297c:1,0x0204244c,0x02042b6c,0x0203ee8c,0x02019b08,0x02020440,0x02072128 cases=150
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e6c5c:32=1 @020e6c60:32=int:0:2 @020e6c64:32=pick:0,0x020e701c @020e6c68:32=int:0:14 @020e6c90:32=pick:0,0xffffffff @020e6c94:32=pick:0,0xffffffff stub=0x0204297c:0,0x0204244c,0x02042b6c,0x0203ee8c,0x02019b08,0x02020440,0x02072128 cases=10 */
void func_020367b4(void)
{
    s32 p[2], q[2], done;

    switch ((u32)data_020e6c5c) {
    case 0:
        func_020371d4();
        return;
    case 10:
        if (func_020822c8(func_020062d0()) <= Get64(data_020e6c90))
            return;
        func_02037f8c(data_020e6c68);
        return;
    case 1:
        /* load the slot, or start a new game in it */
        if (func_0204297c(data_020e6c60) != 0) {
            data_020e6c6c = 0x14;
            func_02037470();
            return;
        }
        func_0204244c(data_020e6c60);
        func_0203ee8c(0);
        func_0201b770(2, 0x3f, 1);
        func_02019b08(0x25, 0);
        func_02019b08(0x26, 1);
        return;
    case 2:
        func_02042b6c(data_020e6c60, 0);
        data_020e6c6c = 0x24;
        func_02037470();
        return;
    case 3:
        func_020374e8();
        return;
    case 4:
        /* delete mode: the toggle and cancel slide in */
        done = 1;
        if (func_02071db4(data_020e6eb4) != 0) {
            ShiftY(data_020e6eb4, 4);
            done = 0;
        }
        if (S32_AT(data_020e6f2c, 4) - data_020e6ca8[0] > 4) {
            ShiftX(data_020e6f2c, -4);
            done = 0;
        } else {
            MoveTo(data_020e6f2c, data_020e6ca8[0], data_020e6ca8[1]);
        }
        if (done) {
            func_02071dac(data_020e6fa4, 1);
            func_02037f8c(5);
        }
        func_02037e8c();
        return;
    case 5:
        /* delete mode: pick a slot or cancel */
        Touch(p);
        if (In(p, data_020e6f2c)) {
            func_02020440(0x44);
            data_020e6c68 = 9;
            func_02037f8c(0xb);
            FadeOut();
            return;
        }
        func_020371d4();
        if (data_020e6c64 == NULL)
            return;
        if (func_02043680(data_020e6c60) != 0) {
            func_02072128(data_020e6fa4, 0x12);
            func_02071dac(data_020e6fa4, 0);
            func_02037e8c();
            func_02037f8c(6);
            return;
        }
        data_020e6c68 = 0xc;
        func_02037f8c(0xb);
        FadeOut();
        return;
    case 6:
        /* yes / no slide in, cancel slides out */
        done = 1;
        if (S32_AT(data_020e6dc4, 0) - data_020e6c78[1] > 4) {
            ShiftY(data_020e6dc4, -4);
            done = 0;
        } else {
            MoveTo(data_020e6dc4, data_020e6c78[0], data_020e6c78[1]);
        }
        if (S32_AT(data_020e6e3c, 0) - data_020e6c98[1] > 4) {
            ShiftY(data_020e6e3c, -4);
            done = 0;
        } else {
            MoveTo(data_020e6e3c, data_020e6c98[0], data_020e6c98[1]);
        }
        if (func_02071db4(data_020e6f2c) != 0) {
            ShiftX(data_020e6f2c, 4);
            done = 0;
        }
        if (done) {
            func_02071dac(data_020e6fa4, 1);
            func_02037f8c(7);
        }
        func_02037e8c();
        return;
    case 7:
        /* yes deletes the slot, no goes back */
        Touch(p);
        if (In(p, data_020e6dc4)) {
            func_02020440(0x44);
            SetDeadline(1);
            data_020e6c68 = 8;
            func_02037f8c(0xb);
            FadeOut();
            return;
        }
        Touch(q);
        if (!In(q, data_020e6e3c))
            return;
        func_02020440(0x44);
        SetDeadline(1);
        data_020e6c68 = 9;
        func_02037f8c(0xb);
        FadeOut();
        return;
    case 8: {
        u8 *b;
        func_0204244c(data_020e6c60);
        b = data_020e6c64;
        data_020e6c60 = -1;
        func_02071e5c(b);
        if (TEXT(data_020e6c64) != NULL)
            func_02003cbc(TEXT(data_020e6c64), 0);
        data_020e6c64 = NULL;
        func_02037638();
        func_02037e8c();
        FadeIn();
        data_020e6c68 = 0;
        func_02037f8c(0xb);
        func_02037f8c(0);
        return;
    }
    case 9:
        func_02037638();
        func_02037e8c();
        FadeIn();
        data_020e6c68 = 0;
        func_02037f8c(0xb);
        return;
    case 12:
        /* nothing to delete */
        func_02072128(data_020e6fa4, 0x14);
        MoveTo(data_020e6f2c, data_020b6764, data_020e6ca8[1]);
        if (TEXT(data_020e6f2c) != NULL)
            func_02003cbc(TEXT(data_020e6f2c), 1);
        func_02037e8c();
        FadeIn();
        data_020e6c68 = 0xd;
        func_02037f8c(0xb);
        return;
    case 13:
        SetDeadline(0);
        data_020e6c68 = 0xe;
        func_02037f8c(0xa);
        return;
    case 14:
        data_020e6c68 = 0xe;
        func_02037f8c(0xa);
        FadeOut();
        data_020e6c68 = 9;
        func_02037f8c(0xb);
        return;
    case 11:
        if (func_0201b650(data_020df0fc) != 0)
            return;
        func_02037f8c(data_020e6c68);
        return;
    }
}

/* 0x020371d4: touches on the slots and the delete toggle
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e6dc4:32=int:0:192 @020e6dc8:32=int:0:128 @020e6dcc:32=int:0:192 @020e6dd0:32=int:128:256 @020e6e3c:32=int:0:192 @020e6e40:32=int:0:128 @020e6e44:32=int:0:192 @020e6e48:32=int:128:256 @020e6f2c:32=int:0:192 @020e6f30:32=int:0:256 @020e6f34:32=int:0:192 @020e6f38:32=int:0:256 @020e6eb4:32=int:0:60 @020e6eb8:32=int:0:100 @020e6ebc:32=int:60:192 @020e6ec0:32=int:100:256 @020e6edc:32=pick:0,1 @020e701c:32=int:0:60 @020e7020:32=int:0:128 @020e7024:32=int:60:192 @020e7028:32=int:128:256 @020e7094:32=int:0:192 @020e7098:32=0 @020e709c:32=192 @020e70a0:32=256 stub=0x02072128,0x02020440 cases=100 */
void func_020371d4(void)
{
    s32 p[2];
    s32 hit = 0, i;
    u8 *s;

    Touch(p);
    if (func_02081d34(func_02011f44(), 0xc) == 0) {
        /* nothing */
    } else if (In(p, data_020e6eb4)) {
        if (S32_AT(data_020e6eb4, 0x28) == 0) {
            S32_AT(data_020e6eb4, 0x28) = 1;
            if (TEXT(data_020e6eb4) != NULL)
                func_02003cbc(TEXT(data_020e6eb4), 1);
            func_02072128(data_020e6fa4, 0x13);
            func_02071dac(data_020e6fa4, 0);
            func_02020440(0x44);
            func_02071e5c(data_020e6eb4);
            func_02037f8c(4);
        } else {
            S32_AT(data_020e6eb4, 0x28) = 0;
            if (TEXT(data_020e6eb4) != NULL)
                func_02003cbc(TEXT(data_020e6eb4), 0);
            func_02020440(0x44);
            func_02037f8c(0);
        }
    } else {
        for (i = 0, s = data_020e701c; !hit && i < 3;) {
            if (In(p, s)) {
                hit = 1;
                data_020e6c64 = s;
                data_020e6c60 = i % 3;
                if (TEXT(s) != NULL)
                    func_02003cbc(TEXT(s), 1);
                func_02020440(0x44);
                if (S32_AT(data_020e6eb4, 0x28) == 0) {
                    data_020e6c68 = func_02043680(data_020e6c60) != 0 ? 1 : 2;
                    func_02037f8c(10);
                }
            } else {
                s += 0x78;
                i++;
            }
        }
    }
    func_0203a028();
}

/* 0x02037470: start the game from the chosen slot (state data_020e6c6c)
 * @difftest @020e6c6c:32=pick:0x14,0x24 stub=0x02019b08 cases=10 */
void func_02037470(void)
{
    func_0201b770(2, 0x3f, 1);
    func_02019b08(0x1e, 0);
    func_02019b08(data_020e6c6c, 1);
    func_02019660(func_020436f0(data_020bf6a0)[0xf] >> 1);
    func_02019640(func_020436f0(data_020bf6a0)[0x10]);
}

/* 0x020374e8: back to the title (state 10)
 * @difftest stub=0x02019b08 cases=5 */
void func_020374e8(void)
{
    func_0201b770(1, 0x3f, 1);
    func_02019b08(0xa, data_020df0fc);
}

/* 0x02037520: set up the load screen
 * @difftest stub=0x0200aad0,0x020107a8,0x02043780 cases=5 */
void func_02037520(void)
{
    Cnt cnt;

    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(3, data_020aaca4.v);
    func_0200af18(3);
    func_0200b0ac(3);
    func_0200aad0(3, (u32)data_020be8d8, 0, 0, 0, 0);
    cnt = data_020b66fc;
    func_0200b168(0, cnt.v);
    func_0200b0ac(0);
    func_02013a5c(cnt);
    func_0200af08(0, 0);
    func_0201391c(1, 2, -1);
    func_0200b098(1);
    func_0200b098(2);
    data_020be8d0 = 1;
    data_020e6c64 = NULL;
    func_02043780();
    func_02037894();
    func_02037f8c(0);
}

/* 0x02037638: put the buttons back (leaving delete mode)
 * @difftest cases=5 */
void func_02037638(void)
{
    s32 i;

    MoveTo(data_020e6eb4, data_020e6cc0[0], data_020e6cc0[1]);
    S32_AT(data_020e6eb4, 0x28) = 0;
    if (TEXT(data_020e6eb4) != NULL)
        func_02003cbc(TEXT(data_020e6eb4), 0);
    func_02071dac(data_020e6fa4, 1);
    if (TEXT(data_020e6fa4) != NULL)
        U32_AT(TEXT(data_020e6fa4), 4) &= ~8u;
    func_02072128(data_020e6fa4, 0x11);
    MoveTo(data_020e6dc4, data_020e6c78[0], data_020b6768);
    MoveTo(data_020e6e3c, data_020e6c98[0], data_020b6768);
    MoveTo(data_020e6f2c, data_020b6764, data_020e6ca8[1]);
    if (TEXT(data_020e6f2c) != NULL)
        func_02003cbc(TEXT(data_020e6f2c), 1);
    data_020e6c64 = NULL;
    data_020e6c60 = -1;
    for (i = 0; i < 3; i++) {
        if (TEXT(SLOT(i)) != NULL)
            func_02003cbc(TEXT(SLOT(i)), 0);
    }
}

/* create a button at (x, y) of size `size` with text settings `t` */
static void Button(u8 *b, s32 x, s32 y, const s32 *size, const u8 *t)
{
    s32 right = x + size[0], bottom = y + size[1];

    func_020036e8((s32 *)b, &y, &x, &bottom, &right);
    func_020108f4(b + 0x14, t);
    func_020107a8(b);
}

/* 0x02037894: create the buttons and show the slots' progress and time
 * @difftest stub=0x020107a8 cases=5 */
void func_02037894(void)
{
    char info[0x14], time[0x14];
    s32 i;
    u8 *s;

    Button(SLOT(0), data_020e6c80[0], data_020e6c80[1], data_020e6cb8, data_020b6728);
    Button(SLOT(1), data_020e6c88[0], data_020e6c88[1], data_020e6cb8, data_020b673c);
    Button(SLOT(2), data_020e6ca0[0], data_020e6ca0[1], data_020e6cb8, data_020b6750);
    for (i = 0, s = data_020e701c; i < 3; i++, s += 0x78) {
        func_02072128(s, 0xd);
        func_02071dac(s, 1);
        if (func_02043680(i) != 0) {
            const char *f;
            u8 *a = func_020436f0(i), *b = func_020436f0(i), *c = func_020436f0(i);
            func_020804f4(info, data_020be8f0, a[1], b[2], c[0]);
            if (func_020436f0(i)[0xd] < 10 && func_020436f0(i)[0xe] < 10)
                f = data_020be8fc;
            else if (func_020436f0(i)[0xd] < 10)
                f = data_020be908;
            else if (func_020436f0(i)[0xe] < 10)
                f = data_020be914;
            else
                f = data_020be920;
            a = func_020436f0(i);
            b = func_020436f0(i);
            c = func_020436f0(i);
            func_020804f4(time, f, a[0xc], b[0xd], c[0xe]);
            func_02071eb0(s, info);
            func_02071e94(s, time);
        }
    }

    Button(data_020e6eb4, data_020e6cc0[0], data_020e6cc0[1], data_020e6cc8, data_020b6700);
    func_02071dac(data_020e6eb4, 1);
    func_02072128(data_020e6eb4, 0xf);
    S32_AT(data_020e6eb4, 0x28) = 0;
    S32_AT(data_020e6fa4, 0) = data_020e6cd0[1];
    S32_AT(data_020e6fa4, 4) = data_020e6cd0[0];
    S32_AT(data_020e6fa4, 8) = data_020e6cd0[1] + data_020e6c70[1];
    S32_AT(data_020e6fa4, 0xc) = data_020e6cd0[0] + data_020e6c70[0];
    func_02071dac(data_020e6fa4, 1);
    if (TEXT(data_020e6fa4) != NULL)
        U32_AT(TEXT(data_020e6fa4), 4) &= ~8u;
    func_02072128(data_020e6fa4, 0x11);
    Button(data_020e6dc4, data_020e6c78[0], data_020b6768, data_020e6cc8, data_020b6700);
    func_02071dac(data_020e6dc4, 1);
    func_02072128(data_020e6dc4, 0x21);
    Button(data_020e6e3c, data_020e6c98[0], data_020b6768, data_020e6cc8, data_020b6700);
    func_02071dac(data_020e6e3c, 1);
    func_02072128(data_020e6e3c, 0x22);
    Button(data_020e6f2c, data_020b6764, data_020e6ca8[1], data_020e6cd8, data_020b6714);
    if (TEXT(data_020e6f2c) != NULL)
        func_02003cbc(TEXT(data_020e6f2c), 1);
    func_02037e8c();
}

/* 0x02037e8c: lay out the buttons' texts
 * @difftest cases=5 */
void func_02037e8c(void)
{
    s32 i;
    u8 *s;

    func_0200af08(0, 0);
    data_020be8d0 = 1;
    func_0201391c(3, 0, -1);
    data_020be8d0 = func_02072078(data_020e6fa4, data_020be8d0);
    func_0201391c(1, 2, -1);
    data_020be8d0 = func_02072078(data_020e6eb4, data_020be8d0);
    data_020be8d0 = func_02072078(data_020e6dc4, data_020be8d0);
    data_020be8d0 = func_02072078(data_020e6e3c, data_020be8d0);
    for (i = 0, s = data_020e701c; i < 3; i++, s += 0x78) {
        if (func_02043680(i) != 0)
            data_020be8d0 = func_02071f10(s, data_020be8d0);
        else
            data_020be8d0 = func_02072078(s, data_020be8d0);
    }
}

/* 0x02037f8c: set the screen's state
 * @difftest int:0:15 cases=5 */
void func_02037f8c(s32 state)
{
    data_020e6c5c = state;
}

/* 0x02037f9c: destroy the slot buttons
 * @difftest cases=3 */
void func_02037f9c(void)
{
    func_020a6d58(data_020e701c, 3, 0x78, FN(func_02072130));
}
