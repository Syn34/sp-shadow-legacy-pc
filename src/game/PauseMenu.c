/*
 * The pause menu (touch screen): resume, options, save, quit (with a yes /
 * no confirmation), driven by the d-pad or the stylus.
 * ARM9 main, 0x0203a948 - 0x0203c00c (16 functions).
 *
 * Buttons are 0x78-byte text entries ({top, left, bottom, right}, +0x10
 * text actor, +0x14 text settings, +0x28 selected): data_020e8c90,
 * data_020e8948, data_020e89c0, data_020e8a38 the four items (positions
 * data_020e8928, {x, y} each), data_020e8ab0 the cursor, data_020e8b28 /
 * data_020e8ba0 yes / no and data_020e8c18 the message line.
 * data_020beb2c is the menu's state (func_0203bf6c), data_020e8830 the
 * state after a fade (2) or a wait (3, data_020e8828 ticks),
 * data_020e8824 the selected item and data_020e882c the game state to
 * return to.
 *
 * Testing: with a faked pad, the state switch, the save
 * (func_02042480, which writes the backup memory) and sound effects
 * stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT / colour value passed by value in a register */

extern Cnt data_020aaca4, data_020b6814;
extern const u8 data_020b6818[], data_020b682c[], data_020b6840[];   /* text settings */
extern const s32 data_020beb08, data_020beb14, data_020beb28, data_020beb44, data_020beb4c,
    data_020beb54, data_020beb58, data_020beb5c, data_020beb64, data_020beb68, data_020beb70,
    data_020beb7c;
extern s32 data_020beb2c;
extern const char data_020beb80[];   /* the menu's background */
extern s8 data_020e8820;
extern s32 data_020e8824;
extern s32 data_020e8828;
extern u32 data_020e882c;
extern s32 data_020e8830;
extern s32 data_020e8834[2], data_020e883c[2], data_020e8844[2], data_020e884c[2],
    data_020e8854[2];
extern s32 data_020e8928[8];
extern u8 data_020e8948[], data_020e89c0[], data_020e8a38[], data_020e8ab0[], data_020e8b28[],
    data_020e8ba0[], data_020e8c18[], data_020e8c90[];

void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_02019b08(u32 state, u32 screen);
u32 func_02019b60(u32 screen);
void func_0201b770(u32 a, u32 b, u32 c);
void func_0201b7f0(u32 screen);
void func_0201b808(u32 screen);
s32 func_0201b650(u32 screen);
s32 func_02020440(u32 v);
void func_020203b0(void);
void func_0201844c(void);
void func_02042480(s32 a);
void func_02071dac(void *b, u32 on);
void func_02072128(void *b, u32 text);
u32 func_02072078(void *b, u32 tile);
void func_020108f4(void *text, const u8 *s);
void func_020107a8(void *b);
void func_0200af18(s32 layer);
void func_0200af08(s32 layer, u32 v);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_02013a5c(Cnt color);

void func_0203a948(void);
void func_0203b0fc(void);
void func_0203b564(void);
void func_0203b610(void);
void func_0203b6bc(void);
void func_0203b75c(void);
void func_0203b91c(void);
void func_0203b9cc(void);
void func_0203ba48(void);
void func_0203baa0(void);
void func_0203badc(s32 on);
void func_0203bb58(void);
void func_0203bf6c(s32 state);
void func_0203bf7c(void);

#define TEXT(b) PTR_AT(b, 0x10)
#define PAD() func_02011f44()

static s32 In(const s32 *p, const u8 *b)
{
    s32 in = 0;

    if (p[0] > S32_AT(b, 4) && p[0] < S32_AT(b, 0xc) && p[1] < S32_AT(b, 8) && p[1] > S32_AT(b, 0))
        in = 1;
    return in;
}

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

static void Free(u8 *b)
{
    if (TEXT(b) != NULL) {
        func_020044a0(TEXT(b));
        TEXT(b) = NULL;
    }
}

static void Frame(u8 *b, s32 f)
{
    if (TEXT(b) != NULL)
        func_02003cbc(TEXT(b), f);
}

