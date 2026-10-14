/*
 * Pickups: the items an opened chest or a defeated enemy leaves (actor
 * class 0xa). A pickup pops up and falls to the ground, then may shrink
 * away; the hero collects it by touching it (A for some).
 * ARM9 main, 0x02044b44 - 0x02045900 (22 functions).
 *
 * The object (0x134 bytes) is a model (vtable data_020bf8d0, func_0205b8c8
 * / func_0205b784) with an actor at +0x90 (vtable data_020bf8ec; the
 * functions at 0x020458c0.. are its this-adjusting entry points) and a
 * draw callback at +0x11c (vtables data_020bf938 / data_020bf760, member
 * functions data_020bf888 / data_020bf890: draw and its shadow).
 * +0xdc velocity, +0xe8 position (with the last one at +0xf4), +0x100
 * place on the ground on the next frame, +0x101 falling, +0x102
 * shrinking, +0x104 the animated model, +0x10c scale, +0x110 life time,
 * +0x114 age. data_020ebca0 counts the pickups.
 *
 * Testing: with fake objects; the inventory, the sounds, the 3D state
 * and the camera stubbed where they need real objects.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020bf860, data_020bf864, data_020bf868, data_020bf86c, data_020bf870,
    data_020bf874;
extern const s32 data_020bf878, data_020bf87c, data_020bf880, data_020bf884;   /* the box */
extern const u32 data_020bf888[2], data_020bf890[2];   /* the member functions */
extern u8 data_020bf8d0[], data_020bf8ec[];           /* vtables */
extern u8 data_020bf938[], data_020bf760[];           /* the callbacks' vtables */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */
extern u8 data_020def74[];           /* the inventory */
extern s32 data_020ebca0;            /* pickups alive */
extern u8 *data_020ebd34;            /* models, 0xc bytes each */

int rand(void);
s32 func_0205a0d4(void *inv, void *actor);
const char *func_02045f18(void *actor);
u8 *func_02011f34(void);
s32 func_02011bc8(void);
void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
void func_02076b44(void *obj, const char *text, u32 fbits);
void func_02072418(void *obj, u32 a, u32 fbits);
s32 func_02014260(void *item);
void func_02029790(s32 *out, const s32 *a, const s32 *b);
s32 func_0201a0a0(s32 *xz, void *obj);
void *func_020062e0(void);
void *func_020062b0(void);
void *func_020062c0(void);
void func_02080f60(void *settings, u32 a, u32 b);
void func_02080e84(void *settings);
void func_02080bf4(void *settings, void *m);
void func_0202de98(s32 *m);
void func_02017550(s32 *out, const u8 *cam);
void func_0202de68(s32 *m, const s32 *v);
void func_020022e4(void *self);
void func_0201751c(s32 *out, const u8 *cam);
void func_0202dc88(s32 *m, const s32 *rot);
void func_02002974(void *self);
void func_02005b98(s32 *out, const void *src);
void func_0202dc6c(s32 *m, const s32 *v);
void MI_Copy64B(const void *src, void *dest);
void *func_02018208(void);
void func_02082e48(void *a, void *b);
s32 func_0208f070(s32 num, s32 den);
void func_0204a1f4(void *anim, u32 dt);
u32 func_020186ec(void);
void func_0201c010(s32 *out, const s32 *pos);
void func_020040f4(u8 *dst, const u8 *src);
void func_02082ce8(void *cam);
void *func_02019670(void);
void func_02043cc8(u8 *list, void *item);
void func_02017abc(void *list, void *item);
void *func_0204a2b8(void *p, u32 kind, s32 *v);
void *func_0207fe74(u32 size);
void func_0207fe44(void *p);
void func_02008130(s32 group, s32 index);
void *func_020506c0(void *actor);
void func_0205b784(void *self);
void func_0205b8c8(void *self);
void func_02050890(void *actor, s32 *pos, s32 *box);

void func_02044c98(u8 *self, u32 dt);
void func_02044ddc(u8 *self);
void func_02044e60(u8 *self);
void func_02045004(u8 *self, u32 dt);
void func_02045370(u8 *self);
void func_020453a4(u8 *self, u32 kind);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define ACTOR(self) ((self) != NULL ? (self) + 0x90 : NULL)

