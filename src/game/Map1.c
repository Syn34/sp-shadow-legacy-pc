/*
 * Map.cpp: entering and running a map ("mapInit..." steps), activating
 * the actors of the current map mode, the pause menu check and the BG
 * set-up of a level.
 * ARM9 main, 0x020186fc - 0x02019620 (23 functions).
 *
 * Levels are 0x60-byte records at data_020c3b60 indexed by bits 1-7 of
 * save byte 0xf (see Protagonist1.c).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

extern Cnt data_020b566c, data_020b5670, data_020b5674, data_020b5678;
extern s8 data_020bd638;
extern const char data_020bd718[], data_020bd738[], data_020bd75c[], data_020bd784[],
    data_020bd7a0[], data_020bd7c4[], data_020bd7e0[], data_020bd800[], data_020bd820[],
    data_020bd840[], data_020bd864[], data_020bd884[], data_020bd8a8[], data_020bd8c4[],
    data_020bd8e4[], data_020bd8ec[], data_020bd920[], data_020bd954[], data_020bd96c[],
    data_020bd994[], data_020bd9b8[], data_020bd9f0[];
extern u8 *data_020c3b60;            /* levels, 0x60 bytes each */
extern u16 data_020d9f88;
extern u32 data_020da580[];
extern u8 data_020def18[];
extern s8 data_020df118;
extern s8 data_020df124;
extern u32 data_020df12c;            /* map mode */
extern s8 data_020df17c;
extern u8 data_020e1f50[];

void *func_02005d5c(s32 type);
void *func_02005d78(void);
void func_02004978(void);
void *func_020091e8(u32 a, u32 b);
void func_0200af18(s32 layer);
u32 func_020081d8(void);
u32 func_020081ec(s32 group);
void func_0200925c(s32 handle);
u8 *func_02009384(void);
void *func_02009394(void);
void func_0200af2c(u32 bits);
void func_0200afb0(u32 mode, u32 flags);
void func_0200b098(s32 layer);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b1a0(u32 screen);
void func_0200c150(u8 *z);
void *func_0200fcc0(void);
s32 func_02011bc8(void);
void func_02011bd8(u8 *self);
u8 *func_02011f34(void);
void *func_02011f44(void);
void func_020121c0(void);
void func_020121fc(void);
void func_02013b78(u8 *p, u32 v);
s32 func_02014094(u8 *self);
void func_02016340(s32 *pos, u8 *a);
void func_02017920(u8 *self);
void func_02018044(void (*fn)(void));
void func_020183e8(void);
u32 func_020186ec(void);
void func_02019640(s32 v);
void func_02019660(s32 v);
void *func_02019670(void);
s32 func_02019ad0(u32 id);
void func_02019b08(u32 a, u32 b);
s32 func_02019b98(u32 v);
void func_0201a20c(s32 a, s32 b, s32 c);
void func_0201ad94(void);
s32 func_0201b650(u32 screen);
void func_0201b770(u32 a, u32 b, u32 c);
void func_0201b984(void);
void func_0201bdd8(u32 layer, u32 a, u32 b);
void func_0201bed4(u32 layer, u32 a, u32 b);
void func_0201c494(Cnt a, Cnt b, Cnt c, u32 lvl, u32 d, u32 e);
void func_0201d19c(void);
void func_0201d248(void);
void func_0201e470(u32 v);
void func_0201e890(void);
void func_0201eb6c(void);
void func_0201ebac(void);
void func_0201ec40(void);
void func_0201f4a4(void);
void func_0201f69c(s32 a, s32 b, s32 c, s32 d);
void func_0201f940(void *actor);
void func_0201f980(u32 v);
void func_0201f998(void);
void func_020202f0(s32 v);
void func_02020390(void);
void func_02020440(u32 v);
void func_02028da4(void);
void func_0202930c(u32 v);
s32 func_02040f48(void *obj);
void func_02040f84(void *obj, u32 dt);
void func_02042808(u32 group);
void func_020428ac(void);
s32 func_020436d0(void);
s32 func_020436e0(void);
s32 func_02043708(void);
void func_0204378c(u32 screen);
void func_020437c4(u32 screen);
s32 func_020437fc(void);
void *func_02043ae0(void *e);
void func_020504b8(void *actor);
s32 func_0205f4dc(void *snd);
void func_0205fbb4(void *snd, u8 *data);
void func_02063ca8(void *e);
void func_02064104(void *e, void *cam);
void *func_02064348(void *e);
void func_02072840(void *hud, u32 dt);
void func_02072f2c(void *hud, u32 v);
void func_020731c0(void *hud);
void func_0207fe20(void);
void *func_0207fe74(u32 size);       /* operator new */
s32 func_02081d34(void *pad, u32 key);
void func_02090af4(u32 reg, const s32 *mtx, s32 cx, s32 cy, s32 x, s32 y);

