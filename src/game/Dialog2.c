/*
 * Dialogue box, part 2: opening a text (speaker portrait, box sprite, text
 * loading), text layout steps and inline control codes.
 * ARM9 main, 0x02010210 - 0x02010ec8 (15 functions).
 *
 * data_020daac0 is the dialogue state:
 *   +0x08 text position, +0x0c/+0x10 line start/length, +0x14/+0x18/+0x1c
 *   text box x/width/y, +0x20 sound of the current mark, +0x24, +0x28,
 *   +0x2c/+0x30 text pointers, +0x34 font, +0x3c/+0x3d/+0x3e/+0x40 flags.
 * data_020dab7c is the box (rect, +0x10 sprite actor, +0x14 animation).
 * Text records: data_020c3b64, 0x2c bytes (+0x20 kind, +0x24 arg, +0x28
 * end offset in the language file).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c1894;
extern u8 *data_020c3b0c;            /* scene nodes, 0x48 bytes each */
extern u8 *data_020c3b6c;
extern u32 *data_020c3b80;           /* {speaker, portrait} pairs */
extern const s32 data_020bcf0c, data_020bcf10, data_020bcf2c, data_020bcf34, data_020bcf38,
    data_020bcf40, data_020bcf48, data_020bcf50;
extern const u32 data_020bcf60[5];
extern const char data_020bcf74[], data_020bcf88[], data_020bcf9c[], data_020bcfb0[],
    data_020bcfc4[], data_020bcfd8[], data_020bcfec[], data_020bd000[];
extern u8 data_020d7b8c[];
extern s8 data_020daa58;
extern u8 data_020daa5c;
extern u8 *data_020daa64;            /* text buffer */
extern u32 data_020daa70;
extern u32 data_020daa74;
extern u32 data_020daa78;
extern u32 data_020daa7c;
extern u32 data_020daa80;
extern u8 data_020daac0[];
extern u8 data_020daaec[];
extern u8 data_020dab04[];
extern u8 data_020dab7c[];
extern u8 data_020dab90[];
extern u8 data_020dabf4[], data_020dac6c[];
extern u32 data_020e6054, data_020e6058;
extern u8 data_020ebbb8[];

u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_02003e08(void *actor, void *anim);
s32 func_02011d88(void);
s32 func_02011e08(void);
u32 func_02011ebc(void);
u32 func_02011efc(void);
s32 func_02013644(u32 a, u32 b, u32 c, u32 d, u32 e, void *f, u32 g, s32 *h);
void func_020136c0(u32 a, u32 b, u32 c, u32 d, u32 e, void *f, u32 g);
void *func_02013770(u32 a, void *b, u32 c);
s32 func_020138f8(void);
s32 func_02013cf8(void *obj);
void func_02013d20(void *obj, u32 v);
u32 func_02013e90(void *obj);
void func_0201430c(void *obj, u32 v);
void func_02019b08(u32 a, u32 b);
void func_0201b770(u32 a, u32 b, u32 c);
void func_0203dec8(u32 v);
void func_0204a794(u32 idx, u32 v);   /* set progress counter data_020ebce4[idx] */
void func_02071e5c(void *obj);
void func_02071eb0(void *obj, u32 v);
void func_02071f10(void *obj, u32 v);
void func_020803d0(void *file, u32 pos, u32 whence);
void func_020804ac(void *file, const char *path);

/* this file's own functions, called before their definition */
void func_020107a8(u8 *box);
void func_020108f4(u8 *dst, const u8 *src);
s32 func_02010920(void);
s32 func_02010948(void);
u8 *func_02010970(void);
void func_02010c0c(void);
void func_02010c54(void);
void func_02010e64(void);

#define D data_020daac0
#define TEXT(i) (data_020c3b64 + (i) * 0x2c)

static inline void HideSprite(u8 *box)
{
    if (PTR_AT(box, 0x10) != NULL)
        U32_AT(PTR_AT(box, 0x10), 4) &= ~8u;
}

