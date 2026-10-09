/*
 * The touch-screen menu (items, part 1): the collection icons' destructor
 * and constructor, then the items page: tab frames, the selected item's
 * name and description, equipment, gems and counters.
 * ARM9 main, 0x02032a88 - 0x020336b8 (10 functions).
 *
 * The inventory is *data_020f62b0 (func_02034350): +0x08 five equipped
 * gems {u8 kind, u8 charge, ...} (8 bytes each, count at +0x129), +0x11c
 * u16 counter, +0x11e u16 equipment upgrades, +0x120 nine items (count at
 * +0x12a; 0x63 is the last kind), +0x12b..+0x12f counters.
 * data_020be568 is the selected tab (0-4 gems, 5- items, -1 none);
 * data_020e5aa0 / data_020e5b0c are the name and description boxes and
 * data_020e5cf4 six 0x7c-byte number displays.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020be460[];           /* collection icon vtable */
extern const char data_020be580[];   /* "%s %s" */
extern const char data_020be588[];   /* empty text */
extern s32 data_020be568;
extern s8 data_020df10c;
extern u8 data_020e52d0[];
extern s8 data_020e5a54, data_020e5a58, data_020e5a60;
extern u8 data_020e5aa0[], data_020e5b0c[], data_020e5b78[];
extern u8 data_020e5cf4[];
extern u8 *data_020f62b0;

void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void func_02076748(void *p);
void func_020767d8(void *p);
void func_02074e4c(void *p);
void func_02074e50(void *p);
s32 func_02020440(u32 v);
void func_0200ac20(s32 layer, u32 x, u32 y, s32 w, s32 h, u32 tile, u32 stride);
void func_0200ace4(s32 layer, u32 x, u32 y, s32 w, s32 h, u32 v);
const char *func_02028a20(u32 id);
void func_020752e4(void *box, const char *text, s32 a, s32 b, s32 c, s32 d);
void func_020750a4(void *box);
char *func_02080bdc(char *dst, const char *src);                 /* strcpy */
s32 func_020804f4(char *dst, const char *fmt, ...);              /* sprintf */
void func_02074f68(void *num, u32 value, u32 flag);
void func_02074e5c(void *num, u32 dt);
u8 *func_02034350(void);

void func_02032b64(void);
void *func_02032aac(u8 *p);

#define FN(f) ((void *(*)(void *))(void (*)(void))(f))
#define INV() func_02034350()
#define NUMBER(i) (data_020e5cf4 + (i) * 0x7c)

/* 0x02032a88: destroy the collection icons
 * @difftest stub=0x02076748 cases=3 */
void func_02032a88(void)
{
    func_020a6d58(data_020e52d0, 0x20, 0x3c, FN(func_02032aac));
}

/* 0x02032aac: collection icon destructor
 * @difftest zero:0x40 cases=5 */
void *func_02032aac(u8 *p)
{
    PTR_AT(p, 0) = data_020be460;
    func_02076748(p);
    func_02074e4c(p + 0x34);
    return p;
}

/* 0x02032ad8: collection icon constructor
 * @difftest zero:0x40 cases=5 */
void *func_02032ad8(u8 *p)
{
    func_02074e50(p + 0x34);
    func_020767d8(p);
    PTR_AT(p, 0) = data_020be460;
    return p;
}

/* 0x02032b08: select tab `tab` (-1: none)
 * @difftest @020be568:32=pick:-1,0,3,6 pick:-1,0,3,6 cases=40 */
void func_02032b08(s32 tab)
{
    s32 cur = data_020be568;

    if (tab == cur && cur != -1)
        return;
    data_020be568 = tab;
    if (tab != -1)
        func_02020440(0x44);
    func_02032b64();
}

/* 0x02032b64: draw the tab frames, the selected one highlighted
 * @difftest @020be568:32=int:-2:14 cases=40 */
void func_02032b64(void)
{
    s32 i, x, t;

    for (i = 0, x = 8; i <= 4; i++, x += 4)
        func_0200ace4(3, x, 0x10, 2, 2, 0x37f);
    for (i = 0, x = 2; i <= 8; i++, x += 3)
        func_0200ace4(3, x, 0xb, 2, 2, 0x37f);
    t = data_020be568;
    if (t >= 5) {
        func_0200ac20(3, (t - 5) * 3 + 2, 0xb, 2, 2, 0x23e, 0x20);
        return;
    }
    if (t < 0)
        return;
    func_0200ac20(3, t * 4 + 8, 0x10, 2, 2, 0x23e, 0x20);
}