/* this file's own functions, called before their definition */
void func_020186fc(u32 mode);
void func_02018a18(void);
void func_02018bf8(void);
void func_02018d8c(void);
void func_02018df8(void);
void func_02018f5c(void);
void func_0201903c(void);
void func_02019078(void);
void func_020190ac(void);
void func_020190f0(void);
void func_02019280(void);
void func_020192a8(void);
void func_020192ac(void);
void func_02019338(u32 lvl);
void func_02019430(void);
void func_02019520(void);

#define P ((u8 *)data_020deebc)
#define LEVEL_IDX() (func_020436f0(data_020bf6a0)[0xf] >> 1)
#define LEVEL(i) (data_020c3b60 + (i) * 0x60)

static inline void VCall(void *obj, u32 off, u32 v)
{
    VCALL(obj, off, void (*)(void *, u32))(obj, v);
}

static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

static inline void VecPush(struct PtrVec *v, void **value)
{
    if ((u32)v->count < (u32)v->cap) {
        v->count++;
        v->items[v->count - 1] = *value;
    } else {
        func_020062f0(v, value, 0);
    }
}

#define VEC_END(v) ((v)->items + (v)->count)

/* 0x020186fc: activate the actors that belong to map mode `mode`, and park
 * the others
 * @difftest pick:0,1 cases=20 */
void func_020186fc(u32 mode)
{
    void **it = func_020061e0()->items;

    while (it != VEC_END(func_020061e0())) {
        U32_AT(*it, 0x110) |= 2;
        VecPush(func_02006200(), it);
        it++;
    }
    {
        struct PtrVec *v = func_020061e0();
        s32 n = v->count;
        v->count -= n;
    }

    it = func_02006200()->items;
    while (it != VEC_END(func_02006200())) {
        void *a = *it;
        if ((BITS(U32_AT(a, 0x38), 5, 1) == 0 && mode == 0) ||
            (BITS(U32_AT(a, 0x38), 6, 1) == 0 && mode == 1)) {
            U32_AT(a, 0x110) &= ~2u;
            VecPush(func_020061e0(), &a);
            it = func_02006240(func_02006200(), it, 0);
        } else {
            it++;
        }
    }
}

/* 0x02018950: update the map (paused: only the actors) */
void func_02018950(s32 paused)
{
    if (paused == 0) {
        u32 dt = func_020822c0(func_020062d0());
        if (func_02019ad0(0x1e) != 0)
            func_02072840(func_02011f34(), dt);
        func_02004978();
        VCall(func_02019670(), 8, dt);
    } else {
        func_02004978();
    }
}

/* 0x020189b0 */
void func_020189b0(void)
{
    void *s;
    func_02018a18();
    s = func_0200fcc0();
    func_02040f84(s, func_020822c0(func_020062d0()));
    if ((s8)(func_02040f48(func_0200fcc0()) != -1))
        data_020df17c = 0;
    else
        data_020df17c = 1;
}

/* 0x02018a18: open or close the pause menu */
void func_02018a18(void)
{
    if (func_02019b98(1) == 0x18)
        return;
    if (func_02019b98(1) == 0x15)
        return;
    if (func_02011bc8() != 0)
        return;
    if ((s8)(func_02040f48(func_0200fcc0()) != -1))
        return;
    if (func_0205f4dc(func_02009394()) == 1)
        return;
    if (data_020bd638 == 0)
        return;
    if (func_020437fc() == 0) {
        if (func_02081d34(func_02011f44(), 3) == 0)
            return;
        func_02020440(0x44);
        func_020437c4(data_020df0fc);
        func_0201b770(1, 0x3f, 1);
        func_02019b08(0x1a, 1);
        func_02072f2c(func_02011f34(), 1);
        return;
    }
    if (data_020df124 == 0)
        return;
    if (func_0201b650(data_020df0fc) != 0)
        return;
    func_02072f2c(func_02011f34(), 0);
    func_0204378c(data_020df0fc);
    func_02020390();
    data_020df124 = 0;
}

/* 0x02018ba4 */
void func_02018ba4(void)
{
    func_0201f4a4();
    func_0201e470(0);
    func_020121fc();
}

