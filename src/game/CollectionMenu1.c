/*
 * The touch-screen menu (collection and warp map, part 1): closing it, the
 * per-frame updates of the collection and warp pages, and setting them up.
 * ARM9 main, 0x02031834 - 0x02032a88 (8 functions).
 *
 * Icons are 0x3c-byte sprites from data_020e52d0 (count in data_020e5204,
 * at most 0x20; +0x04..+0x10 rectangle, +0x14 shown, +0x18 text actor,
 * +0x1c..+0x2c animation parameters, +0x30, +0x34 item / warp point,
 * +0x38 flags); icon 0 marks where the player is. data_020e5230 is the
 * "back" button, data_020e5264 the description box, data_020e5208 the
 * page (0 collection, 1 warp map) and data_020be438 the selected icon.
 * Collected items are flagged at data_020d9f9c + 0x1c2, unlocked warp
 * points at data_020d9f9c + 0x1a2.
 *
 * Testing: the set-ups run with the numbered-file background loader
 * func_0200a8bc and the sprite animation loader func_02076640 stubbed (they
 * read from the card); the updates with a faked pad (data_020f6f30), hero
 * and text actors.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

/* animation parameters copied into an icon */
typedef struct {
    s32 a, b, c;
    u8 d;
    s32 e;
} IconAnim;

extern Cnt data_020b6574, data_020b6578, data_020b657c;
extern const IconAnim data_020b6580, data_020b6594, data_020b65a8;
extern const s32 data_020be434[], data_020be43c[], data_020be440[], data_020be444[],
    data_020be448[], data_020be44c[], data_020be450[], data_020be454[];
extern s32 data_020be438;
extern u8 *data_020c3b60;            /* 0x60-byte area records */
extern s32 *data_020c3b74;           /* item positions {x, y} */
extern u8 *data_020c3b78;            /* warp points, 0x10 bytes: {state, ?, x, y} */
extern u8 *data_020c3b7c;            /* warp table, 0xc bytes: {area, point, level} */
extern u8 data_020d5b8c[];
extern u8 data_020d7b8c[];
extern u8 data_020d9f9c[];
extern u8 data_020df104;
extern s32 data_020e5204;
extern s32 data_020e5208;
extern u8 data_020e5230[];
extern u8 data_020e5264[];
extern u8 data_020e52d0[];

void *func_02011f44(void);
u8 *func_02011f34(void);
s32 func_02011bc8(void);
s32 func_02081d34(void *pad, u32 key);
void func_02076b44(void *obj, const char *text, u32 fbits);
void func_02074ca0(void *sprite, const s32 *pos);
void func_020748e0(void *sprite, const s32 *pos);
void func_02074c14(void *sprite, u32 a, u32 tile, u32 tile2, u32 b, s32 c);
void func_02076640(void *sprite);
void *func_020752cc(void *box, s32 text);
void func_02075040(void);
void func_020750a4(void *box);
void func_02075420(void *box, const s32 *rect);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
void func_02019660(u32 v);
void func_02019640(u32 v);
s32 func_02019650(void);
void func_0204378c(u32 screen);
s32 func_0204a80c(u32 counter);
s32 func_0204ab08(u32 spell);
void func_02013d64(void *obj, s32 v);
void func_0203a028(void);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200a8bc(s32 layer, s32 handle, u32 tile, u32 pal, u32 x, u32 y);
void func_0200ac20(s32 layer, u32 x, u32 y, s32 w, s32 h, u32 tile, u32 stride);
void func_0200ace4(s32 layer, u32 x, u32 y, s32 w, s32 h, u32 v);
void func_0200add4(s32 layer, u32 tile, u32 pix);
void func_0202f330(const u32 *pos);
void func_0202f6dc(s32 *out, const u8 *pad);

void func_02031d3c(void);
void func_020327c4(void);
void func_02032980(void);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define ICON(i) (data_020e52d0 + (i) * 0x3c)
#define AREA(a) (data_020c3b60 + (a) * 0x60)

static void Touch(s32 *p)
{
    func_0202f6dc(p, func_02011f44());
}

/* method 0 (show) with its address in r2, like the original */
static void Show(u8 *s, u32 on)
{
    void (*m)(void *, u32, void *) = VCALL(s, 0, void (*)(void *, u32, void *));

    m(s, on, (void *)m);
}

/* put an icon at (left, top) */
static void PlaceIcon(u8 *s, s32 top, s32 left, const IconAnim *t)
{
    S32_AT(s, 4) = top;
    S32_AT(s, 8) = left;
    S32_AT(s, 0xc) = top + 0x10;
    S32_AT(s, 0x10) = left + 0x10;
    Show(s, 1);
    s[0x14] = 1;
    S32_AT(s, 0x1c) = t->a;
    S32_AT(s, 0x20) = t->b;
    S32_AT(s, 0x24) = t->c;
    s[0x28] = t->d;
    S32_AT(s, 0x2c) = t->e;
    func_02076640(s);
    Show(s, 1);
    s[0x30] = 0;
    func_02003cbc(PTR_AT(s, 0x18), 0);
}

