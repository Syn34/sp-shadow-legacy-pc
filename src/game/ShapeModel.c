/*
 * The actors' shape objects (0x1a8 bytes, made by func_020504b8): a model
 * of the actor's kind with an animation driver, drawn with the lights
 * dimmed by its fade and with a blob shadow under it.
 * ARM9 main, 0x02052f10 - 0x02053a94 (19 functions).
 *
 * Layout: the object base (func_0205b8c8 / func_0205b784, vtable
 * data_020c0728), +0x90 a sub-object (func_0205b76c / func_0205b728, vtable
 * data_020c0740; this-adjusting entry points subtract 0x90), +0x94 the
 * shadow's alpha, +0x98 kind, +0xa0 the ground height, +0xa4 the shadow's
 * size, +0xa8 fade (0x333 - 0x1000), +0xac the animation driver (0xa4
 * bytes, func_02071864), +0xb0 the model (Model.c), +0x18c flags (bit 0
 * shadow, 1 shown, 2 loaded, 3 fading out, 4 fading in), +0x190 the draw
 * callback (vtables data_020c07ac / data_020bf760, members data_020c06e0
 * and data_020c06e8). The shadow texture (data_020f5370) is loaded with
 * the first shape object and freed with the last (data_020f535c counts).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020b6cc4;
extern const u32 data_020c06e0[2], data_020c06e8[2];   /* the member functions */
extern u8 data_020c0728[], data_020c0740[], data_020c07ac[], data_020bf760[];   /* vtables */
extern const char data_020c074c[], data_020c0774[], data_020c0790[];   /* messages, file name */
extern u16 data_020f535c;            /* shape objects */
extern u32 data_020f5360;            /* the shadow texture's VRAM key */
extern u8 data_020f5370[];           /* the shadow texture */

void *func_020062e0(void);
void func_02005b98(s32 *out, const void *src);
s32 func_0208f070(s32 num, s32 den);
void func_0208ea6c(void *mtx44);
void func_02080f60(void *settings, u32 a, u32 b);
void MI_Copy64B(const void *src, void *dest);
void func_02080e84(void *settings);
void func_02080bf4(void *settings, void *m);
void func_020862e8(u32 a, u32 b, u32 c);
void func_02089408(void *mdl, u32 a, u32 b);
void func_0202de98(s32 *m);
void func_02017550(s32 *out, const u8 *cam);
void func_0202de68(s32 *m, const s32 *v);
void func_020022e4(void *self);
void func_0201751c(s32 *out, const u8 *cam);
void func_0202dc88(s32 *m, const s32 *rot);
void func_02002974(void *self);
void func_0202dc6c(s32 *m, const s32 *v);
void func_02048a04(s32 *out, const u8 *g, s32 i);
u32 func_020489cc(s32 v, const s32 *lo, const s32 *hi);
void func_02048964(u16 *c, u32 b, u32 g, u32 r, u8 a);
void func_0204e948(void *model, u32 alpha);
void func_020814c8(void *settings);
void func_0204ebb4(void *model);
void *func_02019670(void);
void *func_02018208(void);
void func_02043cc8(u8 *list, void *item);
void func_0207163c(void *anim, u32 dt);
void func_0204ed80(void *model, u32 id, s32 frame);
void func_0204f094(void *model);
void func_02071800(void *anim);
void func_02071838(void *anim);
void func_0207fe44(void *p);
void func_02082e70(void *vram, u32 key);
void func_02080778(void *tex);
void func_0204f8d4(void *model, u32 kind, u32 a, u32 bones);
void *func_0207fe74(u32 size);
void *func_02071864(void *anim);
void func_02071804(void *anim, u32 kind, void *actor);
void func_02080aec(void *tex, const char *file);
u32 func_02082f00(void *vram, void *tex);
void func_0204f964(void *model);
void func_0205b728(void *self);
void func_0205b784(void *self);
void func_0205b8c8(void *self);
void func_0205b76c(void *self);
void func_0204fae8(void *model);

s32 func_020533f0(s32 a, s32 b);
void func_0205359c(u8 *self);
void func_02053064(u8 *self);
u8 *func_02053764(u8 *self);
u8 *func_020537cc(u8 *self);

