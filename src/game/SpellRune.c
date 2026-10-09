/*
 * The rune practice page (touch screen): the player draws the rune of
 * spell data_020ea664 (0xe-0x13) over its picture; a recognised rune for
 * that spell ends the practice (state 0x14), anything else clears the
 * page for another try.
 * ARM9 main, 0x0203d674 - 0x0203df2c (9 functions).
 *
 * Strokes are recorded like on the spells page (SpellsMenu2.c):
 * data_020ea69c holds the points (0xffff pairs end a stroke), its +0x100c
 * marks an ended stroke and +0x1010 counts points; data_020ea670 is the
 * last stylus position, data_020becd0 the drawing sound, data_020ea660 set
 * when the rune was right and data_020ea65c when the page was cleared.
 *
 * Testing: with a faked pad and stroke buffer, the rune recogniser
 * func_02053a94 stubbed (to each rune and a score, through the stub's
 * output word) and the state switch and background loader stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

extern Cnt data_020aacac;
extern s32 data_020becd0;            /* the drawing sound's handle */
extern const char data_020becd4[];   /* "B_SpellsBKG_RevShock.bin" */
extern const char data_020becf0[];   /* "B_SpellsBKG_RevMove.bin" */
extern const char data_020bed08[];   /* "B_SpellsBKG_RevTeleport.bin" */
extern const char data_020bed24[];   /* "B_SpellsBKG_RevBanish.bin" */
extern const char data_020bed40[];   /* "B_SpellsBKG_RevCyclone.bin" */
extern const char data_020bed5c[];   /* "B_Spells_BKG.bin" */
extern s8 data_020e3a80;
extern u32 data_020e3a84;
extern s8 data_020ea658;
extern s8 data_020ea65c;
extern s8 data_020ea660;
extern s32 data_020ea664;
extern s32 data_020ea670[2];
extern PtrVec data_020ea69c;
extern u8 data_020eb69c[];
extern s8 data_020eb6a8;

void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
s32 func_02081cd0(void *pad, u32 key);
u32 func_02081c84(void *pad);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
s32 func_02020440(u32 v);
void func_0202040c(s32 handle);
void func_020095d8(s32 layer, u32 color, s32 x0, s32 y0, s32 x1, s32 y1);
s32 func_02053a94(PtrVec *strokes, s32 *score);
void func_02053c38(PtrVec *strokes);
void func_02053d1c(PtrVec *strokes);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200af18(s32 layer);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);

void func_0203d674(void);
void func_0203dad0(void);

static void Touch(s32 *p)
{
    func_0202f6dc(p, func_02011f44());
}

/* the spell's picture */
static const char *Picture(void)
{
    switch ((u32)(data_020ea664 - 0xe)) {
    case 0: return data_020becf0;
    case 1: return data_020bed08;
    case 2: return data_020becd4;
    case 3: return data_020bed24;
    case 4: return data_020bed40;
    case 5: return data_020bed5c;
    }
    return data_020bed5c;
}

static void PushPoint(s32 x, s32 y)
{
    PtrVec *v = &data_020ea69c;

    ((u16 *)v->items)[v->count * 2] = x;
    ((u16 *)v->items)[v->count * 2 + 1] = y;
    v->count++;
}

/* 0x0203d674: clear the page for another try
 * @difftest @020ea664:32=int:0xd:0x15 stub=0x0200aad0 cases=20 */
void func_0203d674(void)
{
    u32 old = data_020df0fc;

    if (old != 1)
        data_020df0fc = 1;
    func_0200aad0(0, (u32)Picture(), 0, 0, 0, 0);
    func_0202040c(data_020becd0);
    data_020becd0 = -1;
    data_020e3a84 = 0;
    data_020e3a80 = 0;
    data_020ea660 = 0;
    data_020ea658 = 0;
    data_020df0fc = old;
}

/* 0x0203d820: leave the page
 * @difftest cases=5 */
