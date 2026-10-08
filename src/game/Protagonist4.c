/*
 * CProtagonist, part 4: setting up the hero actor ("heroInitCharacter"),
 * camera mode, a random shuffle used by a minigame, the hero's speech
 * bubble, facing from the camera or the D-pad, walking speed and turning
 * the camera.
 * ARM9 main, 0x02016340 - 0x02016f30 (10 functions).
 *
 * The CProtagonist (data_020deebc, see Protagonist3.c) also has:
 *   +0x00 u16, +0x10 target camera angle, +0x14 facing angle, +0x18 camera
 *   mode, +0x1d/+0x1e shuffle position/direction, +0x50 shuffle table.
 * func_02016f24 returns its camera object: +0x10/+0x14/+0x18 angles,
 * +0x8c dirty, +0xac view.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const char data_020bd0e8[];   /* "returned from: SetPosition" */
extern const char data_020bd104[];   /* "returned from: heroInitCharacter" */
extern const fx32 data_020b6cc8;     /* half the width of a facing sector */
extern u8 *data_020c3b0c;            /* object kinds, 0x48 bytes each */
extern u8 data_020d9f9c[];           /* script variables */
extern u8 data_020daac0[];           /* dialogue state */
extern u8 data_020def74[];
extern u32 data_020f6390;

fx32 func_02001ef4(u32 dir);
void *func_020091e8(u32 a, u32 b);
s32 func_02011f14(void);
void *func_02011f44(void);
void func_020141c8(u8 *self);
void func_02014214(u8 *self);
void func_020145bc(u8 *self, u8 *other);
void func_020157bc(void);
void func_020157e0(u8 *a);
void func_02015ffc(void);
void func_02016f30(void *self);
void func_02017028(void *self);
void func_0201756c(void *self);
void func_020176c8(void *self);
void func_0201784c(void *self);
void func_02017abc(void *list, void *item);
s32 func_0201a0a0(s32 *xz, void *obj);
s32 func_0204a80c(u32 counter);
s32 func_0204a868(u32 counter);
void *func_020507f4(void *e, s32 *pos, s32 *rect);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
s32 func_0205a3ec(void *obj, u32 id);
u32 func_02081c84(void *pad);
void *func_0207fe74(u32 size);       /* operator new */
s32 func_020a4e00(s32 num, s32 den);
int rand(void);

/* this file's own functions, called before their definition */
void func_02016984(u8 *self, fx32 v);

#define P ((u8 *)data_020deebc)
#define HERO(self) ((u8 *)PTR_AT(self, 4))

static inline void VCall(void *obj, u32 off, u32 v)
{
    VCALL(obj, off, void (*)(void *, u32))(obj, v);
}

static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

static inline s32 Wrap(s32 a, s32 lo, s32 hi, s32 turn)
{
    while (a < lo)
        a += turn;
    while (a > hi)
        a -= turn;
    return a;
}

