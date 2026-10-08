/*
 * Dialogue box, part 3: the per-frame state machine, yes/no choices and
 * shop answers, opening and closing the box, and the dialogue state
 * accessors.
 * ARM9 main, 0x02010ec8 - 0x02011f54 (27 functions).
 *
 * The dialogue actor (func_02011bd8 sets it up) updates through
 * func_02010ec8, whose state lives in data_020daac0 +0x38:
 *   0 opening delay, 1 typing text, 2 pause (/p), 3 waiting for the
 *   button (or a choice), 4 closing, 5 close, 6 idle.
 * data_020daac0 +0x00 is the conversation (data_020c3b68, 12 bytes each:
 * +4 first and +8 last text) and +0x04 the current text.
 * The three choice sprites are data_020dabf4 (yes), data_020dac6c (no) and
 * data_020dace4 (next-page arrow); data_020dab7c is the box itself.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Rgb16;     /* passed by value in r0 */

extern const u8 data_020aacb0[], data_020aacc4[], data_020aacd8[];
extern Rgb16 data_020b5674;
extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcb94[];           /* RTTI: CActor */
extern s8 data_020bcefc;
extern const s32 data_020bcf00, data_020bcf04, data_020bcf08, data_020bcf14, data_020bcf18,
    data_020bcf1c, data_020bcf20, data_020bcf24, data_020bcf28, data_020bcf30, data_020bcf3c,
    data_020bcf44, data_020bcf4c, data_020bcf54, data_020bcf58, data_020bcf5c;
extern const char data_020bcfec[], data_020bd018[], data_020bd03c[];
extern s8 data_020daa54;
extern s8 data_020daa58;
extern s8 data_020daa5c;
extern s8 data_020daa60;
extern u8 *data_020daa64;            /* text buffer */
extern s32 data_020daa68;            /* opening delay */
extern u32 data_020daa6c;
extern u8 *data_020daa70;
extern s32 data_020daa74;
extern u32 data_020daa78;
extern u32 data_020daa7c;
extern u32 data_020daa80;
extern u8 data_020daac0[];
extern u8 data_020dab04[];
extern u8 data_020dab7c[];
extern u8 data_020dabf4[], data_020dac08[];
extern u8 data_020dac6c[], data_020dac80[];
extern u8 data_020dace4[], data_020dacf8[];
extern void *data_020f641c;
extern void *data_020f6f30;

u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_0200a1bc(s32 layer);
void func_0200af18(s32 layer);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
void func_0200ff90(void);
void func_02003c74(void *actor, s32 a, s32 b);
void func_02003e68(s32 group, s32 layer);
void func_02010210(void);
void func_020107a8(u8 *box);
void func_020108f4(u8 *dst, const u8 *src);
u32 func_02010ad0(s32 more);
void func_02010bac(void);
void func_02010dfc(void);
void func_02010ea4(u32 v);
void func_02010eb4(u32 v);
void func_02010ec4(void *p);
void func_02013a5c(Rgb16 color);
void func_0201430c(void *obj, u32 v);
void func_02020440(u32 v);
s32 func_0203ded8(void);
s32 func_0203dee8(void);
void func_0204378c(u32 screen);
void func_0204a794(u32 idx, u32 v);
void func_02071dac(void *obj, u32 v);
void func_02072418(void *obj, u32 a, u32 fbits);
void func_02076b44(void *obj, u32 a, u32 b);
s32 func_02081cd0(void *pad, u32 key);
s32 func_02081d34(void *pad, u32 key);
void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */

/* this file's own functions, called before their definition */
u32 func_02011b88(void);
void func_02011b98(u32 v);
u32 func_02011ba8(void);
void func_02011bb8(u32 v);
s32 func_02011bc8(void);
s32 func_02011d78(void);
s32 func_02011d88(void);
u32 func_02011dd0(void);
s32 func_02011e08(void);
s32 func_02011e74(void);
void func_02011e9c(u32 v);
void func_02011eec(u32 idx);
u32 func_02011efc(void);
s32 func_02011f14(void);
void func_02011f24(u32 v);
void *func_02011f34(void);
void *func_02011f44(void);
void func_02011274(void);
void func_0201146c(void);
void func_020116e8(void);

#define D data_020daac0
#define SPRITE(box) PTR_AT(box, 0x10)
#define CAMERA_FOCUS() PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4)

static inline void VCall(void *obj, u32 off, u32 v)
{
    VCALL(obj, off, void (*)(void *, u32))(obj, v);
}

static inline void ShowSprite(u8 *box)
{
    if (SPRITE(box) != NULL)
        U32_AT(SPRITE(box), 4) |= 8;
}

static inline void HideSprite(u8 *box)
{
    if (SPRITE(box) != NULL)
        U32_AT(SPRITE(box), 4) &= ~8u;
}