static void Hide(u8 *b)
{
    if (TEXT(b) != NULL)
        U32_AT(TEXT(b), 4) |= 8;
}

static void Unhide(u8 *b)
{
    if (TEXT(b) != NULL)
        U32_AT(TEXT(b), 4) &= ~8u;
}

static void Button(u8 *b, s32 x, s32 y, s32 w, s32 h, const u8 *t)
{
    s32 right = x + w, bottom = y + h;

    func_020036e8((s32 *)b, &y, &x, &bottom, &right);
    func_020108f4(b + 0x14, t);
    func_020107a8(b);
}

static void Place(u8 *b, s32 x, s32 y, s32 w, s32 h)
{
    S32_AT(b, 0) = y;
    S32_AT(b, 4) = x;
    S32_AT(b, 8) = y + h;
    S32_AT(b, 0xc) = x + w;
}

/* fade the touch screen in, then state `next` */
static void FadeIn(s32 next)
{
    func_0201b770(1, 0x3f, 1);
    func_0201b808(data_020df0fc);
    data_020e8830 = next;
    func_0203bf6c(2);
}

/* the cursor goes to item `i` */
static void Cursor(s32 i)
{
    MoveTo(data_020e8ab0, data_020e8928[i * 2], data_020e8928[i * 2 + 1]);
}

/* 0x0203a948: leave the menu
 * @difftest @020e8820:8=pick:0,1 stub=0x0201844c cases=10 */
void func_0203a948(void)
{
    Free(data_020e8c18);
    Free(data_020e8ba0);
    Free(data_020e8b28);
    Free(data_020e8a38);
    Free(data_020e89c0);
    Free(data_020e8948);
    Free(data_020e8c90);
    Free(data_020e8ab0);
    if (data_020e8820 != 0)
        func_0201844c();
}

/* 0x0203aa9c: the pause menu's per-frame update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @$K+0x14:32=pick:0,0,1 @$K+0x2c:32=pick:0,0,1 @$K+0x34:32=pick:0,0,1 @$K+0x6c:32=pick:0,1 @020beb2c:32=int:0:14 @020e8828:32=pick:-5,5 @020e8830:32=int:0:13 @020e8b28:32=int:0:96 @020e8b2c:32=int:0:128 @020e8b30:32=int:96:192 @020e8b34:32=int:128:256 @020e8b50:32=pick:0,1 @020e8ba0:32=0 @020e8ba4:32=0 @020e8ba8:32=192 @020e8bac:32=256 stub=0x02019b08,0x02042480,0x02020440 cases=200 */
void func_0203aa9c(void)
{
    s32 p[2], q[2];

    switch ((u32)data_020beb2c) {
    case 1:
        func_0203b0fc();
        return;
    case 2:
        if (func_0201b650(data_020df0fc) != 0)
            return;
        func_0203bf6c(data_020e8830);
        return;
    case 3:
        data_020e8828 -= func_020822c0(func_020062d0());
        if (data_020e8828 > 0)
            return;
        func_0203bf6c(data_020e8830);
        return;
    case 4:
        func_0203ba48();
        return;
    case 5:
        func_0203baa0();
        return;
    case 6:
        /* save */
        func_0203badc(0);
        func_02072128(data_020e8c18, 0xc);
        Hide(data_020e8c18);
        func_02071dac(data_020e8c18, 1);
        func_0203bf7c();
        func_02042480(0);
        FadeIn(7);
        return;
    case 7:
        data_020e8828 = 0x1800;
        data_020e8830 = 8;
        func_0203bf6c(3);
        return;
    case 8:
        func_0201b770(1, 0x3f, 1);
        func_0201b7f0(data_020df0fc);
        data_020e8830 = 9;
        func_0203bf6c(2);
        return;
    case 9:
        func_0203badc(1);
        func_02071dac(data_020e8c18, 0);
        Frame(data_020e8ab0, 0);
        func_0203bf7c();
        FadeIn(1);
        return;
    case 10:
        /* quit: ask */
        func_0203badc(0);
        func_02072128(data_020e8c18, 0x12);
        func_02071dac(data_020e8c18, 1);
        Hide(data_020e8ba0);
        func_02071dac(data_020e8ba0, 1);
        Frame(data_020e8ba0, 1);
        S32_AT(data_020e8ba0, 0x28) = 1;
        Hide(data_020e8b28);
        func_02071dac(data_020e8b28, 1);
        Frame(data_020e8b28, 0);
        S32_AT(data_020e8b28, 0x28) = 0;
        func_0203bf7c();
        FadeIn(0xb);
        return;
    case 11:
        if (func_02081d34(PAD(), 5) != 0) {
            func_02020440(0x44);
            Frame(data_020e8ba0, 0);
            S32_AT(data_020e8ba0, 0x28) = 0;
            Frame(data_020e8b28, 1);
            S32_AT(data_020e8b28, 0x28) = 1;
            return;
        }
        if (func_02081d34(PAD(), 4) != 0) {
            func_02020440(0x44);
            Frame(data_020e8ba0, 1);
            S32_AT(data_020e8ba0, 0x28) = 1;
            Frame(data_020e8b28, 0);
            S32_AT(data_020e8b28, 0x28) = 0;
            return;
        }
        if (func_02081d34(PAD(), 1) != 0) {
            if (S32_AT(data_020e8b28, 0x28) == 1)
                func_0203b9cc();
            else
                func_0203b91c();
            return;
        }
        func_0202f6dc(p, PAD());
        if (In(p, data_020e8b28)) {
            func_0203b9cc();
            return;
        }
        func_0202f6dc(q, PAD());
        if (In(q, data_020e8ba0))
            func_0203b91c();
        return;
    case 12:
        /* quit: no */
        func_0203badc(1);
        func_02071dac(data_020e8c18, 0);
        Unhide(data_020e8ba0);
        func_02071dac(data_020e8ba0, 0);
        Unhide(data_020e8b28);
        func_02071dac(data_020e8b28, 0);
        func_0203bf7c();
        FadeIn(1);
        return;
    }
}