/* (re)create the box sprite in the rect {b, a, b + d, a + c} */
static void SetupBox(s32 a, s32 b, s32 c, s32 d, const u8 *anim)
{
    s32 t[4], bd, ac;
    if (PTR_AT(data_020dab7c, 0x10) != NULL) {
        func_020044a0(PTR_AT(data_020dab7c, 0x10));
        PTR_AT(data_020dab7c, 0x10) = NULL;
    }
    t[0] = a;
    t[1] = b;
    t[2] = c;
    t[3] = d;
    ac = a + c;
    bd = b + d;
    func_020036e8((s32 *)data_020dab7c, &t[1], &t[0], &bd, &ac);
    func_020108f4(data_020dab90, anim);
    func_020107a8(data_020dab7c);
    if (PTR_AT(data_020dab7c, 0x10) != NULL)
        func_02003cbc(PTR_AT(data_020dab7c, 0x10), 0);
    if (PTR_AT(data_020dab7c, 0x10) != NULL)
        U32_AT(PTR_AT(data_020dab7c, 0x10), 4) |= 8;
}

/* 0x02010210: open the current text */
void func_02010210(void)
{
    u32 speaker, n, found;
    s32 i;
    u8 *node;

    U32_AT(D, 0x2c) = (u32)func_02010970();
    U32_AT(D, 0x2c) = (u32)func_02013770(data_020daa70, PTR_AT(D, 0x2c), 0x200);
    U32_AT(D, 0x30) = U32_AT(D, 0x2c);
    D[0x3d] = 0;
    speaker = func_02011ebc();
    func_02071e5c(data_020dab04);
    n = U32_AT(data_020d7b8c, 0xe24);
    found = 0;
    i = 0;
    while (i < (s32)n && found == 0) {
        u32 *e = data_020c3b80 + i * 2;
        if (speaker == e[0]) {
            func_02071eb0(data_020dab04, e[1]);
            found = 1;
        }
        i++;
    }
    func_02010e64();
    func_02010c0c();

    node = data_020c3b0c + speaker * 0x48;
    if (S32_AT(node, 8) != -1 || S32_AT(node, 0xc) != -1 || S32_AT(node, 0x10) != -1) {
        u32 pos[5];
        pos[0] = 0;
        pos[1] = 0;
        pos[2] = 0;
        pos[3] = 0;
        pos[4] = 0;
        node = data_020c3b0c + speaker * 0x48;
        pos[0] = U32_AT(node, 8);
        pos[1] = U32_AT(node, 0xc);
        pos[2] = U32_AT(node, 0x10);
        SetupBox(data_020bcf48, data_020bcf10, data_020bcf0c, data_020bcf2c, (const u8 *)pos);
    } else {
        u32 def[5];
        for (i = 0; i < 5; i++)
            def[i] = data_020bcf60[i];
        SetupBox(data_020bcf38, data_020bcf34, data_020bcf40, data_020bcf50, (const u8 *)def);
    }

    if (S32_AT(TEXT(func_02011efc()), 0x20) != 0) {
        data_020daa7c = U32_AT(TEXT(func_02011efc()), 0x20);
        data_020daa74 = U32_AT(TEXT(func_02011efc()), 0x24);
        func_0204a794(data_020daa7c, data_020daa74);
        if (func_02010920()) {
            data_020e6054 = data_020daa7c;
            data_020e6058 = 0xf;
            data_020daa58 = 1;
            func_0203dec8(data_020daa7c);
            func_0201b770(1, 0x3f, 1);
            func_02019b08(0xf, 1);
        } else if (func_02010948()) {
            data_020daa58 = 1;
        } else {
            func_0201430c(data_020deebc, 1);
        }
        if (data_020daa7c == 3 && func_02013cf8(data_020deebc) <= 0)
            func_02013d20(data_020deebc, func_02013e90(data_020deebc));
        if (data_020daa58 != 0) {
            u32 k = data_020daa74;
            data_020daa5c = 0;
            switch (k) {
            case 1:
                data_020daa78 = U32_AT(data_020c3b6c + data_020daa7c * 0x18, 0x8);
                break;
            case 2:
                data_020daa78 = U32_AT(data_020c3b6c + data_020daa7c * 0x18, 0xc);
                break;
            case 3:
                data_020daa78 = U32_AT(data_020c3b6c + data_020daa7c * 0x18, 0x10);
                break;
            case 4:
                data_020daa78 = U32_AT(data_020c3b6c + data_020daa7c * 0x18, 0x14);
                break;
            }
        }
    }
    if (func_02011e08() != 0)
        return;
    HideSprite(data_020dac6c);
    HideSprite(data_020dabf4);
}

