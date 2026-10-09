/*
 * The touch-screen menu (items, part 2): the items page's per-frame update
 * and its set-up, the inventory accessor and the page's array destructors.
 * ARM9 main, 0x020336b8 - 0x020343ac (6 functions).
 *
 * The page has seventeen touch rectangles at data_020e5be4 ({top, left,
 * bottom, right}): 0 and 1 the two equipment upgrades (used while held),
 * 2 the gem button, 3-7 the five gems and 8-16 the nine items. Text boxes
 * and number displays share a layout: +0x18/+0x1c mode, +0x24 shown, +0x30
 * BGxCNT of the layer, +0x34 layer, +0x38 first tile, +0x3c fill tile,
 * +0x44 redraw flag.
 *
 * Testing: the update runs with a faked pad (data_020f6f30) and inventory
 * (data_020f62b0); the set-up with the background loader func_0200aad0
 * stubbed (it reads from the card).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

extern Cnt data_020b65bc, data_020b65c0, data_020b65c4, data_020b65c8;
extern const char data_020be58c[];   /* "B_Collection_BKG.bin" */
extern const char data_020be5a4[];   /* "B_InventoryItems_BKG.bin" */
extern const s32 data_020be4ac, data_020be4b0, data_020be4b4, data_020be4b8,
    data_020be4bc, data_020be4c0, data_020be4c4, data_020be4c8, data_020be4cc,
    data_020be4d0, data_020be4d4, data_020be4d8, data_020be4dc, data_020be4e0,
    data_020be4e4, data_020be4e8, data_020be4ec, data_020be4f0, data_020be4f4,
    data_020be4f8, data_020be4fc, data_020be500, data_020be504, data_020be508,
    data_020be50c, data_020be510, data_020be514, data_020be518, data_020be51c,
    data_020be520, data_020be524, data_020be528, data_020be52c, data_020be530,
    data_020be534, data_020be538, data_020be53c, data_020be540, data_020be544,
    data_020be548, data_020be54c, data_020be550, data_020be554, data_020be558,
    data_020be55c, data_020be560, data_020be564, data_020be56c, data_020be570,
    data_020be574, data_020be578, data_020be57c;
extern s32 data_020be568;
extern s8 data_020df10c;
extern u8 data_020e5a50;
extern s8 data_020e5a54, data_020e5a58, data_020e5a60;
extern u8 data_020e5a5c;
extern u8 data_020e5aa0[], data_020e5b0c[], data_020e5b78[];
extern u8 data_020e5be4[];
extern u8 data_020e5cf4[];
extern u8 data_020ebbb8[];
extern u8 *data_020f62b0;

void *func_02011f44(void);
s32 func_02011bc8(void);
s32 func_02081d34(void *pad, u32 key);
void func_0202f330(const u32 *pos);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_02075040(void);
void func_02075420(void *box, const s32 *rect);
void func_020752e4(void *box, const char *text, s32 a, s32 b, s32 c, s32 d);
void func_02074f68(void *num, u32 value, u32 flag);
void func_0205a5b4(u8 *inv);
void func_0205a6a8(u8 *inv);
void func_0205a17c(u8 *inv, u32 gem);
void func_0203a028(void);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200af18(s32 layer);
void func_0200aec8(s32 layer, u32 v, u32 pal);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
const char *func_02028a20(u32 id);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void func_02074fe0(void *p);
void func_020036c0(s32 *rect);

void func_02032b08(s32 tab);
void func_02032c94(void);
void func_02032e34(void);
void func_020333e0(void);
void func_020334d8(void);
u8 *func_02034350(void);

#define FN(f) ((void *(*)(void *))(void (*)(void))(f))
#define INV() func_02034350()
#define RECT(i) ((s32 *)data_020e5be4 + (i) * 4)
#define NUMBER(i) (data_020e5cf4 + (i) * 0x7c)

