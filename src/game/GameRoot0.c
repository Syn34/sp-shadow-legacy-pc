/*
 * Helpers around the game's root actor list (the part before game_root.c):
 * a pickup burst effect, the per-player colours, boss-battle bookkeeping,
 * a player's defeat and guarded calls into the main game object.
 * ARM9 main, 0x020298d8 - 0x0202a5d0 (24 functions).
 *
 * g_GameRoot is a vector {items, count, capacity} of actors: [0] the
 * level / player object (+0x8dc current character, +0x8e4 flag), [1] the
 * hero, [2] the main game object (C++ class, methods called through its
 * vtable; +0x300 mode, +0x308 value, +0x31c clamped value), [3] the HUD
 * counter. g_GameRootValid says whether it is set up; data_020e36b0 is
 * the game mode (0 story, 1 ..., other) and data_020e36b4 the camera
 * effect object.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: actor base */
extern u8 data_020bdf70[];           /* RTTI: C3dBaseActor */
extern const u32 data_020bde94[], data_020bde9c[], data_020bdea0[], data_020bdea8[],
    data_020bdeb0[], data_020bdeb4[], data_020bdeb8[], data_020bdebc[],
    data_020bdec0[], data_020bdec8[];
extern const char data_020bdee8[];   /* "Boss: Boss2Hero %d" */
extern s8 g_GameRootValid;
extern PtrVec g_GameRoot;
extern s32 data_020e36ac;
extern s32 data_020e36b0;
extern void *data_020e36b4;
extern u8 data_020d9f9c[];

void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */
void *func_0207fe74(u32 size);                              /* operator new */
void *func_02051d78(void *e);
void *func_02051c38(void *e, u32 v);
void func_020573c4(void *e, u32 v);
void func_02005b98(s32 *out, const void *src);
void func_0201751c(s32 *out, const u8 *obj);
void func_02058900(void *e, u32 v, s32 *pos, s32 *rot);
void *func_02055eb0(void *e, u32 v);
void func_02055ad8(void *e, u32 v, u32 w);
void func_020557ec(void *e, void *emitter);
void func_020551b4(void *e);
void func_020550ac(void *e, u32 v);
void func_02077e44(void *obj, s32 *a, s32 *b);
s32 func_020145ac(s32 a, s32 b, const char *fmt, ...);
s8 func_0207768c(void *obj);
u8 *func_020436f0(u32 id);
s32 func_02014094(u8 *self);
s32 func_02043708(void);
void func_0201b5a4(u32 a, u32 b);
void func_02019b08(u32 state, u32 screen);
void func_02019660(u32 v);
void func_02019640(u32 v);
void func_02038100(const u32 *colours, void *obj);
PtrVec *func_02078af0(void *obj);
void func_02078960(void *obj);
void func_02078f70(void *obj, s32 v);
void func_020633ec(void *fx);
void *func_0206341c(void *fx, u32 a, u32 b, u32 c);
void func_02063598(void *fx, s32 *pos, s32 v);
void func_020780e4(void *obj, s32 *pos);
s32 *func_020785b8(void *obj, s32 *pos);

void func_0202a148(s32 *out, u8 *obj);
s32 func_0202a298(void);

#define ROOT(i) ((u8 *)g_GameRoot.items[i])

/* one effect of the burst: kind `k`, emitter `em`, at the actor's place */
static void *NewEffect(void *obj, u32 k, u32 em, s32 *pos, s32 *rot)
{
    void *e = func_0207fe74(0x3c88);

    if (e != NULL)
        e = func_02051d78(e);
    if (e == NULL)
        return NULL;
    func_02051c38(e, k);
    func_020573c4(e, em);
    func_02005b98(pos, obj);
    func_0201751c(rot, obj);
    func_02058900(e, 0, pos, rot);
    return e;
}

/* a group of effects */
static void *NewGroup(u32 n)
{
    void *g = func_0207fe74(0xf0);

    if (g != NULL)
        g = func_02055eb0(g, n);
    return g;
}