/* 0x020107a8: create the box sprite */
void func_020107a8(u8 *box)
{
    u8 *a;
    PTR_AT(box, 0x10) = func_02005cd0(0);
    U32_AT(PTR_AT(box, 0x10), 4) |= 8;
    U32_AT(PTR_AT(box, 0x10), 0x38) |= 0x2000;
    U32_AT(PTR_AT(box, 0x10), 0x110) |= 4;
    U32_AT(PTR_AT(box, 0x10), 0x94) = 0;
    U8_AT(PTR_AT(box, 0x10), 0xb1) = 1;
    U32_AT(PTR_AT(box, 0x10), 0xa8) &= ~0xc00u;
    a = PTR_AT(box, 0x10);
    U32_AT(a, 0xac) = (U32_AT(a, 0xac) & ~0xf000u) | ((U32_AT(box, 0x1c) & 0xf) << 12);
    U32_AT(PTR_AT(box, 0x10), 0xa8) &= ~0x2000u;
    U32_AT(PTR_AT(box, 0x10), 0xac) = (U32_AT(PTR_AT(box, 0x10), 0xac) & ~0xc00u) | 0x400;
    a = PTR_AT(box, 0x10);
    U32_AT(a, 0xac) = (U32_AT(a, 0xac) & ~0xf000u) | ((data_020c1894 & 0xf) << 12);
    U8_AT(PTR_AT(box, 0x10), 0xe0) = 0;
    data_020c1894++;
    if (data_020c1894 == 0xf)
        data_020c1894 = 1;
    func_02003b98(PTR_AT(box, 0x10), S32_AT(box, 4), S32_AT(box, 0));
    func_02003e08(PTR_AT(box, 0x10), box + 0x14);
    U32_AT(PTR_AT(box, 0x10), 0xac) = (U32_AT(PTR_AT(box, 0x10), 0xac) & ~0xc00u) | 0x400;
}

/* 0x020108f4: copy box animation data
 * @difftest ptr:0x14:4 ptr:0x14:4 */
void func_020108f4(u8 *dst, const u8 *src)
{
    u32 a = U32_AT(src, 0), b = U32_AT(src, 4);
    U32_AT(dst, 0) = a;
    U32_AT(dst, 4) = b;
    U32_AT(dst, 8) = U32_AT(src, 8);
    U8_AT(dst, 0xc) = U8_AT(src, 0xc);
    U32_AT(dst, 0x10) = U32_AT(src, 0x10);
}

/* 0x02010920: is the text kind a shop/menu (0xe-0x13)
 * @difftest @020daa7c:32=int:0:24 */
s32 func_02010920(void) { return (s8)(data_020daa7c - 0xe <= 5); }

/* 0x02010948: is the text kind 1-3
 * @difftest @020daa7c:32=int:0:8 */
s32 func_02010948(void) { return (s8)(data_020daa7c - 1 <= 2); }

/* 0x02010970: load the current text from the language file */
u8 *func_02010970(void)
{
    u32 file[0x48 / 4], start, len;
    switch (data_020ebbb8[0xa]) {
    case 0: func_020804ac(file, data_020bcf74); break;
    case 1: func_020804ac(file, data_020bcf88); break;
    case 2: func_020804ac(file, data_020bcf9c); break;
    case 3: func_020804ac(file, data_020bcfb0); break;
    case 4: func_020804ac(file, data_020bcfc4); break;
    case 5: func_020804ac(file, data_020bcfd8); break;
    default: func_020804ac(file, data_020bcf74); break;
    }
    if (func_02011efc() == 0)
        start = 0;
    else
        start = U32_AT(TEXT(func_02011efc() - 1), 0x28) + 2;
    func_020803d0(file, start, 0);
    len = U32_AT(TEXT(func_02011efc()), 0x28) - start;
    func_02080418(data_020daa64, len, 1, file);
    func_02080458(file);
    data_020daa64[len] = 0;
    return data_020daa64;
}