void func_0203d820(void)
{
    func_02053d1c(&data_020ea69c);
    func_0202040c(data_020becd0);
    data_020becd0 = -1;
    data_020e3a84 = 0;
    data_020e3a80 = 0;
    data_020ea660 = 0;
    data_020ea658 = 0;
}

/* 0x0203d888: the page's per-frame update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1,0x10,0x11 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e3a80:8=pick:0,1 @020e3a84:32=int:0:30 @020eb6a8:8=pick:0,1 @020ea664:32=int:0xe:0x13 $P=zero:0x100 @020ea69c:32=$P @020ea6a0:32=int:0:0x42 @020ea6a4:32=0x40 stub=0x02053a94:0:0,0x0200aad0,0x02019b08,0x02020440 cases=150 */
void func_0203d888(void)
{
    s32 p[2], q[2], r[2], t1[2], t2[2], t3[2];

    data_020ea65c = 0;
    if (func_02081d34(func_02011f44(), 0xc) != 0) {
        Touch(p);
        data_020ea670[0] = p[0];
        data_020ea670[1] = p[1];
        return;
    }
    if (func_02081cd0(func_02011f44(), 0xc) != 0 && func_02081c84(func_02011f44()) == 0) {
        Touch(p);
        if ((u32)p[0] >= 0xde)
            return;
        Touch(q);
        if (q[1] <= 0x28)
            return;
        if (data_020e3a80 == 0) {
            data_020e3a80 = 1;
            if (data_020becd0 == -1)
                data_020becd0 = func_02020440(0x53);
        }
        data_020e3a84 = 0;
        Touch(r);
        data_020eb69c[0xc] = 0;
        if (!((u32)data_020ea69c.count >= (u32)data_020ea69c.cap)) {
            U32_AT(data_020eb69c, 0x10)++;
            PushPoint(r[0], r[1]);
        }
        Touch(t1);
        Touch(t2);
        func_020095d8(0, 0x46, data_020ea670[0], data_020ea670[1], t1[0], t2[1]);
        Touch(t3);
        data_020ea670[0] = t3[0];
        data_020ea670[1] = t3[1];
        return;
    }
    if (data_020e3a80 != 0)
        func_0203dad0();
}

/* 0x0203dad0: the stylus is up: end the stroke; after a pause, read the
 * rune
 * @difftest $P=zero:0x100 @020ea69c:32=$P @020ea6a0:32=int:0:0x42 @020ea6a4:32=0x40 @020e3a84:32=int:19:22 @020eb6a8:8=pick:0,1 @020ea664:32=int:0xe:0x13 stub=0x02053a94:0:0x1000,0x0200aad0,0x02019b08 cases=30
 * @difftest $P=zero:0x100 @020ea69c:32=$P @020ea6a0:32=int:0:0x42 @020ea6a4:32=0x40 @020e3a84:32=int:19:22 @020eb6a8:8=pick:0,1 @020ea664:32=int:0xe:0x13 stub=0x02053a94:1:0x1000,0x0200aad0,0x02019b08 cases=30
 * @difftest $P=zero:0x100 @020ea69c:32=$P @020ea6a0:32=int:0:0x42 @020ea6a4:32=0x40 @020e3a84:32=int:19:22 @020eb6a8:8=pick:0,1 @020ea664:32=int:0xe:0x13 stub=0x02053a94:2:0x1000,0x0200aad0,0x02019b08 cases=30
 * @difftest $P=zero:0x100 @020ea69c:32=$P @020ea6a0:32=int:0:0x42 @020ea6a4:32=0x40 @020e3a84:32=int:19:22 @020eb6a8:8=pick:0,1 @020ea664:32=int:0xe:0x13 stub=0x02053a94:3:0x1000,0x0200aad0,0x02019b08 cases=30
 * @difftest $P=zero:0x100 @020ea69c:32=$P @020ea6a0:32=int:0:0x42 @020ea6a4:32=0x40 @020e3a84:32=int:19:22 @020eb6a8:8=pick:0,1 @020ea664:32=int:0xe:0x13 stub=0x02053a94:4:0x1000,0x0200aad0,0x02019b08 cases=30
 * @difftest $P=zero:0x100 @020ea69c:32=$P @020ea6a0:32=int:0:0x42 @020ea6a4:32=0x40 @020e3a84:32=int:19:22 @020eb6a8:8=pick:0,1 @020ea664:32=int:0xe:0x13 stub=0x02053a94:5:0x1000,0x0200aad0,0x02019b08 cases=30
 * @difftest $P=zero:0x100 @020ea69c:32=$P @020ea6a0:32=int:0:0x42 @020ea6a4:32=0x40 @020e3a84:32=int:19:22 @020eb6a8:8=pick:0,1 @020ea664:32=int:0xe:0x13 stub=0x02053a94:6:0x1000,0x0200aad0,0x02019b08 cases=30
 * @difftest $P=zero:0x100 @020ea69c:32=$P @020ea6a0:32=int:0:0x42 @020ea6a4:32=0x40 @020e3a84:32=int:19:22 @020eb6a8:8=pick:0,1 @020ea664:32=int:0xe:0x13 stub=0x02053a94:2:0xc00,0x0200aad0,0x02019b08 cases=10 */
