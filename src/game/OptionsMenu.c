/*
 * The options page (touch screen): music and sound effect volume sliders
 * and the backlight switch.
 * ARM9 main, 0x02038b54 - 0x0203a02c (11 functions).
 *
 * Buttons are 0x78-byte text entries ({top, left, bottom, right}, +0x10
 * text actor, +0x14 text settings, +0x28 pressed): data_020e7a38 back,
 * data_020e7e70 the backlight switch (with the texts data_020e80c8 /
 * data_020e79c0 for on / off), data_020e7b28 / data_020e7ab0 music down /
 * up with the knob data_020e7ba0, data_020e7c90 / data_020e7c18 effects
 * down / up with the knob data_020e7d08. A held arrow moves its knob a
 * pixel a frame within data_020e78d4 {left, right}; the knob position maps
 * to a volume 0..0x7f (func_0203959c / func_020396a8, in fx32 through the
 * game's soft-float routines).
 * data_020e78b8 is the pressed button, data_020e78bc the knob it moves,
 * data_020e78b0 the direction (1 left), data_020e78a0 set for the effects
 * slider, data_020e78a4 the backlight state and data_020e78ac the state
 * to return to.
 *
 * Testing: the set-up runs with the background and text loaders
 * (func_0200aad0, func_020107a8) and the backlight query func_02099404
 * stubbed, leaving with the settings save func_02042b24 stubbed, the update with a faked pad, the backlight switch
 * func_020994d0 and sound effects stubbed (they wait for the ARM7 or the
 * card).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 * func_020393ac: when the backlight query fails the original reads an
 * uninitialised stack word; the C reads 0 ("off").
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT / colour value passed by value in a register */

extern Cnt data_020aaca4, data_020b6780;
extern const u8 data_020b6784[], data_020b6798[], data_020b67ac[], data_020b67c0[],
    data_020b67d4[], data_020b67e8[];   /* text settings */
extern const u16 data_020be968;      /* slider width in pixels */
extern const s32 data_020be96c, data_020be970, data_020be974, data_020be978, data_020be97c,
    data_020be980, data_020be984, data_020be988, data_020be98c, data_020be990, data_020be994,
    data_020be998, data_020be99c, data_020be9a0, data_020be9a4, data_020be9a8, data_020be9ac,
    data_020be9b0, data_020be9b4, data_020be9bc, data_020be9c4, data_020be9c8, data_020be9cc,
    data_020be9d0, data_020be9d4, data_020be9d8, data_020be9dc, data_020be9e0, data_020be9e4,
    data_020be9ec, data_020be9f0, data_020be9f4, data_020be9f8, data_020be9fc, data_020bea00,
    data_020bea04, data_020bea08, data_020bea0c, data_020bea10, data_020bea14, data_020bea18,
    data_020bea1c, data_020bea24, data_020bea28, data_020bea2c, data_020bea30, data_020bea34,
    data_020bea38, data_020bea3c, data_020bea40, data_020bea44, data_020bea48, data_020bea4c,
    data_020bea50, data_020bea54, data_020bea5c, data_020bea60, data_020bea68;
extern const char data_020bea6c[];   /* the page's background */
extern u8 data_020ebbb8[];           /* settings: +8 effects volume, +9 music volume */
extern s8 data_020e78a0;
extern s8 data_020e78a4;
extern u8 data_020e78a8;
extern u32 data_020e78ac;
extern s32 data_020e78b0;
extern s32 data_020e78b4;
extern u8 *data_020e78b8;
extern u8 *data_020e78bc;
extern s32 data_020e78c0;
extern s32 data_020e78d4[2];
extern u8 data_020e79c0[], data_020e7a38[], data_020e7ab0[], data_020e7b28[], data_020e7ba0[],
    data_020e7c18[], data_020e7c90[], data_020e7d08[], data_020e7d80[], data_020e7df8[],
    data_020e7e70[], data_020e7ee8[], data_020e7f60[], data_020e7fd8[], data_020e8050[],
    data_020e80c8[];

void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
s32 func_02081cd0(void *pad, u32 key);
s32 func_02081c98(void *pad, u32 key);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_02019b08(u32 state, u32 screen);
u32 func_02019b60(u32 screen);
void func_0201b770(u32 a, u32 b, u32 c);
s32 func_02020440(u32 v);
void func_020204d8(u32 v);
void func_02020504(u32 v);
void func_020994d0(s32 lcd, s32 on);
void func_02099404(u32 *top, u32 *bottom);
void func_02042b24(void);
void func_02071dac(void *b, u32 on);
void func_02072128(void *b, u32 text);
u32 func_02072078(void *b, u32 tile);
void func_020108f4(void *text, const u8 *s);
void func_020107a8(void *b);
s32 func_0208f070(s32 a, s32 b);     /* fx32 divide */
void func_0200af18(s32 layer);
void func_0200af08(s32 layer, u32 v);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_02013a5c(Cnt color);

