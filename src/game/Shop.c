/*
 * The shop (touch screen): up to fifteen items with their prices; touching
 * an item puts one in the cart, "buy" pays with the inventory's money and
 * adds the cart to the inventory, "exit" leaves.
 * ARM9 main, 0x0203c00c - 0x0203d674 (17 functions).
 *
 * The shop data_020e8d0c's stock is data_020e9230 + shop * 0x168: fifteen
 * 0x18-byte entries {item, price, ?, cost, sold-out flag, text}, item -1
 * ending the list; an item with a flag (index into data_020d9f9c) can only
 * be bought once. Shown items (count data_020e8d08) have an icon sprite
 * (data_020e8f24, 0x34 bytes, +0x18 the entry), a stock display
 * (data_020e97d0) and a cart display (data_020e9f14, 0x7c-byte number
 * displays, value at +0x70); data_020beb98 is the touched item (-1 none),
 * data_020e8e44 / data_020e8eb4 the money and cart total lines,
 * data_020e8dd8 the hint box, data_020e8d70 / data_020e8da4 the buy and
 * exit buttons.
 * The money and total lines are a counter text class (vtable
 * data_020bec60): +0x58 the shown value, +0x5c the target it counts
 * towards, +0x64 bit 0 to blank leading zeros, +0x68 four digits.
 *
 * Testing: the set-up runs with the numbered-file background loader
 * func_0200a8bc and func_0201ad2c stubbed (they read from the card); the
 * shopkeeper's lines with the dialogue functions (func_02011664,
 * func_0201146c, func_02018158) stubbed and a faked speaker
 * (data_020def18 + 8); the update also with a faked pad, stock entries and
 * displays, sound effects and the state switch stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

extern Cnt data_020b6854, data_020b6858, data_020b685c;
extern s32 data_020beb98;
extern const s32 data_020beb9c, data_020beba4, data_020beba8, data_020bebac, data_020bebb0,
    data_020bebb4, data_020bebb8, data_020bebbc, data_020bebc0, data_020bebc4, data_020bebcc,
    data_020bebd0, data_020bebd4, data_020bebdc, data_020bebe0, data_020bebe4, data_020bebe8,
    data_020bebf0, data_020bebf8, data_020bebfc, data_020bec04, data_020bec1c, data_020bec20,
    data_020bec24, data_020bec28, data_020bec2c, data_020bec30, data_020bec34, data_020bec3c,
    data_020bec40, data_020bec44, data_020bec48, data_020bec4c, data_020bec50;
extern s32 data_020bebec;
extern u8 data_020d9f9c[];
extern u8 data_020def18[];
extern s32 data_020e8d08;
extern s32 data_020e8d0c;
extern u8 data_020e8d70[], data_020e8da4[], data_020e8dd8[], data_020e8e44[], data_020e8eb4[];
extern u8 data_020e8f24[];
extern u8 data_020e9230[];
extern u8 data_020e97d0[], data_020e9f14[];
extern const char data_020bec54[];   /* " " */
extern u8 data_020be3e4[], data_020bec60[];   /* vtables */

