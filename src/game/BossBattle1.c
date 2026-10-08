/*
 * BossBattle.cpp, part 1: the boss battle's per-frame update, the defeat
 * message, leaving the battle and setting it up.
 * ARM9 main, 0x0202aa14 - 0x0202b968 (11 functions).
 *
 * The battle's actors are the vector g_GameRoot (see GameRoot0.c: [0] the
 * level object, [1] the hero, [2] the boss, [3] the HUD counter, [4] the
 * arena). data_020e36e0 and data_020e36f8 are the two collision lists
 * whose actors are tested against each other; data_020e36b4 is the camera
 * effect object, data_020e36b8 the battle camera target.
 *
 * Several methods are called with the method's own address left in r1 (the
 * original loads it there before `blx r1`); the C passes it too, since the
 * deleting destructor reads r1 as its flag.
 *
 * Testing: func_0202b41c runs with the save-data call func_02042480 stubbed
 * (it waits for the save hardware); func_0202b5f4 is not difftested (see
 * there).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

/* collision result filled in by an actor's hit test (method +0x28) */
typedef struct {
    u8 hit;
    s32 push;      /* +0x04: new +0x4c of the first actor */
    s32 one;       /* +0x08 */
    u32 z[9];      /* +0x0c..+0x2c, cleared once per frame */
    s32 a, b;      /* +0x30, +0x34 */
} Hit;

extern const char data_020bdefc[];   /* "bossBattleExit::actorKillAll - active\n" */
extern const char data_020bdf24[];   /* "bossBattleExit::actorKillAll - inactive\n" */
extern Cnt data_020b5674, data_020b5678;
extern u8 data_020bde90[];
extern u8 *data_020c3b0c;            /* 0x48-byte boss records */
extern u8 data_020d9f9c[];
extern u8 data_020def18[];
extern u8 data_020df114[];
extern s8 data_020df10c;
extern s8 data_020e36a0;
extern u8 data_020e36a8;
extern s32 data_020e36ac;
extern s32 data_020e36b0;
extern void *data_020e36b4;
extern void *data_020e36b8;
extern PtrVec data_020e36e0, data_020e36f8;
extern s32 data_020e3710[];          /* gravity */
extern u8 data_020e371c[];           /* popups (Text2.c) */
extern s8 g_GameRootValid;
extern PtrVec g_GameRoot;

void func_02018a18(void);
u32 func_020822c0(void *timer);
s32 func_020437fc(void);
s8 func_0202a724(void);
s8 func_0202a768(void);
s8 func_0202a7ac(void);
s32 func_0204a868(u32 counter);
s8 func_0207768c(void *obj);
void func_02078810(void *lvl, s32 *pos);
s32 func_020784c0(void *lvl, s32 *pos);
void func_02077f68(void *lvl);
void func_020784b4(void *lvl);
void func_02079c1c(void *arena, void *boss);
void func_020298d8(void *node);
void func_02020440(u32 v);
void func_020202f0(u32 v);
void func_02013be8(void *obj, s32 v);
void func_02019660(u32 v);
void func_02019640(u32 v);
void func_02011664(u32 text, u32 v);
s32 func_0208eff8(s32 v);
void func_02029ba8(void);
void func_020293b4(u8 *self);
void func_02029790(s32 *out, const s32 *a, const s32 *b);
void func_0202a104(s32 *out, const s32 *a, const s32 *b);
void func_0202a148(s32 *out, u8 *obj);
void func_0208ea6c(void *mtx44);
void MI_Copy64B(const void *src, void *dest);
void func_02080e84(void *settings);
void func_020718e4(void *obj, s32 hp);
u8 *func_02011f34(void);
void func_02072840(void *hud, u32 dt);
void func_02004864(void);
void *func_0200fcc0(void);
void func_02040f84(void *obj, u32 dt);
void func_020186dc(u32 v);
void func_02042480(u32 v);
void func_02072ff0(void *hud);
void *func_020062a0(void);
void *func_020062b0(void);
void *func_02019670(void);
void func_02029870(void);
void func_020298b0(void);
PtrVec *func_02006200(void);
PtrVec *func_020061e0(void);
void func_020043c8(PtrVec *list);
s32 func_02004490(const char *fmt, ...);
void func_020814c8(void *settings);
void *func_02018208(void);
void func_02082e10(void *obj, u32 a, u32 b);
void func_02082f64(void *obj);
void func_02013b78(u8 *p, u32 v);
void func_02015edc(void);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b098(s32 layer);
void func_0200af18(s32 layer);
void func_020813b0(void *settings, const u16 *colour);
void func_02080f60(void *settings, u32 a, u32 b);
void *func_0207fe74(u32 size);
void *func_02043ae0(void *e);
void *func_02064348(void *e);
void func_02064104(void *e, void *cam);
void func_02063ca8(void *e);
void func_020798d8(void *hud, void *hero, void *boss, void *fx);
void func_020731c0(void *hud);
s32 func_02011bc8(void);
void func_02017920(u8 *self);
void *func_02005d78(void);
void func_02011bd8(u8 *self);
void func_02015ffc(void);