/* 0x0203b0fc: the menu itself: d-pad, A and the stylus
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @$K+0x24:32=pick:0,0,0,1 @$K+0x44:32=pick:0,0,1 @$K+0x3c:32=pick:0,0,1 @$K+0x14:32=pick:0,0,1 @$K+0x6c:32=pick:0,1 @020e8824:32=int:-1:5 @020e8c90:32=0 @020e8c94:32=0 @020e8c98:32=48 @020e8c9c:32=256 @020e8948:32=48 @020e894c:32=0 @020e8950:32=96 @020e8954:32=256 @020e89c0:32=96 @020e89c4:32=0 @020e89c8:32=144 @020e89cc:32=256 @020e8a38:32=144 @020e8a3c:32=0 @020e8a40:32=192 @020e8a44:32=256 @020e8928:32=int:0:200 @020e892c:32=int:0:200 @020e8930:32=int:0:200 @020e8934:32=int:0:200 @020e8938:32=int:0:200 @020e893c:32=int:0:200 @020e8940:32=int:0:200 @020e8944:32=int:0:200 stub=0x02020440 cases=200 */
void func_0203b0fc(void)
{
    s32 p[2];

    if (func_02081d34(PAD(), 3) != 0) {
        func_0203b75c();
        return;
    }
    if (func_02081d34(PAD(), 7) != 0) {
        data_020e8824 = (data_020e8824 + 1) % 4;
        Cursor(data_020e8824);
        func_02020440(0x44);
        return;
    }
    if (func_02081d34(PAD(), 6) != 0) {
        if (--data_020e8824 < 0)
            data_020e8824 = 3;
        Cursor(data_020e8824);
        func_02020440(0x44);
        return;
    }
    if (func_02081d34(PAD(), 1) != 0) {
        Frame(data_020e8ab0, 1);
        switch ((u32)data_020e8824) {
        case 0: func_0203b75c(); return;
        case 1: func_0203b6bc(); return;
        case 2: func_0203b610(); return;
        case 3: func_0203b564(); return;
        }
        return;
    }
    if (func_02081d34(PAD(), 0xc) == 0)
        return;
    func_0202f6dc(p, PAD());
    if (In(p, data_020e8c90)) {
        data_020e8824 = 0;
        Frame(data_020e8ab0, 1);
        func_0203b75c();
    } else if (In(p, data_020e8948)) {
        data_020e8824 = 1;
        Frame(data_020e8ab0, 1);
        func_0203b6bc();
    } else if (In(p, data_020e89c0)) {
        data_020e8824 = 2;
        Frame(data_020e8ab0, 1);
        func_0203b610();
    } else if (In(p, data_020e8a38)) {
        data_020e8824 = 3;
        Frame(data_020e8ab0, 1);
        func_0203b564();
    }
}