void *func_02011f44(void);
s32 func_02011bc8(void);
void func_0201146c(void);
void func_02011664(u32 text, s32 a);
void func_02018158(void);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_0203a028(void);
void func_02019b08(u32 state, u32 screen);
u32 func_02019b98(u32 screen);
void func_02019b40(u32 screen);
void func_0201b51c(u32 a, u32 b, u32 c);
void func_0201b5a4(u32 a, u32 b);
void func_0201ad2c(s32 a, s32 *v);
void func_0204378c(u32 screen);
void func_020437c4(u32 screen);
s32 func_02020440(u32 v);
void func_020748e0(void *sprite, const s32 *pos);
void func_02074c14(void *sprite, u32 a, u32 tile, u32 tile2, u32 b, s32 c);
void func_02074f68(void *num, u32 value, u32 flag);
void func_02074e5c(void *num, u32 dt);
void func_02075040(void);
void func_020750a4(void *box);
void func_02075420(void *box, const s32 *rect);
void func_020752e4(void *box, const char *text, s32 a, s32 b, s32 c, s32 d);
void func_02077010(void *line, const s32 *rect);
const char *func_02028a20(u32 id);
u8 *func_02034350(void);
s32 func_0205a9fc(u8 *inv, s32 item, s32 count, s32 a, s32 b, s32 test);
void func_0205a834(u8 *inv, s32 delta);
void func_0203d410(u8 *line, s32 value, s32 now);
void func_0203d3a8(u8 *line, s32 dt);
void func_0203d474(u8 *line);
void func_02075614(s32 *counter);
void func_02076eac(void *line, const char *text, s32 a, s32 b, s32 c, s32 d);
void func_02076e48(void *line);
char *func_02080bdc(char *dst, const char *src);                 /* strcpy */
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void *func_02074fe0(void *p);
void *func_020317e4(u8 *p);
void func_02074e50(void *p);
void func_02076ccc(void *p);
void func_02077180(void *p);
void func_02077078(void *p);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200af18(s32 layer);
void func_0200aec8(s32 layer, u32 v, u32 pal);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200a8bc(s32 layer, s32 handle, u32 tile, u32 pal, u32 x, u32 y);

u32 func_0203c070(s32 item);
void func_0203c2bc(void);
void func_0203c48c(void);
void func_0203c8d0(u32 text, s32 shop);

#define FN(f) ((void *(*)(void *))(void (*)(void))(f))
#define ICON(i) (data_020e8f24 + (i) * 0x34)
#define STOCK(i) (data_020e97d0 + (i) * 0x7c)
#define CART(i) (data_020e9f14 + (i) * 0x7c)
#define ENTRY(i) ((u8 *)PTR_AT(ICON(i), 0x18))

static void Touch(s32 *p)
{
    func_0202f6dc(p, func_02011f44());
}

/* was the sprite touched (its +0x1c bit 3, updated by func_020748e0) */
static s32 Touched(u8 *s)
{
    s32 p[2];

    Touch(p);
    func_020748e0(s, p);
    return (s8)((U32_AT(s, 0x1c) & 8) != 0);
}

/* method 0 (show) with its address in r2, like the original */
static void Show(u8 *s, u32 on)
{
    void (*m)(void *, u32, void *) = VCALL(s, 0, void (*)(void *, u32, void *));

    m(s, on, (void *)m);
}

/* place a number display or text box */
static void Display(u8 *b, s32 m)
{
    S32_AT(b, 0x34) = 2;
    PTR_AT(b, 0x30) = (void *)&data_020b6854;
    if (S32_AT(b, 0x18) != m || S32_AT(b, 0x1c) != 0)
        b[0x44] = 1;
    S32_AT(b, 0x18) = m;
    S32_AT(b, 0x1c) = 0;
}

/* lay out a text line at `rect` */
static void Line(u8 *l, const s32 *rect)
{
    if (S32_AT(l, 0x1c) != 3 || S32_AT(l, 0x20) != 0)
        l[0x34] = 1;
    {
        s32 c = S32_AT(l, 0x24);
        S32_AT(l, 0x1c) = 3;
        S32_AT(l, 0x20) = 0;
        func_0201391c(3, 0, c);
    }
    func_02077010(l, rect);
}

/* 0x0203c00c: open shop `shop`, returning to state `ret` (-1: the current one)
 * @difftest pick:-1,5 int:0:3 stub=0x02019b08 cases=10 */
void func_0203c00c(s32 ret, s32 shop)
{
    data_020e8d0c = shop;
    if (ret != -1)
        data_020bebec = ret;
    else
        data_020bebec = func_02019b98(1);
    func_0201b51c(0x3f, 1, 1);
    func_02019b08(0x15, 1);
}