/* 0x02018bc8: enter the map */
void func_02018bc8(void)
{
    func_02028da4();
    func_02018bf8();
    func_0207fe20();
    data_020df118 = 0;
}

/* 0x02018bf8: set up the map */
void func_02018bf8(void)
{
    void *snd;
    func_02019520();
    func_02019430();
    func_02018044(func_020183e8);
    func_0202930c(0);
    func_02019338(LEVEL_IDX());
    func_02004490(data_020bd718);
    func_0201d19c();
    func_0201d248();
    func_0200c150(data_020e1f50);
    func_020192ac();
    func_02004490(data_020bd738);
    func_0200925c(S32_AT(LEVEL(LEVEL_IDX()), 0x18));
    snd = func_02009394();
    func_0205fbb4(snd, func_02009384());
    func_02004490(data_020bd75c);
    func_020190f0();
    func_02004490(data_020bd784);
    func_020190ac();
    func_02018df8();
    func_02004490(data_020bd7a0);
    func_02019078();
    func_02004490(data_020bd7c4);
    func_0201903c();
    func_02004490(data_020bd7e0);
    func_02018f5c();
    func_02004490(data_020bd800);
    func_0201f4a4();
    func_02004490(data_020bd820);
    func_020192a8();
    func_02004490(data_020bd840);
    func_02019280();
    func_02004490(data_020bd864);
    func_020186fc(data_020df12c);
    func_02004490(data_020bd884);
    func_020731c0(func_02011f34());
    func_02004490(data_020bd8a8);
    func_02018d8c();
    func_02004490(data_020bd8c4);
}

/* 0x02018d8c: start the map's music */
void func_02018d8c(void)
{
    if (func_020186ec() == 1)
        func_020202f0(2);
    else
        func_020202f0(S32_AT(LEVEL(LEVEL_IDX()), 0x2c));
}

/* 0x02018df8: load the map's appearance groups ("mapInitAppearances") */
void func_02018df8(void)
{
    u32 total = 0, n = func_020081d8(), i;

    for (i = 1; i < n; i++)
        total += func_020081ec(i & 0xff);
    if (data_020d9f88 & 1)
        total += func_020081ec(0);
    func_0207fe28(data_020bd8e4, 0x24c);
    VCall((u8 *)func_020062b0() + 4, 0xc, total);
    VCall((u8 *)func_02019670() + 4, 0xc, total);
    VCall((u8 *)func_020062c0() + 4, 0xc, total);
    VCall((u8 *)func_020062a0() + 4, 0xc, total);
    func_0207fe24();
    if (LEVEL_IDX() == 0x19) {
        func_02042808(1);
        func_02004490(data_020bd8ec);
        func_02042808(0);
        func_02004490(data_020bd920);
        return;
    }
    for (i = 1; i < n; i++) {
        func_02042808(i & 0xff);
        func_02004490(data_020bd8ec);
    }
    if (data_020d9f88 & 1) {
        func_02042808(0);
        func_02004490(data_020bd920);
    }
}

/* 0x02018f5c: set up the camera ("mapInitCamera") */
void func_02018f5c(void)
{
    u32 a, b, c, d;
    func_0201f998();
    func_0201f980(2);
    func_0201f940(PTR_AT(P, 4));
    a = LEVEL_IDX();
    b = LEVEL_IDX();
    c = LEVEL_IDX();
    d = LEVEL_IDX();
    func_0201f69c((s16)S32_AT(LEVEL(d), 0x3c), (s16)S32_AT(LEVEL(c), 0x40),
                  (s16)S32_AT(LEVEL(b), 0x44), (s16)S32_AT(LEVEL(a), 0x48));
}

/* 0x0201903c: set up the dialogue actor ("mapInitDialog") */
void func_0201903c(void)
{
    func_02017920(data_020def18);
    func_02013b78(data_020def18, (u32)func_02005d78());
    func_02011bd8(PTR_AT(data_020def18, 4));
}

/* 0x02019078: create the screen wipe ("mapInitWipe") */
void func_02019078(void)
{
    void *e = func_0207fe74(0x64);
    if (e != NULL)
        func_02043ae0(e);
    func_02004490(data_020bd954);
}

/* 0x020190ac */
void func_020190ac(void)
{
    void *e = func_0207fe74(0x1ac);
    if (e != NULL)
        e = func_02064348(e);
    func_02064104(e, func_02016f24(data_020deebc));
    func_02063ca8(e);
}

