/*
 * Effect objects: an optional model with models attached to its bones
 * (Attach.c) and particle effect players (Emitter.c), following a parent
 * object at an offset.
 * ARM9 main, 0x02054fec - 0x02056288 (35 functions).
 *
 * Layout: the object base (func_0205b8c8 / func_0205b784, vtable
 * data_020c09f4), +0x90 a task (func_020592a8 / func_0205924c, vtable
 * data_020c0a14; this-adjusting entry points subtract 0x90), +0x94 flags
 * (bit 1 to be removed, 2 hidden), +0x98 the actor kind, +0xa0 the offset
 * from the parent, +0xac alpha, +0xb0 the draw callback (vtables
 * data_020c0a38 / data_020bf760), +0xc8 playing, +0xc9 loaded, +0xca
 * removed when done, +0xcb turns with the parent, +0xcc the offset is not
 * turned, +0xd0 the parent, +0xd4 the model, +0xd8 the players and +0xe4
 * the attached models ({data, size, capacity} each).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020b6cc4;
extern const fx32 data_020c0960, data_020c0964, data_020c0968, data_020c096c, data_020c0970,
    data_020c0974, data_020c0978, data_020c097c, data_020c0980, data_020c0984, data_020c0988,
    data_020c098c;                   /* default offsets */
extern const u32 data_020c0990[2], data_020c0998[2], data_020c09a0[2], data_020c09a8[2],
    data_020c09b0[2], data_020c09b8[2];   /* the callback's member functions */
extern u8 data_020c09f4[], data_020c0a14[], data_020c0a38[], data_020bf760[];   /* vtables */
extern const char data_020c0a24[];   /* source file name */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */
extern u32 data_020df100;
extern s8 data_020df10c;

void *func_020062e0(void);
void *func_020062d0(void);
void *func_020062a0(void);
u64 func_020822c8(void *timer);
void func_0204ed80(void *model, u32 id, s32 frame);
void func_0204ed20(void *model);
void func_0204e948(void *model, u32 alpha);
void func_0204ebb4(void *model);
void func_0204efec(void *model, u32 dt);
void func_0204f964(void *model);
u8 *func_0204fa44(void *model, u32 kind);
void func_02051068(u8 *self, u32 a);
void func_020510bc(u8 *self);
u8 *func_020512b0(u8 *self);
u8 *func_020512c8(u8 *self, u32 a, void *model, const char *bone);
void func_020573dc(void *e);
void func_02080f60(void *settings, u32 a, u32 b);
void func_02080e84(void *settings);
void MI_Copy64B(const void *src, void *dest);
void func_0202de98(s32 *m);
void func_02017550(s32 *out, const void *obj);
void func_0202de68(s32 *m, const s32 *v);
void func_020022e4(void *self);
void func_0201751c(s32 *out, const void *obj);
void func_0202dc88(s32 *m, const s32 *rot);
void func_02002974(void *self);
void func_02005b98(s32 *out, const void *src);
void func_0202dc6c(s32 *m, const s32 *v);
void func_02029790(s32 *out, const s32 *a, const s32 *b);
void *func_02019670(void);
void func_02043cc8(u8 *list, void *item);
void func_02049b78(u8 *list, void *item);
void *func_0207fe74(u32 size);
void func_0207fe44(void *p);
void func_0207fe28(const char *file, s32 line);
void func_0207fe24(void);
void func_020062f0(void *vec, void **value, u32 tag);
void func_02006330(void *v, u32 n);
void func_02006acc(void *v);
void func_0205b8c8(void *self);
void func_0205b784(void *self);
void func_020592a8(void *task);
void func_0205924c(void *task);

void func_02055018(u8 *self, s32 on);
void func_02055094(u8 *self, s32 x, s32 y, s32 z);
s32 func_020550c0(u8 *self);
void func_02055424(u8 *self);
void func_0205555c(u8 *self);
void func_020555b0(u8 *self);
void func_02055970(u8 *self);
void func_02055990(u8 *self);
u8 *func_02055c08(u8 *self);
u8 *func_02055c78(u8 *self);