#define ROOT(i) ((u8 *)g_GameRoot.items[i])
#define FOR_EACH(p, v) for ((p) = (v).items; (p) != (v).items + (v).count; (p)++)
/* call method `off` of `obj` with the method's address in r1, like the original */
#define METHOD0(obj, off) \
    do { \
        void *o_ = (obj); \
        void (*m_)(void *, void *) = VCALL(o_, off, void (*)(void *, void *)); \
        m_(o_, (void *)m_); \
    } while (0)
#define BOSS_REC(b) U32_AT(data_020c3b0c + U32_AT(b, 8) * 0x48, 0x24)

static s32 FxSquare(s32 x)
{
    return (s32)(((s64)x * x + 0x800) >> 12);
}

/* 0x0202aa14: per-frame update of the boss battle
 * @difftest cases=5
 * @difftest @020ebba0:8=pick:0,1 @020bde90:8=pick:0,1 @020e36a4:8=1 $V=zero:0x100 @$V+0x28:32=0x02070a18 @$V+0x4c:32=0x02070a18 @$V+0x50:32=0x02070a18 @$V+0x54:32=0x02070a18 @$V+0x58:32=0x02070a18 @$V+0x6c:32=0x02070a18 @$V+0x70:32=0x0202b1ac $W=zero:0x10 @$W+8:32=0x02070a18 $H=zero:0x10 @$H+0xc:32=1 $N=zero:0x100 @$N+0xac:32=$H $A=zero:0x900 @$A+0:32=$V @$A+0x44:32=$N @$A+0x114:32=$W @$A+0x38:32=4 $B=zero:0x300 @$B+0:32=$V @$B+0x44:32=$N @$B+0x114:32=$W @$B+0x38:32=4 $R=ptr:0x10:4 @$R+0:32=$A @$R+4:32=$B @$R+8:32=$A @020e36c8:32=$R @020e36cc:32=3 $L1=ptr:4:4 @$L1+0:32=$A @020e36e0:32=$L1 @020e36e4:32=1 $L2=ptr:4:4 @$L2+0:32=$B @020e36f8:32=$L2 @020e36fc:32=1 cases=20 */