/* 0x020336b8: the items page's per-frame update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1,0x10 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 $I=zero:0x140 @020f62b0:32=$I @$I+0x129:8=int:0:5 @$I+0x12a:8=int:0:9 @$I+0x12c:8=pick:0,1 @$I+0x12d:8=pick:0,1 @$I+0x120:8=int:0x1a:0x22 @$I+0x121:8=int:0x1a:0x22 @$I+0x8:8=int:0xd:0x14 @$I+0x10:8=int:0xd:0x14 @$I+0x11e:16=int:0:0x1ff @020be568:32=int:-1:13 @020e5a60:8=pick:0,1 @020e5a54:8=pick:0,1 @020e5a58:8=pick:0,1 @020df10c:8=pick:0,1 cases=100 */
void func_020336b8(void)
{
    s32 p[2], q[2];
    s32 hit = 0x11, i;

    func_02075040();
    if (func_02081d34(func_02011f44(), 0xc) != 0) {
        func_0202f6dc(p, func_02011f44());
        func_0202f330((const u32 *)p);
    }
    for (i = 0; i < 0x11; i++) {
        s32 *r = RECT(i);
        s32 in = 0;
        func_0202f6dc(q, func_02011f44());
        if (q[0] > r[1] && q[0] < r[3] && q[1] < r[2] && q[1] > r[0])
            in = 1;
        if (in) {
            hit = i;
            break;
        }
    }

    /* the equipment upgrades are used while held */
    if (hit == 0) {
        if (data_020e5a60 == 0 && INV()[0x12d] != 0)
            func_0205a5b4(INV());
        data_020e5a60 = 1;
        data_020e5a58 = 0;
    } else {
        data_020e5a60 = 0;
    }
    if (hit == 1) {
        if (data_020e5a54 == 0 && INV()[0x12c] != 0)
            func_0205a6a8(INV());
        data_020e5a54 = 1;
        data_020e5a58 = 0;
    } else {
        data_020e5a54 = 0;
    }
    /* the gem button uses the selected gem */
    if (hit == 2 && func_02011bc8() == 0 && data_020df10c == 0) {
        if (data_020e5a58 == 0 && (u32)data_020be568 <= 4)
            func_0205a17c(INV(), data_020be568);
        data_020e5a58 = 1;
    } else {
        data_020e5a58 = 0;
    }

    /* the selected item is gone */
    if (data_020be568 >= 5 && (u32)(data_020be568 - 5) >= INV()[0x12a]) {
        func_02032b08(-1);
        data_020e5a5c = 0;
    }

    if (hit >= 3 && hit <= 7 && INV()[0x129] > hit - 3) {
        func_02032b08(hit - 3);
    } else if (hit >= 8 && hit <= 0x10 && INV()[0x12a] > (u32)(hit - 8)) {
        func_02032b08(hit - 3);
        i = data_020be568 - 5;
        data_020e5a5c = INV()[0x120 + i];
    } else if (hit >= 3 && hit <= 0x10) {
        func_02032b08(-1);
    } else if (hit == 2) {
        func_02032b08(-1);
    }

    func_0203a028();
    func_020334d8();
    func_020333e0();
    func_02032e34();
    func_02032c94();
    data_020e5a50 = 0;
}

/* put a text box or number display on `layer` */
static void Layer(u8 *b, s32 layer, Cnt *cnt)
{
    S32_AT(b, 0x34) = layer;
    PTR_AT(b, 0x30) = (void *)cnt;
}

static void Mode(u8 *b, s32 m, s32 n)
{
    if (S32_AT(b, 0x18) != m || S32_AT(b, 0x1c) != n)
        b[0x44] = 1;
    S32_AT(b, 0x18) = m;
    S32_AT(b, 0x1c) = n;
}

static void Place(u8 *b, s32 top, s32 left, s32 bottom, s32 right, u32 tile)
{
    s32 r[4];

    S32_AT(b, 0x24) = 1;
    r[0] = top;
    r[1] = left;
    r[2] = bottom;
    r[3] = right;
    func_02075420(b, r);
    S32_AT(b, 0x38) = tile;
    if (S32_AT(b, 0x3c) != -1)
        S32_AT(b, 0x3c) = 0x37f;
}

/* 0x02033a40: set up the items page
 * @difftest $I=zero:0x140 @020f62b0:32=$I @$I+0x12b:8=int:0:99 @$I+0x12f:8=int:0:99 @$I+0x11c:16=int:0:999 @020df10c:8=pick:0,1 @020ebbc2:8=pick:0,1 stub=0x0200aad0 cases=10 */