#define MODEL(self) ((u8 *)PTR_AT(self, 0xd4))
#define NPLAYERS(self) U32_AT(self, 0xdc)
#define PLAYER(self, i) ((u8 *)PTR_AT(PTR_AT(self, 0xd8), (i) * 4))
#define NATTACHED(self) U32_AT(self, 0xe8)
#define ATTACHED(self, i) ((u8 *)PTR_AT(PTR_AT(self, 0xe4), (i) * 4))
#define ACTOR(self) ((self) != NULL ? (self) + 0x90 : NULL)
#define OBJCALL(o, off) (*(void (**)())(*(u8 **)(o) + (off)))

/* 0x02054fec: show the effect
 * @difftest $M=zero:0xdc @$M+0x14:32=u32 $S=zero:0xf0 @$S+0x94:32=u32 @$S+0xd4:32=0 $S cases=2
 * @difftest $M=zero:0xdc @$M+0x14:32=u32 $S=zero:0xf0 @$S+0x94:32=u32 @$S+0xd4:32=$M $S cases=3 */
void func_02054fec(u8 *self)
{
    U32_AT(self, 0x94) &= ~4u;
    if (MODEL(self) != NULL)
        U32_AT(MODEL(self), 0x14) &= ~0x80u;
}

/* 0x02055018: hide the effect (and freeze its model) or not
 * @difftest $S=zero:0xf0 @$S+0x94:32=u32 @$S+0x98:32=pick:0,0x35 $S pick:0,1 cases=5
 * @difftest $M=zero:0xdc @$M+0x14:32=u32 $S=zero:0xf0 @$S+0x94:32=u32 @$S+0xd4:32=$M $S pick:0,1 cases=5 */
void func_02055018(u8 *self, s32 on)
{
    u8 *m;

    if (U32_AT(self, 0x98) == 0x35)
        return;
    U32_AT(self, 0x94) = (U32_AT(self, 0x94) & ~4u) | ((on & 1) << 2);
    m = MODEL(self);
    if (m == NULL)
        return;
    if (on)
        U32_AT(m, 0x14) |= 0x80;
    else
        U32_AT(m, 0x14) &= ~0x80u;
}

/* 0x02055070: set the alpha
 * @difftest ptr:0xf0:4 u32 cases=3 */
void func_02055070(u8 *self, u32 a)
{
    U32_AT(self, 0xac) = a;
}

/* 0x02055078: set the offset (not turned with the parent)
 * @difftest ptr:0xf0:4 s32 s32 s32 cases=3 */
void func_02055078(u8 *self, s32 x, s32 y, s32 z)
{
    func_02055094(self, x, y, z);
    self[0xcc] = 1;
}

/* 0x02055094: set the offset
 * @difftest ptr:0xf0:4 s32 s32 s32 cases=3 */
void func_02055094(u8 *self, s32 x, s32 y, s32 z)
{
    self[0xcc] = 0;
    S32_AT(self, 0xa0) = x;
    S32_AT(self, 0xa4) = y;
    S32_AT(self, 0xa8) = z;
}

/* 0x020550ac: play animation `id` of the model
 * @difftest $S=zero:0xf0 @$S+0xd4:32=0x1234 $S u32 stub=0x0204ed80 cases=3 */
void func_020550ac(u8 *self, u32 id)
{
    func_0204ed80(MODEL(self), id, 0);
}

/* 0x020550c0: whether the model's animation and all the players are done
 * @difftest $M=zero:0xdc @$M+0x14:32=pick:0,1 $A=zero:0x3d00 @$A+0x3bc4:8=pick:0,1 $D=zero:8 @$D:32=$A @$D+4:32=0 $S=zero:0xf0 @$S+0xd4:32=$M @$S+0xd8:32=$D @$S+0xdc:32=int:0:3 $S cases=20 */
s32 func_020550c0(u8 *self)
{
    s32 done = 1;
    u32 i;

    if (MODEL(self) != NULL && BITS(U32_AT(MODEL(self), 0x14), 0, 1) == 0)
        return 0;
    for (i = 0; i < NPLAYERS(self); i++) {
        if (PLAYER(self, i) != NULL && S8_AT(PLAYER(self, i), 0x3bc4) != 0)
            done = 0;
    }
    return done;
}

/* 0x02055148: whether it is playing
 * @difftest ptr:0xf0:4 cases=3 */
s8 func_02055148(u8 *self)
{
    return S8_AT(self, 0xc8);
}