static inline void FreeSprite(u8 *box)
{
    if (SPRITE(box) != NULL) {
        func_020044a0(SPRITE(box));
        SPRITE(box) = NULL;
    }
}

static inline void SpriteFrame(u8 *box, s32 frame)
{
    if (SPRITE(box) != NULL)
        func_02003cbc(SPRITE(box), frame);
}

/* (re)create a hidden choice sprite in the rect {b, a, b + d, a + c} */
static void MakeChoice(u8 *box, s32 a, s32 b, s32 c, s32 d, const u8 *anim, s32 frame)
{
    s32 t[4], bd, ac;
    FreeSprite(box);
    t[0] = a;
    t[1] = b;
    t[2] = c;
    t[3] = d;
    ac = a + c;
    bd = b + d;
    func_020036e8((s32 *)box, &t[1], &t[0], &bd, &ac);
    func_020108f4(box + 0x14, anim);
    func_020107a8(box);
    SpriteFrame(box, frame);
    HideSprite(box);
}

static inline void Reset(void)
{
    data_020daa60 = 0;
    data_020daa7c = 0;
    data_020daa5c = 0;
    data_020daa58 = 0;
}

/* 0x02010ec8: dialogue actor update
 * @difftest u32 @020daaf8:32=int:0:7 @020daae4:32=int:0:3 @020daa6c:32=int:0xb8:0xc0 @020daa68:32=int:-3:3 @020daa60:8=pick:0,1 cases=150 */
void func_02010ec8(void *self)
{
    (void)self;
    switch (func_02011ba8()) {
    case 0:
        data_020daa68 -= func_020822c0(func_020062d0());
        if (data_020daa68 > 0)
            return;
        func_020116e8();
        func_02010210();
        func_02011bb8(1);
        return;
    case 1:
        if (func_02011d78() == 0) {
            func_02010bac();
            D[0x3d] = 1;
        } else {
            func_02010dfc();
            func_02013a5c(data_020b5674);
            func_0201391c(1, 0, -1);
            D[0x3d] = func_02010ad0(S32_AT(D, 8));
            if ((s8)D[0x3c] != 0) {
                D[0x3d] = 1;
                U32_AT(D, 8) = U32_AT(D, 0x28);
                U32_AT(D, 0x30) = U32_AT(D, 0x2c) + U32_AT(D, 0x28);
                D[0x3c] = 0;
            }
            if (U32_AT(D, 0x20) != 0)
                func_02020440(U32_AT(D, 0x20));
            if (U32_AT(D, 0x24) != 0)
                func_02011bb8(2);
        }
        if (func_02011d88() != 1)
            return;
        func_02011bb8(3);
        if (func_02011e74() != 0)
            func_02011f24(data_020daa58);
        if (func_02011e08() == 1) {
            if (func_02011e74() == 0)
                return;
            ShowSprite(data_020dac6c);
            ShowSprite(data_020dabf4);
            return;
        }
        if (func_02011f14() != 0)
            return;
        ShowSprite(data_020dace4);
        return;
    case 2:
        S32_AT(D, 0x24) -= 1;
        if (S32_AT(D, 0x24) > 0)
            return;
        func_02011bb8(1);
        return;
    case 3:
        if (func_02011f14() == 1) {
            func_02011274();
            return;
        }
        if (func_02081d34(func_02011f44(), 10)) {
            func_0200ff90();
            return;
        }
        if (func_02081cd0(func_02011f44(), 5)) {
            SpriteFrame(data_020dabf4, 1);
            SpriteFrame(data_020dac6c, 0);
            func_02011e9c(1);
            return;
        }
        if (!func_02081cd0(func_02011f44(), 4))
            return;
        SpriteFrame(data_020dabf4, 0);
        SpriteFrame(data_020dac6c, 1);
        func_02011e9c(0);
        return;
    case 4:
        data_020daa6c += 3;
        if (data_020daa6c < 0xbe)
            return;
        func_02011bb8(5);
        data_020daa6c = 0xbe;
        return;
    case 5:
        func_0201146c();
        func_02011bb8(6);
        return;
    }
}

/* 0x02011274: wait for a shop or menu opened by the text to finish
 * @difftest @020daa5c:8=pick:0,1 @020daa78:32=pick:0,1,2,3 @020daa7c:32=pick:0,14,15,16 @020dab00:8=pick:0,1 cases=80 */
