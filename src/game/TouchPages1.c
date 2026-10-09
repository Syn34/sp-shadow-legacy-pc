/*
 * The touch-screen pages (part 1): an empty page, the records page and the
 * page where the stylus spins the hero.
 * ARM9 main, 0x020343ac - 0x02034d50 (13 functions).
 *
 * The records page lists fourteen entries (data_020b65d4, 0x14 bytes:
 * {text, counter, threshold, x, y}) whose counter has reached the
 * threshold, each with a second column from data_020be62c; touching the
 * rectangle data_020e5fe8 opens state 0x17.
 * On the spin page a stroke longer than 4.0 (in pixels << 12) sets the
 * hero's spin speed (data_020e5ff8, at most 5.0, or 3.0 unless the hero's
 * flag 0x4000 is set) and turns it along the stroke; the speed then decays by
 * 0x266 per frame and the page closes a second after the last stroke.
 * data_020e5ffc / data_020e6004 hold the stroke's previous and current
 * touch positions.
 *
 * Testing: the updates run with a faked pad (data_020f6f30) and hero (with
 * a zeroed camera); func_02034d1c with the path display's destructor
 * func_02055bdc stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT / colour value passed by value in a register */

extern Cnt data_020aacac, data_020b65cc, data_020b65d0;
extern const s32 data_020b65d4[];    /* 14 x {text, counter, threshold, x, y} */
extern const s32 data_020b6ccc, data_020b6ce0;
extern const s32 data_020be5d0, data_020be5d4, data_020be5d8, data_020be5dc;
extern const u8 *const data_020be62c[];
extern const s32 data_020be664;
extern const char data_020be668[];   /* "UI_SpellsMove_BKG.bin" */
extern s8 data_020df10c;
extern u8 data_020d9f9c[];
extern s8 data_020e3a80;
extern u32 data_020e3a84;
extern s32 data_020e5fe8[4];
extern s32 data_020e5ff8;
extern s32 data_020e5ffc[2];
extern s32 data_020e6004[2];
extern void *data_020e6028;
extern const s32 data_020b6cc4;

void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
s32 func_02081cd0(void *pad, u32 key);
u32 func_02081c84(void *pad);
void func_0202f330(const u32 *pos);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_0203a028(void);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200af18(s32 layer);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200a8bc(s32 layer, s32 handle, u32 tile, u32 pal, u32 x, u32 y);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_0200add4(s32 layer, u32 tile, u32 pix);
void func_02013a5c(Cnt color);
u32 func_0201341c(u32 tile, s32 x, s32 y, const u8 *str, u32 align);
s32 func_0204a88c(u32 counter);
const char *func_02028a20(u32 id);
void func_02019b08(u32 state, u32 screen);
void func_0201b770(u32 a, u32 b, u32 c);
void func_020095d8(s32 layer, u32 color, s32 x0, s32 y0, s32 x1, s32 y1);
void func_020150a0(s32 *out, const s32 *a, const s32 *b);
s32 func_0208eff8(s32 v);
s32 func_0208f30c(s32 x, s32 y);
void func_02016d04(void *self, s32 speed);
void func_02016df8(void *self, s32 step);
s32 func_0202df68(const s32 *v);
void func_0202ded8(s32 *v, const s32 *s);
void func_0202dea4(s32 *v);
void func_02055bdc(void *p);
void func_02020144(void);

void func_02034478(void);
void func_02034878(void);
void func_020347ac(void);
void func_020348b8(void);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))

static void Touch(s32 *p)
{
    func_0202f6dc(p, func_02011f44());
}

/* a touch on the side tabs switches page */
static void SideTabs(void)
{
    s32 p[2];

    Touch(p);
    func_0202f330((const u32 *)p);
}

/* 0x020343ac: the empty page's update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 cases=30 */
void func_020343ac(void)
{
    if (func_02081d34(func_02011f44(), 0xc) != 0)
        SideTabs();
    func_0203a028();
}

/* 0x020343f0: set up the empty page
 * @difftest stub=0x0200a8bc cases=5 */
void func_020343f0(void)
{
    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020aacac.v);
    func_0200a8bc(0, 0x11b, 0, 0, 0, 0);
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
}

/* 0x02034478: clear BG2 (32 x 24 tiles)
 * @difftest cases=3 */