/* the item number used for texts and tiles */
static s32 ItemKind(u8 c)
{
    return c == 0x63 ? 0x20 : c - 0x1a;
}

/* 0x02032c94: show the selected item's name and description
 * @difftest $I=zero:0x140 @020f62b0:32=$I @$I+0x120:8=int:0x1a:0x3a @$I+0x121:8=pick:0x63,0x1b @$I+0x8:8=int:0xd:0x18 @$I+0xc:32=int:0:3 @$I+0x10:8=int:0xd:0x18 @$I+0x14:32=int:0:3 @020be568:32=pick:-1,0,1,5,6 cases=60 */
void func_02032c94(void)
{
    s32 t = data_020be568;

    if (t == -1) {
        func_020752e4(data_020e5aa0, data_020be588, -1, -1, -1, 1);
        func_020752e4(data_020e5b0c, data_020be588, -1, -1, -1, 1);
    } else if (t >= 5) {
        s32 k = ItemKind(INV()[0x120 + (t - 5)]);
        func_020752e4(data_020e5aa0, func_02028a20(k + 0x2a), -1, -1, -1, 1);
        func_020752e4(data_020e5b0c, func_02028a20(k + 0x53), -1, -1, -1, 1);
    } else {
        char name[0x24], line[0x40];
        s32 g = INV()[8 + t * 8] - 0xd;
        s32 sel = data_020be568;
        func_02080bdc(name, func_02028a20(U32_AT(INV() + sel * 8, 4) + 0x94));
        func_020804f4(line, data_020be580, name, func_02028a20(g + 0x7c));
        func_020752e4(data_020e5aa0, line, -1, -1, -1, 1);
        func_020752e4(data_020e5b0c, func_02028a20(g + 0x88), -1, -1, -1, 1);
    }
    func_020750a4(data_020e5aa0);
    func_020750a4(data_020e5b0c);
}

/* one equipment slot: the highest upgrade owned, highlighted or not */
static void Upgrade(u32 x, u32 hl, u32 b3, u32 b2, u32 b1, u32 t3, u32 t2, u32 t1, u32 t0)
{
    if (U16_AT(INV(), 0x11e) & b3)
        func_0200ac20(1, x, 4, 4, 4, hl ? t3 + 4 : t3, 0x20);
    else if (U16_AT(INV(), 0x11e) & b2)
        func_0200ac20(1, x, 4, 4, 4, hl ? t2 + 4 : t2, 0x20);
    else if (U16_AT(INV(), 0x11e) & b1)
        func_0200ac20(1, x, 4, 4, 4, hl ? t1 + 4 : t1, 0x20);
    else if (hl)
        func_0200ac20(1, x, 4, 4, 4, t0, 0x20);
    else
        func_0200ace4(1, x, 4, 4, 4, 0x37f);
}

/* 0x02032e34: draw the equipment and update the counters
 * @difftest $I=zero:0x140 @020f62b0:32=$I @$I+0x11e:16=int:0:0x1ff @$I+0x12b:8=int:0:99 @$I+0x11c:16=int:0:999 @020e5a60:8=pick:0,1 @020e5a54:8=pick:0,1 cases=60 */
void func_02032e34(void)
{
    s32 i;

    Upgrade(1, data_020e5a60 != 0, 4, 2, 1, 0x138, 0x130, 0x128, 0x124);
    Upgrade(6, data_020e5a54 != 0, 0x100, 0x80, 0x40, 0x1b8, 0x1b0, 0x1a8, 0x1a4);
    if (U16_AT(INV(), 0x11e) & 0x20)
        func_0200ac20(1, 0xb, 4, 4, 4, 0x39c, 0x20);
    else if (U16_AT(INV(), 0x11e) & 0x10)
        func_0200ac20(1, 0xb, 4, 4, 4, 0x398, 0x20);
    INV();
    func_0200ac20(1, 0xc, 0x14, 3, 3, 0x375, 0x20);
    func_02074f68(NUMBER(0), INV()[0x12d], 1);
    func_02074f68(NUMBER(1), INV()[0x12c], 1);
    func_02074f68(NUMBER(2), INV()[0x12b], 1);
    func_02074f68(NUMBER(3), INV()[0x12e], 1);
    func_02074f68(NUMBER(4), U16_AT(INV(), 0x11c), 1);
    func_02074f68(NUMBER(5), INV()[0x12f], 1);
    for (i = 0; i < 6; i++)
        func_02074e5c(NUMBER(i), func_020822c0(func_020062d0()));
}