/* the model of the animation `p` is showing */
static void *Model(u8 *p)
{
    u8 *e = ((u8 **)PTR_AT(PTR_AT(p, 0x10), 0))[U16_AT(p, 0x1c)];
    s32 i = S32_AT(e, 4);

    if (i <= -1)
        return NULL;
    return PTR_AT(data_020ebd34, i * 0xc);
}

/* 0x02044b44: a pickup touched: the hero collects it
 * @difftest $S=zero:0x134 @$S+0xb4:32=int:-0x80000:0x80000 @$S+0x9c:32=pick:0xa,0xa,3 @$S+0x101:8=pick:0,0,1 $O=zero:0x60 @$O+0xc:32=pick:0,0,0,1 @$O+8:32=1 @$O+0x24:32=int:-0x80000:0x80000 $S $O stub=0x0205a0d4:1,0x02045f18:0,0x02011bc8:0,0x02076b44,0x02072418,0x02081d34:1,0x02014260:1 cases=100
 * @difftest $S=zero:0x134 @$S+0x9c:32=0xa @$S+0x101:8=pick:0,1 $O=zero:0x60 @$O+8:32=2 @020deec4:32=$O $S $O stub=0x0205a0d4:0,0x02014260:0 cases=10
 * @difftest $S=zero:0x134 @$S+0x9c:32=0xa $O=zero:0x60 @$O+8:32=1 $S $O stub=0x0205a0d4:1,0x02045f18:0,0x02011bc8:1,0x02076b44,0x02072418,0x02081d34:0,0x02014260:1 cases=5 */
void func_02044b44(u8 *self, u8 *other)
{
    s32 d, r;

    if (S32_AT(other, 0xc) != 0)
        return;
    if (S32_AT(other, 8) != 1)
        other = HERO();
    d = (S32_AT(other, 0x24) - S32_AT(self, 0xb4)) >> 4;
    if (d < 0)
        d = -d;
    if (d >= 0x8000)
        return;
    if (S32_AT(self, 0x9c) != 0xa)
        return;
    if (func_0205a0d4(data_020def74, ACTOR(self)) != 0) {
        const char *t = func_02045f18(ACTOR(self));
        u8 *pad = func_02011f34();

        if (func_02011bc8() == 0) {
            func_02076b44(pad + 0x7c4, t, 0x45800000);
            func_02076b44(pad + 0x768, NULL, 0);
        }
        func_02072418(func_02011f34(), 7, 0x45000000);
        U8_AT(data_020deebc, 0x1f) = 1;
        r = func_02081d34(func_02011f44(), 0xa);
    } else {
        r = 1;
    }
    if (r == 0)
        return;
    if (S8_AT(self, 0x101) != 0)
        return;
    if (func_02014260(ACTOR(self)) != 0)
        U32_AT(self, 0xc8) |= 2;
}

/* 0x02044c98: falling (bounce off the ground)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0x134 @$S+0xdc:32=int:-0x1000:0x1000 @$S+0xe0:32=int:-0x1000:0x1000 @$S+0xe4:32=int:-0x50000:0x20000 @$S+0xe8:32=int:-0x10000:0x10000 @$S+0xec:32=int:-0x10000:0x10000 @$S+0xf0:32=int:-0x8000:0x8000 $S int:0:0x2000 cases=60 */
void func_02044c98(u8 *self, u32 dt)
{
    s32 v[2], t[3], u[2], g;

    S32_AT(self, 0xf4) = S32_AT(self, 0xe8);
    S32_AT(self, 0xf8) = S32_AT(self, 0xec);
    S32_AT(self, 0xfc) = S32_AT(self, 0xf0);
    v[0] = S32_AT(self, 0xf4);
    v[1] = S32_AT(self, 0xf8);
    S32_AT(self, 0xb8) = v[0];
    S32_AT(self, 0xbc) = v[1];
    S32_AT(self, 0xe4) -= 0x5000;
    if (S32_AT(self, 0xe4) < -0x40000)
        S32_AT(self, 0xe4) = -0x40000;
    func_02029790(t, (s32 *)(self + 0xf4), (s32 *)(self + 0xdc));
    S32_AT(self, 0xe8) = t[0];
    S32_AT(self, 0xec) = t[1];
    S32_AT(self, 0xf0) = t[2];
    u[0] = SHL(S32_AT(self, 0xe8), 4);
    u[1] = SHL(S32_AT(self, 0xec), 4);
    g = func_0201a0a0(u, NULL) >> 4;
    if (S32_AT(self, 0xf0) >= g)
        return;
    if (S32_AT(self, 0xe4) < 0) {
        S32_AT(self, 0xf0) = g;
        S32_AT(self, 0xb4) = SHL(g, 4);
        self[0x101] = 0;
    }
    S32_AT(self, 0xdc) = data_020bf870;
    S32_AT(self, 0xe0) = data_020bf874;
    S32_AT(self, 0xe4) = data_020bf860;
    S32_AT(self, 0xe8) = S32_AT(self, 0xf4);
    S32_AT(self, 0xec) = S32_AT(self, 0xf8);
}