/* 0x0203c070: an item's icon tile
 * @difftest int:0:0x70 cases=300 */
u32 func_0203c070(s32 item)
{
    switch (item) {
    case 0xa: return 0x98;
    case 0xb: return 0x118;
    case 0xc: return 0x9c;
    case 0xe: return 4;
    case 0xf: return 8;
    case 0x10: return 0xc;
    case 0x11: return 0x10;
    case 0x12: return 0x14;
    case 0x13: return 0x18;
    case 0x14: return 0x1c;
    case 0x15: return 0x80;
    case 0x16: return 0x84;
    case 0x17: return 0x88;
    case 0x18: return 0x8c;
    case 0x31: return 0x200;
    case 0x33: return 0x204;
    case 0x36: return 0x208;
    case 0x5b: return 0x180;
    case 0x5c: return 0x184;
    case 0x5d: return 0x104;
    case 0x5e: return 0x108;
    case 0x5f: return 0x110;
    case 0x60: return 0x114;
    case 0x61: return 0x100;
    case 0x62: return 0x10c;
    case 0x64: return 0x188;
    case 0x65: return 0x18c;
    }
    return 0;
}

/* 0x0203c2bc: empty the cart back into the stock
 * @difftest @020e8d08:32=int:0:4 cases=10 */
void func_0203c2bc(void)
{
    s32 i;

    func_0203d410(data_020e8eb4, 0, 1);
    for (i = 0; i < data_020e8d08; i++) {
        func_02074f68(STOCK(i), S32_AT(STOCK(i), 0x70) + S32_AT(CART(i), 0x70), 1);
        func_02074f68(CART(i), 0, 1);
        if (S32_AT(STOCK(i), 0x70) == 0)
            U32_AT(ICON(i), 0x1c) |= 0x20000;
    }
    data_020beb98 = -1;
}

/* 0x0203c380: leave the shop
 * @difftest @020e8d08:32=int:0:4 stub=0x02018158,0x0201146c cases=10 */
void func_0203c380(void)
{
    s32 i;

    if (PTR_AT(data_020e8e44, 0x18) != NULL) {
        func_020044a0(PTR_AT(data_020e8e44, 0x18));
        PTR_AT(data_020e8e44, 0x18) = NULL;
    }
    data_020e8e44[0x14] = 0;
    if (PTR_AT(data_020e8eb4, 0x18) != NULL) {
        func_020044a0(PTR_AT(data_020e8eb4, 0x18));
        PTR_AT(data_020e8eb4, 0x18) = NULL;
    }
    data_020e8eb4[0x14] = 0;
    data_020e8dd8[0x14] = 0;
    data_020e8d70[0x14] = 0;
    data_020e8da4[0x14] = 0;
    for (i = 0; i < data_020e8d08; i++) {
        ICON(i)[0x14] = 0;
        STOCK(i)[0x14] = 0;
        CART(i)[0x14] = 0;
    }
    func_02018158();
    if (func_02011bc8() != 0)
        func_0201146c();
    func_02018158();
    func_0204378c(0);
}