/* 0x020298d8: spawn the pickup burst at actor `obj`
 * @difftest ptr:0x100:4 cases=10 */
void func_020298d8(void *obj)
{
    s32 pos0[3], rot0[3], pos1[3], rot1[3], pos2[3], rot2[3], pos3[3], rot3[3];
    void *e, *g;

    e = NewEffect(obj, 0x7ab, 0x481, pos0, rot0);
    if (e != NULL) {
        g = NewGroup(3);
        if (g != NULL) {
            func_02055ad8(g, 0, 1);
            func_020557ec(g, e);
            e = NewEffect(obj, 0x7aa, 0x4c7, pos1, rot1);
            if (e != NULL) {
                func_020557ec(g, e);
                e = NewEffect(obj, 0x7a9, 0x484, pos2, rot2);
                if (e != NULL)
                    func_020557ec(g, e);
            }
            func_020551b4(g);
        }
    }
    e = NewEffect(obj, 0x710, 0x4df, pos3, rot3);
    if (e == NULL)
        return;
    g = NewGroup(1);
    if (g == NULL)
        return;
    func_02055ad8(g, 0xc2, 1);
    func_020557ec(g, e);
    func_020550ac(g, 0x191);
    func_020550ac(g, 0x192);
    func_020551b4(g);
}

/* 0x02029b78: point the hero's camera at the main game object
 * @difftest $R=ptr:0x10:4 @020e36c8:32=$R cases=20 */
void func_02029b78(void)
{
    void *o = ROOT(2);

    PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = o;
}

/* 0x02029ba8: boss battle: let every actor add to the boss/hero counts
 * @difftest cases=5 */
void func_02029ba8(void)
{
    s32 a, b;
    void **p;

    if (g_GameRootValid == 0)
        return;
    a = 0;
    b = 2;
    func_02077e44(ROOT(0), &a, &b);
    for (p = g_GameRoot.items; p != g_GameRoot.items + g_GameRoot.count; p++) {
        if (*p != g_GameRoot.items[0])
            VCALL(*p, 0x70, void (*)(void *, s32 *, s32 *))(*p, &a, &b);
    }
    func_020145ac(a, b, data_020bdee8, func_0202a298());
}

/* 0x02029c80: set +0x230 of every actor
 * @difftest s8 cases=10
 * @difftest $R=ptr:0x10:4 $A=ptr:0x240:4 $B=ptr:0x240:4 @020e36c8:32=$R @020e36cc:32=int:0:3 @$R+0:32=$A @$R+4:32=$B s8 cases=20 */
void func_02029c80(s8 v)
{
    void **p;

    for (p = g_GameRoot.items; p != g_GameRoot.items + g_GameRoot.count; p++)
        ((u8 *)*p)[0x230] = v;
}

/* 0x02029cc4: a player was defeated
 * @difftest $T=zero:16 @$T+4:32=0x020bdf70 $W=ptr:0x120:4 @$W+0:32=$T+8 $C=ptr:0xb0:4 @$W+0x44:32=$C $D=ptr:0x10:4 @$C+0xac:32=$D $L=ptr:0x900:4 $G=ptr:0x60:4 $GC=ptr:0xb0:4 @$G+0x44:32=$GC $GD=ptr:0x10:4 @$GC+0xac:32=$GD @$GD+0xc:32=pick:0,1 $R=ptr:0x10:4 @020e36c8:32=$R @020e36cc:32=3 @$R+0:32=$L @$R+8:32=$G @020e36b0:32=pick:0,1,2 $W cases=60 */