void func_0202aa14(void)
{
    u8 *lvl, *hero, *boss, *n;
    void **p, **q;
    u32 dt;
    s32 i;

    func_02018a18();
    if (g_GameRootValid == 0)
        return;
    lvl = ROOT(0);
    boss = ROOT(2);
    hero = ROOT(1);
    dt = func_020822c0(func_020062d0());
    if (func_020437fc() == 0) {
        if ((s8)data_020bde90[0] == 0) {
            s32 v[3];
            func_02029790(v, (s32 *)(hero + 0x5c), data_020e3710);
            S32_AT(hero, 0x5c) = v[0];
            S32_AT(hero, 0x60) = v[1];
            S32_AT(hero, 0x64) = v[2];
        }
        FOR_EACH(p, g_GameRoot)
            VCALL(*p, 0x4c, void (*)(void *, u32))(*p, dt);
        FOR_EACH(p, g_GameRoot)
            VCALL(*p, 0x50, void (*)(void *, u32))(*p, dt);

        if (func_0202a724() != 0) {
            ((u8 *)data_020deebc)[0x1f] = 0;
        } else {
            s32 a[3];
            func_0202a148(a, hero);
            func_02078810(lvl, a);
        }
        {
            s32 b[3];
            func_0202a148(b, hero);
            if (func_020784c0(lvl, b) != 0
                && ((func_0202a7ac() != 0 && func_0204a868(5) != 0)
                    || (func_0202a768() != 0 && func_0204a868(0x12) != 0)
                    || func_0202a724() != 0)) {
                if (data_020e36b0 == 0) {
                    data_020d9f9c[0x78] = 1;
                } else if (data_020e36b0 == 1) {
                    data_020d9f9c[0x8c] = 1;
                } else if (data_020e36b0 == 2) {
                    data_020d9f9c[0x100] = 1;
                    data_020d9f9c[0x1fb] = 0;
                }
                func_02077f68(lvl);
            }
        }

        if (func_0207768c(boss) != 0) {
            n = PTR_AT(boss, 0x44);
            if (n[0x94] <= 0x1d && (s8)lvl[0x8e4] == 0) {
                func_020298d8(n);
                func_02020440(0x54);
                func_02013be8(data_020deebc, BOSS_REC(boss));
                if (func_0202a724() == 0)
                    func_02079c1c(g_GameRoot.items[4], boss);
                if (func_0202a768() != 0)
                    func_020202f0(0x23);
                else
                    func_020202f0(0x22);
                func_020784b4(lvl);
            }
            if (func_0202a724() != 0) {
                if ((s8)lvl[0x8e4] == 0) {
                    data_020d9f9c[0x100] = 1;
                    data_020d9f9c[0x1fb] = 0;
                    data_020df114[0] = 1;
                    func_02019660(9);
                    func_02019640(4);
                    U32_AT(lvl, 0x8e0) = 0x10;
                    PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = hero;
                    func_02011664(0x22e, 1);
                    func_020202f0(0x22);
                    func_020784b4(lvl);
                    func_02013be8(data_020deebc, BOSS_REC(boss));
                }
                n = PTR_AT(boss, 0x44);
                if (n[0x94] == 0x1d) {
                    func_020298d8(n);
                    func_02020440(0x54);
                }
            }
        }

        FOR_EACH(p, g_GameRoot)
            U32_AT(*p, 0x4c) = 0;

        if ((s8)data_020bde90[0] == 0) {
            Hit h;
            for (i = 0; i < 9; i++)
                h.z[i] = 0;
            FOR_EACH(p, data_020e36e0) {
                FOR_EACH(q, data_020e36f8) {
                    u8 *a = *q, *b = *p;
                    s32 hit = 0;
                    h.hit = 0;
                    h.push = 0;
                    h.one = 0x1000;
                    h.a = 0;
                    h.b = 0;
                    if (b != a && ((U32_AT(b, 0x38) >> 2) & 1) && ((U32_AT(a, 0x38) >> 2) & 1))
                        hit = VCALL(a, 0x28, s32 (*)(void *, void *, Hit *))(a, b, &h);
                    if (hit != 0) {
                        VCALL(*p, 0x6c, void (*)(void *, void *, s32, s32, Hit *))(*p, *q, h.b, h.a, &h);
                        VCALL(*q, 0x6c, void (*)(void *, void *, s32, s32, Hit *))(*q, *p, h.a, h.b, &h);
                        if (h.push != 0)
                            S32_AT(*p, 0x4c) = h.push;
                    }
                }
            }
        }

        FOR_EACH(p, g_GameRoot)
            VCALL(*p, 0x54, void (*)(void *, u32))(*p, dt);
        {
            s32 c[3];
            u32 v;
            func_0202a148(c, hero);
            v = VCALL(lvl, 0x70, u32 (*)(void *, s32 *))(lvl, c);
            U32_AT(PTR_AT(hero, 0x44), 0x98) = v;
        }
        FOR_EACH(p, g_GameRoot) {
            U32_AT(*p, 0x38) &= ~0x8u;
            U32_AT(*p, 0x38) &= ~0x400u;
            U32_AT(*p, 0x38) &= ~0x4000u;
        }
        {
            s32 a[3], b[3], d[3];
            func_0202a148(a, g_GameRoot.items[1]);
            func_0202a148(b, g_GameRoot.items[2]);
            func_0202a104(d, a, b);
            data_020e36ac = func_0208eff8(FxSquare(d[2]) + (FxSquare(d[0]) + FxSquare(d[1])));
        }
        func_02029ba8();
    }

    for (i = 0; i < 8; i++)
        func_020293b4(data_020e371c + i * 0x18);
    {
        u32 m[16];
        u8 *st;
        func_0208ea6c(m);
        U32_AT(func_020062e0(), 0x4a4) = 0x1f;
        st = func_020062e0();
        MI_Copy64B(m, st + 0x3d8);
        U32_AT(st, 0x2cc) |= 4;
        func_02080e84(func_020062e0());
    }
    FOR_EACH(p, g_GameRoot)
        METHOD0(*p, 0x58);
    FOR_EACH(p, g_GameRoot) {
        s32 hp = S32_AT(PTR_AT(PTR_AT(*p, 0x44), 0xac), 0xc);
        if (hp > 0)
            func_020718e4(*p, hp);
    }
    func_02072840(func_02011f34(), dt);
    func_02004864();
    func_02040f84(func_0200fcc0(), dt);
}