void func_0203dad0(void)
{
    s32 score, rune, spell;

    data_020e3a84++;
    if (data_020eb6a8 == 0) {
        data_020eb6a8 = 1;
        if ((u32)data_020ea69c.count < (u32)data_020ea69c.cap)
            PushPoint(0xffff, 0xffff);
    }
    if (data_020e3a84 <= 0x14)
        return;
    data_020e3a84 = 0;
    rune = func_02053a94(&data_020ea69c, &score);
    if (score > 0xc00) {
        spell = data_020ea664;
        switch ((u32)rune) {
        case 0:
            if (spell == 0x10)
                data_020ea660 = 1;
            break;
        case 1:
            if (spell == 0x13)
                data_020ea660 = 1;
            break;
        case 2:
            if (spell == 0xe)
                data_020ea660 = 1;
            break;
        case 3:
            if (spell == 0x11)
                data_020ea660 = 1;
            break;
        case 4:
            if (spell == 0x12)
                data_020ea660 = 1;
            break;
        case 5:
            if (spell == 0xf) {
                data_020ea660 = 1;
                func_0201b770(1, 0x3f, 1);
                func_02019b08(0x14, 1);
            }
            break;
        }
        if (data_020ea660 != 0) {
            func_0201b770(1, 0x3f, 1);
            func_02019b08(0x14, 1);
            return;
        }
    }
    data_020ea65c = 1;
    func_0203d674();
}

/* 0x0203dcf4: set up the page
 * @difftest @020ea664:32=int:0xd:0x15 stub=0x0200aad0 cases=20 */
void func_0203dcf4(void)
{
    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020aacac.v);
    func_0200af18(0);
    func_0200aad0(0, (u32)Picture(), 0, 0, 0, 0);
    func_02053c38(&data_020ea69c);
    data_020e3a80 = 0;
    data_020ea660 = 0;
    data_020ea658 = 0;
    data_020e3a84 = 0;
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
}

/* 0x0203dec8: the spell to practise
 * @difftest int:0xe:0x14 cases=5 */
void func_0203dec8(s32 spell)
{
    data_020ea664 = spell;
}

/* 0x0203ded8: was the page cleared this frame
 * @difftest @020ea65c:8=int:-2:2 cases=5 */
s32 func_0203ded8(void)
{
    return data_020ea65c;
}

/* 0x0203dee8: was the rune right
 * @difftest @020ea660:8=int:-2:2 cases=5 */
s32 func_0203dee8(void)
{
    return data_020ea660;
}

/* 0x0203def8: leave
 * @difftest cases=3 */
void func_0203def8(void)
{
    func_0200b098(0);
    func_0200b098(2);
    func_0200b098(1);
    func_0200b098(3);
}