/* the model set of the model at +0xb0 if it has one */
#define MDL(self) (BITS(U32_AT(self, 0xc4), 5, 1) == 0 ? NULL \
    : (u8 *)PTR_AT(self, 0x130) + U32_AT((u8 *)PTR_AT(self, 0x130) + U16_AT(PTR_AT(self, 0x130), 0xe), 0xc))

/* 0x02052f10: the height offset of kind +0x98
 * @difftest $S=zero:0x1a8 @$S+0x98:32=pick:0,0x15,0x18,1 $S cases=8 */
s32 func_02052f10(void *model)
{
    u32 k = U32_AT(model, 0x98);

    if (k == 0x15)
        return -0x32000;
    if (k == 0x18)
        return 0x5000;
    return 0;
}

/* 0x02052f34: draw the blob shadow (smaller the higher the ground)
 * @difftest $S=ptr:0x1a8:4 @$S+0x18c:32=pick:0,1 @$S+0x94:8=pick:0,0x10,0x1f @$S+0xa0:32=int:-0x10000:0x10000 @$S+0xa4:32=int:0:0x20000 $S stub=0x02080f60,0x02080e84,0x02080bf4 cases=20 */
void func_02052f34(u8 *self)
{
    s32 p[3], m[16], h, q, s;
    u32 a;
    u8 *g;

    if (BITS(U32_AT(self, 0x18c), 0, 1) == 0)
        return;
    if (self[0x94] == 0)
        return;
    func_02005b98(p, self);
    p[1] = p[1] - S32_AT(self, 0xa0);
    h = S32_AT(self, 0xa4);
    q = func_0208f070(S32_AT(self, 0xa0), 0x80000);
    func_0208ea6c(m);
    s = h - FxMul(h, q);
    if (s < 0x4000)
        s = 0x4000;
    m[0] = s;
    m[5] = s;
    m[10] = s;
    func_02080f60(func_020062e0(), 3, 1);
    func_02080f60(func_020062e0(), 4, 0);
    m[12] = p[0];
    m[13] = p[1];
    m[14] = p[2];
    a = self[0x94];
    if (a > 0x18)
        a = 0x18;
    U32_AT(func_020062e0(), 0x4a4) = a;
    g = func_020062e0();
    MI_Copy64B(m, g + 0x3d8);
    U32_AT(g, 0x2cc) |= 4;
    func_02080e84(func_020062e0());
    func_02080bf4(func_020062e0(), data_020f5370);
}

/* 0x02053064: draw the model and its shadow
 * @difftest $S=zero:0x1a8 @$S+0x18c:32=pick:0,2,3 @$S+0xc4:32=pick:0,0x20 $F=zero:0x40 @$F+0xe:16=0x10 @$F+0x1c:32=0x20 @$S+0x130:32=$F @$S+0x98:32=pick:0,1 @$S+0x8c:8=pick:0,1 @$S+0xa8:32=int:0x333:0x1000 $S stub=0x02052f34,0x020862e8,0x02089408,0x02080f60,0x02080e84,0x0204e948,0x020814c8 cases=20 */