void func_02029cc4(void *who)
{
    u8 *a = who;
    u8 *lvl, *c;
    s32 n;

    if (who != NULL)
        a = func_020a6efc(who, data_020bcb88, data_020bdf70, -1);
    lvl = ROOT(0);
    if (data_020e36b0 == 0)
        data_020d9f9c[0x21e]++;
    else if (data_020e36b0 == 1)
        data_020d9f9c[0x220]++;
    else
        data_020d9f9c[0x221]++;

    if (func_0207768c(ROOT(2)) != 0) {
        if ((s16)U16_AT(func_020436f0(data_020bf6a0), 4) <= 0) {
            n = func_02014094((u8 *)data_020deebc);
            if (n > func_02043708())
                n = func_02043708();
            else if (n < 0)
                n = 0;
            U16_AT(func_020436f0(data_020bf6a0), 4) = n;
        }
        c = PTR_AT(a, 0x44);
        U32_AT(c, 4) = 0;
        U32_AT(c, 8) = 0;
        U32_AT(c, 0xc) = 0x1e000;
        c[0x8c] = 1;
        U32_AT(a, 0x6c) = 0;
        U32_AT(a, 0x70) = 0;
        U32_AT(a, 0x74) = 0x1e000;
        a[0xf4] = 1;
        c = PTR_AT(a, 0x44);
        S32_AT(c, 0x10) = g_FxZero;
        S32_AT(c, 0x14) = g_FxPi;
        S32_AT(c, 0x18) = g_FxZero;
        c[0x8c] = 1;
        S32_AT(a, 0x78) = g_FxZero;
        S32_AT(a, 0x7c) = g_FxPi;
        S32_AT(a, 0x80) = g_FxZero;
        a[0xf4] = 1;
        U32_AT(a, 0xf8) = 0;
        U32_AT(a, 0xfc) = 0;
        U32_AT(a, 0x100) = 0x1000;
        U32_AT(a, 0x104) = 0;
        U32_AT(a, 0x108) = 0;
        U32_AT(a, 0x38) |= 0x800000;
        ((u8 *)data_020deebc)[0x1f] = 0;
        return;
    }
    func_0201b5a4(0x3f, 1);
    func_02019b08(0x1e, data_020df0fc);
    PTR_AT(PTR_AT(PTR_AT(who, 0x44), 0xac), 4) = who;
    if (data_020e36b0 == 0) {
        if ((s8)lvl[0x8e4] != 0) {
            data_020d9f9c[0x78] = 1;
            func_02019660(0x24);
        } else {
            func_02019660(0x22);
        }
        func_02019640(1);
        return;
    }
    if (data_020e36b0 == 1) {
        if ((s8)lvl[0x8e4] != 0)
            data_020d9f9c[0x8c] = 1;
        func_02019660(5);
        func_02019640(1);
        return;
    }
    func_02019660(0x1f);
    func_02019640(1);
}

/* 0x02029f3c: give every other active actor its player colours
 * @difftest $R=ptr:0x20:4 $A=ptr:0x40:4 $B=ptr:0x40:4 $C=ptr:0x40:4 $D=ptr:0x40:4 $E=ptr:0x40:4 $F=ptr:0x40:4 @020e36c8:32=$R @020e36cc:32=6 @$R+0:32=$A @$R+4:32=$B @$R+8:32=$C @$R+0xc:32=$D @$R+0x10:32=$E @$R+0x14:32=$F @020deed4:32=0 cases=40
 * @difftest $L=zero:0x900 $V=ptr:0x10:4 $A=ptr:0x40:4 $B=ptr:0x40:4 @$V+0:32=$A @$V+4:32=$B @$L+0x5e8:32=$V @$L+0x5ec:32=int:0:3 $R=ptr:0x10:4 @020e36c8:32=$R @$R+0:32=$L @020deed4:32=0x80 cases=40 */
void func_02029f3c(void)
{
    u32 col[2];
    s32 n = 0;
    void **p;

    col[0] = 0;
    col[1] = 0;
    if (data_020deebc[6] != 0x80) {
        for (p = g_GameRoot.items; p != g_GameRoot.items + g_GameRoot.count; p++) {
            u8 *o = *p;
            if (o == ROOT(0) || o == ROOT(1) || o == ROOT(3))
                continue;
            if (!((U32_AT(o, 0x38) >> 2) & 1))
                continue;
            if (ROOT(2) == o) {
                col[0] = data_020bdea8[0];
                col[1] = data_020bdeb4[0];
            } else if (++n == 1) {
                col[1] = data_020bde9c[0];
                col[0] = data_020bdebc[0];
            } else {
                col[0] = data_020bdeb0[0];
                col[1] = data_020bdec8[0];
            }
            func_02038100(col, *p);
        }
        return;
    }
    {
        PtrVec *v = func_02078af0(ROOT(0));
        for (p = v->items; p != v->items + v->count; p++) {
            if (((U32_AT(*p, 0x38) >> 20) & 1) == 1) {
                col[0] = data_020bde94[0];
                col[1] = data_020bdec0[0];
                func_02038100(col, *p);
            }
        }
    }
}

