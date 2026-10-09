/*
 * The touch-screen menu (status page): the hero's level, experience bar,
 * health and breath, and the twelve upgrade levels shown as dashes.
 * ARM9 main, 0x02035afc - 0x020366a4 (9 functions).
 *
 * data_020e60dc are four 0x58-byte text lines (level, experience, health,
 * breath), data_020e623c twelve 0x6c-byte label boxes and data_020e674c
 * their values (counters from data_020be80c, 0-4 dashes). The experience
 * bar is BG1 scrolled by the part of the level not reached yet (0x98
 * pixels wide; data_020b6af4 the experience each level ends at).
 * Touching the rectangle data_020e60cc opens state 0x16.
 *
 * Testing: the set-up runs with the numbered-file background loader
 * func_0200a8bc and the VBlank wait stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */
typedef struct { s32 q, r; } DivT;

extern Cnt data_020b66ec, data_020b66f0, data_020b66f4, data_020b66f8;
extern const u32 data_020b6af0[], data_020b6af4[];
extern const s32 data_020be79c, data_020be7a0, data_020be7a4, data_020be7a8, data_020be7ac,
    data_020be7b0, data_020be7b4, data_020be7b8, data_020be7bc, data_020be7c0, data_020be7c4,
    data_020be7c8, data_020be7cc, data_020be7d0, data_020be7d4, data_020be7d8, data_020be7dc,
    data_020be7e0, data_020be7e4, data_020be7e8, data_020be7ec, data_020be7f0, data_020be7f4,
    data_020be7f8, data_020be7fc, data_020be800, data_020be804, data_020be808;
extern const u32 data_020be80c[];    /* the twelve counters */
extern const char data_020be83c[];   /* "%s  %d" */
extern const char data_020be844[];   /* "%s   %d/%d" */
extern const char data_020be850[], data_020be854[], data_020be858[], data_020be85c[],
    data_020be860[];                 /* "" "-" "--" "---" "----" */
extern s32 data_020e60cc[4];
extern u8 data_020e60dc[], data_020e6134[], data_020e618c[], data_020e61e4[];
extern u8 data_020e623c[], data_020e674c[];

void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
void func_0202f330(const u32 *pos);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_0203a028(void);
void func_02019b08(u32 state, u32 screen);
void func_02075040(void);
void func_020750a4(void *box);
void func_02075420(void *box, const s32 *rect);
void func_020752e4(void *box, const char *text, s32 a, s32 b, s32 c, s32 d);
void func_02076eac(void *line, const char *text, s32 a, s32 b, s32 c, s32 d);
void func_02076e48(void *line);
void func_02077010(void *line, const s32 *rect);
const char *func_02028a20(u32 id);
s32 func_020804f4(char *dst, const char *fmt, ...);              /* sprintf */
u32 func_02013b84(void *self);
u32 func_02013c84(void *self);
u32 func_02013cf8(void *self);
s32 func_02013e90(void *self);
s32 func_02014004(void *self);
s32 func_02014094(void *self);
s32 func_0204a80c(u32 counter);
void func_0209dff0(DivT *out, s32 n, s32 d);
void func_0200a100(s32 layer, s32 x, s32 y);
void func_0200ac20(s32 layer, u32 x, u32 y, s32 w, s32 h, u32 tile, u32 stride);
void func_0200ace4(s32 layer, u32 x, u32 y, s32 w, s32 h, u32 v);
void func_0200add4(s32 layer, u32 tile, u32 pix);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200af18(s32 layer);
void func_0200aec8(s32 layer, u32 v, u32 pal);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200a8bc(s32 layer, s32 handle, u32 tile, u32 pal, u32 x, u32 y);
void VBlankIntrWait(void);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void func_020754d8(void *p);
void func_020770c0(void *p);

void func_02035afc(void);
void func_02035db0(void);
void func_02035e0c(void);

#define FN(f) ((void *(*)(void *))(void (*)(void))(f))
#define LINE(i) (data_020e60dc + (i) * 0x58)
#define LABEL(i) (data_020e623c + (i) * 0x6c)
#define VALUE(i) (data_020e674c + (i) * 0x6c)

/* 0x02035afc: update the texts
 * @difftest cases=5 */
