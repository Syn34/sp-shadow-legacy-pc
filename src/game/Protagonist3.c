/*
 * CProtagonist, part 3: the hero's per-frame update (level-ups, riding
 * platforms, the camera follower) and creating/killing the breath effect
 * emitters ("heroInit: Set up FireBreath Emitter" and friends).
 * ARM9 main, 0x020157e0 - 0x02016340 (3 functions).
 *
 * The CProtagonist object is data_020deebc: +0x04 hero actor, +0x08 and
 * +0x0c helper objects, +0x18 camera mode, +0x1f, +0x20 flags (bit 1 hit a
 * trigger, bit 3, bit 4 landed, bit 5 alternative control), +0x4c level-up
 * effect, +0x55..+0x58 boosts (Protagonist1.c).
 * The emitters live in data_020dedb0..data_020dedd4.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcef0[];           /* RTTI */
extern const fx32 data_020bd074, data_020bd090, data_020bd098;
extern s32 data_020b6af4[];          /* experience needed for each breath level */
extern s32 data_020b6cc4;
extern u8 *data_020c3b0c;            /* object kinds, 0x48 bytes each */
extern u8 *data_020c3b40;            /* 64 bytes each */
extern void *data_020dedb0, *data_020dedb4, *data_020dedb8, *data_020dedbc, *data_020dedc0,
    *data_020dedc4, *data_020dedc8, *data_020dedd0, *data_020dedd4;
extern u8 data_020dedfc[], data_020dee2c[];

void func_02003b08(s32 *out, const s32 *in, s32 shift);
void func_02005c1c(s32 *out, const s32 *a, const s32 *b);
s32 func_02007458(void *a);
void *func_02009394(void);
u8 *func_02011f34(void);
s32 func_02011bc8(void);
u32 func_02013b84(void *self);
u32 func_02013c84(void *self);
void func_02013bac(void *self, u32 v);
void func_02013d20(void *self, s32 v);
s32 func_02013e90(u8 *self);
void func_0201402c(void *self, s32 v);
s32 func_02014094(u8 *self);
void func_020140c8(u8 *self);
void func_02014138(u8 *self);
void func_020142c0(void *self, u32 n);
void func_020143ac(void);
void func_020150a0(s32 *out, const s32 *a, const s32 *b);
void func_020168a0(void *self);
void func_0201698c(void *self);
void func_02016b4c(void *self);
void func_02016df8(void *self, s32 v);
s32 func_02017a58(void *list, void *item);
void func_02017abc(void *list, void *item);
void func_02019554(void *self);
s32 func_0201b650(u32 screen);
void func_02020440(u32 v);
const char *func_02028a20(u32 id);
void func_0204378c(u32 screen);
void func_020437c4(u32 screen);
void *func_02051c38(void *e, u32 v);
void *func_02051d78(void *e);
void func_02053460(void *obj, u32 dt);
void func_02055078(void *e, s32 x, s32 y, s32 z);
void func_020550ac(void *e, u32 v);
void func_020551b4(void *e);
void func_02055890(void *e, void *obj);
void func_02055ad8(void *e, u32 v, u32 w);
void *func_0205602c(void *e);
void func_020573c4(void *e, u32 v);
void func_02058954(void *e, u32 v, void *obj);
void func_0205f6b0(void *snd, void *sound);
void func_02076b44(void *obj, const char *text, u32 fbits);
void *func_0207fe74(u32 size);       /* operator new */
void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */

#define P ((u8 *)data_020deebc)
#define FLAG(a, bit) ((s8)BITS(U32_AT(a, 0x38), bit, 1))

static inline void VCall(void *obj, u32 off, u32 v)
{
    VCALL(obj, off, void (*)(void *, u32))(obj, v);
}

static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

static inline void StayPut(u8 *a)
{
    S32_AT(a, 0x5c) = S32_AT(a, 0x54);
    S32_AT(a, 0x60) = S32_AT(a, 0x58);
}

static inline void FlashBar(u8 *bar)
{
    bar[0] = 1;
    if ((s8)bar[0] == 0)
        U32_AT(bar, 0x24) = 0;
}