/* 0x0202b1ac
 * @difftest cases=2 */
s32 func_0202b1ac(void)
{
    return 0;
}

/* 0x0202b1b4
 * @difftest cases=2 */
void func_0202b1b4(void)
{
}

/* 0x0202b1b8
 * @difftest cases=2 */
void func_0202b1b8(void)
{
}

/* 0x0202b1bc
 * @difftest cases=2 */
void func_0202b1bc(void)
{
}

/* 0x0202b1c0
 * @difftest cases=2 */
void func_0202b1c0(void)
{
}

/* 0x0202b1c4: show the message for losing a battle (cycles through three
 * per game mode by the number of defeats)
 * @difftest @020e36b0:32=pick:0,1,2 @020da1ba:8=int:0:8 @020da1bc:8=int:0:8 @020da1bd:8=int:0:8 cases=60 */
void func_0202b1c4(void)
{
    u32 c, t;

    if (data_020e36b0 == 0) {
        c = data_020d9f9c[0x21e];
        if (c == 0)
            t = 0x16d;
        else if (c % 3 == 0)
            t = 0x226;
        else if (c % 3 == 1)
            t = 0x224;
        else
            t = 0x225;
    } else if (data_020e36b0 == 1) {
        c = data_020d9f9c[0x220];
        if (c == 0)
            t = 0x39e;
        else if (c % 3 == 0)
            t = 0x221;
        else if (c % 3 == 1)
            t = 0x222;
        else
            t = 0x223;
    } else {
        c = data_020d9f9c[0x221];
        if (c == 0)
            t = 0x22c;
        else if (c % 3 == 0)
            t = 0x229;
        else if (c % 3 == 1)
            t = 0x227;
        else
            t = 0x228;
    }
    func_02011664(t, 1);
}

/* 0x0202b41c: leave the boss battle: delete its actors and restore the
 * normal game state
 * @difftest stub=0x02042480 cases=3 */
void func_0202b41c(void)
{
    void **p;
    u8 *o;

    func_020186dc(0);
    o = func_020436f0(data_020bf6a0);
    o[0xf] &= ~1;
    func_02042480(1);
    FOR_EACH(p, g_GameRoot) {
        METHOD0(*p, 0x48);
        if (*p != NULL)
            METHOD0(*p, 4);   /* deleting destructor */
    }
    g_GameRoot.count = 0;
    data_020e36e0.count = 0;
    data_020e36f8.count = 0;
    func_02072ff0(func_02011f34());
    METHOD0(func_020062b0(), 0xc);
    METHOD0(func_020062a0(), 0xc);
    METHOD0((u8 *)func_02019670() + 4, 0x10);
    func_02029870();
    func_020043c8(func_02006200());
    func_02004490(data_020bdefc);
    func_020043c8(func_020061e0());
    func_02004490(data_020bdf24);
    func_020814c8(func_020062e0());
    func_02082e10(func_02018208(), 0, 0);
    func_02082f64(func_02018208());
    func_02013b78(data_020def18, 0);
    data_020e36b8 = NULL;
    data_020e36b4 = NULL;
    g_GameRootValid = 0;
    data_020df10c = 0;
    func_02015edc();
    U32_AT(func_020062e0(), 0x4b0) = 0;
}