void func_02053064(u8 *self)
{
    u8 *g;
    s32 i, lo, hi;

    if (BITS(U32_AT(self, 0x18c), 1, 1) == 0)
        return;
    func_02052f34(self);
    if (MDL(self) == NULL)
        return;
    if (U32_AT(self, 0x98) == 1) {
        func_020862e8(0x3dad, 0x24e7, 0);
        func_02089408(MDL(self), 0, 0x80);
        func_02089408(MDL(self), 0, 0x40);
    } else {
        func_02089408(MDL(self), 1, 0x40);
        func_02089408(MDL(self), 1, 0x80);
    }
    func_02080f60(func_020062e0(), 0, 0);
    if (S8_AT(self, 0x8c) == 1) {
        s32 t[3], r[3], s[3];

        func_0202de98((s32 *)(self + 0x28));
        func_02017550(t, self);
        func_0202de68((s32 *)(self + 0x28), t);
        func_020022e4(t);
        func_0201751c(r, self);
        func_0202dc88((s32 *)(self + 0x28), r);
        func_02002974(r);
        func_02005b98(s, self);
        func_0202dc6c((s32 *)(self + 0x28), s);
        func_020022e4(s);
    }
    g = func_020062e0();
    MI_Copy64B(self + 0x28, g + 0x3d8);
    U32_AT(g, 0x2cc) |= 4;
    lo = 0;
    hi = 0x1f;
    for (i = 0; i < 4; i++) {
        s32 c[3], f, lim0, lim1;
        u32 r, gr, b;
        u16 col, out;

        func_02048a04(c, func_020062e0(), i);
        f = S32_AT(self, 0xa8);
        col = U16_AT((u8 *)func_020062e0() + i * 0x14, 0x320);
        r = func_020533f0(f, (col & 0x1f) << 12) >> 12;
        gr = func_020533f0(f, ((col >> 5) & 0x1f) << 12) >> 12;
        b = func_020533f0(f, ((col >> 10) & 0x1f) << 12) >> 12;
        lim0 = lo;
        lim1 = hi;
        r = func_020489cc(r, &lim0, &lim1) & 0xff;
        gr = func_020489cc(gr, &lim0, &lim1) & 0xff;
        b = func_020489cc(b, &lim0, &lim1) & 0xff;
        out = 0;
        func_02048964(&out, b, gr, r, (col >> 15) & 1);
        g = (u8 *)func_020062e0() + i * 0x14;
        U16_AT(g, 0x320) = out;
        S32_AT(g, 0x324) = c[0] >= 0x1000 ? 0xfff : c[0];
        S32_AT(g, 0x328) = c[1] >= 0x1000 ? 0xfff : c[1];
        S32_AT(g, 0x32c) = c[2] >= 0x1000 ? 0xfff : c[2];
    }
    func_02080e84(func_020062e0());
    func_0204e948(self + 0xb0, self[0x94]);
    func_020814c8(func_020062e0());
}

/* 0x020533f0: fx32 multiply
 * @difftest s32 s32 cases=40 */
s32 func_020533f0(s32 a, s32 b)
{
    return FxMul(a, b);
}

/* 0x0205340c
 * @difftest $S=zero:0x1a8 @$S+0x18c:32=pick:0,2 $S stub=0x0204ebb4 cases=4 */
void func_0205340c(u8 *self)
{
    if (BITS(U32_AT(self, 0x18c), 1, 1) == 0)
        return;
    func_0204ebb4(self + 0xb0);
}

/* 0x02053440: queue the draw callback
 * @difftest zero:0x1a8 stub=0x02043cc8 cases=3 */
void func_02053440(u8 *self)
{
    func_02043cc8((u8 *)func_02019670() + 4, self + 0x190);
}

/* 0x02053460: animate by `dt` ticks and fade in or out
 * @difftest $H=zero:0xa4 $S=zero:0x1a8 @$S+0xac:32=$H @$S+0x18c:32=pick:0,8,0x10,0x18 @$S+0xa8:32=int:0x300:0x1100 $S int:0:0x400 stub=0x0207163c,0x0204ed80 cases=40 */
void func_02053460(u8 *self, u32 dt)
{
    u8 *h;
    u32 f;

    func_0207163c(PTR_AT(self, 0xac), dt);
    h = PTR_AT(self, 0xac);
    func_0204ed80(self + 0xb0, U32_AT(h, 0x2c), U32_AT(h, 0x38));
    f = U32_AT(self, 0x18c);
    if (BITS(f, 3, 1)) {
        S32_AT(self, 0xa8) -= FxMul((s32)dt, 0x800);
        if (S32_AT(self, 0xa8) > 0x333)
            return;
        S32_AT(self, 0xa8) = 0x333;
        U32_AT(self, 0x18c) &= ~8u;
        return;
    }
    if (BITS(f, 4, 1) == 0)
        return;
    S32_AT(self, 0xa8) += FxMul((s32)dt, 0x800);
    if (S32_AT(self, 0xa8) < 0x1000)
        return;
    S32_AT(self, 0xa8) = 0x1000;
    U32_AT(self, 0x18c) &= ~0x10u;
}