/* 0x02016340: set up the hero actor at `pos` */
void func_02016340(s32 *pos, u8 *a)
{
    u8 *kind, *o, *e;
    s32 xz[2], rect[4], r, d;
    u32 h;
    float f;

    U32_AT(a, 8) = 1;
    U32_AT(a, 0xc) = 0;
    U32_AT(a, 0x110) |= 0x100;
    U32_AT(a, 0x38) |= 0x40000;
    U32_AT(a, 0x110) |= 0x80;
    VCall(a, 0xc, 0);
    U32_AT(a, 0x110) &= ~8u;
    U32_AT(a, 4) |= 8;
    U32_AT(a, 0x38) |= 0x20;
    U32_AT(a, 0x38) |= 0x40;
    U32_AT(a, 0x38) |= 4;
    U32_AT(a, 0x38) |= 0x800000;
    PTR_AT(a, 0x94) = (void *)func_020157e0;
    PTR_AT(a, 0x98) = (void *)func_020157bc;
    PTR_AT(a, 0x9c) = (void *)func_020145bc;
    U32_AT(a, 0x50) = 0x3000;
    func_02003c74(a, pos[0], pos[1]);
    S32_AT(a, 0x30) = S32_AT(a, 0x28);
    S32_AT(a, 0x34) = S32_AT(a, 0x2c);
    func_02004490(data_020bd0e8);

    U16_AT(P, 0) = 0;
    xz[0] = S32_AT(a, 0x54);
    xz[1] = S32_AT(a, 0x58);
    h = *(volatile u16 *)P;   /* re-read: the original does the float rounding at run time */
    f = (float)SHL(h, 16);
    f = h != 0 ? 0.5f + f : f - 0.5f;
    S32_AT(a, 0x24) = func_0201a0a0(xz, a) + (s32)f;
    S32_AT(a, 0x6c) = S32_AT(a, 0x24);
    S32_AT(a, 0x64) = S32_AT(a, 0x24);
    func_02017028(P);
    a[0x10d] = ((u8 *)func_020091e8(func_020436f0(data_020bf6a0)[0x10], 0))[6];
    func_02053648(func_02016f24(data_020deebc), U32_AT(a, 8), PTR_AT(P, 8), 1, 0);
    func_02004490(data_020bd104);
    func_02016984(P, func_02001ef4(a[0x10d]));
    S32_AT(P, 0x10) = S32_AT(P, 0x14);
    func_02016f30(P);

    kind = data_020c3b0c + U32_AT(a, 8) * 0x48;
    o = PTR_AT(a, 0x44);
    S32_AT(o, 0x1c) = S32_AT(kind, 0x2c);
    S32_AT(o, 0x20) = S32_AT(kind, 0x2c);
    S32_AT(o, 0x24) = S32_AT(kind, 0x2c);
    o[0x8c] = 1;
    o = PTR_AT(a, 0x44);
    kind = data_020c3b0c + U32_AT(a, 8) * 0x48;
    U32_AT(o, 0x18c) = (U32_AT(o, 0x18c) & ~1u) | (U32_AT(kind, 0x30) & 1);
    kind = data_020c3b0c + U32_AT(a, 8) * 0x48;
    S32_AT(PTR_AT(a, 0x44), 0xa4) = S32_AT(kind, 0x34);
    U32_AT(PTR_AT(a, 0x44), 0x18c) |= 2;
    PTR_AT(PTR_AT(PTR_AT(a, 0x44), 0xac), 4) = a;
    S32_AT(a, 0x74) = 0;
    S32_AT(a, 0x70) = S32_AT(a, 0x74);
    S32_AT(a, 0x78) = 0;

    kind = data_020c3b0c + U32_AT(a, 8) * 0x48;
    r = S32_AT(kind, 0x38);
    d = S32_AT(kind, 0x3c);
    rect[0] = -d;
    rect[1] = -r;
    rect[2] = d;
    rect[3] = r;
    S32_AT(a, 0x10) = rect[0];
    S32_AT(a, 0x14) = rect[1];
    S32_AT(a, 0x18) = rect[2];
    S32_AT(a, 0x1c) = rect[3];

    func_0201784c(P);
    if (func_0205a3ec(data_020def74, 0xf) > -1)
        func_02014214(P);
    func_02015ffc();
    if (func_0205a3ec(data_020def74, 0x10) > -1)
        func_020141c8(P);
    VCallR1(a, 0x38);
    data_020f6390 = 0;
    func_020176c8(P);
    if (data_020d9f9c[0x182] != 0) {
        func_0201756c(P);
        U32_AT(P, 0x20) |= 0x20;
        data_020d9f9c[0x182] = 1;
    }
    e = func_0207fe74(0x4c);
    if (e != NULL)
        e = func_020507f4(e, (s32 *)(a + 0x28), rect);
    U32_AT(e, 0xc) = 0;
    PTR_AT(P, 0xc) = e;
    func_02017abc((u8 *)func_020062b0() + 4, e);
}

/* 0x02016750: set the camera mode
 * @difftest ptr:0x60:4 int:0:3 @020daa60:8=pick:0,1 */