/* the icon just placed stands for `id`; count it */
static void AddIcon(s32 id)
{
    S32_AT(ICON(data_020e5204), 0x34) = id;
    U32_AT(ICON(data_020e5204), 0x38) &= 0xff000000u;
    U32_AT(ICON(data_020e5204), 0x38) |= 1;
    U32_AT(ICON(data_020e5204), 0x38) |= 0x80000000u;
    data_020e5204++;
}

/* an icon for every collected item not shown yet (`check`) */
static void AddItems(s32 check, const IconAnim *t)
{
    s32 n = S32_AT(data_020d5b8c, 0xfc4);
    s32 i, k;

    for (i = 0; i < n; i++) {
        s32 cnt;
        if ((s8)data_020d9f9c[0x1c2 + i] == 0)
            continue;
        cnt = data_020e5204;
        if (cnt >= 0x20)
            continue;
        if (check) {
            for (k = 0; k < cnt; k++) {
                if (i == S32_AT(ICON(k), 0x34))
                    break;
            }
            if (k != cnt)
                continue;
        }
        PlaceIcon(ICON(cnt), data_020c3b74[i * 2 + 1] - 8, data_020c3b74[i * 2] - 8, t);
        AddIcon(i);
    }
}

/* 0x02031834: close the collection / warp map
 * @difftest @020e5208:32=pick:0,1 cases=10 */
void func_02031834(void)
{
    s32 i;

    if (data_020e5208 == 1) {
        u8 *hud = func_02011f34();
        if (func_02011bc8() == 0) {
            func_02076b44(hud + 0x7c4, NULL, 0);
            func_02076b44(hud + 0x768, NULL, 0);
        }
    }
    data_020e5208 = 0;
    data_020e5230[0x14] = 0;
    data_020e5264[0x14] = 0;
    if (data_020e5204 <= 0)
        return;
    for (i = 0; i < data_020e5204; i++) {
        u8 *s = ICON(i);
        if (PTR_AT(s, 0x18) != NULL) {
            func_020044a0(PTR_AT(s, 0x18));
            PTR_AT(s, 0x18) = NULL;
        }
        s[0x14] = 0;
    }
}

/* 0x02031910: per-frame update of the collection: new items appear;
 * tapping an icon shows its description
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 $A=zero:0x200 @020e5248:32=$A @020e52e8:32=$A @020e5324:32=$A @020e5360:32=$A @020e539c:32=$A @020e5208:32=0 @020e5204:32=int:0:4 @020be438:32=pick:-1,1 stub=0x02076640 cases=60
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 $V=zero:0x40 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=pick:0,0x200 @020deec4:32=$H $A=zero:0x200 @020e5248:32=$A @020e52e8:32=$A @020e5324:32=$A @020e5360:32=$A @020e539c:32=$A @020e5208:32=1 @020e5204:32=int:0:4 stub=0x02076640 cases=30 */
void func_02031910(void)
{
    s32 n, k, j;

    if (data_020e5208 != 0) {
        func_02031d3c();
        return;
    }
    if (func_02081d34(func_02011f44(), 0xc) != 0) {
        s32 p[2];
        Touch(p);
        func_0202f330((u32 *)p);
    }
    AddItems(1, &data_020b6580);

    if (data_020be438 != -1) {
        u8 *s = ICON(data_020be438);
        if ((s8)data_020d9f9c[0x1c2 + S32_AT(s, 0x34)] == 0) {
            S32_AT(s, 0x34) = -1;
            Show(s, 0);
            data_020be438 = -1;
            func_020752cc(data_020e5264, -1);
            func_02075040();
        }
    }

    n = data_020e5204;
    for (k = 1; k < data_020e5204; k++) {
        s32 t[2];
        u8 *s = ICON(k);
        Touch(t);
        func_02074ca0(s, t);
        if (!(U32_AT(s, 0x38) & 4))
            continue;
        for (j = 1; j < data_020e5204; j++) {
            if (j != k)
                U32_AT(ICON(j), 0x38) &= ~0x10000u;
            else
                U32_AT(ICON(j), 0x38) |= 0x10000;
        }
        if (S32_AT(s, 0x34) != -1) {
            data_020be438 = k;
            func_020752cc(data_020e5264, S32_AT(s, 0x34) + 0xba);
            func_02075040();
        } else {
            func_020752cc(data_020e5264, -1);
            func_02075040();
        }
        break;
    }
    (void)n;

    {
        u8 *a = AREA(func_02019650());
        s32 x = S32_AT(a, 0x4c), y = S32_AT(a, 0x50);
        S32_AT(ICON(0), 4) = y;
        S32_AT(ICON(0), 8) = x;
        S32_AT(ICON(0), 0xc) = y + 0x10;
        S32_AT(ICON(0), 0x10) = x + 0x10;
    }
    func_020750a4(data_020e5264);
    func_0203a028();
}