/* 0x020190f0: set up the hero at the start point ("mapInitHero") */
void func_020190f0(void)
{
    s32 pos[2], p2[2];
    u8 *start;

    pos[0] = 0;
    pos[1] = 0;
    start = func_020091e8(func_020436f0(data_020bf6a0)[0x10], 0);
    func_02004490(data_020bd96c);
    pos[0] = SHL(S16_AT(start, 0), 16);
    pos[1] = SHL(S16_AT(start, 2), 16);
    if (PTR_AT(P, 8) == NULL) {
        func_02017920(P);
        func_02013b78(P, (u32)func_02005d5c(0));
        P[0x1f] = 0;
        if (S16_AT(func_020436f0(data_020bf6a0), 4) <= 0) {
            s32 v = func_02014094(P);
            if (v > func_02043708())
                v = func_02043708();
            else if (v < 0)
                v = 0;
            S16_AT(func_020436f0(data_020bf6a0), 4) = v;
        }
        func_02004490(data_020bd994);
    }
    if (func_02016f24(data_020deebc) == NULL) {
        func_020504b8(PTR_AT(P, 4));
        func_02004490(data_020bd9b8);
    }
    p2[0] = pos[0];
    p2[1] = pos[1];
    func_02016340(p2, PTR_AT(P, 4));
    func_02004490(data_020bd9f0);
}

/* 0x02019240 */
void func_02019240(void)
{
    func_02019430();
    func_0201ebac();
    func_02019280();
}

/* 0x02019260 */
void func_02019260(void)
{
    func_0201eb6c();
    func_0200b1a0(0);
}

/* 0x02019280 */
void func_02019280(void)
{
    func_0201b984();
    func_0201e470(1);
    func_0201e890();
    func_020121fc();
}

/* 0x020192a8
 * @difftest */
void func_020192a8(void) {}

/* 0x020192ac: set up the map's collision ("mapInitBGCollision") */
void func_020192ac(void)
{
    u32 a = LEVEL_IDX(), b = LEVEL_IDX(), c = LEVEL_IDX();
    func_0201a20c(S32_AT(LEVEL(c), 0x10), S32_AT(LEVEL(b), 0x14), S32_AT(LEVEL(a), 0x1c));
}

/* 0x02019338: set up the BG layers of level `lvl` ("mapInitGraphics") */
void func_02019338(u32 lvl)
{
    u8 *l;
    func_0201ec40();
    func_0201c494(data_020b5670, data_020b566c, data_020b5674, lvl, 0x4000, 0x2000);
    l = LEVEL(lvl);
    func_0201bed4(0, U32_AT(l, 0), 0x2000);
    func_0201bed4(1, U32_AT(LEVEL(lvl), 8), 0x2000);
    func_0201bdd8(0, U32_AT(LEVEL(lvl), 4), 0x203d8);
    func_0201bdd8(1, U32_AT(LEVEL(lvl), 0xc), 0xe620);
    func_020121c0();
}

/* 0x02019430: display set-up of the map screen */
void func_02019430(void)
{
    s32 m[4];
    func_0200afb0(5, 3);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020b5678.v);
    func_0200b168(2, data_020b5670.v);
    func_0200b168(1, data_020b5674.v);
    func_0200af18(3);
    func_0200af18(2);
    func_0200af18(1);
    func_0200af18(0);
    func_0200b098(1);
    func_0200b098(3);
    m[0] = 0x1000;
    m[3] = 0x1000;
    m[1] = 0;
    m[2] = 0;
    func_02090af4(0x04000020, m, 0x80, 0x80, 0, 0);
}

/* 0x02019520 */
void func_02019520(void)
{
    func_0201ad94();
    data_020da580[data_020df0fc] = 0;
}

/* 0x02019554: open the map screen (once)
 * @difftest @020df118:8=pick:0,1 @020deedc:32=pick:0,0x20 u32 cases=40 */
void func_02019554(void *self)
{
    if (data_020df118 != 0)
        return;
    func_0201b770(2, 0x3f, 1);
    func_02019b08(0x1e, 0);
    if (BITS(data_020deebc[0x20 / 4], 5, 1) == 1)
        func_02019b08(0xc, 1);
    else
        func_02019b08(0x14, 1);
    if (LEVEL_IDX() == 0x19) {
        func_02019660(func_020436e0());
        func_02019640(func_020436d0());
    } else {
        func_020428ac();
    }
    data_020df118 = 1;
}