/* 0x0202a104: out = a - b (3 x fx32)
 * @difftest ptr:12:4 ptr:12:4 ptr:12:4 */
void func_0202a104(s32 *out, const s32 *a, const s32 *b)
{
    s32 x = a[0] - b[0];
    s32 y = a[1] - b[1];
    s32 z = a[2] - b[2];

    out[0] = x;
    out[1] = y;
    out[2] = z;
}

/* 0x0202a148: an actor's position (from +0x68)
 * @difftest ptr:12:4 ptr:0x80:4 */
void func_0202a148(s32 *out, u8 *obj)
{
    func_02005b98(out, obj + 0x68);
}

/* 0x0202a158: the current character
 * @difftest $R=ptr:0x10:4 $L=ptr:0x900:4 @020e36c8:32=$R @$R+0:32=$L cases=20 */
s32 func_0202a158(void)
{
    return (s8)ROOT(0)[0x8dc];
}

/* 0x0202a174: switch character
 * @difftest $L=zero:0x900 $G=ptr:0x10:4 $V=zero:0x100 @$G+0:32=$V @$V+0x8c:32=0x02070a18 $R=ptr:0x10:4 @020e36c8:32=$R @$R+0:32=$L @$R+8:32=$G cases=20 */
void func_0202a174(void)
{
    func_02078960(ROOT(0));
    VCALL(ROOT(2), 0x8c, void (*)(void *, s32))(ROOT(2), (s8)ROOT(0)[0x8dc]);
}

/* 0x0202a1c0
 * @difftest ptr:0x330:4 u8 cases=10 */
void func_0202a1c0(u8 *obj, u8 v)
{
    obj[0x329] = v;
}

/* 0x0202a1c8: set the HUD counter (rounded integer -> fx32 -> int)
 * @difftest s32 cases=10 */
void func_0202a1c8(s32 v)
{
    float f;

    if (g_GameRootValid == 0)
        return;
    if (v > 0)
        f = 0.5f + (float)(v << 12);
    else
        f = (float)(v << 12) - 0.5f;
    func_02078f70(ROOT(3), (s32)f);
}

/* 0x0202a244
 * @difftest u32 cases=5
 * @difftest $G=ptr:0x10:4 $V=zero:0x100 @$G+0:32=$V @$V+0x80:32=0x02070a18 $R=ptr:0x10:4 @020e36a4:8=1 @020e36c8:32=$R @$R+8:32=$G u32 cases=20 */
void func_0202a244(u32 v)
{
    if (g_GameRootValid == 0)
        return;
    VCALL(ROOT(2), 0x80, void (*)(void *, u32))(ROOT(2), v);
}

/* 0x0202a294
 * @difftest cases=2 */
void func_0202a294(void)
{
}

/* 0x0202a298
 * @difftest @020e36ac:32=s32 cases=20 */
s32 func_0202a298(void)
{
    return data_020e36ac >> 12;
}

/* 0x0202a2ac
 * @difftest u32 cases=5
 * @difftest $G=ptr:0x10:4 $V=zero:0x100 @$G+0:32=$V @$V+0x7c:32=0x02070a18 $R=ptr:0x10:4 @020e36a4:8=1 @020e36c8:32=$R @$R+8:32=$G u32 cases=20 */
u32 func_0202a2ac(u32 v)
{
    if (g_GameRootValid == 0)
        return 0;
    return VCALL(ROOT(2), 0x7c, u32 (*)(void *, u32))(ROOT(2), v);
}