/* 0x02055150: stop
 * @difftest $A=zero:0x3d00 $D=zero:8 @$D:32=$A @$D+4:32=$A $S=zero:0xf0 @$S+0xd8:32=$D @$S+0xdc:32=int:0:3 $S cases=5
 * @difftest $S=zero:0xf0 @$S+0xd4:32=0x1234 $S stub=0x0204ed20 cases=2 */
void func_02055150(u8 *self)
{
    u32 i;

    self[0xc8] = 0;
    for (i = 0; i < NPLAYERS(self); i++)
        PLAYER(self, i)[0x3bc5] = 1;
    if (MODEL(self) == NULL)
        return;
    func_0204ed20(MODEL(self));
}

/* 0x020551b4: start
 * @difftest $A=zero:0x3d00 $D=zero:8 @$D:32=$A @$D+4:32=$A $S=zero:0xf0 @$S+0xd8:32=$D @$S+0xdc:32=int:0:3 $S cases=5 */
void func_020551b4(u8 *self)
{
    u32 i;

    self[0xc8] = 1;
    for (i = 0; i < NPLAYERS(self); i++) {
        u8 *e = PLAYER(self, i);
        u64 t;

        e[0x3bc4] = 1;
        e[0x3bc5] = 0;
        t = func_020822c8(func_020062d0());
        U32_AT(e, 0x3c58) = (u32)t;
        U32_AT(e, 0x3c5c) = (u32)(t >> 32);
    }
}

/* 0x0205521c: draw
 * @difftest $S=zero:0xf0 @$S+0xc8:8=pick:0,1 $S cases=3
 * @difftest $T=zero:0x18 @$T+0x14:32=0x02043814 $A=zero:0x3d00 @$A:32=$T $D=zero:8 @$D:32=$A @$D+4:32=$A $S=zero:0xf0 @$S+0xc8:8=1 @$S+0xd4:32=pick:0,0x1234 @$S+0x8c:8=pick:0,1 @$S+0xd8:32=$D @$S+0xdc:32=int:0:3 $S stub=0x02080f60,0x02080e84,0x0204e948 cases=10 */
void func_0205521c(u8 *self)
{
    u32 i;

    if (S8_AT(self, 0xc8) == 0)
        return;
    if (MODEL(self) != NULL) {
        u8 *g;

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
        func_02080e84(func_020062e0());
        func_0204e948(MODEL(self), U32_AT(self, 0xac));
        for (i = 0; i < NATTACHED(self); i++)
            func_02051068(ATTACHED(self, i), U32_AT(self, 0xac));
    }
    for (i = 0; i < NPLAYERS(self); i++)
        OBJCALL(PLAYER(self, i), 0x14)(PLAYER(self, i));
}

/* 0x0205538c
 * @difftest $T=zero:0x18 @$T+0x10:32=0x02043814 @$T:32=0x02043814 $A=zero:0x3d00 @$A:32=$T $D=zero:8 @$D:32=$A @$D+4:32=$A $S=zero:0xf0 @$S+0xd4:32=pick:0,0x1234 @$S+0xd8:32=$D @$S+0xdc:32=int:0:3 @$S+0xe4:32=$D @$S+0xe8:32=int:0:3 $S stub=0x0204ebb4 cases=10 */
void func_0205538c(u8 *self)
{
    u32 i;

    if (MODEL(self) != NULL) {
        func_0204ebb4(MODEL(self));
        for (i = 0; i < NATTACHED(self); i++)
            OBJCALL(ATTACHED(self, i), 0)(ATTACHED(self, i));
    }
    for (i = 0; i < NPLAYERS(self); i++)
        OBJCALL(PLAYER(self, i), 0x10)(PLAYER(self, i));
}

/* 0x02055424: update
 * @difftest $S=zero:0xf0 @$S+0xc8:8=pick:0,1 @$S+0x94:32=pick:0,4 @$S+0xca:8=pick:0,1 $S stub=0x02043cc8,0x020573dc cases=8 */