u32 func_0203959c(u32 v, u32 max);
u32 func_020396a8(s32 x, u32 max);
void func_020397b0(void);
void func_02039f9c(void);
void func_02038b54(void);

#define TEXT(b) PTR_AT(b, 0x10)

static s32 In(const s32 *p, const u8 *b)
{
    s32 in = 0;

    if (p[0] > S32_AT(b, 4) && p[0] < S32_AT(b, 0xc) && p[1] < S32_AT(b, 8) && p[1] > S32_AT(b, 0))
        in = 1;
    return in;
}

static void Free(u8 *b)
{
    if (TEXT(b) != NULL) {
        func_020044a0(TEXT(b));
        TEXT(b) = NULL;
    }
}

/* create a button at (x, y) of size (w, h) with text settings `t` */
static void Button(u8 *b, s32 x, s32 y, s32 w, s32 h, const u8 *t)
{
    s32 right = x + w, bottom = y + h;

    func_020036e8((s32 *)b, &y, &x, &bottom, &right);
    func_020108f4(b + 0x14, t);
    func_020107a8(b);
}

/* place a text-only entry at (x, y) of size (w, h) */
static void Place(u8 *b, s32 x, s32 y, s32 w, s32 h)
{
    S32_AT(b, 0) = y;
    S32_AT(b, 8) = y + h;
    S32_AT(b, 4) = x;
    S32_AT(b, 0xc) = x + w;
}

/* integer to fx32 through the game's float routines, rounded */
static s32 FxU(u32 v)
{
    float f = (float)(u32)(v << 12);

    return (s32)(v != 0 ? 0.5f + f : f - 0.5f);
}

static s32 FxS(s32 v)
{
    float f = (float)(s32)SHL(v, 12);

    return (s32)(v != 0 ? 0.5f + f : f - 0.5f);
}

/* 0x02038b54: leave the page (saving the settings)
 * @difftest stub=0x02042b24 cases=5 */
void func_02038b54(void)
{
    func_02042b24();
    Free(data_020e7a38);
    Free(data_020e7b28);
    Free(data_020e7ab0);
    Free(data_020e7ba0);
    Free(data_020e7df8);
    Free(data_020e7c90);
    Free(data_020e7c18);
    Free(data_020e7d08);
    Free(data_020e7d80);
    Free(data_020e7e70);
    Free(data_020e7ee8);
}

/* move the knob one pixel and set the volume it stands for */
static u32 MoveKnob(s32 d, u32 v)
{
    u8 *k = data_020e78bc;
    s32 mid = S32_AT(k, 4) + ((S32_AT(k, 0xc) - S32_AT(k, 4)) >> 1);

    if (d > 0 ? mid >= data_020e78d4[1] : mid <= data_020e78d4[0])
        return v;
    S32_AT(k, 4) += d;
    S32_AT(k, 0xc) += d;
    if (TEXT(k) != NULL)
        func_02003b98(TEXT(k), S32_AT(k, 4), S32_AT(k, 0));
    k = data_020e78bc;
    return func_020396a8(S32_AT(k, 4) + ((S32_AT(k, 0xc) - S32_AT(k, 4)) >> 1), data_020e78a8);
}

/* press an arrow: `knob` moves left (`dir` 1) or right */
static void Arrow(u8 *b, u8 *knob, s32 dir)
{
    data_020e78b8 = b;
    data_020e78bc = knob;
    data_020e78b0 = dir;
    data_020e78a8 = 0x7f;
}