/* 0x0203c48c: the shop's per-frame update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 $E=zero:0x18 @$E+0:32=int:0x5b:0x66 @$E+0xc:32=int:0:50 @$E+0x10:32=pick:-1,5 @$E+0x14:32=0x300 @020e8d08:32=int:0:3 @020e8f3c:32=$E @020e8f70:32=$E @020e8fa4:32=$E @020e8f28:32=int:0:100 @020e8f2c:32=int:0:100 @020e8f30:32=int:100:192 @020e8f34:32=int:100:256 @020e9840:32=int:0:3 @020e98bc:32=int:0:3 @020e9f84:32=int:0:3 @020ea000:32=int:0:3 @020beb98:32=pick:-1,0,1 @020e8f10:32=int:0:100 @020e8ea0:32=int:0:100 @020e8d74:32=int:0:100 @020e8d78:32=int:0:100 @020e8d7c:32=int:100:192 @020e8d80:32=int:100:256 $V=zero:0x10 @$V+0xc:32=0x020343a8 $O=zero:0x40 @$O+0:32=$V @020def20:32=$O stub=0x02011664,0x0201146c,0x02018158,0x02020440,0x02019b40,0x0201b5a4 cases=150 */
void func_0203c48c(void)
{
    s32 i;

    func_02075040();
    for (i = 0; i < data_020e8d08; i++) {
        u8 *e = ENTRY(i);
        if (Touched(ICON(i))) {
            /* one more in the cart */
            func_02020440(0x46);
            if (data_020beb98 != i) {
                func_0203c2bc();
                data_020beb98 = i;
                func_0203c8d0(S32_AT(e, 0x14), 0);
            }
            if (S32_AT(STOCK(data_020beb98), 0x70) != 0) {
                func_0203d410(data_020e8eb4, S32_AT(e, 0xc) + S32_AT(data_020e8eb4, 0x5c), 1);
                func_02074f68(STOCK(i), S32_AT(STOCK(i), 0x70) - 1, 1);
                func_02074f68(CART(i), S32_AT(CART(i), 0x70) + 1, 1);
            } else {
                func_0203c8d0(0x37a, 1);
            }
        }
        func_02074e5c(STOCK(i), func_020822c0(func_020062d0()));
        func_02074e5c(CART(i), func_020822c0(func_020062d0()));
    }

    if (data_020beb98 == -1 || S32_AT(CART(data_020beb98), 0x70) == 0) {
        U32_AT(data_020e8d70, 0x1c) |= 0x20000;
        func_020752e4(data_020e8dd8, NULL, -1, -1, -1, 1);
    } else {
        U32_AT(data_020e8d70, 0x1c) &= ~0x20000u;
        func_020752e4(data_020e8dd8, func_02028a20(0x29), -1, -1, -1, 1);
    }

    /* buy */
    if (Touched(data_020e8d70) && data_020beb98 != -1) {
        s32 k = data_020beb98;
        u8 *e = ENTRY(k);
        if ((u32)S32_AT(data_020e8eb4, 0x5c) > (u32)S32_AT(data_020e8e44, 0x5c)) {
            func_0203c8d0(0x379, 1);
            func_0203c2bc();
        } else if (func_0205a9fc(func_02034350(), S32_AT(e, 0), S32_AT(CART(k), 0x70), 3, 3, 1) == 0) {
            func_0203c8d0(0x378, 1);
            func_0203c2bc();
        } else {
            s32 flag;
            u8 *inv = func_02034350();
            func_0205a9fc(inv, S32_AT(e, 0), S32_AT(CART(data_020beb98), 0x70), 3, 3, 0);
            func_0205a834(func_02034350(), (s16)-S32_AT(data_020e8eb4, 0x5c));
            func_0203d410(data_020e8e44, U16_AT(func_02034350(), 0x11c), 1);
            flag = S32_AT(e, 0x10);
            if (flag != -1 && flag < 0x1ae)
                data_020d9f9c[flag] = 1;
            func_02074f68(CART(data_020beb98), 0, 1);
            func_0203c2bc();
        }
    }

    /* exit */
    if (Touched(data_020e8da4)) {
        func_0201b5a4(0x3f, 1);
        func_02019b40(1);
    }
    func_020750a4(data_020e8dd8);
    func_0203d3a8(data_020e8e44, func_020822c0(func_020062d0()));
    func_0203d3a8(data_020e8eb4, func_020822c0(func_020062d0()));
    func_0203a028();
}