/* 0x0203b564: item 3, quit
 * @difftest stub=0x02020440 cases=5 */
void func_0203b564(void)
{
    func_02020440(0x44);
    Cursor(3);
    data_020e8830 = 0xa;
    func_0203bf6c(2);
    func_0201b770(1, 0x3f, 1);
    func_0201b7f0(data_020df0fc);
}

/* 0x0203b610: item 2, save
 * @difftest stub=0x02020440 cases=5 */
void func_0203b610(void)
{
    func_02020440(0x44);
    Cursor(2);
    data_020e8830 = 6;
    func_0203bf6c(2);
    func_0201b770(1, 0x3f, 1);
    func_0201b7f0(data_020df0fc);
}

/* 0x0203b6bc: item 1, options
 * @difftest stub=0x02020440 cases=5 */
void func_0203b6bc(void)
{
    func_02020440(0x44);
    Cursor(1);
    data_020e8828 = 0x266;
    data_020e8830 = 5;
    func_0203bf6c(3);
}

/* 0x0203b75c: item 0, resume
 * @difftest stub=0x02020440 cases=5 */
void func_0203b75c(void)
{
    func_02020440(0x44);
    Cursor(0);
    data_020e8828 = 0x266;
    data_020e8830 = 4;
    func_0203bf6c(3);
}

/* 0x0203b7fc: set up the pause menu
 * @difftest @020e882c:32=pick:0,5 stub=0x0200aad0,0x020107a8,0x020203b0 cases=10 */
void func_0203b7fc(void)
{
    Cnt cnt;

    func_020203b0();
    data_020e8820 = 0;
    func_0203bf6c(1);
    func_0200b168(3, data_020aaca4.v);
    func_0200af18(3);
    func_0200b0ac(3);
    cnt = data_020b6814;
    func_0200b168(0, cnt.v);
    func_0200b0ac(0);
    func_02013a5c(cnt);
    func_0200af08(0, 0);
    func_0201391c(3, 0, -1);
    func_0200aad0(3, (u32)data_020beb80, 0, 0, 0, 0);
    func_0203bb58();
    if (data_020e882c == 0)
        data_020e882c = func_02019b60(data_020df0fc);
    func_0200b098(2);
    func_0200b098(1);
}

/* 0x0203b91c: quit: no
 * @difftest stub=0x02020440 cases=5 */
void func_0203b91c(void)
{
    func_02020440(0x44);
    Frame(data_020e8ba0, 1);
    Frame(data_020e8b28, 0);
    func_0201b770(1, 0x3f, 1);
    func_0201b7f0(data_020df0fc);
    data_020e8830 = 0xc;
    func_0203bf6c(2);
    Frame(data_020e8ab0, 0);
}

/* 0x0203b9cc: quit: yes (back to the title)
 * @difftest stub=0x02020440,0x02019b08 cases=5 */
void func_0203b9cc(void)
{
    func_02020440(0x44);
    Frame(data_020e8ba0, 0);
    Frame(data_020e8b28, 1);
    func_0201b770(2, 0x3f, 1);
    func_02019b08(3, 0);
    func_02019b08(8, 1);
}

/* 0x0203ba48: resume the game
 * @difftest @020e882c:32=pick:0,5 stub=0x02019b08 cases=5 */