/* 0x0205359c: unload (freeing the shadow texture with the last one)
 * @difftest $S=zero:0x1a8 @$S+0xac:32=pick:0,0x1234 $S @020f535c:16=pick:1,2 stub=0x0204f094,0x02071800,0x02071838,0x0207fe44,0x02082e70,0x02080778 cases=8 */
void func_0205359c(u8 *self)
{
    func_0204f094(self + 0xb0);
    if (PTR_AT(self, 0xac) != NULL) {
        u8 *h;

        func_02071800(PTR_AT(self, 0xac));
        h = PTR_AT(self, 0xac);
        if (h != NULL) {
            func_02071838(h);
            func_0207fe44(h);
        }
        PTR_AT(self, 0xac) = NULL;
    }
    self[0x94] = 0x1f;
    S32_AT(self, 0xa0) = 0;
    data_020f535c--;
    if (data_020f535c != 0)
        return;
    func_02082e70(func_02018208(), data_020f5360);
    func_02080778(data_020f5370);
}

/* 0x02053648: load the model of kind `kind` for `actor`
 * @difftest zero:0x1a8 u32 u32 u32 pick:0,1 @020f535c:16=pick:0,1 @020f53a8:8=pick:0,1 stub=0x0204f8d4,0x0207fe74:0,0x02071804,0x02080aec,0x02082f00:0x55 cases=8 */
void func_02053648(u8 *self, u32 kind, void *actor, u32 a, s8 bones)
{
    u8 *p;

    func_0204f8d4(self + 0xb0, kind, a, bones);
    func_02004490(data_020c074c);
    p = func_0207fe74(0xa4);
    if (p != NULL)
        p = func_02071864(p);
    PTR_AT(self, 0xac) = p;
    func_02004490(data_020c0774);
    func_02071804(PTR_AT(self, 0xac), kind, actor);
    U32_AT(self, 0x18c) |= 4;
    self[0x94] = 0x1f;
    S32_AT(self, 0xa0) = 0;
    S32_AT(self, 0x10) = data_020b6cc4;
    S32_AT(self, 0x14) = g_FxZero;
    S32_AT(self, 0x18) = g_FxZero;
    self[0x8c] = 1;
    if (data_020f535c == 0 || S8_AT(data_020f5370, 0x38) == 0) {
        func_02080aec(data_020f5370, data_020c0790);
        data_020f5360 = func_02082f00(func_02018208(), data_020f5370);
    }
    data_020f535c++;
}

/* what both destructors do */
static void Destroy(u8 *self)
{
    PTR_AT(self, 0) = data_020c0728;
    PTR_AT(self, 0x90) = data_020c0740;
    func_0205359c(self);
    PTR_AT(self, 0x190) = data_020c07ac;
    PTR_AT(self, 0x190) = data_020bf760;
    func_0204f964(self + 0xb0);
    func_0205b728(self + 0x90);
    func_0205b784(self);
}

/* 0x02053764: deleting destructor
 * @difftest zero:0x1a8 @020f535c:16=2 stub=0x0204f094,0x0204f964,0x0205b728,0x0205b784,0x0207fe44 cases=3 */
u8 *func_02053764(u8 *self)
{
    Destroy(self);
    func_0207fe44(self);
    return self;
}

/* 0x020537cc: destructor
 * @difftest zero:0x1a8 @020f535c:16=2 stub=0x0204f094,0x0204f964,0x0205b728,0x0205b784 cases=3 */
u8 *func_020537cc(u8 *self)
{
    Destroy(self);
    return self;
}

/* 0x0205382c: constructor
 * @difftest ptr:0x1a8:4 stub=0x0205b8c8,0x0205b76c,0x0204fae8 cases=3 */