/* 0x02044ddc: put the pickup on the ground
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0x134 @$S+0xb8:32=int:-0x10000:0x10000 @$S+0xbc:32=int:-0x10000:0x10000 $S cases=10 */
void func_02044ddc(u8 *self)
{
    s32 v[2], u[2];

    v[0] = S32_AT(self, 0xb8);
    v[1] = S32_AT(self, 0xbc);
    S32_AT(self, 0xf4) = v[0];
    S32_AT(self, 0xf8) = v[1];
    u[0] = SHL(v[0], 4);
    u[1] = SHL(v[1], 4);
    S32_AT(self, 0xfc) = func_0201a0a0(u, NULL) >> 4;
    S32_AT(self, 0xe8) = S32_AT(self, 0xf4);
    S32_AT(self, 0xec) = S32_AT(self, 0xf8);
    S32_AT(self, 0xf0) = S32_AT(self, 0xfc);
}

/* 0x02044e60
 * @difftest ptr:0x134 cases=3 */
void func_02044e60(u8 *self)
{
}

/* 0x02044e64: draw the pickup
 * @difftest $E=zero:8 @$E+4:32=-1 $L=zero:4 @$L+0:32=$E $A=zero:4 @$A+0:32=$L $P=zero:0x20 @$P+0x10:32=$A $S=zero:0x134 @$S+0x104:32=$P @$S+0x10c:32=int:0:0x20000 @$S+0x8c:8=pick:0,1 @$S+0xc8:32=pick:0,0x80 $S stub=0x02080f60,0x02080e84,0x02080bf4 cases=10 */
void func_02044e64(u8 *self)
{
    s32 n;
    u8 *g;

    if (BITS(U32_AT(self, 0xc8), 7, 1) != 0)
        func_02044e60(self);
    func_02080f60(func_020062e0(), 3, 1);
    func_02080f60(func_020062e0(), 4, 0);
    n = S32_AT(self, 0x10c) >> 12;
    S32_AT(func_020062e0(), 0x4a4) = n;
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
    func_02080e84(func_020062e0());
    func_02080bf4(func_020062e0(), Model(PTR_AT(self, 0x104)));
}

/* 0x02044fac: draw the pickup's shadow
 * @difftest $M=zero:0x40 $E=zero:8 @$E+4:32=0 $L=zero:4 @$L+0:32=$E $A=zero:4 @$A+0:32=$L $P=zero:0x20 @$P+0x10:32=$A $S=zero:0x134 @$S+0x104:32=$P $S stub=0x02082e48,0x0204a1f4 cases=3 */
void func_02044fac(u8 *self)
{
    u8 *m = Model(PTR_AT(self, 0x104));

    func_02082e48(func_02018208(), PTR_AT(m, 0x34));
}

/* 0x02045004: shrinking away
 * @difftest $S=zero:0x134 @$S+0x110:32=int:0x1000:0x8000 @$S+0x114:32=int:0:0x8000 @$S+0x10c:32=int:0:0x20000 $S int:1:0x2000 cases=60 */