/* 0x0203c8d0: the shopkeeper says `text` (with `shop`, the shop's variant)
 * @difftest int:0:0x380 int:0:2 @020e8d0c:32=int:0:4 $V=zero:0x10 @$V+0xc:32=0x020343a8 $O=zero:0x40 @$O+0:32=$V @020def20:32=$O stub=0x02011664,0x0201146c,0x02018158 cases=40 */
void func_0203c8d0(u32 text, s32 shop)
{
    void *o;
    void (*m)(void *, u32, void *);

    if (shop != 0) {
        switch ((u32)data_020e8d0c) {
        case 1: text += 6; break;
        case 2: text += 3; break;
        case 3: text += 9; break;
        }
    }
    func_02018158();
    if (func_02011bc8() != 0)
        func_0201146c();
    o = PTR_AT(data_020def18, 8);
    m = VCALL(o, 0xc, void (*)(void *, u32, void *));
    m(o, 0, (void *)m);
    func_02011664(text, 0);
    func_02018158();
}

/* 0x0203c968: set up the shop
 * @difftest @020e8d0c:32=int:0:3 $V=zero:0x10 @$V+0xc:32=0x020343a8 $O=zero:0x40 @$O+0:32=$V @020def20:32=$O stub=0x0200a8bc,0x02011664,0x0201146c,0x02018158,0x0201ad2c cases=10 */