void func_02034478(void)
{
    s32 i, j;

    for (i = 0; i < 0x20; i++)
        for (j = 0; j < 0x18; j++)
            func_0200add4(2, (u16)(i + j * 0x20), 0);
}

/* 0x020344d4
 * @difftest cases=3 */
void func_020344d4(void)
{
}

/* 0x020344d8: the records page's update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e5fe8:32=int:0:0x60 @020e5fec:32=int:0:0x80 @020e5ff0:32=int:0x60:0xc0 @020e5ff4:32=int:0x80:0x100 cases=60 */
void func_020344d8(void)
{
    s32 p[2];

    if (func_02081d34(func_02011f44(), 0xc) != 0) {
        const s32 *r = data_020e5fe8;
        s32 in = 0;
        Touch(p);
        if (p[0] > r[1] && p[0] < r[3] && p[1] < r[2] && p[1] > r[0])
            in = 1;
        if (in)
            func_02019b08(0x17, data_020df0fc);
        else
            SideTabs();
    }
    func_0203a028();
}

/* 0x020345bc: set up the records page
 * @difftest stub=0x0200a8bc cases=5 */
void func_020345bc(void)
{
    Cnt cnt;
    u32 tile = 1;
    const s32 *e;
    s32 i;

    data_020e5fe8[0] = data_020be5d8;
    data_020e5fe8[2] = data_020be5d0;
    data_020e5fe8[1] = data_020be5dc;
    data_020e5fe8[3] = data_020be5d4;
    data_020e5fe8[0] = data_020be5d8 + 0x90;
    data_020e5fe8[2] = data_020be5d0 + 0x90;
    func_0200b168(0, data_020b65cc.v);
    func_0200af18(0);
    func_0200a8bc(0, 0x13d, 0, 0, 0, 0);
    cnt = data_020b65d0;
    func_0200b168(2, cnt.v);
    func_0200af18(2);
    func_02034478();
    func_02013a5c(cnt);
    func_0201391c(0, 0, -1);
    for (i = 0, e = data_020b65d4; i < 0xe; i++, e += 5) {
        if (func_0204a88c(e[1]) >= e[2])
            tile = func_0201341c(tile, e[3], e[4], (const u8 *)func_02028a20(e[0]), 0);
    }
    func_0201391c(2, 0, -1);
    for (i = 0, e = data_020b65d4; i < 0xe; i++, e += 5) {
        if (func_0204a88c(e[1]) >= e[2])
            tile = func_0201341c(tile, e[3] + 0x42, e[4], data_020be62c[i], 0);
    }
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b0ac(2);
    func_0200b098(3);
}

/* 0x020347ac: turn the hero by the spin speed (while it is allowed to)
 * @difftest $V=zero:0x40 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=pick:0,0x4000,0xc000,0x8000 @020deec4:32=$H @020deef8:32=int:0:0x6487 @020e5ff8:32=int:0:0x5000 cases=40 */
void func_020347ac(void)
{
    u32 f = U32_AT(HERO(), 0x38);
    s32 a;

    if ((s8)BITS(f, 14, 1) != 1)
        return;
    a = S32_AT(data_020deebc, 0x3c);
    if ((s8)BITS(f, 15, 1) == 1)
        a += FxMul(data_020b6ce0, data_020e5ff8);
    else
        a += FxMul(data_020b6ccc, data_020e5ff8);
    if (a >= g_FxTwoPi)
        a -= g_FxTwoPi;
    S32_AT(data_020deebc, 0x3c) = a;
}

/* 0x02034878: the spin slows down
 * @difftest @020e5ff8:32=int:0:0x5000 cases=20 */
void func_02034878(void)
{
    data_020e5ff8 -= 0x266;
    if (data_020e5ff8 < 0)
        data_020e5ff8 = 0;
    func_02016d04(data_020deebc, data_020e5ff8);
}

/* 0x020348b8: leave the spin page
 * @difftest stub=0x0200aad0 cases=5 */
void func_020348b8(void)
{
    func_0200aad0(0, (u32)data_020be668, 0, 0, 0, 0);
    data_020e3a80 = 0;
    data_020e5ff8 = 0;
}

/* 0x02034908
 * @difftest cases=3 */
void func_02034908(void)
{
}