/* gain a breath level */
static void LevelUp(u32 lvl)
{
    u8 *hud, *e;
    const char *txt;

    func_02013bac(P, lvl + 1);
    func_020142c0(P, 2);
    func_0201402c(P, func_02014094(P));
    func_02013d20(P, func_02013e90(P));
    hud = func_02011f34();
    if (func_02011bc8() == 0) {
        FlashBar(hud + 0xe0);
        FlashBar(hud + 0x20c);
        FlashBar(hud + 0x354);
        hud[0x23] = 1;
    }
    func_02020440(0x5a);
    txt = func_02028a20(0x20);
    hud = func_02011f34();
    if (func_02011bc8() == 0) {
        func_02076b44(hud + 0x768, txt, 0x46400000);   /* 12800.0f */
        func_02076b44(hud + 0x7c4, NULL, 0);
    }
    e = func_0207fe74(0xf0);
    if (e != NULL)
        e = func_0205602c(e);
    if (e != NULL) {
        func_02055ad8(e, 0x35, 1);
        func_02055890(e, func_02016f24(data_020deebc));
        func_02055078(e, 0, -0xd000, -0x32000);
        func_020550ac(e, 0x7c);
        func_020551b4(e);
        PTR_AT(P, 0x4c) = e;
        func_020437c4(data_020df0fc);
        VCall(PTR_AT(P, 8), 0xc, 0);
        VCall(e, 8, 0);
    }
    if ((s32)func_02013b84(P) <= 4)
        func_0205f6b0(func_02009394(), data_020dedfc);
    else if (func_02013b84(P) == 5)
        func_0205f6b0(func_02009394(), data_020dee2c);
}

/* 0x020157e0: hero update
 * @difftest $W=ptr:0xa0:4 $A=ptr:0x120:4 @$A+0x44:32=$W @$A+0x38:32=u32 @$A+0x110:32=u32 @$A+0xa4:32=pick:0 @$A+8:32=int:0:0x100 @020def08:32=pick:0 @020deedc:32=u32 @020deed4:32=int:0:3 @020b6af4:32=pick:0x7fffffff @020b6af8:32=pick:0x7fffffff @020b6afc:32=pick:0x7fffffff @020b6b00:32=pick:0x7fffffff @020b6b04:32=pick:0x7fffffff @020b6b08:32=pick:0x7fffffff $A cases=60 */
void func_020157e0(u8 *a)
{
    if (func_02016f24(data_020deebc) == NULL || func_0201b650(data_020df0fc) != 0)
        goto done;

    if (FLAG(a, 14) && (func_02007458(a) == 2 || func_02007458(a) == 3)) {
        if (FLAG(a, 17) == 0)
            StayPut(a);
    }

    if ((s8)BITS(U32_AT(PTR_AT(P, 4), 0x110), 3, 1)) {
        func_02019554(P);
        goto boosts;
    }
    if (PTR_AT(P, 0x4c) != NULL) {
        u8 *e;
        StayPut(a);
        e = PTR_AT(P, 0x4c);
        if (e != NULL)
            e += 0x90;
        if (func_02017a58((u8 *)func_020062a0() + 4, e) == 0) {
            PTR_AT(P, 0x4c) = NULL;
            func_0204378c(data_020df0fc);
        }
        goto boosts;
    }

    {
        u32 lvl = func_02013b84(P);
        if (lvl < 0x18 && (s32)func_02013c84(P) >= data_020b6af4[lvl])
            LevelUp(lvl);
    }

    /* riding a platform */
    if (BITS(U32_AT(a, 0x110), 12, 1)) {
        if (PTR_AT(a, 0xa4) != NULL) {
            void *o = PTR_AT(a, 0xa4);
            if (VCALL(o, 0x24, s32 (*)(void *, void *))(o, a) == 0) {
                PTR_AT(a, 0xa4) = NULL;
                U32_AT(a, 0x110) &= ~0x1000u;
                U32_AT(a, 0x38) &= ~0x20000u;
            }
        } else {
            U32_AT(a, 0x110) &= ~0x1000u;
            U32_AT(a, 0x38) &= ~0x20000u;
        }
    } else if (PTR_AT(a, 0xa4) != NULL) {
        void *o = PTR_AT(a, 0xa4);
        if (VCALL(o, 0x24, s32 (*)(void *, void *))(o, a) == 0) {
            PTR_AT(a, 0xa4) = NULL;
            U32_AT(a, 0x38) &= ~0x20000u;
        } else {
            u8 *pl = PTR_AT(a, 0xa4);
            if (pl != NULL)
                pl = func_020a6efc(pl, data_020bcb88, data_020bcef0, -1);
            if (pl != NULL) {
                s32 d[2], s[2], n[2];
                func_020150a0(d, (s32 *)((u8 *)PTR_AT(a, 0xa4) + 0x28), (s32 *)(pl + 0x64));
                func_02003b08(s, d, 4);
                d[0] = s[0];
                d[1] = s[1];
                func_02005c1c(n, (s32 *)(a + 0x5c), d);
                S32_AT(a, 0x5c) = n[0];
                S32_AT(a, 0x60) = n[1];
            }
        }
    }

    if (BITS(U32_AT(P, 0x20), 5, 1))
        func_0201698c(P);
    else
        func_02016b4c(P);
    {
        void *hero = func_02016f24(data_020deebc);
        func_02053460(hero, func_020822c0(func_020062d0()));
    }
    func_020168a0(P);
    P[0x1f] = 0;
    {
        u32 kind = U32_AT(a, 8);
        u8 *cam = PTR_AT(func_02016f24(data_020deebc), 0xac);
        if (U32_AT(cam, 0x2c) == U32_AT(data_020c3b40, S32_AT(data_020c3b0c + kind * 0x48, 0x1c) * 64))
            func_02016df8(P, data_020b6cc4);
    }

    /* move the follower next to the hero */
    if (FLAG(a, 8) && (u32)(S32_AT(P, 0x18) - 1) <= 1) {
        s32 v0 = data_020bd074, v2 = data_020bd090, x, z;
        u32 idx;
        s16 sn, cs;
        u8 *f;
        (void)data_020bd098;
        idx = FxRadToIdx(S32_AT(func_02016f24(data_020deebc), 0x14)) >> 4;
        sn = FX_SinCosTable_[idx * 2];
        cs = FX_SinCosTable_[idx * 2 + 1];
        x = FxMul(v0, cs) + FxMul(v2, sn);
        z = FxMul(v2, cs) - FxMul(v0, sn);
        f = PTR_AT(P, 0xc);
        S32_AT(f, 0x28) = S32_AT(a, 0x28) + x;
        S32_AT(f, 0x2c) = S32_AT(a, 0x2c) + z;
        func_02017abc((u8 *)func_020062c0() + 4, PTR_AT(P, 0xc));
    }

boosts:
    if (P[0x55] != 0)
        func_02014138(P);
    if (P[0x57] != 0)
        func_020140c8(P);
    func_020143ac();
done:
    U32_AT(P, 0x20) &= ~2u;
    U32_AT(P, 0x20) &= ~8u;
}