void func_02045004(u8 *self, u32 dt)
{
    S32_AT(self, 0x114) += dt;
    if (S32_AT(self, 0x114) > FxMul(S32_AT(self, 0x110), 0xf33)) {
        s32 q = func_0208f070(dt, S32_AT(self, 0x110));

        S32_AT(self, 0x10c) -= FxMul(q, 0x1f000);
    }
    if (S32_AT(self, 0x10c) >= 0x1000)
        return;
    S32_AT(self, 0x10c) = 0x1000;
    U32_AT(self, 0xc8) |= 2;
}

/* 0x020450e0: per-frame update
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $T=zero:0x20 @$T+0x10:32=0x020343a8 $S=zero:0x134 @$S+0:32=$T @$S+0x100:8=pick:0,1 @$S+0x101:8=pick:0,1 @$S+0x102:8=pick:0,1 @$S+0x110:32=0x4000 @$S+0x94:32=pick:0,8 @$S+0xc8:32=pick:0,0x20,0x40 $S int:0:0x2000 stub=0x0204a1f4,0x020186ec:0 cases=40
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $T=zero:0x20 @$T+0x10:32=0x020343a8 $S=zero:0x134 @$S+0:32=$T @$S+0x94:32=8 @$S+0xc8:32=pick:0x20,0x40 $S int:0:0x2000 stub=0x0204a1f4,0x020186ec:1 cases=10 */
void func_020450e0(u8 *self, u32 dt)
{
    if (S8_AT(self, 0x100) != 0) {
        func_02044ddc(self);
        self[0x100] = 0;
    }
    if (S8_AT(self, 0x101) != 0)
        func_02044c98(self, dt);
    if (S8_AT(self, 0x102) != 0)
        func_02045004(self, dt);
    func_0204a1f4(PTR_AT(self, 0x104), dt);
    if (BITS(U32_AT(self, 0x94), 3, 1) == 0)
        return;
    if (BITS(U32_AT(self, 0xc8), 5, 1) != 0 && func_020186ec() == 0) {
        void (*m)(void *, void *) = VCALL(self, 0x10, void (*)(void *, void *));
        m(self, (void *)m);
        return;
    }
    if (BITS(U32_AT(self, 0xc8), 6, 1) == 0)
        return;
    if (func_020186ec() != 1)
        return;
    {
        void (*m)(void *, void *) = VCALL(self, 0x10, void (*)(void *, void *));
        m(self, (void *)m);
    }
}

/* 0x020451e8: place the pickup's model and draw it if in view
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0x134 @$S+0xf4:32=int:-0x10000:0x10000 @$S+0xf8:32=int:-0x10000:0x10000 @$S+0xfc:32=int:-0x10000:0x10000 @$S+0xa0:32=int:-0x10000:0 @$S+0xa4:32=int:-0x10000:0 @$S+0xa8:32=int:0:0x10000 @$S+0xac:32=int:0:0x10000 $S stub=0x0201f490:0x020deebc,0x02043cc8,0x02017abc cases=20 */
void func_020451e8(u8 *self)
{
    s32 v[3], p[3], b[4];
    u32 cam[0x2c0 / 4];
    s32 vis;

    v[0] = S32_AT(self, 0xf4);
    v[1] = S32_AT(self, 0xf8);
    v[2] = S32_AT(self, 0xfc);
    func_0201c010(p, v);
    S32_AT(self, 4) = p[0];
    S32_AT(self, 8) = p[1];
    S32_AT(self, 0xc) = p[2];
    self[0x8c] = 1;
    b[0] = S32_AT(self, 0xa0) + S32_AT(self, 8);
    b[1] = S32_AT(self, 0xa4) + S32_AT(self, 4);
    b[2] = S32_AT(self, 0xa8) + S32_AT(self, 8);
    b[3] = S32_AT(self, 0xac) + S32_AT(self, 4);
    func_020040f4((u8 *)cam, func_020062e0());
    vis = 0;
    if (S8_AT(cam, 0x284) != 0) {
        vis = S32_AT(cam, 0x2ac) >= b[1] && S32_AT(cam, 0x2a4) <= b[3]
              && S32_AT(cam, 0x2a0) <= b[2] && S32_AT(cam, 0x2a8) >= b[0];
    }
    func_02082ce8(cam);
    if (vis != 0) {
        func_02043cc8((u8 *)func_02019670() + 4, self + 0x11c);
        func_02017abc((u8 *)func_020062c0() + 4, ACTOR(self));
        U32_AT(self, 0xc8) |= 0x2000;
        return;
    }
    U32_AT(self, 0xc8) &= ~0x2000u;
}