void func_0203ba48(void)
{
    data_020e8820 = 1;
    func_0201b770(1, 0x3f, 1);
    func_02019b08(data_020e882c, data_020df0fc);
    data_020e882c = 0;
}

/* 0x0203baa0: open the options page
 * @difftest stub=0x02019b08 cases=5 */
void func_0203baa0(void)
{
    func_0201b770(1, 0x3f, 1);
    func_02019b08(0xa, data_020df0fc);
    func_0203a948();
}

/* 0x0203badc: show or hide the items and the cursor
 * @difftest int:0:2 cases=10 */
void func_0203badc(s32 on)
{
    if (TEXT(data_020e8ab0) != NULL) {
        u32 *f = (u32 *)((u8 *)TEXT(data_020e8ab0) + 4);
        *f = (*f & ~8u) | ((on & 1) << 3);
    }
    func_02071dac(data_020e8c90, on);
    func_02071dac(data_020e8948, on);
    func_02071dac(data_020e89c0, on);
    func_02071dac(data_020e8a38, on);
}

/* 0x0203bb58: create the menu's buttons
 * @difftest stub=0x020107a8 cases=5 */
void func_0203bb58(void)
{
    data_020e8824 = 0;
    Button(data_020e8ab0, data_020beb58, data_020beb44, data_020beb14, data_020beb70, data_020b6818);
    Frame(data_020e8ab0, 0);
    Place(data_020e8c90, data_020e8928[0], data_020e8928[1], data_020beb64, data_020beb08);
    func_02072128(data_020e8c90, 8);
    func_02071dac(data_020e8c90, 1);
    Place(data_020e8948, data_020e8928[2], data_020e8928[3], data_020beb4c, data_020beb7c);
    func_02072128(data_020e8948, 9);
    func_02071dac(data_020e8948, 1);
    Place(data_020e89c0, data_020e8928[4], data_020e8928[5], data_020beb5c, data_020beb54);
    func_02072128(data_020e89c0, 0xa);
    func_02071dac(data_020e89c0, 1);
    Place(data_020e8a38, data_020e8928[6], data_020e8928[7], data_020beb68, data_020beb28);
    func_02072128(data_020e8a38, 0xb);
    func_02071dac(data_020e8a38, 1);

    Button(data_020e8b28, data_020e8844[0], data_020e8844[1], data_020e8854[0], data_020e8854[1],
           data_020b682c);
    func_02071dac(data_020e8b28, 0);
    Unhide(data_020e8b28);
    func_02072128(data_020e8b28, 0x21);
    S32_AT(data_020e8b28, 0x28) = 0;
    Frame(data_020e8b28, 0);
    Button(data_020e8ba0, data_020e884c[0], data_020e884c[1], data_020e8854[0], data_020e8854[1],
           data_020b6840);
    func_02071dac(data_020e8ba0, 0);
    Unhide(data_020e8ba0);
    func_02072128(data_020e8ba0, 0x22);
    S32_AT(data_020e8ba0, 0x28) = 1;
    Frame(data_020e8ba0, 1);
    Place(data_020e8c18, data_020e8834[0], data_020e8834[1], data_020e883c[0], data_020e883c[1]);
    func_02071dac(data_020e8c18, 0);
    Unhide(data_020e8c18);
    func_02072128(data_020e8c18, 0x12);
    func_0203bf7c();
}

/* 0x0203bf6c: set the menu's state
 * @difftest int:0:14 cases=5 */
void func_0203bf6c(s32 state)
{
    data_020beb2c = state;
}

/* 0x0203bf7c: lay out the texts
 * @difftest cases=5 */
void func_0203bf7c(void)
{
    u32 t;

    func_0200af08(0, 0);
    t = func_02072078(data_020e8c90, 1);
    t = func_02072078(data_020e8948, t);
    t = func_02072078(data_020e89c0, t);
    t = func_02072078(data_020e8a38, t);
    t = func_02072078(data_020e8b28, t);
    t = func_02072078(data_020e8ba0, t);
    func_02072078(data_020e8c18, t);
}