/* 0x02038d00: the options page's update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1,0x10,0x11 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e7a38:32=int:0:192 @020e7a3c:32=int:0:256 @020e7a40:32=int:0:192 @020e7a44:32=int:0:256 @020e7e70:32=int:0:192 @020e7e74:32=int:0:256 @020e7e78:32=int:0:192 @020e7e7c:32=int:0:256 @020e7e98:32=pick:0,1 @020e7b28:32=int:0:192 @020e7b2c:32=int:0:256 @020e7b30:32=int:0:192 @020e7b34:32=int:0:256 @020e7c90:32=0 @020e7c94:32=0 @020e7c98:32=192 @020e7c9c:32=256 @020e7ba4:32=int:0:200 @020e7bac:32=int:20:256 @020e7d0c:32=int:0:200 @020e7d14:32=int:20:256 @020e78d4:32=int:0:100 @020e78d8:32=int:100:256 @020e78b8:32=pick:0,0x020e7b28 @020e78bc:32=pick:0,0x020e7ba0,0x020e7d08 @020e78b0:32=pick:0,1 @020e78a0:8=pick:0,1 @020e78a8:8=int:0:0x7f stub=0x020994d0,0x02020440,0x02019b08 cases=200 */
void func_02038d00(void)
{
    s32 p[2];

    if (func_02081d34(func_02011f44(), 0xc) != 0) {
        data_020e78b8 = NULL;
        func_0202f6dc(p, func_02011f44());
        if (In(p, data_020e7a38)) {
            /* back */
            if (TEXT(data_020e7a38) != NULL)
                func_02003cbc(TEXT(data_020e7a38), 0);
            func_02020440(0x44);
            func_0201b770(1, 0x3f, 0);
            func_02019b08(data_020e78ac, data_020df0fc);
            data_020e78ac = 0;
            func_02038b54();
        } else if (In(p, data_020e7e70)) {
            /* the backlight switch */
            if (S32_AT(data_020e7e70, 0x28) == 0) {
                S32_AT(data_020e7e70, 0x28) = 1;
                if (TEXT(data_020e7e70) != NULL)
                    func_02003cbc(TEXT(data_020e7e70), 0);
                func_02071dac(data_020e80c8, 0);
                func_02071dac(data_020e79c0, 1);
                func_020994d0(2, 0);
            } else {
                S32_AT(data_020e7e70, 0x28) = 0;
                if (TEXT(data_020e7e70) != NULL)
                    func_02003cbc(TEXT(data_020e7e70), 1);
                func_02071dac(data_020e80c8, 1);
                func_02071dac(data_020e79c0, 0);
                func_020994d0(2, 1);
            }
            func_02039f9c();
        } else if (In(p, data_020e7b28)) {
            Arrow(data_020e7b28, data_020e7ba0, 1);
        } else if (In(p, data_020e7ab0)) {
            Arrow(data_020e7ab0, data_020e7ba0, 0);
        } else if (In(p, data_020e7c90)) {
            data_020e78a0 = 1;
            Arrow(data_020e7c90, data_020e7d08, 1);
        } else if (In(p, data_020e7c18)) {
            data_020e78a0 = 1;
            Arrow(data_020e7c18, data_020e7d08, 0);
        }
        if (data_020e78b8 == NULL)
            return;
        S32_AT(data_020e78b8, 0x28) = 1;
        if (TEXT(data_020e78b8) != NULL)
            func_02003cbc(TEXT(data_020e78b8), 0);
    } else if (func_02081cd0(func_02011f44(), 0xc) != 0) {
        u32 v;
        if (data_020e78bc == NULL)
            return;
        v = data_020e78a0 != 0 ? data_020ebbb8[8] : data_020ebbb8[9];
        v = MoveKnob(data_020e78b0 == 0 ? 1 : -1, v);
        if (data_020e78a0 != 0) {
            func_02020504(v);
            func_02020440(0x44);
        } else {
            func_020204d8(v);
        }
    } else if (func_02081c98(func_02011f44(), 0xc) != 0) {
        if (data_020e78b8 == NULL)
            return;
        S32_AT(data_020e78b8, 0x28) = 0;
        if (TEXT(data_020e78b8) != NULL)
            func_02003cbc(TEXT(data_020e78b8), 1);
        data_020e78b8 = NULL;
        data_020e78bc = NULL;
        data_020e78a0 = 0;
    }
}

/* 0x020393ac: set up the options page
 * @difftest stub=0x0200aad0,0x020107a8,0x02099404 cases=5 */
void func_020393ac(void)
{
    u32 top = 0, bottom = 0;
    Cnt cnt;

    data_020e78b4 = func_0203959c(data_020ebbb8[8], 0x7f);
    data_020e78c0 = func_0203959c(data_020ebbb8[9], 0x7f);
    func_02099404(&top, &bottom);
    data_020e78a0 = 0;
    data_020e78bc = NULL;
    data_020e78b8 = NULL;
    data_020e78a4 = top == 1;
    func_0200b168(3, data_020aaca4.v);
    func_0200af18(3);
    func_0200b0ac(3);
    cnt = data_020b6780;
    func_0200b168(0, cnt.v);
    func_0200b0ac(0);
    func_02013a5c(cnt);
    func_0200af08(0, 0);
    func_0201391c(3, 0, -1);
    func_0200aad0(3, (u32)data_020bea6c, 0, 0, 0, 0);
    func_020397b0();
    if (data_020e78a4 == 0) {
        S32_AT(data_020e7e70, 0x28) = 1;
        if (TEXT(data_020e7e70) != NULL)
            func_02003cbc(TEXT(data_020e7e70), 0);
        func_02071dac(data_020e80c8, 0);
        func_02071dac(data_020e79c0, 1);
        func_02039f9c();
    }
    func_0200b098(1);
    func_0200b098(2);
    if (data_020e78ac == 0)
        data_020e78ac = func_02019b60(data_020df0fc);
}