void func_02035afc(void)
{
    char buf[0x44];
    const char *t;
    s32 i;

    func_02075040();
    t = func_02028a20(0xb6);
    func_020804f4(buf, data_020be83c, t, func_02013b84(data_020deebc) + 1);
    func_02076eac(data_020e60dc, buf, -1, -1, -1, 0);
    {
        const char *s = func_02028a20(0xb8);
        u32 xp = func_02013c84(data_020deebc);
        func_020804f4(buf, data_020be844, s, xp, data_020b6af4[func_02013b84(data_020deebc)]);
    }
    func_02076eac(data_020e6134, buf, -1, -1, -1, 0);
    {
        const char *s = func_02028a20(0xb7);
        s32 a = func_02014004(data_020deebc);
        func_020804f4(buf, data_020be844, s, a, func_02014094(data_020deebc));
    }
    func_02076eac(data_020e618c, buf, -1, -1, -1, 0);
    {
        const char *s = func_02028a20(0xb9);
        u32 a = func_02013cf8(data_020deebc);
        func_020804f4(buf, data_020be844, s, a, func_02013e90(data_020deebc));
    }
    func_02076eac(data_020e61e4, buf, -1, -1, -1, 0);
    for (i = 0; i < 4; i++)
        func_02076e48(LINE(i));
    for (i = 0; i < 0xc; i++) {
        const char *f;
        func_020750a4(LABEL(i));
        switch (func_0204a80c(data_020be80c[i])) {
        case 1: f = data_020be854; break;
        case 2: f = data_020be858; break;
        case 3: f = data_020be85c; break;
        case 4: f = data_020be860; break;
        default: f = data_020be850; break;
        }
        func_020804f4(buf, f);
        func_020752e4(VALUE(i), buf, -1, -1, -1, 1);
        func_020750a4(VALUE(i));
    }
}

/* 0x02035db0: clear BG2 (32 x 24 tiles)
 * @difftest cases=3 */
void func_02035db0(void)
{
    s32 i, j;

    for (i = 0; i < 0x20; i++)
        for (j = 0; j < 0x18; j++)
            func_0200add4(2, (u16)(i + j * 0x20), 0);
}

/* 0x02035e0c: the experience bar
 * @difftest cases=5 */
void func_02035e0c(void)
{
    u32 lv = func_02013b84(data_020deebc);
    u32 next = data_020b6af4[lv];
    u32 base = 0;
    u32 xp = func_02013c84(data_020deebc);
    DivT d;
    s32 w;

    if (lv != 0)
        base = data_020b6af0[lv];
    func_0209dff0(&d, (xp - base) * 0x98, next - base);
    w = 0x98 - d.q;
    func_0200a100(1, w, 0);
    func_0200ac20(1, 6, 2, 0x13, 2, 0x300, 0x20);
    if ((w >> 3) > 0)
        func_0200ace4(1, 6, 2, w >> 3, 2, 0x31f);
}

/* 0x02035ef0: hide the page
 * @difftest cases=5
 * @difftest $A=zero:0x200 @020e60f4:32=$A @020e6150:32=$A cases=5 */
void func_02035ef0(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        u8 *e = LINE(i);
        if (PTR_AT(e, 0x18) != NULL) {
            func_020044a0(PTR_AT(e, 0x18));
            PTR_AT(e, 0x18) = NULL;
        }
        e[0x14] = 0;
    }
    for (i = 0; i < 0xc; i++) {
        VALUE(i)[0x14] = 0;
        LABEL(i)[0x14] = 0;
    }
}

/* 0x02035f70: the status page's update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e60cc:32=int:0:0x60 @020e60d0:32=int:0:0x80 @020e60d4:32=int:0x60:0xc0 @020e60d8:32=int:0x80:0x100 cases=60 */
void func_02035f70(void)
{
    s32 p[2];

    if (func_02081d34(func_02011f44(), 0xc) != 0) {
        const s32 *r = data_020e60cc;
        s32 in = 0;
        func_0202f6dc(p, func_02011f44());
        if (p[0] > r[1] && p[0] < r[3] && p[1] < r[2] && p[1] > r[0])
            in = 1;
        if (in) {
            func_02019b08(0x16, data_020df0fc);
        } else {
            s32 q[2];
            func_0202f6dc(q, func_02011f44());
            func_0202f330((const u32 *)q);
        }
    }
    func_02035e0c();
    func_02035afc();
    func_0203a028();
}

static void Mode(u8 *b, s32 m)
{
    S32_AT(b, 0x34) = 2;
    PTR_AT(b, 0x30) = (void *)&data_020b66f0;
    if (S32_AT(b, 0x18) != m || S32_AT(b, 0x1c) != 0)
        b[0x44] = 1;
    S32_AT(b, 0x18) = m;
    S32_AT(b, 0x1c) = 0;
    S32_AT(b, 0x24) = 0;
}

/* 0x0203605c: set up the status page
 * @difftest stub=0x0200a8bc,0x0200071c cases=5 */