/* 0x0202b5f0
 * @difftest cases=2 */
void func_0202b5f0(void)
{
}

/* 0x0202b5f4: set up the boss battle (display, camera, actors, HUD).
 * Not difftested: the camera-target and camera-effect constructors load
 * files from the card, which the emulator cannot do; reviewed against the
 * disassembly only. */
void func_0202b5f4(void)
{
    u16 colour;
    u8 *p, *n, *h;
    void **it;

    data_020df10c = 1;
    data_020e36a0 = 0;
    func_0200afb0(4, 2);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020b5678.v);
    func_0200b168(1, data_020b5674.v);
    func_0200b098(1);
    func_0200af18(0);
    func_0200af18(1);
    func_0200af18(2);
    func_0200af18(3);
    /* the original sets the fields of an uninitialised GXRgb one by one:
     * r = 0x1e, g = 0, b = 0x1e, bit 15 clear */
    colour = 0x781e;
    func_020813b0(func_020062e0(), &colour);
    func_02080f60(func_020062e0(), 3, 1);
    func_02080f60(func_020062e0(), 2, 1);
    func_02080f60(func_020062e0(), 0, 0);
    U32_AT(func_020062e0(), 0x4b0) = 1;
    p = (u8 *)func_020062a0() + 4;
    VCALL(p, 0xc, void (*)(void *, u32))(p, 10);

    p = func_0207fe74(0x64);
    if (p != NULL)
        p = func_02043ae0(p);
    data_020e36b8 = p;
    n = PTR_AT(p, 0x44);
    U32_AT(n, 0x1c) = 0x133;
    U32_AT(n, 0x20) = 0x133;
    U32_AT(n, 0x24) = 0x133;
    n[0x8c] = 1;
    FOR_EACH(it, g_GameRoot) {
        if (*it == NULL)
            return;
        METHOD0(*it, 0x44);
    }

    p = func_0207fe74(0x1ac);
    if (p != NULL)
        p = func_02064348(p);
    data_020e36b4 = p;
    func_02064104(data_020e36b4, func_02016f24(data_020deebc));
    U32_AT(data_020e36b4, 0x180) = 0x3000;
    func_02063ca8(data_020e36b4);
    func_020298b0();
    if (func_0202a724() != 0)
        func_020798d8(g_GameRoot.items[3], g_GameRoot.items[1], NULL, data_020e36b4);
    else
        func_020798d8(g_GameRoot.items[3], g_GameRoot.items[1], g_GameRoot.items[2], NULL);
    PTR_AT(PTR_AT(PTR_AT(g_GameRoot.items[1], 0x44), 0xac), 4) = g_GameRoot.items[2];
    PTR_AT(PTR_AT(PTR_AT(g_GameRoot.items[2], 0x44), 0xac), 4) = g_GameRoot.items[1];

    func_020731c0(func_02011f34());
    h = func_02011f34();
    if (func_02011bc8() == 0) {
        /* inlined setters: each also clears a field when the flag is 0,
         * which it never is right after being set */
        h[0xe0] = 1;
        h[0x20c] = 1;
        h[0x354] = 1;
        h[0x23] = 1;
    }
    g_GameRootValid = 1;
    data_020e36a8 = 0;
    func_02017920(data_020def18);
    func_02013b78(data_020def18, (u32)func_02005d78());
    func_02011bd8(PTR_AT(data_020def18, 4));
    func_02015ffc();
}

/* 0x0202b964
 * @difftest cases=2 */
void func_0202b964(void)
{
}