void func_02055424(u8 *self)
{
    u32 i;

    if (S8_AT(self, 0xc8) == 0)
        return;
    func_020555b0(self);
    if (S8_AT(self, 0xcb) != 0)
        func_0205555c(self);
    if (BITS(U32_AT(self, 0x94), 2, 1)) {
        func_02055970(self);
        return;
    }
    for (i = 0; i < NPLAYERS(self); i++)
        OBJCALL(PLAYER(self, i), 0xc)(PLAYER(self, i), 0x87);
    if (MODEL(self) != NULL) {
        func_0204efec(MODEL(self), 0x87);
        for (i = 0; i < NATTACHED(self); i++)
            func_020510bc(ATTACHED(self, i));
    }
    if (S8_AT(self, 0xca) != 0 && func_020550c0(self) != 0) {
        U32_AT(self, 0x94) |= 2;
        return;
    }
    func_02055970(self);
}

/* 0x0205555c: turn with the parent
 * @difftest $S=zero:0xf0 $S cases=2
 * @difftest $P=ptr:0x100:4 $S=zero:0xf0 @$S+0xd0:32=$P $S cases=3 */
void func_0205555c(u8 *self)
{
    s32 r[3];

    if (PTR_AT(self, 0xd0) == NULL)
        return;
    func_0201751c(r, PTR_AT(self, 0xd0));
    S32_AT(self, 0x10) = r[0];
    S32_AT(self, 0x14) = r[1];
    S32_AT(self, 0x18) = r[2];
    self[0x8c] = 1;
}

/* 0x020555b0: follow the parent
 * @difftest $P=ptr:0x100:4 @$P+0x14:32=int:0:0x6488 $S=ptr:0xf0:4 @$S+0xd0:32=$P @$S+0xdc:32=0 @$S+0xcc:8=pick:0,1 @$S+0xa0:32=int:-0x10000:0x10000 @$S+0xa8:32=int:-0x10000:0x10000 $S cases=20 */
void func_020555b0(u8 *self)
{
    s32 o[3], pp[3], out[3];
    u32 i;
    u8 *p;

    for (i = 0; i < NPLAYERS(self); i++)
        func_020573dc(PLAYER(self, i));
    p = PTR_AT(self, 0xd0);
    if (p == NULL)
        return;
    o[0] = S32_AT(self, 0xa0);
    o[1] = S32_AT(self, 0xa4);
    o[2] = S32_AT(self, 0xa8);
    if (S8_AT(self, 0xcc) == 0) {
        u32 idx = FxRadToIdx(S32_AT(p, 0x14)) >> 4;
        s32 s = FX_SinCosTable_[idx * 2], c = FX_SinCosTable_[idx * 2 + 1];
        s32 x = o[0], z = o[2];

        o[0] = FxMul(x, c) + FxMul(z, s);
        o[2] = FxMul(z, c) - FxMul(x, s);
    }
    func_02005b98(pp, PTR_AT(self, 0xd0));
    func_02029790(out, pp, o);
    S32_AT(self, 4) = out[0];
    S32_AT(self, 8) = out[1];
    S32_AT(self, 0xc) = out[2];
    self[0x8c] = 1;
}

/* 0x0205572c: attach a model of kind `a` to bone `bone`
 * @difftest $D=ptr:0x10:4 $S=zero:0xf0 @$S+0xe4:32=$D @$S+0xe8:32=int:0:2 @$S+0xec:32=4 $S u32 u32 stub=0x0207fe74:0 cases=5 */
void func_0205572c(u8 *self, u32 a, const char *bone)
{
    u8 *p = func_0207fe74(0xfc);

    if (p != NULL)
        p = func_020512c8(p, a, MODEL(self), bone);
    func_0207fe28(data_020c0a24, 0xe1);
    if (NATTACHED(self) < U32_AT(self, 0xec)) {
        NATTACHED(self)++;
        PTR_AT(PTR_AT(self, 0xe4), (NATTACHED(self) - 1) * 4) = p;
    } else {
        func_020062f0(self + 0xe4, (void **)&p, 0);
    }
    func_0207fe24();
}

/* 0x020557ec: add particle player `e`
 * @difftest $D=ptr:0x10:4 $S=zero:0xf0 @$S+0xd8:32=$D @$S+0xdc:32=int:0:2 @$S+0xe0:32=4 $S u32 cases=5 */
void func_020557ec(u8 *self, void *e)
{
    self[0xc9] = 1;
    func_0207fe28(data_020c0a24, 0xd5);
    if (NPLAYERS(self) < U32_AT(self, 0xe0)) {
        NPLAYERS(self)++;
        PTR_AT(PTR_AT(self, 0xd8), (NPLAYERS(self) - 1) * 4) = e;
    } else {
        func_020062f0(self + 0xd8, &e, 0);
    }
    func_0207fe24();
}