/* 0x02015edc: delete the breath emitters ("CProtagonist::KillEmitters()")
 * @difftest cases=10 */
void func_02015edc(void)
{
    void *p;
    p = data_020dedc8;
    if (p != NULL)
        VCallR1(p, 4);
    p = data_020dedd0;
    data_020dedc8 = NULL;
    if (p != NULL)
        VCallR1(p, 4);
    p = data_020dedc4;
    data_020dedd0 = NULL;
    if (p != NULL)
        VCallR1(p, 4);
    p = data_020dedb4;
    data_020dedc4 = NULL;
    if (p != NULL)
        VCallR1(p, 4);
    p = data_020dedb8;
    data_020dedb4 = NULL;
    if (p != NULL)
        VCallR1(p, 4);
    data_020dedb8 = NULL;
    data_020dedbc = NULL;
    data_020dedd4 = NULL;
    data_020dedb0 = NULL;
    data_020dedc0 = NULL;
}

static void *NewEmitter(void)
{
    void *e = func_0207fe74(0x3c88);
    if (e != NULL)
        e = func_02051d78(e);
    return e;
}

static void *NewEffect(void)
{
    void *e = func_0207fe74(0xf0);
    if (e != NULL)
        e = func_0205602c(e);
    return e;
}

#define HERO() func_02016f24(data_020deebc)

/* 0x02015ffc: create the breath emitters ("heroInit") */
void func_02015ffc(void)
{
    data_020dedc8 = NewEmitter();
    func_02051c38(data_020dedc8, 0x710);
    func_020573c4(data_020dedc8, 0x4df);
    func_02058954(data_020dedc8, 0, HERO());

    data_020dedb0 = NewEffect();
    func_02055ad8(data_020dedb0, 0xc3, 1);
    func_020550ac(data_020dedb0, 0x193);

    data_020dedd0 = NewEmitter();
    func_02051c38(data_020dedd0, 0x7ab);
    func_020573c4(data_020dedd0, 0x481);
    func_02058954(data_020dedd0, 0, HERO());

    data_020dedc4 = NewEmitter();
    func_02051c38(data_020dedc4, 0x7aa);
    func_020573c4(data_020dedc4, 0x4c7);
    func_02058954(data_020dedc4, 0, HERO());

    data_020dedb4 = NewEmitter();
    func_02051c38(data_020dedb4, 0x7a9);
    func_020573c4(data_020dedb4, 0x484);
    func_02058954(data_020dedb4, 0, HERO());

    data_020dedc0 = NewEffect();
    func_02055ad8(data_020dedc0, 0x133, 1);
    func_02055890(data_020dedc0, HERO());
    func_020550ac(data_020dedc0, 0x2da);
    func_020550ac(data_020dedc0, 0x2db);

    data_020dedb8 = NewEmitter();
    func_02051c38(data_020dedb8, 0x299);
    func_020573c4(data_020dedb8, 0x920);
    func_02058954(data_020dedb8, 0, HERO());

    data_020dedbc = NewEffect();
    func_02055ad8(data_020dedbc, 0x136, 1);
    func_02055890(data_020dedbc, HERO());
    func_020550ac(data_020dedbc, 0x40c);

    data_020dedd4 = NewEffect();
    func_02055ad8(data_020dedd4, 0xbb, 1);
    func_02055890(data_020dedd4, HERO());
    func_020550ac(data_020dedd4, 0x16e);
}