void func_0203c968(void)
{
    s32 a[4], b[4], c[4], r[4];
    s32 i, v;

    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020b6858.v);
    func_0200af18(0);
    func_0200a8bc(0, 0x162, 0, 0, 0, 0);
    func_0200b168(1, data_020b685c.v);
    func_0200af18(1);
    func_0200a8bc(1, 0x161, 0, 0, 0, 0);
    func_0200aec8(1, 0x1f, 0);
    func_0200b168(2, data_020b6854.v);
    func_0200aec8(2, 0x37f, 0);
    data_020beb98 = -1;
    data_020e8d08 = 0;

    /* the items in stock, five per row */
    for (i = 0; i < 0xf; i++) {
        u8 *e = data_020e9230 + data_020e8d0c * 0x168 + i * 0x18;
        s32 flag, n, dx, dy;
        u32 tile;
        u8 *s;
        if (S32_AT(e, 0) == -1)
            break;
        flag = S32_AT(e, 0x10);
        if (flag != -1 && data_020d9f9c[flag] != 0)
            continue;
        tile = func_0203c070(S32_AT(e, 0));
        n = data_020e8d08;
        dx = (n % 5) * 0x30;
        dy = (n / 5) * 0x28;
        a[0] = data_020bebe8 + dy;
        a[1] = data_020bec24 + dx;
        a[2] = data_020bebe0 + dy;
        a[3] = data_020bebdc + dx;
        b[0] = data_020bec48 + dy;
        b[1] = data_020bec40 + dx;
        b[2] = data_020bebd0 + dy;
        b[3] = data_020bebcc + dx;
        c[0] = data_020bec2c + dy;
        c[1] = data_020bebc4 + dx;
        c[2] = data_020bec1c + dy;
        c[3] = data_020bebbc + dx;

        s = ICON(n);
        S32_AT(s, 4) = a[0];
        S32_AT(s, 8) = a[1];
        S32_AT(s, 0xc) = a[2];
        S32_AT(s, 0x10) = a[3];
        Show(s, 1);
        s[0x14] = 1;
        func_02074c14(ICON(data_020e8d08), 1, tile, tile, 0x20, 0x90);
        PTR_AT(ICON(data_020e8d08), 0x18) = e;
        U32_AT(ICON(data_020e8d08), 0x1c) &= 0xff000000u;
        U32_AT(ICON(data_020e8d08), 0x1c) |= 1;

        Display(STOCK(data_020e8d08), 0);
        S32_AT(STOCK(data_020e8d08), 0x78) = 1;
        func_02075420(STOCK(data_020e8d08), b);
        {
            u8 *d = STOCK(data_020e8d08);
            S32_AT(d, 0x38) = data_020e8d08 * 4 + 0x380;
            if (S32_AT(d, 0x3c) != -1)
                S32_AT(d, 0x3c) = 0x37f;
        }
        func_02074f68(STOCK(data_020e8d08), S32_AT(e, 4), 1);

        Display(CART(data_020e8d08), 0);
        S32_AT(CART(data_020e8d08), 0x78) = 1;
        func_02075420(CART(data_020e8d08), c);
        {
            u8 *d = CART(data_020e8d08);
            S32_AT(d, 0x38) = 0x382 + data_020e8d08 * 4;
            if (S32_AT(d, 0x3c) != -1)
                S32_AT(d, 0x3c) = 0x37f;
        }
        func_02074f68(CART(data_020e8d08), 0, 1);
        data_020e8d08++;
    }

    v = data_020bebb8;
    func_0201ad2c(0, &v);

    /* money and cart total */
    r[0] = data_020bec04;
    r[1] = data_020bebfc;
    r[2] = data_020bebf8;
    r[3] = data_020beb9c;
    Line(data_020e8e44, r);
    func_0203d410(data_020e8e44, U16_AT(func_02034350(), 0x11c), 1);
    r[0] = data_020bec30;
    r[1] = data_020bec50;
    r[2] = data_020bec44;
    r[3] = data_020bec34;
    Line(data_020e8eb4, r);
    S32_AT(data_020e8eb4, 0x64) = 1;
    func_0203d410(data_020e8eb4, 0, 1);

    /* the hint box */
    Display(data_020e8dd8, 3);
    S32_AT(data_020e8dd8, 0x24) = 1;
    r[0] = data_020bec28;
    r[1] = data_020bec20;
    r[2] = data_020bebb4;
    r[3] = data_020beba8;
    func_02075420(data_020e8dd8, r);
    S32_AT(data_020e8dd8, 0x38) = 0x3e0;
    if (S32_AT(data_020e8dd8, 0x3c) != -1)
        S32_AT(data_020e8dd8, 0x3c) = 0x37f;
    func_020752e4(data_020e8dd8, func_02028a20(0x29), -1, -1, -1, 1);

    /* buy and exit */
    S32_AT(data_020e8d70, 0x10) = data_020bebc0;
    S32_AT(data_020e8d70, 4) = data_020bebf0;
    S32_AT(data_020e8d70, 8) = data_020bec4c;
    S32_AT(data_020e8d70, 0xc) = data_020bec3c;
    Show(data_020e8d70, 1);
    data_020e8d70[0x14] = 1;
    func_02074c14(data_020e8d70, 0, 0x32e, 0x321, 0x20, 0x28a);
    S32_AT(data_020e8da4, 4) = data_020bebe4;
    S32_AT(data_020e8da4, 8) = data_020bebd4;
    S32_AT(data_020e8da4, 0xc) = data_020bebb0;
    S32_AT(data_020e8da4, 0x10) = data_020beba4;
    Show(data_020e8da4, 1);
    data_020e8da4[0x14] = 1;
    func_02074c14(data_020e8da4, 0, 0x31a, 0x27a, 0x20, -1);

    func_0200b0ac(0);
    func_0200b0ac(1);
    func_0200b0ac(2);
    func_0200b098(3);
    func_020437c4(0);
    func_0203c48c();
}

/* 0x0203d3a8: a counter line counts towards its target
 * @difftest $L=zero:0x70 @$L+0:32=0x020bec60 @$L+0x58:32=int:0:20 @$L+0x5c:32=int:0:20 @$L+0x64:32=pick:0,1 $L pick:0,1 cases=40 */
void func_0203d3a8(u8 *l, s32 dt)
{
    if (dt > 0 && S32_AT(l, 0x58) != S32_AT(l, 0x5c)) {
        func_02075614((s32 *)(l + 0x58));
        func_0203d474(l);
        func_02076eac(l, (char *)l + 0x68, -1, -1, -1, 0);
    }
    func_02076e48(l);
}