/* 0x02010ad0: lay out the next part of the text; returns 1 when the box is full */
u32 func_02010ad0(s32 more)
{
    u32 font = D[0x34];
    u32 x = func_02013644(U32_AT(D, 0x14), font * 0x38 + 0x48, U32_AT(D, 0x1c), 0xa9,
                          U32_AT(D, 0x18), data_020daaec, font, &more);
    if (more == 0) {
        U32_AT(D, 0x2c) = U32_AT(D, 0x30);
    } else {
        U32_AT(D, 8) = 0;
        U32_AT(D, 0x1c) += func_020138f8() + 2;
        U32_AT(D, 0x18) -= func_020138f8();
        U32_AT(D, 0x30) = U32_AT(D, 0x2c);
        U32_AT(D, 0x14) = x;
    }
    return U32_AT(D, 0x18) <= (u32)func_020138f8();
}

/* 0x02010bac: draw the text laid out so far
 * @difftest cases=30 */
void func_02010bac(void)
{
    u32 font = D[0x34];
    func_020136c0(U32_AT(D, 0x14), font * 0x38 + 0x48, U32_AT(D, 0x1c), 0xa9, U32_AT(D, 0x18),
                  data_020daaec, font);
    U32_AT(D, 0x30) = U32_AT(D, 0x2c);
}

/* 0x02010c0c: load the box background */
void func_02010c0c(void)
{
    func_0200aad0(1, (u32)data_020bcfec, 0x100, 1, 0, 0);
    func_02071f10(data_020dab04, 1);
}

/* 0x02010c54: look for a '/' control code in the current line
 * @difftest $T=ptr:0x40 @020daaec:32=$T @020daac8:32=int:0:0x30 @020daacc:32=int:0:0x30 cases=200
 * @difftest $T=ptr:0x40 @$T+8:8=pick:47,47,0 @$T+9:8=pick:103,40,43,33,63,112,110,121,122,65 @020daaec:32=$T @020daac8:32=int:9:0x14 @020daacc:32=int:0:0x10 cases=200 */
void func_02010c54(void)
{
    s32 end = S32_AT(D, 8), i = end - S32_AT(D, 0xc);
    if (i < end) {
        u8 *p = (u8 *)PTR_AT(D, 0x2c) + i;
        do {
            if (*p == '/')
                break;
            i++;
            p++;
        } while (i < end);
    }
    U32_AT(D, 0x20) = 0;
    if (i >= end)
        return;
    switch (((u8 *)PTR_AT(D, 0x2c))[i + 1]) {
    case 'g':
        U32_AT(D, 0x20) = 0xc3;
        break;
    case '(':
        U32_AT(D, 0x20) = 0xc5;
        break;
    case '+':
        U32_AT(D, 0x20) = 0xc6;
        break;
    case '!':
        U32_AT(D, 0x20) = 0xc7;
        break;
    case '?':
        U32_AT(D, 0x20) = 0xc4;
        break;
    case 'p':
        U32_AT(D, 8) = i + 2;
        U32_AT(D, 0x24) = 0xf;
        break;
    case 'n':
        D[0x3c] = 1;
        U32_AT(D, 0x28) = i + 2;
        func_02004490(data_020bd000, i + 2);
        break;
    case 'y':
        U32_AT(D, 0x20) = 0xc7;
        break;
    case 'z':
        U32_AT(D, 0x20) = 0xc5;
        break;
    }
}

/* 0x02010dfc: advance to the next line of the text */
void func_02010dfc(void)
{
    s32 n, p;
    if (func_02011d88() != 0)
        return;
    n = S32_AT(D, 0x10);
    p = S32_AT(D, 8) + n;
    S32_AT(D, 0xc) = n;
    S32_AT(D, 8) = p;
    if (((u8 *)PTR_AT(D, 0x2c))[p - 1] == '/') {
        S32_AT(D, 8) = p + 1;
        S32_AT(D, 0xc) = n + 1;
    }
    func_02010c54();
}

/* 0x02010e64: reset the text layout
 * @difftest */
void func_02010e64(void)
{
    U32_AT(D, 8) = -U32_AT(D, 0x10);
    U32_AT(D, 0x1c) = data_020daa80 * 8 + 0x84;
    U32_AT(D, 0x18) = 0x20;
    U32_AT(D, 0x14) = 0x40;
}

/* 0x02010ea4
 * @difftest u8 */
void func_02010ea4(u32 v) { D[0x3e] = v; }

/* 0x02010eb4: set the font
 * @difftest u8 */
void func_02010eb4(u32 v) { D[0x34] = v; }

/* 0x02010ec4
 * @difftest u32 */
void func_02010ec4(void *p) { (void)p; }