/* 0x02031d10: show or hide an icon's text
 * @difftest $T=ptr:8:4 $S=ptr:0x3c:4 @$S+0x18:32=pick:0 $S u8 cases=5
 * @difftest $T=ptr:8:4 $S=ptr:0x3c:4 @$S+0x18:32=$T $S u8 cases=20 */
void func_02031d10(u8 *s, u8 on)
{
    u8 *t = PTR_AT(s, 0x18);

    if (t == NULL)
        return;
    U32_AT(t, 4) = (U32_AT(t, 4) & ~8u) | ((on & 1) << 3);
    s[0x15] = on;
}

/* 0x02031d3c: per-frame update of the warp map: tapping a warp point
 * travels there (it costs magic); "back" returns to the game
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 $V=zero:0x40 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=pick:0,0x200 @020deec4:32=$H $A=zero:0x200 @020e5248:32=$A @020e52e8:32=$A @020e5324:32=$A @020e5360:32=$A @020e539c:32=$A @020e5204:32=int:0:4 cases=60 */
void func_02031d3c(void)
{
    s32 p[2];
    s32 i;

    Touch(p);
    func_020748e0(data_020e5230, p);
    if (U32_AT(data_020e5230, 0x1c) & 8) {
        func_0201b770(1, 0x3f, 0);
        func_02019b08(0xe, 1);
        func_0204378c(0);
        U32_AT(HERO(), 0x38) &= ~0x200u;
    } else {
        for (i = 0; i < data_020e5204; i++) {
            s32 t[2];
            u8 *s = ICON(i);
            Touch(t);
            func_02074ca0(s, t);
            if (!(U32_AT(s, 0x38) & 4))
                continue;
            func_02019660(U32_AT(data_020c3b78 + S32_AT(s, 0x34) * 0x10, 0));
            data_020df104 = 1;
            func_02019640(0);
            func_0204378c(0);
            U32_AT(HERO(), 0x38) &= ~0x200u;
            func_02013d64(data_020deebc, -func_0204ab08(0xf));
        }
    }
    if (!((U32_AT(HERO(), 0x38) >> 9) & 1)) {
        void *h;
        func_0204378c(0);
        h = HERO();
        PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = h;
        func_0201b770(1, 0x3f, 0);
        func_02019b08(0xe, 1);
    }
    func_0203a028();
}

/* 0x02031f14: set up the collection page
 * @difftest $A=zero:0x200 @020e5248:32=$A @020e52e8:32=$A @020e5324:32=$A @020e5360:32=$A @020e539c:32=$A @020e53d8:32=$A @020e5414:32=$A @020e5450:32=$A @020e548c:32=$A stub=0x0200a8bc,0x02076640 cases=5 */
void func_02031f14(void)
{
    s32 rect[4];

    func_02032980();
    func_0200ac20(1, 0x1a, 0, 6, 6, 0, 0x20);
    func_0200ac20(1, 0x1a, 6, 6, 6, 6, 0x20);
    func_0200ac20(1, 0x1a, 0xc, 6, 6, 0xc, 0x20);
    func_0200ac20(1, 0x1a, 0x12, 6, 6, 0x12, 0x20);
    data_020e5204 = 0;
    {
        u8 *a = AREA(func_02019650());
        PlaceIcon(ICON(0), S32_AT(a, 0x50) - 8, S32_AT(a, 0x4c) - 8, &data_020b6594);
    }
    S32_AT(ICON(0), 0x34) = -1;
    data_020e5204++;
    U32_AT(ICON(0), 0x38) &= 0xff000000u;
    U32_AT(ICON(0), 0x38) |= 1;
    U32_AT(ICON(0), 0x38) |= 0x80000000u;
    AddItems(0, &data_020b6580);

    data_020be438 = -1;
    S32_AT(data_020e5264, 0x34) = 2;
    PTR_AT(data_020e5264, 0x30) = (void *)&data_020b6574;
    if (U32_AT(data_020e5264, 0x18) != 0 || U32_AT(data_020e5264, 0x1c) != 0)
        data_020e5264[0x44] = 1;
    U32_AT(data_020e5264, 0x18) = 0;
    U32_AT(data_020e5264, 0x1c) = 0;
    U32_AT(data_020e5264, 0x24) = 1;
    rect[0] = data_020be44c[0];
    rect[1] = data_020be440[0];
    rect[2] = data_020be450[0];
    rect[3] = data_020be454[0];
    func_02075420(data_020e5264, rect);
    if (S32_AT(data_020e5264, 0x3c) != -1)
        S32_AT(data_020e5264, 0x3c) = 0x1f;
    S32_AT(data_020e5264, 0x38) = 0x304;
    func_0200b0ac(0);
    func_0200b0ac(1);
    func_0200b0ac(2);
    func_0200b098(3);
}