/* 0x02055890: follow object `p`
 * @difftest $P=ptr:0x100:4 $A=zero:0x3d00 $D=zero:8 @$D:32=$A @$D+4:32=$A $S=zero:0xf0 @$S+0xd8:32=$D @$S+0xdc:32=int:0:3 @020df10c:8=pick:0,1 $S $P cases=8 */
void func_02055890(u8 *self, u8 *p)
{
    u32 i;

    PTR_AT(self, 0xd0) = p;
    if (data_020df10c != 0) {
        s32 s = S32_AT(PTR_AT(self, 0xd0), 0x1c);

        S32_AT(self, 0x1c) = s;
        S32_AT(self, 0x20) = s;
        S32_AT(self, 0x24) = s;
        self[0x8c] = 1;
    }
    for (i = 0; i < NPLAYERS(self); i++) {
        u8 *e = PLAYER(self, i);
        s32 v[3], w[3];

        PTR_AT(e, 0x3bdc) = p;
        func_02005b98(v, p);
        S32_AT(e, 0x3be0) = v[0];
        S32_AT(e, 0x3be4) = v[1];
        S32_AT(e, 0x3be8) = v[2];
        func_02005b98(w, p);
        S32_AT(e, 0x3bec) = w[0];
        S32_AT(e, 0x3bf0) = w[1];
        S32_AT(e, 0x3bf4) = w[2];
    }
}

/* 0x02055970: queue the draw callback
 * @difftest zero:0xf0 stub=0x02043cc8 cases=3 */
void func_02055970(u8 *self)
{
    func_02043cc8((u8 *)func_02019670() + 4, self + 0xb0);
}

/* 0x02055990: unload everything
 * @difftest $T=zero:8 @$T+4:32=0x02043814 $A=zero:0x3d00 @$A:32=$T $D=zero:0xc @$D:32=$A @$D+4:32=0 @$D+8:32=$A $S=zero:0xf0 @$S+0xd8:32=$D @$S+0xdc:32=int:0:3 @$S+0xd4:32=pick:0,0x1234 @$S+0xe4:32=$D @$S+0xe8:32=0 $S stub=0x0204f964,0x0207fe44 cases=10 */
void func_02055990(u8 *self)
{
    u8 *m;
    u32 i;

    self[0xc9] = 0;
    m = MODEL(self);
    if (m != NULL) {
        func_0204f964(m);
        func_0207fe44(m);
        PTR_AT(self, 0xd4) = NULL;
    }
    for (i = 0; i < NATTACHED(self); i++) {
        u8 *a = ATTACHED(self, i);

        if (a != NULL) {
            func_020512b0(a);
            func_0207fe44(a);
        }
    }
    for (i = 0; i < NPLAYERS(self); i++) {
        u8 *e = PLAYER(self, i);

        if (e != NULL)
            OBJCALL(e, 4)(e);
    }
    S32_AT(self, 0xa0) = data_020c0980;
    S32_AT(self, 0xa4) = data_020c0974;
    S32_AT(self, 0xa8) = data_020c0984;
    self[0xcc] = 0;
    NPLAYERS(self) -= NPLAYERS(self);
    NATTACHED(self) -= NATTACHED(self);
}

/* 0x02055ad8: set up with the model of actor kind `kind` (0: none)
 * @difftest zero:0xf0 pick:0 pick:0,1 @020df10c:8=pick:0,1 stub=0x02049b78 cases=5
 * @difftest zero:0xf0 int:1:0x40 pick:0,1 stub=0x0207fe74:0,0x02049b78 cases=5 */