/* 0x0203490c: the spin page's update
 * @difftest $H=zero:0x100 @$H+0x38:32=pick:0,0x800000,0x804000,0x80c000 $C=zero:0xb0 @$H+0x44:32=$C @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1,0x10 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e6004:32=int:0:256 @020e6008:32=int:0:192 @020e5ffc:32=int:0:256 @020e6000:32=int:0:192 @020e3a80:8=pick:0,1 @020e3a84:32=int:55:65 @020e5ff8:32=int:0:0x5000 @020df10c:8=pick:0,0,0,1 cases=100 */
void func_0203490c(void)
{
    s32 d[2], v[2], w[3], t1[2], t2[2], n;

    if (data_020df10c != 0) {
        u32 f;
        func_0201b770(1, 0x3f, 1);
        func_02019b08(0x14, 1);
        f = U32_AT(data_020deebc, 0x20) & ~0x20u;
        U32_AT(data_020deebc, 0x20) = f;
        data_020d9f9c[0x182] = BITS(f, 5, 1);
        return;
    }
    if (PTR_AT(data_020deebc, 8) == NULL) {
        /* nothing */
    } else if (func_02081d34(func_02011f44(), 0xc) != 0) {
        /* a stroke starts */
        data_020e3a84 = 0;
        Touch(t1);
        data_020e5ffc[0] = t1[0];
        data_020e5ffc[1] = t1[1];
        data_020e6004[0] = t1[0];
        data_020e6004[1] = t1[1];
    } else if (func_02081cd0(func_02011f44(), 0xc) != 0
               && func_02081c84(func_02011f44()) == 0
               && (s8)BITS(U32_AT(HERO(), 0x38), 23, 1) != 0) {
        s32 len;

        if (data_020e3a80 == 0)
            data_020e3a80 = 1;
        data_020e3a84 = 0;
        Touch(t1);
        Touch(t2);
        func_020095d8(0, 0x46, data_020e6004[0], data_020e6004[1], t1[0], t2[1]);
        Touch(t1);
        data_020e6004[0] = t1[0];
        data_020e6004[1] = t1[1];
        func_020150a0(d, data_020e6004, data_020e5ffc);
        func_02003b08(v, d, 0xc);
        d[0] = v[0];
        d[1] = v[1];
        w[0] = v[0];
        w[1] = data_020be664;
        w[2] = v[1];
        len = func_0208eff8(FxMul(v[0], v[0]) + FxMul(w[1], w[1]) + FxMul(v[1], v[1]));
        if (len > 0x4000) {
            data_020e5ff8 = len - 0x3000;
            if (data_020e5ff8 > 0x5000)
                data_020e5ff8 = 0x5000;
            if ((s8)BITS(U32_AT(HERO(), 0x38), 14, 1) == 0 && data_020e5ff8 > 0x3000)
                data_020e5ff8 = 0x3000;
            func_02016d04(data_020deebc, data_020e5ff8);
            n = func_0202df68(w);
            if (n > 1)
                func_0202ded8(w, &n);
            else
                func_0202dea4(w);
            S32_AT(data_020deebc, 0x10) = func_0208f30c(w[0], w[2]);
            func_02016df8(data_020deebc, data_020b6cc4);
        } else {
            func_02034878();
        }
        func_020347ac();
        data_020e5ffc[0] = data_020e6004[0];
        data_020e5ffc[1] = data_020e6004[1];
    } else if (data_020e3a80 != 0) {
        /* no stroke: slow down, close after a second */
        func_02034878();
        func_020347ac();
        if (++data_020e3a84 > 0x3c)
            func_020348b8();
    }
    func_0203a028();
}

/* 0x02034c74: set up the spin page
 * @difftest stub=0x0200aad0 cases=5 */
void func_02034c74(void)
{
    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020aacac.v);
    func_0200af18(0);
    func_0200aad0(0, (u32)data_020be668, 0, 0, 0, 0);
    data_020e3a80 = 0;
    data_020e5ff8 = 0;
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
}

/* 0x02034d1c: close the travel page (free its path, pause the music)
 * @difftest @020e6028:32=0 stub=0x02055bdc cases=5 */
void func_02034d1c(void)
{
    func_02055bdc(data_020e6028);
    data_020e6028 = NULL;
    func_02020144();
}