void func_02016750(u8 *self, s32 mode)
{
    if (func_02011f14() == 1 && mode == 0)
        data_020daac0[0x40] = 1;
    S32_AT(self, 0x18) = mode;
}

/* 0x02016790: step the shuffle; 1 when it turned round
 * @difftest $T=ptr:0x60:4 @$T+0x1e:8=pick:0,1,0xff @$T+0x1d:8=int:0:8 @$T+0x51:8=int:0:8 @$T+0x52:8=int:0:8 @$T+0x53:8=int:0:8 $T @020deed4:32=int:0:4 @020ebcee:8=int:0:3 cases=150 */
s32 func_02016790(u8 *self)
{
    s32 ret = 0, q;
    u32 end, pos;

    if (func_0204a868(10) == 0)
        return 0;
    self[0x1d] += (s8)self[0x1e];
    if ((s8)self[0x1e] < 0 && self[0x1d] == 0)
        self[0x1e] = 1;
    if ((s8)self[0x1e] <= 0)
        return 0;
    end = self[0x50 + func_0204a80c(10)];
    pos = self[0x1d];
    q = func_020a4e00(100, end - pos);
    if (q <= 0 || (data_020deebc[0x18 / 4] & 3) == 0)
        return 0;
    if (pos == end) {
        self[0x1d] = 0;
        return 1;
    }
    if ((s32)((rand() & 0x7fffu) % 101) < q) {
        ret = 1;
        self[0x1d] = end - self[0x1d];
        self[0x1e] = 0xff;
    }
    return ret;
}

/* 0x020168a0: place the hero's speech bubble, or remove it when its time
 * is up */
void func_020168a0(u8 *self)
{
    u8 *h = HERO(self), *b;
    s32 v[2];
    u64 now;

    if (PTR_AT(h, 0xec) == NULL)
        return;
    now = func_020822c8(func_020062d0());
    if (now > Get64(h + 0xf0)) {
        U32_AT(PTR_AT(h, 0xec), 4) &= ~8u;
        VCall(PTR_AT(HERO(self), 0xec), 0xc, 1);
        return;
    }
    v[0] = 0;
    v[1] = 0;
    S32_AT(HERO(self), 0xf8) -= 0x666;
    func_02003890(HERO(self), v);
    v[1] = 0xc0 - v[1];
    b = PTR_AT(HERO(self), 0xec);
    func_02003b98(b, v[0] - (U16_AT(b, 0xb0) >> 1), v[1] + (S32_AT(HERO(self), 0xf8) >> 12));
}

/* 0x02016984: set the facing angle
 * @difftest ptr:0x20:4 u32 */
void func_02016984(u8 *self, fx32 v) { S32_AT(self, 0x14) = v; }

/* 0x0201698c: the hero's facing (8 directions) from the camera angle
 * @difftest $K=ptr:0x20:4 @$K+0x14:32=int:-30000:30000 $C=ptr:0x48:4 @$C+0x44:32=$K $H=ptr:0x120:4 $T=ptr:0x60:4 @$T+4:32=$H @$T+8:32=$C $T */
void func_0201698c(u8 *self)
{
    s32 a = S32_AT(func_02016f24(self), 0x14);
    s32 w = data_020b6cc8;
    s32 t = g_Fx3Pi_4 + w, h = g_FxPi_2 + w, q = g_FxPi_4 + w;
    u32 dir;

    if (a >= t)
        dir = 0;
    else if (a >= h)
        dir = 1;
    else if (a < h && a >= q)
        dir = 2;
    else if (a < q && a >= w)
        dir = 3;
    else if (a < w && a >= g_FxZero)
        dir = 4;
    else if (a < g_FxZero && a >= -w)
        dir = 4;
    else if (a < -w && a >= -q)
        dir = 5;
    else if (a < -q && a >= -h)
        dir = 6;
    else if (a < -h && a >= -t)
        dir = 5;
    else
        dir = 0;
    HERO(self)[0x10d] = dir;
}

/* 0x02016b4c: the hero's facing from the D-pad
 * @difftest $H=ptr:0x120:4 $T=ptr:0x60:4 @$T+4:32=$H $T */