/* 0x02045370: delete the animated model
 * @difftest zero:0x134 cases=3
 * @difftest $T=zero:0x20 @$T+4:32=0x020343a8 $P=zero:0x20 @$P+0:32=$T $S=zero:0x134 @$S+0x104:32=$P $S cases=3 */
void func_02045370(u8 *self)
{
    u8 *p = PTR_AT(self, 0x104);

    if (p == NULL)
        return;
    {
        void (*m)(void *, void *) = VCALL(p, 4, void (*)(void *, void *));
        m(p, (void *)m);
    }
}

/* 0x020453a4: give the pickup its model (actor kind `kind`)
 * @difftest zero:0x134 int:0:0x40 stub=0x0204a2b8 cases=5 */
void func_020453a4(u8 *self, u32 kind)
{
    s32 v[2];
    void *p;
    s32 sc;

    v[0] = 0;
    v[1] = 0;
    S32_AT(self, 0x98) = kind;
    p = func_0207fe74(0x20);
    if (p != NULL)
        p = func_0204a2b8(p, kind, v);
    PTR_AT(self, 0x104) = p;
    sc = S32_AT(data_020c3b0c + kind * 0x48, 0x2c);
    S32_AT(self, 0x1c) = sc;
    S32_AT(self, 0x20) = sc;
    S32_AT(self, 0x24) = sc;
    self[0x8c] = 1;
}

/* what both destructors do */
static void Destroy(u8 *self)
{
    PTR_AT(self, 0) = data_020bf8d0;
    PTR_AT(self, 0x90) = data_020bf8ec;
    data_020ebca0--;
    func_02045370(self);
    if (S8_AT(self, 0xd2) != -1 && S16_AT(self, 0xd0) != -1)
        func_02008130(S8_AT(self, 0xd2), S16_AT(self, 0xd0));
    PTR_AT(self, 0x11c) = data_020bf938;
    PTR_AT(self, 0x11c) = data_020bf760;
    func_020506c0(self + 0x90);
    func_0205b784(self);
}

/* 0x0204541c: the pickup's deleting destructor
 * @difftest $S=zero:0x134 @$S+0xd0:16=pick:-1,3 @$S+0xd2:8=pick:-1,2 $S stub=0x02008130,0x020506c0,0x0205b784,0x0207fe44 cases=8 */
u8 *func_0204541c(u8 *self)
{
    Destroy(self);
    func_0207fe44(self);
    return self;
}

/* 0x020454b0: the pickup's destructor
 * @difftest $S=zero:0x134 @$S+0xd0:16=pick:-1,3 @$S+0xd2:8=pick:-1,2 $S stub=0x02008130,0x020506c0,0x0205b784 cases=8 */
u8 *func_020454b0(u8 *self)
{
    Destroy(self);
    return self;
}

/* a random life time, made fx32 through the game's float routines */
static s32 LifeTime(void)
{
    float f;

    if ((((u32)rand() & 0x7fff) & 3) > 0)
        f = 0.5f + (float)(s32)((((u32)rand() & 0x7fff) & 3) << 12);
    else
        f = (float)(s32)((((u32)rand() & 0x7fff) & 3) << 12) - 0.5f;
    return (s32)f;
}

/* 0x0204553c: the pickup's constructor (actor kind `kind` at `pos`)
 * @difftest zero:0x134 int:0:0x40 ptr:8 int:0:1 stub=0x0205b8c8,0x02050890,0x02017abc,0x020453a4 cases=20 */