/* 0x0203d410: set a counter line's target (`now`: show it at once)
 * @difftest $L=zero:0x70 @$L+0:32=0x020bec60 @$L+0x64:32=pick:0,1 $L int:0:9999 pick:0,1 cases=40 */
void func_0203d410(u8 *l, s32 v, s32 now)
{
    if (now != 0) {
        S32_AT(l, 0x5c) = v;
        S32_AT(l, 0x58) = S32_AT(l, 0x5c);
    } else {
        S32_AT(l, 0x5c) = v;
    }
    if (now == 0)
        return;
    func_0203d474(l);
    func_02076eac(l, (char *)l + 0x68, -1, -1, -1, 1);
}

/* 0x0203d474: format the shown value as four digits
 * @difftest $L=zero:0x70 @$L+0x58:32=int:-20:12000 @$L+0x64:32=pick:0,1 $L cases=200 */
void func_0203d474(u8 *l)
{
    s32 v = S32_AT(l, 0x58), i;

    for (i = 3; i >= 0; i--) {
        l[0x68 + i] = v % 10 + '0';
        v /= 10;
    }
    l[0x6c] = 0;
    if ((S32_AT(l, 0x64) & 1) == 0)
        return;
    for (i = 0; i < 4; i++) {
        if (l[0x68 + i] != '0')
            break;
    }
    if (i == 4)
        func_02080bdc((char *)l + 0x68, data_020bec54);
    else
        func_02080bdc((char *)l + 0x68, (char *)l + 0x68 + i);
}

/* 0x0203d538: destroy the cart displays
 * @difftest cases=3 */
void func_0203d538(void)
{
    func_020a6d58(data_020e9f14, 0xf, 0x7c, func_02074fe0);
}

/* 0x0203d55c: destroy the stock displays
 * @difftest cases=3 */
void func_0203d55c(void)
{
    func_020a6d58(data_020e97d0, 0xf, 0x7c, func_02074fe0);
}

/* 0x0203d580: destroy the item icons
 * @difftest cases=3 */
void func_0203d580(void)
{
    func_020a6d58(data_020e8f24, 0xf, 0x34, FN(func_020317e4));
}

/* 0x0203d5a4: the shop sprite constructor
 * @difftest zero:0x40 cases=5 */
void *func_0203d5a4(u8 *p)
{
    func_02074e50(p + 0x18);
    func_02076ccc(p);
    PTR_AT(p, 0) = data_020be3e4;
    return p;
}

/* 0x0203d5d4: the counter line constructor
 * @difftest zero:0x80 cases=5 */
void *func_0203d5d4(u8 *p)
{
    func_02077180(p);
    S32_AT(p, 0x5c) = 0;
    S32_AT(p, 0x58) = S32_AT(p, 0x5c);
    PTR_AT(p, 0) = data_020bec60;
    MI_CpuFill8(p + 0x68, '0', 4);
    p[0x6c] = 0;
    S32_AT(p, 0x64) = 0;
    return p;
}

/* 0x0203d624: the counter line destructor
 * @difftest zero:0x80 cases=5 */
void *func_0203d624(u8 *p)
{
    PTR_AT(p, 0) = data_020bec60;
    func_02077078(p);
    return p;
}

/* 0x0203d648: show or hide a counter line
 * @difftest $T=zero:0x10 $L=zero:0x70 @$L+0x18:32=$T $L int:0:2 cases=20
 * @difftest zero:0x70 int:0:2 cases=5 */
void func_0203d648(u8 *l, u32 on)
{
    l[0x15] = on;
    if (PTR_AT(l, 0x18) != NULL) {
        u32 *f = (u32 *)((u8 *)PTR_AT(l, 0x18) + 4);
        *f = (*f & ~8u) | ((on & 1) << 3);
    }
}