/* the area whose warp points the map shows */
static s32 WarpArea(s32 m)
{
    switch (m) {
    case 2:
        return 1;
    case 3:
    case 40:
        return 0x27;
    case 4:
    case 34:
    case 36:
        return 7;
    case 5:
        return 0x1e;
    case 8:
    case 10:
    case 11:
    case 22:
    case 23:
        return 9;
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 35:
        return 0xc;
    case 20:
    case 21:
    case 37:
        return 0x13;
    case 29:
    case 33:
        return 0x1c;
    case 38:
        return 0x1a;
    default:
        return m;
    }
}

/* 0x02032354: set up the warp map
 * @difftest $A=zero:0x200 @020e5248:32=$A @020e52e8:32=$A @020e5324:32=$A @020e5360:32=$A @020e539c:32=$A @020e53d8:32=$A @020e5414:32=$A @020e5450:32=$A @020e548c:32=$A stub=0x0200a8bc,0x02076640 cases=5 */
void func_02032354(void)
{
    s32 lv, a;

    data_020e5208 = 1;
    func_02032980();
    S32_AT(data_020e5230, 0x10) = data_020be434[0];
    S32_AT(data_020e5230, 4) = data_020be448[0];
    S32_AT(data_020e5230, 8) = data_020be444[0];
    S32_AT(data_020e5230, 0xc) = data_020be43c[0];
    Show(data_020e5230, 1);
    data_020e5230[0x14] = 1;
    func_02074c14(data_020e5230, 1, 0x18, 0xd8, 0x20, -1);
    data_020e5204 = 0;
    lv = func_0204a80c(0xb);
    a = WarpArea(func_02019650());
    if (a != 0x19) {
        s32 region = S32_AT(AREA(a), 0x28);
        s32 n = S32_AT(data_020d7b8c, 0x404);
        s32 i;
        for (i = 0; i < n; i++) {
            u8 *w = data_020c3b7c + i * 0xc;
            s32 id, cnt;
            u8 *pt;
            if (region != S32_AT(w, 0))
                continue;
            if (S32_AT(w, 8) > lv)
                continue;
            id = S32_AT(w, 4);
            if (data_020d9f9c[id + 0x1a2] == 0)
                continue;
            cnt = data_020e5204;
            if (cnt >= 0x20)
                continue;
            pt = data_020c3b78 + id * 0x10;
            PlaceIcon(ICON(cnt), S32_AT(pt, 0xc), S32_AT(pt, 8), &data_020b65a8);
            AddIcon(S32_AT(data_020c3b7c + i * 0xc, 4));
        }
    }
    func_0200b0ac(0);
    func_0200b0ac(1);
    func_0200b0ac(2);
    func_0200b098(3);
}

/* 0x020327c4: the map background for the furthest world reached
 * @difftest @020da14e:8=pick:0,1 @020da14f:8=pick:0,0,1 @020da150:8=pick:0,0,1 @020da151:8=pick:0,0,1 @020da152:8=pick:0,0,1 @020da153:8=pick:0,0,1 @020da154:8=pick:0,0,1 stub=0x0200a8bc cases=60 */
void func_020327c4(void)
{
    s32 id;

    if (data_020d9f9c[0x1b8] != 0)
        id = 0x132;
    else if (data_020d9f9c[0x1b7] != 0)
        id = 0x131;
    else if (data_020d9f9c[0x1b6] != 0)
        id = 0x130;
    else if (data_020d9f9c[0x1b5] != 0)
        id = 0x12f;
    else if (data_020d9f9c[0x1b4] != 0)
        id = 0x12e;
    else if (data_020d9f9c[0x1b3] != 0)
        id = 0x12d;
    else if (data_020d9f9c[0x1b2] != 0)
        id = 0x12c;
    else
        id = 0x12b;
    func_0200a8bc(0, id, 0, 0, 0, 0);
}

/* 0x02032980: the common set-up of both pages
 * @difftest stub=0x0200a8bc,0x02076640 cases=5 */
void func_02032980(void)
{
    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020b657c.v);
    func_020327c4();
    func_0200b168(1, data_020b6578.v);
    func_0200a8bc(1, 0x133, 0, 0, 0, 0);
    func_0200add4(1, 0x1f, 0);
    func_0200ace4(1, 0, 0, 0x20, 0x18, 0x1f);
    func_0200b168(2, data_020b6574.v);
    func_0200ace4(2, 0, 0, 0x20, 0x18, 0x1f);
    func_02075040();
}