u8 *func_0204553c(u8 *self, u32 kind, s32 *pos, u32 shrink)
{
    s32 box[4];
    u8 *d = self + 0x11c;
    u32 m1[2], m2[2];

    func_0205b8c8(self);
    box[0] = data_020bf884;
    box[1] = data_020bf880;
    box[2] = data_020bf87c;
    box[3] = data_020bf878;
    func_02050890(self + 0x90, pos, box);
    PTR_AT(self, 0) = data_020bf8d0;
    PTR_AT(self, 0x90) = data_020bf8ec;
    S32_AT(self, 0xdc) = data_020bf864;
    S32_AT(self, 0xe0) = data_020bf868;
    S32_AT(self, 0xe4) = data_020bf86c;
    S32_AT(self, 0xe8) = 0;
    S32_AT(self, 0xec) = 0;
    S32_AT(self, 0xf0) = 0;
    S32_AT(self, 0xf4) = 0;
    S32_AT(self, 0xf8) = 0;
    S32_AT(self, 0xfc) = 0;
    self[0x100] = 1;
    self[0x101] = 0;
    self[0x102] = shrink;
    S32_AT(self, 0x108) = -1;
    S32_AT(self, 0x10c) = 0x1f000;
    S32_AT(self, 0x114) = 0;
    U16_AT(self, 0x118) = 0;
    U16_AT(self, 0x11a) = 0;
    m1[0] = data_020bf888[0];
    m1[1] = data_020bf888[1];
    m2[0] = data_020bf890[0];
    m2[1] = data_020bf890[1];
    PTR_AT(d, 0) = data_020bf760;
    PTR_AT(d, 0) = data_020bf938;
    PTR_AT(d, 4) = self;
    U32_AT(d, 8) = m1[0];
    U32_AT(d, 0xc) = m1[1];
    U32_AT(d, 0x10) = m2[0];
    U32_AT(d, 0x14) = m2[1];
    S32_AT(self, 0x110) = LifeTime() + 0x4000;
    U32_AT(self, 0xc8) = (U32_AT(self, 0xc8) & ~4u)
                         | ((S32_AT(data_020c3b0c + kind * 0x48, 0x40) > 0) << 2);
    U32_AT(self, 0x94) |= 8;
    func_02017abc((u8 *)func_020062b0() + 4, ACTOR(self));
    func_020453a4(self, kind);
    data_020ebca0++;
    return self;
}

/* 0x020457b4: the draw callback's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_020457b4(u8 *self)
{
    PTR_AT(self, 0) = data_020bf938;
    PTR_AT(self, 0) = data_020bf760;
    return self;
}

/* 0x020457d0: the draw callback's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_020457d0(u8 *self)
{
    PTR_AT(self, 0) = data_020bf938;
    PTR_AT(self, 0) = data_020bf760;
    func_0207fe44(self);
    return self;
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

/* 0x02045800: call the draw callback's first member function
 * @difftest $D=zero:0x18 @$D+8:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+8:32=pick:8,0x020343a8 @$D+0xc:32=pick:0,1,9 $D cases=10 */
void func_02045800(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 8) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 8);
}

/* 0x02045860: call the draw callback's second member function
 * @difftest $D=zero:0x18 @$D+0x10:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+0x10:32=pick:8,0x020343a8 @$D+0x14:32=pick:0,1,9 $D cases=10 */
void func_02045860(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 0x10) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 0x10);
}

/* 0x020458c0: the actor's deleting destructor
 * @difftest $S=zero:0x134 @$S+0xd0:16=-1 $S+0x90 stub=0x02008130,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_020458c0(u8 *actor)
{
    return func_0204541c(actor - 0x90);
}

/* 0x020458d0: the actor's collision callback
 * @difftest $S=zero:0x134 @$S+0x9c:32=0xa $O=zero:0x60 @$O+8:32=1 @$O+0xc:32=pick:0,1 $S+0x90 $O stub=0x0205a0d4:0,0x02014260:1 cases=5 */
void func_020458d0(u8 *actor, u8 *other)
{
    func_02044b44(actor - 0x90, other);
}

/* 0x020458e0: the actor's per-frame update
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0x134 @$S+0x110:32=0x4000 $S+0x90 int:0:0x2000 stub=0x0204a1f4 cases=5 */
void func_020458e0(u8 *actor, u32 dt)
{
    func_020450e0(actor - 0x90, dt);
}

/* 0x020458f0: the actor's destructor
 * @difftest $S=zero:0x134 @$S+0xd0:16=-1 $S+0x90 stub=0x02008130,0x020506c0,0x0205b784 cases=3 */
u8 *func_020458f0(u8 *actor)
{
    return func_020454b0(actor - 0x90);
}
