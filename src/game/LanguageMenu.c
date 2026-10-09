/*
 * The language screen (touch screen): six languages, chosen with the
 * stylus or the d-pad and confirmed with "OK"; the first time it starts
 * on the console's language.
 * ARM9 main, 0x0203e3d4 - 0x0203ecd4 (8 functions).
 *
 * Buttons are 0x78-byte text entries ({top, left, bottom, right}, +0x10
 * text actor, +0x14 text settings): data_020eb870 the six languages (names
 * data_020bee20), data_020eb780 the cursor and data_020eb7f8 "OK".
 * data_020eb758 is the selected language; the settings data_020ebbb8 keep
 * it at +0xa, with +0xb set once a language was chosen.
 *
 * Testing: with a faked pad, the language switch (func_02028a50,
 * func_02062884) and the state switch stubbed; leaving with the settings
 * save func_02042b24 stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT / colour value passed by value in a register */

extern Cnt data_020aaca4, data_020b6860;
extern const u8 data_020b6864[], data_020b6878[];   /* text settings */
extern const s32 data_020bedb0, data_020bedb4, data_020bedb8, data_020bedbc, data_020bedc0,
    data_020bedc4, data_020bedc8, data_020bedcc, data_020bedd0, data_020bedd4, data_020bedd8,
    data_020beddc, data_020bede0;
extern const char *const data_020bee20[];   /* the languages' names */
extern const char data_020bee38[];   /* "B_PauseScreenBKG.bin" */
extern u8 data_020ebbb8[];
extern s32 data_020eb758;
extern u8 data_020eb780[], data_020eb7f8[], data_020eb870[];

void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
void func_02019b40(u32 screen);
void func_02028a50(u32 language);
void func_02062884(void);
void func_02042b24(void);
void func_02094440(u8 *cfg);
void func_02071dac(void *b, u32 on);
u32 func_02071f10(void *b, u32 tile);
void func_02071eb0(void *b, const char *s);
void func_020108f4(void *text, const u8 *s);
void func_020107a8(void *b);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200af18(s32 layer);
void func_0200af08(s32 layer, u32 v);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_02013a5c(Cnt color);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void *func_02072130(void *p);

void func_0203e3d4(void);
void func_0203e878(void);
void func_0203e8b0(void);
void func_0203e8f0(void);

#define LANG(i) (data_020eb870 + (i) * 0x78)
#define TEXT(b) PTR_AT(b, 0x10)

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

static void Button(u8 *b, s32 x, s32 y, s32 w, s32 h, const u8 *t)
{
    s32 right = x + w, bottom = y + h;

    func_020036e8((s32 *)b, &y, &x, &bottom, &right);
    func_020108f4(b + 0x14, t);
    func_020107a8(b);
}

/* 0x0203e3d4: leave (saving the settings)
 * @difftest stub=0x02042b24 cases=5 */
void func_0203e3d4(void)
{
    s32 i;

    func_02042b24();
    for (i = 0; i < 6; i++)
        Free(LANG(i));
    Free(data_020eb7f8);
    Free(data_020eb780);
}

/* 0x0203e464: the language screen's per-frame update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x44:32=pick:0,0,1 @$K+0x3c:32=pick:0,0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020eb758:32=int:-1:7 @020eb870:32=0 @020eb874:32=0 @020eb878:32=32 @020eb87c:32=128 @020eb8e8:32=32 @020eb8ec:32=0 @020eb8f0:32=64 @020eb8f4:32=128 @020eb7f8:32=int:0:192 @020eb7fc:32=128 @020eb800:32=192 @020eb804:32=256 @020ebbc3:8=pick:0,1 stub=0x02028a50,0x02062884,0x02019b08,0x02019b40,0x02042b24 cases=150 */
void func_0203e464(void)
{
    s32 old = data_020eb758;
    s32 p[2];

    if (func_02081d34(func_02011f44(), 0xc) != 0) {
        s32 hit = 0, i, y;
        u8 *e;
        func_0202f6dc(p, func_02011f44());
        for (i = 0, e = data_020eb870, y = 0x1b; i < 6 && hit == 0; i++, e += 0x78, y += 0x16) {
            if (In(p, e)) {
                hit = 1;
                data_020eb758 = i;
                MoveTo(data_020eb780, data_020bedc8, y);
            }
        }
        if (In(p, data_020eb7f8)) {
            /* OK */
            data_020ebbb8[0xa] = data_020eb758;
            func_02028a50(data_020ebbb8[0xa]);
            func_02062884();
            if (data_020ebbb8[0xb] != 0)
                func_0203e878();
            else
                func_0203e8b0();
            data_020ebbb8[0xb] = 1;
        }
    } else if (func_02081d34(func_02011f44(), 7) != 0) {
        data_020eb758 = (data_020eb758 + 1) % 6;
    } else if (func_02081d34(func_02011f44(), 6) != 0) {
        if (--data_020eb758 < 0)
            data_020eb758 = 5;
        data_020eb758 = data_020eb758 % 6;
    }
    if (old == data_020eb758)
        return;
    if (TEXT(LANG(old)) != NULL)
        U32_AT(TEXT(LANG(old)), 4) &= ~8u;
    if (TEXT(LANG(data_020eb758)) != NULL)
        U32_AT(TEXT(LANG(data_020eb758)), 4) |= 8;
}