void func_02055ad8(u8 *self, u32 kind, s8 autoRemove)
{
    if (kind != 0) {
        u8 *m;
        s32 sc;

        self[0xc9] = 1;
        m = func_0207fe74(0xdc);
        if (m != NULL)
            m = func_0204fa44(m, kind);
        PTR_AT(self, 0xd4) = m;
        sc = S32_AT(data_020c3b0c + kind * 0x48, 0x2c);
        S32_AT(self, 0x1c) = sc;
        S32_AT(self, 0x20) = sc;
        S32_AT(self, 0x24) = sc;
        self[0x8c] = 1;
        U32_AT(self, 0x98) = kind;
    }
    S32_AT(self, 4) = 0;
    S32_AT(self, 8) = 0;
    S32_AT(self, 0xc) = 0;
    self[0x8c] = 1;
    if (data_020df10c != 0) {
        S32_AT(self, 0x10) = g_FxZero;
        S32_AT(self, 0x14) = g_FxZero;
        S32_AT(self, 0x18) = g_FxZero;
    } else {
        S32_AT(self, 0x10) = data_020b6cc4;
        S32_AT(self, 0x14) = g_FxZero;
        S32_AT(self, 0x18) = g_FxZero;
    }
    self[0x8c] = 1;
    self[0xca] = autoRemove;
    func_02049b78((u8 *)func_020062a0() + 4, ACTOR(self));
}

/* 0x02055bdc: end (and remove it if things are being removed)
 * @difftest ptr:0xf0:4 @020df100:32=pick:0,1 cases=4 */
void func_02055bdc(u8 *self)
{
    self[0xc8] = 0;
    if (data_020df100 != 0)
        U32_AT(self, 0x94) |= 2;
}

/* what both destructors do */
static void Destroy(u8 *self)
{
    PTR_AT(self, 0) = data_020c09f4;
    PTR_AT(self, 0x90) = data_020c0a14;
    func_02055990(self);
    func_02006acc(self + 0xe4);
    func_02006acc(self + 0xd8);
    PTR_AT(self, 0xb0) = data_020c0a38;
    PTR_AT(self, 0xb0) = data_020bf760;
    func_0205924c(self + 0x90);
    func_0205b784(self);
}

/* 0x02055c08: deleting destructor
 * @difftest zero:0xf0 stub=0x02006acc,0x0205924c,0x0205b784,0x0207fe44 cases=3 */
u8 *func_02055c08(u8 *self)
{
    Destroy(self);
    func_0207fe44(self);
    return self;
}

/* 0x02055c78: destructor
 * @difftest zero:0xf0 stub=0x02006acc,0x0205924c,0x0205b784 cases=3 */
u8 *func_02055c78(u8 *self)
{
    Destroy(self);
    return self;
}

/* what the constructors share: the draw callback with members m1, m2 and
 * the fields */
static void Init(u8 *self, const u32 *m1, const u32 *m2)
{
    u8 *d = self + 0xb0;
    u32 a[2], b[2];

    b[0] = m2[0];
    b[1] = m2[1];
    a[0] = m1[0];
    a[1] = m1[1];
    PTR_AT(d, 0) = data_020bf760;
    PTR_AT(d, 0) = data_020c0a38;
    PTR_AT(d, 4) = self;
    U32_AT(d, 8) = a[0];
    U32_AT(d, 0xc) = a[1];
    U32_AT(d, 0x10) = b[0];
    U32_AT(d, 0x14) = b[1];
    self[0xc8] = 0;
    self[0xc9] = 0;
    self[0xca] = 1;
    self[0xcb] = 0;
    self[0xcc] = 0;
    U32_AT(self, 0xd0) = 0;
    U32_AT(self, 0xd4) = 0;
    U32_AT(self, 0xd8) = 0;
    U32_AT(self, 0xdc) = 0;
    U32_AT(self, 0xe0) = 0;
    U32_AT(self, 0xe4) = 0;
    U32_AT(self, 0xe8) = 0;
    U32_AT(self, 0xec) = 0;
}

/* 0x02055ce0: constructor: room for `nplayers` players and `nattached`
 * attached models
 * @difftest ptr:0xf0:4 int:0:3 int:0:3 stub=0x0205b8c8,0x020592a8,0x02006330 cases=5 */
u8 *func_02055ce0(u8 *self, u32 nplayers, u32 nattached)
{
    func_0205b8c8(self);
    func_020592a8(self + 0x90);
    PTR_AT(self, 0) = data_020c09f4;
    PTR_AT(self, 0x90) = data_020c0a14;
    S32_AT(self, 0xa0) = data_020c096c;
    S32_AT(self, 0xa4) = data_020c0988;
    S32_AT(self, 0xa8) = data_020c097c;
    Init(self, data_020c09b0, data_020c09b8);
    func_0207fe28(data_020c0a24, 0x42);
    if (nplayers > U32_AT(self, 0xe0))
        func_02006330(self + 0xd8, nplayers);
    if (nattached > U32_AT(self, 0xec))
        func_02006330(self + 0xe4, nattached);
    func_0207fe24();
    NPLAYERS(self) -= NPLAYERS(self);
    NATTACHED(self) -= NATTACHED(self);
    U32_AT(self, 0xac) = 0x1f;
    return self;
}