void func_0203605c(void)
{
    s32 a[4], b[4], l1[4], v1[4], l2[4], v2[4];
    s32 i;

    data_020e60cc[0] = data_020be7e8;
    data_020e60cc[2] = data_020be7e0;
    data_020e60cc[1] = data_020be7e4;
    data_020e60cc[3] = data_020be7dc;
    data_020e60cc[0] = data_020be7e8 + 0x90;
    data_020e60cc[2] = data_020be7e0 + 0x90;
    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b098(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
    func_0200b168(0, data_020b66ec.v);
    func_0200af18(0);
    func_0200a8bc(0, 0x179, 0, 0, 0, 0);
    func_0200b168(1, data_020b66f4.v);
    func_0200aec8(1, 0x37f, 0);
    func_0200b168(2, data_020b66f0.v);
    func_0200af18(2);
    func_02035db0();
    func_0200aec8(2, 0x37f, 0);
    func_0200b168(3, data_020b66f8.v);
    func_0200aec8(3, 0x37f, 0);
    for (i = 0; i < 0x13; i++)
        func_0200ac20(3, i + 6, 2, 1, 2, 0x313, 0x20);

    /* the four text lines, in two columns */
    a[0] = data_020be7d8;
    a[1] = data_020be7a0;
    a[2] = data_020be7d0;
    a[3] = data_020be7cc;
    b[0] = data_020be804;
    b[1] = data_020be7c4;
    b[2] = data_020be7c0;
    b[3] = data_020be7bc;
    for (i = 0; i < 4; i++) {
        u8 *e = LINE(i);
        if (S32_AT(e, 0x1c) != 0 || S32_AT(e, 0x20) != 0)
            e[0x34] = 1;
        S32_AT(e, 0x1c) = 0;
        S32_AT(e, 0x20) = 0;
        func_0201391c(S32_AT(e, 0x1c), S32_AT(e, 0x20), S32_AT(e, 0x24));
        if (i % 2 == 0) {
            func_02077010(e, a);
            a[0] += 0xc;
            a[2] += 0xc;
        } else {
            func_02077010(e, b);
            b[0] += 0xc;
            b[2] += 0xc;
        }
    }

    /* the twelve labels and values, six per column */
    l1[0] = data_020be7fc;
    l1[1] = data_020be7f4;
    l1[2] = data_020be7ec;
    l1[3] = data_020be7b4;
    v1[0] = data_020be7b0;
    v1[1] = data_020be7ac;
    v1[2] = data_020be7a8;
    v1[3] = data_020be7a4;
    l2[0] = data_020be808;
    l2[1] = data_020be7d4;
    l2[2] = data_020be7f0;
    l2[3] = data_020be79c;
    v2[0] = data_020be7c8;
    v2[1] = data_020be800;
    v2[2] = data_020be7b8;
    v2[3] = data_020be7f8;
    for (i = 0; i < 0xc; i++) {
        u8 *v = VALUE(i), *l = LABEL(i);
        Mode(v, 2);
        Mode(l, 0);
        if (i < 6) {
            func_02075420(l, l1);
            func_02075420(v, v1);
            l1[0] += 0xc;
            l1[2] += 0xc;
            v1[0] += 0xc;
            v1[2] += 0xc;
        } else {
            func_02075420(l, l2);
            func_02075420(v, v2);
            l2[0] += 0xc;
            l2[2] += 0xc;
            v2[0] += 0xc;
            v2[2] += 0xc;
        }
        S32_AT(l, 0x38) = 0xc0;
        if (S32_AT(l, 0x3c) != -1)
            S32_AT(l, 0x3c) = 0x1869f;
        S32_AT(v, 0x38) = 0xc0;
        if (S32_AT(v, 0x3c) != -1)
            S32_AT(v, 0x3c) = 0x200;
        func_020752e4(l, func_02028a20(i + 0xaa), -1, -1, -1, 1);
    }
    func_02035e0c();
    func_02035afc();
    VBlankIntrWait();
    func_0200b0ac(0);
    func_0200b0ac(1);
    func_0200b0ac(2);
    func_0200b0ac(3);
}

/* 0x02036638: destroy the label boxes
 * @difftest cases=3 */
void func_02036638(void)
{
    func_020a6d58(data_020e623c, 0xc, 0x6c, FN(func_020754d8));
}

/* 0x0203665c: destroy the value boxes
 * @difftest cases=3 */
void func_0203665c(void)
{
    func_020a6d58(data_020e674c, 0xc, 0x6c, FN(func_020754d8));
}

/* 0x02036680: destroy the text lines
 * @difftest cases=3 */
void func_02036680(void)
{
    func_020a6d58(data_020e60dc, 4, 0x58, FN(func_020770c0));
}