/* 0x020333e0: draw the nine item slots
 * @difftest cases=5
 * @difftest $I=zero:0x140 @020f62b0:32=$I @$I+0x12a:8=int:0:9 @$I+0x120:8=int:0x1a:0x3a @$I+0x121:8=int:0x1a:0x3a @$I+0x122:8=int:0x1a:0x3a @$I+0x123:8=int:0x1a:0x3a @$I+0x124:8=int:0x1a:0x3a @$I+0x125:8=int:0x1a:0x3a @$I+0x126:8=int:0x1a:0x3a @$I+0x127:8=int:0x1a:0x3a @$I+0x128:8=int:0x1a:0x3a cases=40
 * @difftest $I=zero:0x140 @020f62b0:32=$I @$I+0x12a:8=int:1:3 @$I+0x120:8=pick:0x63,0x1a @$I+0x121:8=pick:0x63,0x40 @$I+0x122:8=int:0x24:0x3a cases=20 */
void func_020333e0(void)
{
    s32 n = INV()[0x12a];
    s32 i, x;

    for (i = 0, x = 0; i < 9; i++, x += 3) {
        if (i < n) {
            s32 k = ItemKind(INV()[0x120 + i]);
            s32 t = k * 3 + 0x240;
            if (k >= 0x1e)
                t = (k - 0x1e) * 3 + 0x360;
            else if (k >= 0x14)
                t = (k - 0x14) * 3 + 0x300;
            else if (k >= 0xa)
                t = (k - 0xa) * 3 + 0x2a0;
            func_0200ac20(1, x, 9, 3, 3, (u16)t, 0x20);
        } else {
            func_0200ace4(1, x, 9, 3, 3, 0x37f);
        }
    }
}

/* 0x020334d8: draw the five gems (tile by kind and charge) and the
 * page's corner button
 * @difftest @020e5a58:8=pick:0,1 @020df10c:8=pick:0,1 cases=10
 * @difftest $I=zero:0x140 @020f62b0:32=$I @$I+0x129:8=int:0:5 @$I+0x8:8=int:0xd:0x18 @$I+0x9:8=int:0:0x60 @$I+0x10:8=int:0xd:0x18 @$I+0x11:8=int:0:0x60 @$I+0x18:8=int:0xd:0x18 @$I+0x19:8=int:0:0x60 @$I+0x20:8=int:0xd:0x18 @$I+0x21:8=int:0:0x60 @$I+0x28:8=int:0xd:0x18 @$I+0x29:8=int:0:0x60 @020e5a58:8=pick:0,1 cases=60 */
void func_020334d8(void)
{
    s32 n = INV()[0x129];
    s32 i, x;

    for (i = 0, x = 6; i < 5; i++, x += 4) {
        if (i < n) {
            s32 a = INV()[8 + i * 8] - 0xd;
            u32 b = INV()[9 + i * 8];
            s32 lvl = 0, t;
            if (b < 0x21)
                lvl = 2;
            else if (b < 0x42)
                lvl = 1;
            t = a * 3;
            if (a >= 0xa)
                t = (a - 8) * 10 + 0x120;
            func_0200ac20(1, x, 0xe, 3, 3, (u16)(lvl * 0x60 + t), 0x20);
        } else {
            func_0200ace4(1, x, 0xe, 3, 3, 0x37f);
        }
    }
    func_0200ac20(0, 1, 0xd, 4, 4, data_020e5a58 != 0 ? 0x300 : 0x1a1, 0x20);
    if (data_020df10c == 0)
        func_020750a4(data_020e5b78);
}

/* 0x0203366c: hide the boxes and counters
 * @difftest cases=3 */
void func_0203366c(void)
{
    s32 i;

    data_020e5aa0[0x14] = 0;
    data_020e5b0c[0x14] = 0;
    data_020e5b78[0x14] = 0;
    for (i = 0; i < 6; i++)
        NUMBER(i)[0x14] = 0;
}