void func_02033a40(void)
{
    s32 *rc = RECT(0);
    s32 i, x, tile;

    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020b65c0.v);
    func_0200af18(0);
    func_0200aad0(0, (u32)data_020be58c, 0, 0, 0, 0);
    func_0200b168(1, data_020b65c8.v);
    func_0200af18(1);
    func_0200aad0(1, (u32)data_020be5a4, 0, 0, 0, 0);
    func_0200aec8(1, 0x1f, 0);
    func_0200b168(2, data_020b65c4.v);
    func_0200aec8(2, 0x37f, 0);
    func_0200b168(3, data_020b65bc.v);
    func_0200aec8(3, 0x1f, 0);

    /* the touch rectangles */
    rc[0x00] = data_020be540;
    rc[0x01] = data_020be53c;
    rc[0x02] = data_020be538;
    rc[0x03] = data_020be534;
    rc[0x04] = data_020be4d8;
    rc[0x05] = data_020be52c;
    rc[0x06] = data_020be528;
    rc[0x07] = data_020be524;
    rc[0x08] = data_020be520;
    rc[0x09] = data_020be51c;
    rc[0x0a] = data_020be518;
    rc[0x0b] = data_020be514;
    rc[0x0c] = data_020be4ac;
    rc[0x0d] = data_020be50c;
    rc[0x0e] = data_020be508;
    rc[0x0f] = data_020be504;
    rc[0x10] = data_020be57c;
    rc[0x11] = data_020be530;
    rc[0x12] = data_020be550;
    rc[0x13] = data_020be560;
    rc[0x14] = data_020be564;
    rc[0x15] = data_020be56c;
    rc[0x16] = data_020be4ec;
    rc[0x17] = data_020be55c;
    rc[0x18] = data_020be4e8;
    rc[0x19] = data_020be54c;
    rc[0x1a] = data_020be544;
    rc[0x1b] = data_020be4dc;
    rc[0x1c] = data_020be4b0;
    rc[0x1d] = data_020be4d4;
    rc[0x1e] = data_020be4d0;
    rc[0x1f] = data_020be4c4;
    for (i = 0, x = 0; i <= 8; i++, x += 0x18) {
        s32 *r = RECT(8 + i);
        r[0] = data_020be510;
        r[1] = x;
        r[2] = data_020be4f8;
        r[3] = x + 0x18;
    }

    data_020e5a50 = 0;
    data_020e5a58 = 0;
    data_020e5a54 = 0;
    data_020e5a60 = 0;
    data_020be568 = -1;

    /* name and description */
    Layer(data_020e5aa0, 2, &data_020b65c4);
    Mode(data_020e5aa0, 0, 0);
    Place(data_020e5aa0, data_020be578, data_020be554, data_020be570, data_020be4f0, 0x304);
    Layer(data_020e5b0c, 2, &data_020b65c4);
    Mode(data_020e5b0c, 0, 0);
    Place(data_020e5b0c, data_020be4b4, data_020be548, data_020be4bc, data_020be4c0, 0x324);

    /* the counters */
    for (i = 0, x = 0, tile = 0x3c0; i < 5; i++, x += 0x28) {
        u8 *n = NUMBER(i);
        Layer(n, 3, &data_020b65bc);
        Mode(n, 3, 0);
        Place(n, data_020be4c8, x, data_020be4fc, x + 0x30, tile);
        tile += 8;
        if (tile % 32 >= 0x18)
            tile += 8;
    }
    Layer(NUMBER(5), 3, &data_020b65bc);
    Mode(NUMBER(5), 3, 0);
    Place(NUMBER(5), data_020be574, data_020be4b8, data_020be4e0, data_020be4cc, tile + 4);

    /* the gem button's caption */
    if (data_020df10c == 0) {
        Layer(data_020e5b78, 2, &data_020b65c4);
        if (data_020ebbb8[0xa] == 0)
            Mode(data_020e5b78, 3, 0);
        else
            Mode(data_020e5b78, 1, 0);
        Place(data_020e5b78, data_020be500, data_020be4e4, data_020be4f4, data_020be558, 0x344);
        func_020752e4(data_020e5b78, func_02028a20(0x28), -1, -1, -1, 1);
    }

    func_02074f68(NUMBER(0), INV()[0x12d], 1);
    func_02074f68(NUMBER(1), INV()[0x12c], 1);
    func_02074f68(NUMBER(2), INV()[0x12b], 1);
    func_02074f68(NUMBER(3), INV()[0x12e], 1);
    func_02074f68(NUMBER(4), U16_AT(INV(), 0x11c), 1);
    func_02074f68(NUMBER(5), INV()[0x12f], 1);
    func_0200b0ac(0);
    func_0200b0ac(1);
    func_0200b0ac(2);
    func_0200b0ac(3);
    func_020336b8();
}

/* 0x02034350: the inventory
 * @difftest cases=3 */
u8 *func_02034350(void)
{
    return data_020f62b0;
}

/* 0x02034360: destroy the number displays
 * @difftest cases=3 */
void func_02034360(void)
{
    func_020a6d58(data_020e5cf4, 6, 0x7c, FN(func_02074fe0));
}

/* 0x02034384: destroy the touch rectangles
 * @difftest cases=3 */
void func_02034384(void)
{
    func_020a6d58(data_020e5be4, 0x11, 0x10, FN(func_020036c0));
}

/* 0x020343a8
 * @difftest cases=3 */
void func_020343a8(void)
{
}