u8 *func_0205382c(u8 *self)
{
    u8 *d = self + 0x190;
    u32 m1[2], m2[2];

    func_0205b8c8(self);
    func_0205b76c(self + 0x90);
    PTR_AT(self, 0) = data_020c0728;
    PTR_AT(self, 0x90) = data_020c0740;
    S32_AT(self, 0xa4) = 0x18000;
    S32_AT(self, 0xa8) = 0x1000;
    PTR_AT(self, 0xac) = NULL;
    func_0204fae8(self + 0xb0);
    m2[0] = data_020c06e8[0];
    m2[1] = data_020c06e8[1];
    m1[0] = data_020c06e0[0];
    m1[1] = data_020c06e0[1];
    PTR_AT(d, 0) = data_020bf760;
    PTR_AT(d, 0) = data_020c07ac;
    PTR_AT(d, 4) = self;
    U32_AT(d, 8) = m1[0];
    U32_AT(d, 0xc) = m1[1];
    U32_AT(d, 0x10) = m2[0];
    U32_AT(d, 0x14) = m2[1];
    U32_AT(self, 0x18c) = 0;
    U32_AT(self, 0x18c) |= 2;
    S32_AT(self, 0x1c) = 0x59a;
    S32_AT(self, 0x20) = 0x59a;
    S32_AT(self, 0x24) = 0x59a;
    self[0x8c] = 1;
    S32_AT(self, 0x10) = data_020b6cc4;
    S32_AT(self, 0x14) = g_FxZero;
    S32_AT(self, 0x18) = g_FxZero;
    self[0x8c] = 1;
    return self;
}

/* 0x02053958: the draw callback's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_02053958(u8 *cb)
{
    PTR_AT(cb, 0) = data_020c07ac;
    PTR_AT(cb, 0) = data_020bf760;
    return cb;
}

/* 0x02053974: the draw callback's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_02053974(u8 *cb)
{
    PTR_AT(cb, 0) = data_020c07ac;
    PTR_AT(cb, 0) = data_020bf760;
    func_0207fe44(cb);
    return cb;
}

/* call member function pointer `m` ({function, adjustment * 2 + virtual}) on `obj` */
static void CallMember(u8 *obj, const u8 *m)
{
    s32 adj = S32_AT(m, 4);
    u8 *t = obj + (adj >> 1);
    void (*f)(void *, void *);

    if (adj & 1)
        f = *(void (**)(void *, void *))(*(u8 **)t + U32_AT(m, 0));
    else
        f = (void (*)(void *, void *))PTR_AT(m, 0);
    f(t, (void *)f);
}

/* 0x020539a4: call the draw callback's first member function
 * @difftest $D=zero:0x18 @$D+8:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+8:32=8 @$D+0xc:32=pick:1,9 $D cases=5 */
void func_020539a4(u8 *cb)
{
    if (PTR_AT(cb, 4) == NULL)
        return;
    if (PTR_AT(cb, 8) == NULL)
        return;
    CallMember(PTR_AT(cb, 4), cb + 8);
}

/* 0x02053a04: call the draw callback's second member function
 * @difftest $D=zero:0x18 @$D+0x10:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+0x10:32=8 @$D+0x14:32=pick:1,9 $D cases=5 */
void func_02053a04(u8 *cb)
{
    if (PTR_AT(cb, 4) == NULL)
        return;
    if (PTR_AT(cb, 0x10) == NULL)
        return;
    CallMember(PTR_AT(cb, 4), cb + 0x10);
}

/* The sub-object's this-adjusting entry points (the object is 0x90 before). */

/* 0x02053a64
 * @difftest $S=zero:0x1a8 $S+0x90 @020f535c:16=2 stub=0x0204f094,0x0204f964,0x0205b728,0x0205b784,0x0207fe44 cases=3 */
u8 *func_02053a64(u8 *a)
{
    return func_02053764(a - 0x90);
}

/* 0x02053a74
 * @difftest $S=zero:0x1a8 $S+0x90 @020f535c:16=2 stub=0x0204f094,0x0204f964,0x0205b728,0x0205b784 cases=3 */
u8 *func_02053a74(u8 *a)
{
    return func_020537cc(a - 0x90);
}

/* 0x02053a84
 * @difftest $S=zero:0x1a8 $S+0x90 cases=3 */
void func_02053a84(u8 *a)
{
    func_02053064(a - 0x90);
}