void func_02011274(void)
{
    if (data_020daa5c != 0) {
        if (data_020daa78 == U32_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 0x2c))
            return;
        /* the original tests (kind == 1 && arg == 1 && kind == 2 ...), which
           never holds, so the menu is always closed */
        func_0201430c(data_020deebc, 1);
        Reset();
        func_0200ff90();
        return;
    }
    if (data_020daa78 == U32_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 0x2c)) {
        data_020daa5c = 1;
        return;
    }
    if (func_0203dee8() != 0) {
        u32 k = data_020daa7c;
        if (k == 0xe) {
            func_0201430c(data_020deebc, 1);
        } else if (k == 0x10) {
            func_0204a794(0xc, 1);
            func_0201430c(data_020deebc, 1);
        } else if (k == 0xf) {
            func_0204a794(0xb, 1);
            func_0201430c(data_020deebc, 1);
        }
        Reset();
        func_0200ff90();
        return;
    }
    if ((s8)D[0x40] == 0 && func_0203ded8() == 0)
        return;
    func_0200ff90();
    D[0x40] = 0;
    func_02011f24(0);
    data_020daa58 = 0;
}

/* 0x0201146c: close the dialogue box */
void func_0201146c(void)
{
    void *cam, *a;
    data_020daa54 = 0;
    data_020daa7c = 0;
    data_020daa60 = 0;
    data_020daa58 = 0;
    data_020daa74 = -1;
    FreeSprite(data_020dab7c);
    FreeSprite(data_020dac6c);
    FreeSprite(data_020dabf4);
    FreeSprite(data_020dace4);
    if (data_020bcefc == 1) {
        func_0204378c(data_020df0fc);
    } else {
        VCall((void *)data_020deebc[2], 0xc, 1);
        a = CAMERA_FOCUS();
        if (a != NULL)
            a = func_020a6efc(a, data_020bcb88, data_020bcb94, -1);
        if (a != NULL)
            VCall(a, 0xc, 1);
    }
    cam = (void *)data_020deebc[2];
    CAMERA_FOCUS() = cam;
    func_0200af18(1);
    func_0200b098(1);
    VCall(func_02011f34(), 0, 1);
    a = func_02011f34();
    if (func_02011bc8() == 0) {
        func_02076b44((u8 *)a + 0x7c4, 0, 0);
        func_02076b44((u8 *)a + 0x768, 0, 0);
    }
    func_02011bb8(6);
}

/* 0x02011664: start a conversation */
void func_02011664(u32 conv, u32 flag)
{
    data_020bcefc = flag;
    VCall(func_02011f34(), 0, 0);
    func_02072418(func_02011f34(), 0, 0x45000000);   /* 2048.0f */
    data_020daa68 = 0x266;
    func_02011bb8(0);
    func_02011b98(conv);
    func_02010eb4(0);
    func_02010ea4(1);
    data_020daa54 = 1;
}

/* 0x020116e8: set up the box and the choice sprites */
void func_020116e8(void)
{
    u8 *a;
    func_02020440(0x4c);
    func_02011eec(func_02011dd0());
    func_02011e9c(0);
    D[0x40] = 0;
    data_020daa7c = 0;
    data_020daa78 = 0;
    data_020daa60 = 0;
    data_020daa58 = 0;
    data_020daa74 = -1;
    data_020daa6c = 0xbe;
    data_020daa80 = 0;
    func_0200b0ac(1);
    func_0200a1bc(1);
    func_0200af18(1);
    func_0200aad0(1, (u32)data_020bcfec, 0x100, 1, 0, 0);
    func_02013a5c(data_020b5674);
    func_0201391c(1, 0, -1);
    MakeChoice(data_020dabf4, data_020bcf18, data_020bcf14, data_020bcf4c, data_020bcf44,
               data_020aacb0, 0);
    MakeChoice(data_020dac6c, data_020bcf3c, data_020bcf54, data_020bcf58, data_020bcf30,
               data_020aacc4, 1);
    MakeChoice(data_020dace4, data_020bcf5c, data_020bcf08, data_020bcf00, data_020bcf04,
               data_020aacd8, 0);
    VCall((void *)data_020deebc[2], 0xc, 0);
    a = CAMERA_FOCUS();
    if (a != NULL)
        a = func_020a6efc(a, data_020bcb88, data_020bcb94, -1);
    func_02003e68(func_02011b88(), data_020df0fc);
    if (a == NULL)
        return;
    if (U32_AT(CAMERA_FOCUS(), 0xc) == 4) {
        if (U32_AT(CAMERA_FOCUS(), 8) == 0x10a)
            VCall(a, 0xc, 0);
        CAMERA_FOCUS() = (void *)data_020deebc[2];
        return;
    }
    VCall(a, 0xc, 0);
}

/* 0x02011b88: current conversation
 * @difftest */
u32 func_02011b88(void) { return U32_AT(D, 0); }

/* 0x02011b98
 * @difftest u32 */
void func_02011b98(u32 v) { U32_AT(D, 0) = v; }

/* 0x02011ba8: box state
 * @difftest */