/* 0x02055e94: the draw callback's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_02055e94(u8 *cb)
{
    PTR_AT(cb, 0) = data_020c0a38;
    PTR_AT(cb, 0) = data_020bf760;
    return cb;
}

/* 0x02055eb0: constructor: room for `nplayers` players
 * @difftest ptr:0xf0:4 int:0:3 stub=0x0205b8c8,0x020592a8,0x02006330 cases=5 */
u8 *func_02055eb0(u8 *self, u32 nplayers)
{
    func_0205b8c8(self);
    func_020592a8(self + 0x90);
    PTR_AT(self, 0) = data_020c09f4;
    PTR_AT(self, 0x90) = data_020c0a14;
    S32_AT(self, 0xa0) = data_020c098c;
    S32_AT(self, 0xa4) = data_020c0970;
    S32_AT(self, 0xa8) = data_020c0964;
    Init(self, data_020c0998, data_020c0990);
    func_0207fe28(data_020c0a24, 0x2e);
    if (nplayers > U32_AT(self, 0xe0))
        func_02006330(self + 0xd8, nplayers);
    func_0207fe24();
    U32_AT(self, 0xac) = 0x1f;
    NPLAYERS(self) -= NPLAYERS(self);
    return self;
}

/* 0x0205602c: constructor
 * @difftest ptr:0xf0:4 stub=0x0205b8c8,0x020592a8 cases=3 */
u8 *func_0205602c(u8 *self)
{
    func_0205b8c8(self);
    func_020592a8(self + 0x90);
    PTR_AT(self, 0) = data_020c09f4;
    PTR_AT(self, 0x90) = data_020c0a14;
    S32_AT(self, 0xa0) = data_020c0960;
    S32_AT(self, 0xa4) = data_020c0978;
    S32_AT(self, 0xa8) = data_020c0968;
    Init(self, data_020c09a0, data_020c09a8);
    U32_AT(self, 0xac) = 0x1f;
    return self;
}

/* 0x02056158: the draw callback's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_02056158(u8 *cb)
{
    PTR_AT(cb, 0) = data_020c0a38;
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

/* 0x02056188: call the draw callback's first member function
 * @difftest $D=zero:0x18 @$D+8:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+8:32=8 @$D+0xc:32=pick:1,9 $D cases=5 */
void func_02056188(u8 *cb)
{
    if (PTR_AT(cb, 4) == NULL)
        return;
    if (PTR_AT(cb, 8) == NULL)
        return;
    CallMember(PTR_AT(cb, 4), cb + 8);
}

/* 0x020561e8: call the draw callback's second member function
 * @difftest $D=zero:0x18 @$D+0x10:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+0x10:32=8 @$D+0x14:32=pick:1,9 $D cases=5 */
void func_020561e8(u8 *cb)
{
    if (PTR_AT(cb, 4) == NULL)
        return;
    if (PTR_AT(cb, 0x10) == NULL)
        return;
    CallMember(PTR_AT(cb, 4), cb + 0x10);
}

/* The task's this-adjusting entry points (the object is 0x90 before). */

/* 0x02056248
 * @difftest $S=zero:0xf0 $S+0x90 stub=0x02006acc,0x0205924c,0x0205b784,0x0207fe44 cases=3 */
u8 *func_02056248(u8 *a)
{
    return func_02055c08(a - 0x90);
}

/* 0x02056258
 * @difftest $S=zero:0xf0 $S+0x90 pick:0,1 cases=3 */
void func_02056258(u8 *a, s32 on)
{
    func_02055018(a - 0x90, on);
}

/* 0x02056268
 * @difftest $S=zero:0xf0 $S+0x90 cases=3 */
void func_02056268(u8 *a)
{
    func_02055424(a - 0x90);
}

/* 0x02056278
 * @difftest $S=zero:0xf0 $S+0x90 stub=0x02006acc,0x0205924c,0x0205b784 cases=3 */
u8 *func_02056278(u8 *a)
{
    return func_02055c78(a - 0x90);
}