/* 0x0203959c: the knob position for volume `v` of `max`
 * @difftest int:0:0x7f pick:0,0x7f,0x40 cases=60 */
u32 func_0203959c(u32 v, u32 max)
{
    s32 m = FxU(max);
    s32 q = func_0208f070(FxU(v), m);

    return data_020e78d4[0] + (FxMul(q, FxS(data_020be968)) >> 12) - 6;
}

/* 0x020396a8: the volume (of `max`) for knob position `x`
 * @difftest int:0:0x100 pick:0,0x7f cases=60 */
u32 func_020396a8(s32 x, u32 max)
{
    s32 w = FxS(data_020be968);
    s32 q = func_0208f070(FxU(x - data_020e78d4[0]), w);

    return (FxMul(q, FxU(max)) >> 12) & 0xff;
}

/* 0x020397b0: create the page's buttons
 * @difftest stub=0x020107a8 cases=5 */
void func_020397b0(void)
{
    Button(data_020e7a38, data_020bea24, data_020bea2c, data_020be998, data_020bea18, data_020b6784);
    Button(data_020e7e70, data_020bea14, data_020be9e4, data_020bea0c, data_020be984, data_020b6798);
    Place(data_020e80c8, data_020bea04, data_020be9cc, data_020be990, data_020be9f8);
    func_02072128(data_020e80c8, 0x27);
    func_02071dac(data_020e80c8, 1);
    Place(data_020e79c0, data_020be9f4, data_020be9ec, data_020be974, data_020be98c);
    func_02072128(data_020e79c0, 0x26);
    func_02071dac(data_020e79c0, 0);
    Button(data_020e7d08, data_020e78b4, data_020be9a4, data_020be980, data_020be970, data_020b67e8);
    Button(data_020e7ba0, data_020e78c0, data_020be96c, data_020bea68, data_020bea60, data_020b67e8);
    Button(data_020e7b28, data_020bea5c, data_020bea54, data_020bea50, data_020bea48, data_020b67ac);
    Button(data_020e7ab0, data_020bea44, data_020bea40, data_020bea3c, data_020bea34, data_020b67c0);
    Button(data_020e7df8, data_020bea08, data_020be9bc, data_020be9dc, data_020be9b0, data_020b67d4);
    Button(data_020e7c90, data_020be988, data_020be9ac, data_020bea00, data_020be9a0, data_020b67ac);
    Button(data_020e7c18, data_020be9f0, data_020be9a8, data_020be9e0, data_020be9d8, data_020b67c0);
    Button(data_020e7d80, data_020be9d0, data_020bea4c, data_020be9c8, data_020bea38, data_020b67d4);
    if (TEXT(data_020e7a38) != NULL)
        func_02003cbc(TEXT(data_020e7a38), 1);
    if (TEXT(data_020e7b28) != NULL)
        func_02003cbc(TEXT(data_020e7b28), 1);
    if (TEXT(data_020e7ab0) != NULL)
        func_02003cbc(TEXT(data_020e7ab0), 1);
    if (TEXT(data_020e7c90) != NULL)
        func_02003cbc(TEXT(data_020e7c90), 1);
    if (TEXT(data_020e7c18) != NULL)
        func_02003cbc(TEXT(data_020e7c18), 1);
    Place(data_020e7f60, data_020bea1c, data_020bea28, data_020bea10, data_020be9fc);
    func_02072128(data_020e7f60, 0x23);
    func_02071dac(data_020e7f60, 1);
    Place(data_020e7fd8, data_020be99c, data_020be97c, data_020be9d4, data_020be9c4);
    func_02072128(data_020e7fd8, 0x24);
    func_02071dac(data_020e7fd8, 1);
    Place(data_020e8050, data_020bea30, data_020be9b4, data_020be978, data_020be994);
    func_02072128(data_020e8050, 0x25);
    func_02071dac(data_020e8050, 1);
    func_02039f9c();
}

/* 0x02039f9c: lay out the texts
 * @difftest cases=5 */
void func_02039f9c(void)
{
    u32 t;

    func_0200af08(0, 0);
    func_0201391c(3, 0, -1);
    t = func_02072078(data_020e7f60, 1);
    t = func_02072078(data_020e7fd8, t);
    t = func_02072078(data_020e8050, t);
    t = func_02072078(data_020e80c8, t);
    func_02072078(data_020e79c0, t);
}

/* 0x0203a01c
 * @difftest cases=3 */
void func_0203a01c(void)
{
}

/* 0x0203a020
 * @difftest cases=3 */
void func_0203a020(void)
{
}

/* 0x0203a024
 * @difftest cases=3 */
void func_0203a024(void)
{
}

/* 0x0203a028: the touch pages' common end of frame (empty)
 * @difftest cases=3 */
void func_0203a028(void)
{
}