u32 func_02011ba8(void) { return U32_AT(D, 0x38); }

/* 0x02011bb8
 * @difftest u32 */
void func_02011bb8(u32 v) { U32_AT(D, 0x38) = v; }

/* 0x02011bc8: is a conversation running
 * @difftest */
s32 func_02011bc8(void) { return data_020daa54; }

/* 0x02011bd8: set up the dialogue actor */
void func_02011bd8(u8 *self)
{
    s32 x, y, w, h;
    U32_AT(self, 0xc) = 0x13;
    U32_AT(self, 8) = 0;
    U32_AT(self, 4) &= ~8u;
    U32_AT(self, 0x38) |= 0x40000;
    U32_AT(self, 0x110) &= ~0x100u;
    U32_AT(self, 0x38) |= 0x20;
    U32_AT(self, 0x38) |= 0x40;
    U32_AT(self, 0x38) &= ~4u;
    PTR_AT(self, 0x98) = (void *)func_02010ec4;
    PTR_AT(self, 0x94) = (void *)func_02010ec8;
    U32_AT(self, 0x9c) = 0;
    func_02003c74(self, 0, 0);
    U32_AT(self, 0x24) = 0;
    U32_AT(self, 0x50) = 0;
    y = data_020bcf28;
    h = data_020bcf20;
    x = data_020bcf24;
    w = data_020bcf1c;
    S32_AT(data_020dab04, 0) = x;
    S32_AT(data_020dab04, 4) = y;
    S32_AT(data_020dab04, 8) = x + w;
    S32_AT(data_020dab04, 0xc) = y + h;
    func_02071dac(data_020dab04, 1);
    self[0x10d] = 0;
    func_02004490(data_020bd018);
}

/* 0x02011cf0: allocate the text buffers */
void func_02011cf0(void)
{
    if (U32_AT(D, 0x10) == 0)
        U32_AT(D, 0x10) = 5;
    data_020daa54 = 0;
    func_02011bb8(6);
    data_020daa70 = func_0207ff70(0x200, data_020bd03c, 0x102);
    data_020daa64 = func_0207ff70(0x200, data_020bd03c, 0x103);
}

/* 0x02011d78: is the text drawn letter by letter
 * @difftest */
s32 func_02011d78(void) { return (s8)D[0x3e]; }

/* 0x02011d88: is the box full / the text done
 * @difftest */
s32 func_02011d88(void) { return (s8)D[0x3d]; }

/* 0x02011d98: last text of the conversation
 * @difftest @020daac0:32=int:0:40 */
u32 func_02011d98(void) { return (u16)U32_AT(data_020c3b68 + func_02011b88() * 0xc, 8); }

/* 0x02011dd0: first text of the conversation
 * @difftest @020daac0:32=int:0:40 */
u32 func_02011dd0(void) { return (u16)U32_AT(data_020c3b68 + func_02011b88() * 0xc, 4); }

/* 0x02011e08: does the current text end with a choice
 * @difftest @020daac4:32=int:0:200 */
s32 func_02011e08(void)
{
    return (s8)(U32_AT(data_020c3b64 + func_02011efc() * 0x2c, 0x10) != 0 ||
                U32_AT(data_020c3b64 + func_02011efc() * 0x2c, 0x14) != 0);
}

/* 0x02011e74: has the whole text been shown
 * @difftest $T=ptr:4 @$T+0:8=pick:0,0,65 @020daaec:32=$T */
s32 func_02011e74(void) { return (s8)(*(u8 *)PTR_AT(D, 0x2c) == 0); }

/* 0x02011e9c: choose yes (1) or no (0)
 * @difftest u8 */
void func_02011e9c(u32 v) { D[0x3f] = v; }

/* 0x02011eac
 * @difftest */
s32 func_02011eac(void) { return (s8)D[0x3f]; }

/* 0x02011ebc: speaker of the current text
 * @difftest @020daac4:32=int:0:200 */
u32 func_02011ebc(void) { return U32_AT(data_020c3b64 + func_02011efc() * 0x2c, 0xc); }

/* 0x02011eec
 * @difftest u32 */
void func_02011eec(u32 idx) { U32_AT(D, 4) = idx; }

/* 0x02011efc: current text
 * @difftest */
u32 func_02011efc(void) { return (u16)U32_AT(D, 4); }

/* 0x02011f14: is a shop or menu open
 * @difftest */
s32 func_02011f14(void) { return data_020daa60; }

/* 0x02011f24
 * @difftest u8 */
void func_02011f24(u32 v) { data_020daa60 = v; }

/* 0x02011f34
 * @difftest */
void *func_02011f34(void) { return data_020f641c; }

/* 0x02011f44: the input state
 * @difftest */
void *func_02011f44(void) { return data_020f6f30; }