/* 0x0203e78c: set up the language screen
 * @difftest stub=0x0200aad0,0x020107a8 cases=5 */
void func_0203e78c(void)
{
    Cnt cnt;

    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(3, data_020aaca4.v);
    func_0200af18(3);
    func_0200b0ac(3);
    func_0200aad0(3, (u32)data_020bee38, 0, 0, 0, 0);
    cnt = data_020b6860;
    func_0200b168(0, cnt.v);
    func_0200b0ac(0);
    func_02013a5c(cnt);
    func_0200af08(0, 0);
    func_0201391c(1, 0, -1);
    func_0200b098(2);
    func_0200b098(1);
    func_0203e8f0();
}

/* 0x0203e878: back to the previous state
 * @difftest stub=0x02019b40,0x02042b24 cases=5 */
void func_0203e878(void)
{
    func_0201b770(1, 0x3f, 1);
    func_02019b40(data_020df0fc);
    func_0203e3d4();
}

/* 0x0203e8b0: on to the title (states 1 / 2)
 * @difftest stub=0x02019b08,0x02042b24 cases=5 */
void func_0203e8b0(void)
{
    func_0201b770(2, 0x3f, 1);
    func_02019b08(1, 0);
    func_02019b08(2, 1);
    func_0203e3d4();
}

/* 0x0203e8f0: create the buttons; the cursor starts on the language set
 * (the console's, the first time)
 * @difftest @020ebbc3:8=pick:0,1 @020ebbc2:8=int:0:5 stub=0x020107a8 cases=20 */
void func_0203e8f0(void)
{
    u8 cfg[0x50];
    u8 *e;
    s32 i, y, lang;
    u32 t;

    for (i = 0, e = data_020eb870, y = 0x1b; i < 6; i++, e += 0x78, y += 0x16) {
        s32 x = data_020bedd0;
        S32_AT(e, 0) = y;
        S32_AT(e, 4) = x;
        S32_AT(e, 8) = y + data_020bedd4;
        S32_AT(e, 0xc) = x + data_020bedc4;
        func_02071eb0(e, data_020bee20[i]);
        if (TEXT(e) != NULL)
            func_02003cbc(TEXT(e), 0);
        func_02071dac(e, 1);
        if (i > 0 && TEXT(e) != NULL)
            U32_AT(TEXT(e), 4) &= ~8u;
    }
    Button(data_020eb7f8, data_020bede0, data_020bedb4, data_020bedbc, data_020bedb0, data_020b6878);
    if (TEXT(data_020eb7f8) != NULL)
        func_02003cbc(TEXT(data_020eb7f8), 0);
    if (TEXT(data_020eb7f8) != NULL)
        U32_AT(TEXT(data_020eb7f8), 4) |= 8;
    for (i = 0, t = 1; i < 6; i++)
        t = func_02071f10(LANG(i), t);
    Button(data_020eb780, data_020bedb8, data_020beddc, data_020bedd8, data_020bedcc, data_020b6864);
    if (TEXT(data_020eb780) != NULL)
        func_02003cbc(TEXT(data_020eb780), 0);

    if (data_020ebbb8[0xb] == 0) {
        func_02094440(cfg);
        switch (cfg[0]) {
        case 0: data_020ebbb8[0xa] = 0; break;
        case 1: data_020ebbb8[0xa] = 0; break;
        case 2: data_020ebbb8[0xa] = 1; break;
        case 3: data_020ebbb8[0xa] = 2; break;
        case 4: data_020ebbb8[0xa] = 3; break;
        case 5: data_020ebbb8[0xa] = 4; break;
        default: data_020ebbb8[0xa] = 0; break;
        }
    }
    lang = data_020ebbb8[0xa];
    y = lang * 0x16 + 0x1b;
    data_020eb758 = lang;
    MoveTo(data_020eb780, data_020bedc0, y);
}

/* 0x0203ec7c: destroy the language buttons
 * @difftest cases=3 */
void func_0203ec7c(void)
{
    func_020a6d58(data_020eb870, 6, 0x78, func_02072130);
}

/* 0x0203eca0: leave
 * @difftest cases=3 */
void func_0203eca0(void)
{
    func_0200b098(0);
    func_0200b098(2);
    func_0200b098(1);
    func_0200b098(3);
}