void func_02016b4c(u8 *self)
{
    switch (func_02081c84(func_02011f44())) {
    case 0x40:
        HERO(self)[0x10d] = 0;
        S32_AT(self, 0x14) = g_FxPi;
        break;
    case 0x50:
        HERO(self)[0x10d] = 1;
        S32_AT(self, 0x14) = g_Fx3Pi_4;
        break;
    case 0x10:
        HERO(self)[0x10d] = 2;
        S32_AT(self, 0x14) = g_FxPi_2;
        break;
    case 0x90:
        HERO(self)[0x10d] = 3;
        S32_AT(self, 0x14) = g_FxPi_4;
        break;
    case 0x80:
        HERO(self)[0x10d] = 4;
        S32_AT(self, 0x14) = g_FxZero;
        break;
    case 0xa0:
        HERO(self)[0x10d] = 5;
        S32_AT(self, 0x14) = -g_FxPi_4;
        break;
    case 0x20:
        HERO(self)[0x10d] = 6;
        S32_AT(self, 0x14) = -g_FxPi_2;
        break;
    case 0x60:
        HERO(self)[0x10d] = 7;
        S32_AT(self, 0x14) = -g_Fx3Pi_4;
        break;
    default:
        S32_AT(self, 0x14) = g_FxZero;
        break;
    }
}

/* 0x02016d04: walk at `speed` in the direction of the target camera angle
 * @difftest $H=ptr:0x120:4 $T=ptr:0x60:4 @$T+4:32=$H $T int:0:0x3000 @020deecc:32=int:-30000:30000 */
void func_02016d04(u8 *self, s32 speed)
{
    s32 v[2], out[2], sh[2];
    u32 idx;

    v[0] = 0;
    v[1] = 0;
    S32_AT(HERO(self), 0x50) = speed;
    idx = FxRadToIdx(S32_AT(data_020deebc, 0x10)) >> 4;
    v[1] = FX_SinCosTable_[idx * 2 + 1];
    v[0] = FX_SinCosTable_[idx * 2];
    func_02003798(out, v, &speed);
    v[0] = out[0];
    v[1] = out[1];
    func_02003b08(sh, v, 4);
    v[0] = sh[0];
    v[1] = sh[1];
    S32_AT(HERO(self), 0x70) = v[0];
    S32_AT(HERO(self), 0x74) = v[1];
}

/* 0x02016df8: turn the camera towards the target angle, at most `step`
 * @difftest $K=ptr:0x90:4 @$K+0x14:32=int:-60000:60000 $C=ptr:0x48:4 @$C+0x44:32=$K $T=ptr:0x60:4 @$T+8:32=$C @$T+0x10:32=int:-60000:60000 $T int:0:0x4000 */
void func_02016df8(u8 *self, s32 step)
{
    s32 target = S32_AT(self, 0x10), cur, d, base;
    u8 *c = func_02016f24(self);
    cur = S32_AT(c, 0x14);
    target = Wrap(target, g_FxZero, g_FxTwoPi, g_FxTwoPi);
    cur = Wrap(cur, g_FxZero, g_FxTwoPi, g_FxTwoPi);
    d = Wrap(target - cur, -g_FxPi, g_FxPi, g_FxTwoPi);
    if ((d < 0 ? -d : d) > step)
        d = step * (d < 0 ? -1 : 1);
    base = S32_AT(func_02016f24(self), 0x14);
    c = func_02016f24(self);
    {
        s32 z = S32_AT(c, 0x18), x = S32_AT(c, 0x10);
        S32_AT(c, 0x10) = x;
        S32_AT(c, 0x14) = d + base;
        S32_AT(c, 0x18) = z;
    }
    c[0x8c] = 1;
}

/* 0x02016f24: the camera object
 * @difftest $C=ptr:0x48:4 $O=ptr:0x10:4 @$O+8:32=$C $O */
void *func_02016f24(void *self) { return PTR_AT(PTR_AT(self, 8), 0x44); }