/* 0x0202a300
 * @difftest u32 u32 cases=5
 * @difftest $G=ptr:0x10:4 $V=zero:0x100 @$G+0:32=$V @$V+0x74:32=0x02070a18 $R=ptr:0x10:4 @020e36a4:8=1 @020e36c8:32=$R @$R+8:32=$G $H=ptr:0x10:4 @$R+4:32=pick:0,1 u32 u32 cases=20 */
void func_0202a300(u32 unused, u32 v)
{
    /* the original reads the hero before checking g_GameRootValid */
    void *hero = *(void *volatile *)&g_GameRoot.items[1];

    (void)unused;
    if (g_GameRootValid == 0)
        return;
    if (hero == NULL)
        return;
    VCALL(ROOT(2), 0x74, void (*)(void *, void *, u32))(ROOT(2), hero, v);
}

/* 0x0202a364
 * @difftest cases=5
 * @difftest $R=ptr:0x10:4 $G=ptr:0x320:4 @020e36a4:8=1 @020e36c8:32=$R @$R+8:32=$G cases=20 */
s32 func_0202a364(void)
{
    if (g_GameRootValid == 0)
        return 0;
    return S32_AT(ROOT(2), 0x308);
}

/* 0x0202a390: clamp the main object's value for modes 0xf and 0x11
 * @difftest cases=5
 * @difftest $R=ptr:0x10:4 $G=ptr:0x320:4 @020e36a4:8=1 @020e36c8:32=$R @$R+8:32=$G @$G+0x300:32=pick:0xf,0x11,3 @$G+0x308:32=int:-5:20 cases=60 */
void func_0202a390(void)
{
    u8 *g;
    s32 v;

    if (g_GameRootValid == 0)
        return;
    g = ROOT(2);
    v = S32_AT(g, 0x308);
    if (S32_AT(g, 0x300) == 0xf) {
        S32_AT(g, 0x31c) = (s32)data_020bdea0[0] >= v ? v : (s32)data_020bdea0[0];
    } else if (S32_AT(g, 0x300) == 0x11) {
        v += 2;
        S32_AT(g, 0x31c) = (s32)data_020bdeb8[0] >= v ? v : (s32)data_020bdeb8[0];
    }
}

/* 0x0202a414
 * @difftest @020e36b4:32=0 cases=5 */
void func_0202a414(void)
{
    if (data_020e36b4 == NULL)
        return;
    func_020633ec(data_020e36b4);
}

/* 0x0202a448
 * @difftest $N=zero:0x200 $W=ptr:0x60:4 @$W+0x44:32=$N $F=zero:0x200 @020e36b4:32=$F $W u32 u32 cases=20 */
void *func_0202a448(u32 a, u32 b, u32 c)
{
    return func_0206341c(data_020e36b4, a, b, c);
}

/* 0x0202a478
 * @difftest ptr:12:4 s32 cases=5 */
void func_0202a478(s32 *pos, s32 v)
{
    if (g_GameRootValid == 0)
        return;
    func_02063598(data_020e36b4, pos, v);
}

/* 0x0202a4c0: camera effect above the hero
 * @difftest s32 cases=5 */
void func_0202a4c0(s32 v)
{
    s32 pos[4];

    if (g_GameRootValid == 0)
        return;
    func_0202a148(pos, g_GameRoot.items[1]);
    func_020780e4(ROOT(0), pos);
    pos[1] += 0xa000;
    func_02063598(data_020e36b4, pos, v);
}

/* 0x0202a544
 * @difftest cases=5 */
void func_0202a544(void)
{
    s32 pos[3], q[3];
    s32 *r;

    if (g_GameRootValid == 0)
        return;
    func_0202a148(pos, g_GameRoot.items[1]);
    r = func_020785b8(ROOT(0), pos);
    q[0] = r[0];
    q[1] = r[1];
    q[2] = r[2];
    func_02063598(data_020e36b4, q, -1);
}
