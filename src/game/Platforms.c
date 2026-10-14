/*
 * Platforms: map objects of actor class 0x11 that the hero can stand on
 * (some rise when he stands on them, kind 0x11b; some follow a path of
 * points, func_020088e4). A platform shows a message when the hero gets
 * on (+0x78) or off (+0x79) in his other form.
 * ARM9 main, 0x02046154 - 0x02047780 (23 functions).
 *
 * The object (0x80 bytes, vtable data_020bfa38) is an actor with a draw
 * callback at +0x4c (vtables data_020bfa8c / data_020bf760, member
 * functions data_020bfa20 / data_020bfa28). +0x64 last position, +0x6c
 * wait at a path point, +0x70 height, +0x74 rise, +0x7a the hero is
 * below, +0x7b waiting, +0x7c state saved with the game, +0x7d time the
 * hero has stood on a rising platform. The hero's actor (data_020deebc
 * +4) keeps the platform he stands on at +0xa4.
 *
 * Testing: with fake actors; the message system, the model set-up, the
 * height map and the camera stubbed or faked where they need real data.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020b6cc4;
extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcb94[];           /* RTTI: an actor */
extern const fx32 data_020bf9fc, data_020bfa04, data_020bfa0c, data_020bfa10;
extern const s32 data_020bfa14[];    /* push speed */
extern const s32 data_020bfa00, data_020bfa08, data_020bfa18, data_020bfa1c;   /* a box */
extern const u32 data_020bfa20[2], data_020bfa28[2];   /* the member functions */
extern u8 data_020bfa38[];           /* the platform's vtable */
extern u8 data_020bfa8c[], data_020bf760[];            /* the callback's vtables */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */

void func_02051370(void *stream, s32 v);
s32 func_02051430(void *stream);
s32 func_02070d7c(void *p);
void func_02070f44(void *p);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
void func_020038d4(s32 *out, const s32 *in, s32 shift);
s32 func_0201a0a0(s32 *xz, void *obj);
u32 func_02019650(void);
void *func_02016f24(void *self);
s32 func_0202df68(const s32 *v);
void func_0202dea4(s32 *v);
void func_0202ded8(s32 *v, const s32 *s);
void func_020297d4(s32 *out, const s32 *v, const s32 *s);
void *func_02009394(void);
void func_0205f6e4(void *obj, u32 v);
void *func_020a6efc(void *p, void *from, void *to, s32 v);
u32 func_020186ec(void);
void func_0201c17c(s32 *out, s32 *pos);
void *func_020062e0(void);
void *func_020062c0(void);
void *func_020062b0(void);
void func_020040f4(u8 *dst, const u8 *src);
void func_02082ce8(void *cam);
void *func_02019670(void);
void func_02043cc8(u8 *list, void *item);
void func_02053440(void *model);
void func_02053460(void *obj, u32 dt);
void func_02017abc(void *list, void *item);
void func_020504b8(void *actor);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
void func_0204ed80(void *anim, u32 id, u32 a);
void func_020088e4(s32 *out, s32 i, s32 k);
void func_020088a8(s32 i, s32 k);
void func_02008130(s32 group, s32 index);
void *func_020506c0(void *actor);
void func_0207fe44(void *p);
void func_02050764(void *actor, s32 *pos);
u8 *func_02008994(s32 i, s32 k);
s32 func_0200820c(u32 id, s32 kind);
void *func_0207fe74(u32 size);
s32 func_0208f070(s32 num, s32 den);
fx32 func_02001ef4(u32 dir);

s32 func_020461d4(u8 *self);
s32 func_02046244(u8 *self);
void func_02046268(u8 *self, u32 v);
void func_02046274(u8 *self, u32 v);
void func_02046280(u8 *self, u8 v);
void func_02046288(u8 *self, u8 v);
s32 func_02046880(u8 *self);
void func_02046bf4(u8 *self);
void func_02046c2c(u8 *self, u32 kind);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define HERO_ACTOR() ((u8 *)PTR_AT(data_020deebc, 4))

/* the actor's appearance (its method +0x38) */
static u8 *Look(u8 *self)
{
    u8 *(*m)(void *, void *) = VCALL(self, 0x38, u8 *(*)(void *, void *));

    return m(self, (void *)m);
}

/* 0x02046154: save (or load) the platform's state
 * @difftest $T=zero:0x40 @$T+0x38:32=0x020343a8 $S=zero:0x80 @$S+0:32=$T @020ebbe4:32=pick:0,1 $S 0x020ebbc8 stub=0x02070d7c:5,0x02051370,0x02051430:1 cases=4 */
void func_02046154(u8 *self, u8 *stream)
{
    if (S32_AT(stream, 0x1c) == 0) {
        func_02051370(stream, func_02070d7c(Look(self)));
        return;
    }
    self[0x7c] = func_02051430(stream) != 0;
}

/* 0x020461d4: the height of the platform's top
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0x80 @$S+8:32=pick:0x11b,3 @$S+0x70:32=int:0:0x100000 @$S+0x74:32=int:0:0x100000 $S cases=10 */
s32 func_020461d4(u8 *self)
{
    s32 v[2], t[2], u[2], h;

    v[0] = S32_AT(self, 0x28);
    v[1] = S32_AT(self, 0x2c);
    func_02003b08(t, v, 4);
    v[0] = t[0];
    v[1] = t[1];
    u[0] = t[0];
    u[1] = t[1];
    h = func_02046244(self);
    return h + func_0201a0a0(u, NULL);
}

/* 0x02046244: the platform's height
 * @difftest $S=zero:0x80 @$S+8:32=pick:0x11b,3 @$S+0x70:32=int:0:0x100000 @$S+0x74:32=int:0:0x100000 $S cases=10 */
s32 func_02046244(u8 *self)
{
    if (S32_AT(self, 8) == 0x11b)
        return S32_AT(self, 0x74);
    return S32_AT(self, 0x70) + S32_AT(self, 0x74);
}

/* 0x02046268: set the rise
 * @difftest ptr:0x80 int:0:0x100 cases=3 */
void func_02046268(u8 *self, u32 v)
{
    S32_AT(self, 0x74) = v << 16;
}

/* 0x02046274: set the height
 * @difftest ptr:0x80 int:0:0x100 cases=3 */
void func_02046274(u8 *self, u32 v)
{
    S32_AT(self, 0x70) = v << 16;
}

/* 0x02046280: set the getting-off message
 * @difftest ptr:0x80 u8 cases=3 */
void func_02046280(u8 *self, u8 v)
{
    self[0x79] = v;
}

/* 0x02046288: set the getting-on message
 * @difftest ptr:0x80 u8 cases=3 */
void func_02046288(u8 *self, u8 v)
{
    self[0x78] = v;
}

/* the hero (form flag `was`) got on platform `self` at height `top` */
static void GotOn(u8 *self, s32 was, s32 top, s32 quiet)
{
    PTR_AT(HERO_ACTOR(), 0xa4) = self;
    U32_AT(HERO(), 0x38) |= 0x20000;
    if (was != (s8)BITS(U32_AT(HERO(), 0x38), 17, 1)) {
        data_020deebc[0x20 / 4] |= 0x10;
        if (was == 0) {
            if (!quiet || S32_AT(self, 8) != 0x11b)
                func_0205f6e4(func_02009394(), self[0x78]);
        } else {
            func_0205f6e4(func_02009394(), self[0x79]);
        }
    }
    if (S32_AT(self, 8) == 0x11b)
        S32_AT(HERO_ACTOR(), 0x68) = top + 0x10000;
    else
        S32_AT(HERO_ACTOR(), 0x68) = top;
}

/* 0x02046290: touched: carry the hero (or another actor) on top
 * @difftest $Y=zero:0x10 @020def5c:32=$Y $G=zero:0x200 @020df578:32=$G+0x80 $T=zero:0x40 @$T+0x38:32=0x020343a8 $H=zero:0x80 @$H+0x38:32=pick:0,0x20000 $A=zero:0x120 @$A+0x24:32=int:-0x40000:0x40000 @$A+0x38:32=pick:0,0x800000 @$A+0x68:32=0 @020deec0:32=$A @020deec4:32=$H $S=zero:0x80 @$S+0:32=$T @$S+8:32=pick:3,0xf0,0x11b,0x11d,0x121,0x12f @$S+0x24:32=pick:0,0x80 @$S+0x70:32=int:0:0x40000 @$S+0x74:32=int:0:0x40000 @$S+0x78:8=1 @$S+0x79:8=2 $O=zero:0x60 @$O+8:32=1 $S $O stub=0x02019650:3,0x02016f24:0x020deeb0,0x0205f6e4 cases=100
 * @difftest $Y=zero:0x10 @020def5c:32=$Y $G=zero:0x200 @020df578:32=$G+0x80 $T=zero:0x40 @$T+0x38:32=0x020343a8 $H=zero:0x80 $P=zero:0x80 @$P+0x28:32=int:-0x10000:0x10000 @$P+0x2c:32=int:-0x10000:0x10000 $A=zero:0x120 @$A+0x24:32=0x100000 @$A+0xa4:32=$P @020deec0:32=$A @020deec4:32=$H $S=zero:0x80 @$S+0:32=$T @$S+8:32=3 @$S+0x28:32=int:-0x10000:0x10000 @$S+0x2c:32=int:-0x10000:0x10000 $O=zero:0x60 @$O+8:32=1 @$O+0x28:32=int:-0x10000:0x10000 @$O+0x2c:32=int:-0x10000:0x10000 $S $O stub=0x02019650:3,0x02016f24:0x020deeb0,0x0205f6e4 cases=50
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $T=zero:0x40 @$T+0x38:32=0x020343a8 $H=zero:0x80 $A=zero:0x120 @020deec0:32=$A @020deec4:32=$H $S=zero:0x80 @$S+0:32=$T @$S+8:32=3 @$S+0x24:32=pick:0,0x80 @$S+0x28:32=int:-0x10000:0x10000 @$S+0x2c:32=int:-0x10000:0x10000 $O=zero:0x60 @$O+8:32=1 @$O+0x28:32=int:-0x10000:0x10000 @$O+0x2c:32=int:-0x10000:0x10000 $Y=zero:0x10 @$Y+4:32=$S @020def5c:32=$Y $S $O stub=0x02019650:1,0x02016f24:0x020deeb0,0x02070f44 cases=10
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $T=zero:0x40 @$T+0x38:32=0x020343a8 $H=zero:0x80 $A=zero:0x120 @020deec0:32=$A @020deec4:32=$H $S=zero:0x80 @$S+0:32=$T @$S+8:32=3 @$S+0x24:32=pick:0,0x80 @$S+0x28:32=int:-0x10000:0x10000 @$S+0x2c:32=int:-0x10000:0x10000 $O=zero:0x60 @$O+8:32=1 @$O+0x28:32=int:-0x10000:0x10000 @$O+0x2c:32=int:-0x10000:0x10000 $Y=zero:0x10 @$Y+4:32=$S @020def5c:32=$Y $S $O stub=0x02019650:3,0x02016f24:0x020deeb0 cases=10
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0x80 @$S+8:32=3 @$S+0x70:32=int:0:0x40000 $C=zero:0x120 @$C+0x24:32=int:0:0x80000 $S $C stub=0x020a6efc cases=10 */
void func_02046290(u8 *self, u8 *other)
{
    if (S32_AT(other, 0xc) == 0 && S32_AT(other, 8) == 1) {
        s32 y = S32_AT(HERO_ACTOR(), 0x24);
        s32 top = func_020461d4(self);
        s32 was;
        u8 *cur;

        if ((S32_AT(self, 8) == 0xf0 || S32_AT(self, 8) == 0x11d)
            && (func_02019650() == 0xc || (s8)BITS(U32_AT(HERO_ACTOR(), 0x38), 23, 1) != 0)) {
            self[0x7a] = 0;
            return;
        }
        if (PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) == self) {
            s32 a[2], b[2], v[3], t[3], len;
            u8 *h;

            if ((s8)BITS(U32_AT(Look(self), 0x24), 7, 1) == 0)
                return;
            if (PTR_AT(HERO_ACTOR(), 0xa4) == self)
                return;
            if (func_02019650() == 1) {
                U32_AT(HERO(), 0x38) &= ~0x200u;
                h = HERO();
                PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = h;
                func_02070f44(Look(self));
                return;
            }
            a[0] = S32_AT(self, 0x28);
            a[1] = S32_AT(self, 0x2c);
            b[0] = S32_AT(other, 0x28);
            b[1] = S32_AT(other, 0x2c);
            v[0] = b[0] - a[0];
            v[1] = data_020bfa10;
            v[2] = b[1] - a[1];
            len = func_0202df68(v);
            if (len > 1)
                func_0202ded8(v, &len);
            else
                func_0202dea4(v);
            func_020297d4(t, v, data_020bfa14);
            S32_AT(HERO_ACTOR(), 0x100) = t[0];
            S32_AT(HERO_ACTOR(), 0x104) = t[2];
            return;
        }
        if (y < top && S32_AT(self, 8) != 0x11b) {
            if (S32_AT(self, 8) == 0x12f || S32_AT(self, 8) == 0x130)
                return;
            if (S32_AT(self, 8) != 0x121)
                self[0x7a] = 1;
            return;
        }
        U32_AT(self, 0x38) |= 8;
        was = (s8)BITS(U32_AT(HERO(), 0x38), 17, 1);
        cur = PTR_AT(HERO_ACTOR(), 0xa4);
        if (cur != NULL && cur != self) {
            s32 dx = S32_AT(self, 0x28) - S32_AT(other, 0x28);
            s32 dy = data_020bfa0c;
            s32 dz = S32_AT(self, 0x2c) - S32_AT(other, 0x2c);
            s32 dy2 = data_020bfa04;
            s32 dz2 = S32_AT(cur, 0x2c) - S32_AT(other, 0x2c);
            s32 dx2 = S32_AT(cur, 0x28) - S32_AT(other, 0x28);

            if (FxMul(dz, dz) + (FxMul(dx, dx) + FxMul(dy, dy))
                >= FxMul(dz2, dz2) + (FxMul(dx2, dx2) + FxMul(dy2, dy2)))
                return;
            GotOn(self, was, top, 0);
            return;
        }
        GotOn(self, was, top, 1);
        return;
    }
    if (S32_AT(other, 0xc) != 5)
        return;
    {
        u8 *o = other;
        s32 u[2], v[2], t[2], y, top;

        if (o != NULL)
            o = func_020a6efc(o, data_020bcb88, data_020bcb94, -1);
        u[0] = S32_AT(o, 0x54);
        u[1] = S32_AT(o, 0x58);
        y = S32_AT(o, 0x24);
        y += func_0201a0a0(u, NULL);
        v[0] = S32_AT(self, 0x28);
        v[1] = S32_AT(self, 0x2c);
        func_02003b08(t, v, 4);
        top = func_020461d4(self);
        if (y < top)
            return;
        U32_AT(self, 0x38) |= 8;
        PTR_AT(o, 0xa4) = self;
        U32_AT(other, 0x38) |= 0x20000;
        S32_AT(o, 0x68) = top;
    }
}

/* 0x02046880: place the platform's model and draw it if in view
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $O=zero:0x200 $A=zero:0x120 @020deec0:32=$A $S=zero:0x80 @$S+0x44:32=$O @$S+8:32=pick:3,0xf0 @$S+0x38:32=pick:0x20,0x40,0x60,0x44,0x40064 @$S+0x74:32=pick:0,0x10000 @$S+0x7a:8=pick:0,1 @$S+0x10:32=int:-0x10:0 @$S+0x14:32=int:-0x10:0 @$S+0x18:32=int:0:0x10 @$S+0x1c:32=int:0:0x10 $S stub=0x0201f490:0x020deebc,0x020186ec:0,0x02043cc8,0x02053440,0x02017abc cases=40
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $O=zero:0x200 $S=zero:0x80 @$S+0x44:32=$O @$S+0x38:32=0x40 @$S+0x74:32=0 @020deec0:32=$S @$S+0xa4:32=$S $S stub=0x0201f490:0x020deebc,0x020186ec:1,0x02043cc8,0x02053440,0x02017abc cases=5 */
s32 func_02046880(u8 *self)
{
    s32 xz[2], p[3], b[4];
    u32 cam[0x2c0 / 4];
    u8 *o;
    s32 vis;

    if (!(BITS(U32_AT(self, 0x38), 6, 1) != 0 && func_020186ec() == 1)) {
        if (BITS(U32_AT(self, 0x38), 5, 1) == 0)
            return 0;
        if (func_020186ec() != 0)
            return 0;
    }
    xz[0] = S32_AT(self, 0x28);
    xz[1] = S32_AT(self, 0x2c);
    func_0201c17c(p, xz);
    p[1] += S32_AT(self, 0x74) >> 4;
    o = PTR_AT(self, 0x44);
    S32_AT(o, 4) = p[0];
    S32_AT(o, 8) = p[1];
    S32_AT(o, 0xc) = p[2];
    o[0x8c] = 1;
    if (S8_AT(self, 0x7a) != 0 && S32_AT(self, 0x74) > 0) {
        o = PTR_AT(self, 0x44);
        S32_AT(o, 0xc) += 0x64000;
        o[0x8c] = 1;
    } else if (PTR_AT(HERO_ACTOR(), 0xa4) == self) {
        s32 h = func_02046244(self);

        o = PTR_AT(self, 0x44);
        S32_AT(o, 0xc) = p[2] - (h >> 4);
        o[0x8c] = 1;
        if (func_02046244(self) == 0) {
            o = PTR_AT(self, 0x44);
            S32_AT(o, 0xc) = p[2] - 0xa0000;
            o[0x8c] = 1;
        }
    }
    b[0] = S32_AT(self, 0x10);
    b[1] = S32_AT(self, 0x14);
    b[2] = S32_AT(self, 0x18);
    b[3] = S32_AT(self, 0x1c);
    if (S32_AT(self, 8) == 0xf0 || S32_AT(self, 8) == 0x11d) {
        b[0] = data_020bfa00;
        b[1] = data_020bfa1c;
        b[2] = data_020bfa18;
        b[3] = data_020bfa08;
    }
    o = PTR_AT(self, 0x44);
    b[0] = S32_AT(o, 8) + SHL(b[0], 12);
    b[1] = S32_AT(o, 4) + SHL(b[1], 12);
    b[2] = S32_AT(o, 8) + SHL(b[2], 12);
    b[3] = S32_AT(o, 4) + SHL(b[3], 12);
    func_020040f4((u8 *)cam, func_020062e0());
    vis = 0;
    if (S8_AT(cam, 0x284) != 0) {
        vis = S32_AT(cam, 0x2ac) >= b[1] && S32_AT(cam, 0x2a4) <= b[3]
              && S32_AT(cam, 0x2a0) <= b[2] && S32_AT(cam, 0x2a8) >= b[0];
    }
    func_02082ce8(cam);
    if (vis != 0) {
        func_02043cc8((u8 *)func_02019670() + 4, self + 0x4c);
        func_02053440(PTR_AT(self, 0x44));
        if (BITS(U32_AT(self, 0x38), 2, 1) != 0)
            func_02017abc((u8 *)func_020062c0() + 4, self);
        U32_AT(self, 0x38) |= 0x2000;
    } else if ((s8)BITS(U32_AT(self, 0x38), 18, 1) != 0) {
        U32_AT(self, 0x38) &= ~0x2000u;
        if (BITS(U32_AT(self, 0x38), 2, 1) != 0)
            func_02017abc((u8 *)func_020062c0() + 4, self);
    } else {
        U32_AT(self, 0x38) &= ~0x2000u;
    }
    return 0;
}

/* 0x02046bf4: the hero is off the platform
 * @difftest $H=zero:0x80 @$H+0x38:32=0x20000 $A=zero:0x120 @020deec0:32=$A @020deec4:32=$H $S=zero:0x80 @$A+0xa4:32=pick:0,1 $S cases=5
 * @difftest $H=zero:0x80 @$H+0x38:32=0x20000 $S=zero:0x80 @020deec0:32=$S @$S+0xa4:32=$S @020deec4:32=$H $S cases=3 */
void func_02046bf4(u8 *self)
{
    u8 *a = HERO_ACTOR();

    if (PTR_AT(a, 0xa4) != self)
        return;
    PTR_AT(a, 0xa4) = NULL;
    U32_AT(HERO(), 0x38) &= ~0x20000u;
}

/* 0x02046c2c: set up a platform of actor kind `kind`
 * @difftest $O=zero:0x200 $S=zero:0x80 @$S+0x44:32=$O $S pick:3,0xf0,0xf6,0x11c,0x121 stub=0x020504b8,0x02053648,0x0204ed80,0x02017abc cases=10 */
void func_02046c2c(u8 *self, u32 kind)
{
    u8 *o;

    S32_AT(self, 0xc) = 0x11;
    S32_AT(self, 8) = kind;
    func_020504b8(self);
    func_02053648(PTR_AT(self, 0x44), kind, self, 0, 0);
    o = PTR_AT(self, 0x44);
    S32_AT(o, 4) = 0;
    S32_AT(o, 8) = 0;
    S32_AT(o, 0xc) = 0;
    o[0x8c] = 1;
    o = PTR_AT(self, 0x44);
    S32_AT(o, 0x10) = data_020b6cc4;
    S32_AT(o, 0x14) = g_FxZero;
    S32_AT(o, 0x18) = g_FxZero;
    o[0x8c] = 1;
    if (kind == 0x121)
        func_0204ed80((u8 *)PTR_AT(self, 0x44) + 0xb0, 0x292, 0);
    if (kind == 0xf6)
        func_0204ed80((u8 *)PTR_AT(self, 0x44) + 0xb0, 0x286, 0);
    if (kind == 0x11c)
        func_0204ed80((u8 *)PTR_AT(self, 0x44) + 0xb0, 0x45c, 0);
    if (kind == 0xf0)
        func_0204ed80((u8 *)PTR_AT(self, 0x44) + 0xb0, 0x463, 0);
    U32_AT(self, 0x38) = (U32_AT(self, 0x38) & ~1u) | 1;
    U32_AT(self, 0x38) |= 4;
    U32_AT(self, 4) |= 8;
    func_02017abc((u8 *)func_020062b0() + 4, self);
}

/* 0x02046d7c: the model's method +0xc
 * @difftest $T=zero:0x20 @$T+0xc:32=0x020343a8 $O=zero:0x10 @$O+0:32=$T $S=zero:0x80 @$S+0x44:32=$O $S cases=3 */
void func_02046d7c(u8 *self)
{
    u8 *o = PTR_AT(self, 0x44);
    void (*m)(void *, void *) = VCALL(o, 0xc, void (*)(void *, void *));

    m(o, (void *)m);
}

/* 0x02046da0: the model's method +8
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x10 @$O+0:32=$T $S=zero:0x80 @$S+0x44:32=$O $S cases=3 */
void func_02046da0(u8 *self)
{
    u8 *o = PTR_AT(self, 0x44);
    void (*m)(void *, void *) = VCALL(o, 8, void (*)(void *, void *));

    m(o, (void *)m);
}

/* 0x02046dc4: per-frame update (rising, following the path)
 * @difftest $O=zero:0x200 $S=zero:0x80 @$S+0x44:32=$O @$S+4:32=pick:0,4 @$S+8:32=pick:0x11b,0x11b,3 @$S+0x38:32=pick:0,8 @$S+0x70:32=0x40000 @$S+0x74:32=int:0:0x50000 @$S+0x7b:8=pick:0,0,1 @$S+0x6c:32=int:1:3 @$S+0x7d:8=int:8:12 $S int:0:0x2000 stub=0x02053460,0x0205f6e4 cases=60
 * @difftest $T=zero:0x40 @$T+0x38:32=0x020343a8 $O=zero:0x200 $S=zero:0x90 @$S+0x84:8=int:0:3 @$S+0x85:8=int:0:4 @$S+0x89:8=int:0:4 @$S+0x8a:8=int:0:3 @$S+0:32=$T @$S+0x44:32=$O @$S+8:32=3 @$S+0x38:32=0x800000 @$S+0x24:32=0x80 @$S+0x28:32=int:-0x8000:0x8000 @$S+0x2c:32=int:-0x8000:0x8000 $S int:0:0x2000 stub=0x02053460,0x020088e4,0x020088a8 cases=40 */
void func_02046dc4(u8 *self, u32 dt)
{
    S32_AT(self, 0x64) = S32_AT(self, 0x28);
    S32_AT(self, 0x68) = S32_AT(self, 0x2c);
    if ((s8)BITS(U32_AT(self, 4), 2, 1) == 0) {
        if (S32_AT(self, 8) == 0x11b) {
            u32 on = BITS(U32_AT(self, 0x38), 3, 1);

            if (on == 0 && S32_AT(self, 0x74) > 0) {
                S32_AT(self, 0x74) -= 0x10000;
                if (S32_AT(self, 0x74) < 0) {
                    self[0x7d] = 0;
                    S32_AT(self, 0x74) = 0;
                }
            } else if (on != 0) {
                if (self[0x7d] < 0xa) {
                    self[0x7d]++;
                    if (self[0x7d] == 0xa)
                        func_0205f6e4(func_02009394(), self[0x78]);
                } else {
                    S32_AT(self, 0x74) += 0x10000;
                }
                if (S32_AT(self, 0x74) > S32_AT(self, 0x70))
                    S32_AT(self, 0x74) = S32_AT(self, 0x70);
            }
        }
        if (S8_AT(self, 0x7b) != 0) {
            S32_AT(self, 0x6c)--;
            if (S32_AT(self, 0x6c) == 0)
                self[0x7b] = 0;
        } else if ((s8)BITS(U32_AT(self, 0x38), 23, 1) != 0
                   && (s8)BITS(U32_AT(Look(self), 0x24), 7, 1) != 0) {
            s32 a[2], n[2], m[2], v[3], len, d;
            u32 k;

            a[0] = S32_AT(self, 0x28);
            a[1] = S32_AT(self, 0x2c);
            k = Look(self)[0x85];
            func_020088e4(n, Look(self)[0x84], k);
            func_020038d4(m, n, 4);
            n[0] = m[0];
            n[1] = m[1];
            v[0] = n[0] - a[0];
            v[1] = data_020bf9fc;
            v[2] = n[1] - a[1];
            d = FxMul(v[2], v[2]) + (FxMul(v[0], v[0]) + FxMul(v[1], v[1]));
            len = func_0202df68(v);
            if (len > 1)
                func_0202ded8(v, &len);
            else
                func_0202dea4(v);
            if (d < 0x5000) {
                func_020088a8(Look(self)[0x84], k);
                if (k != Look(self)[0x89])
                    Look(self)[0x85] = k + 1;
                else
                    Look(self)[0x85] = 0;
                S32_AT(self, 0x6c) = Look(self)[0x8a];
                if (S32_AT(self, 0x6c) != 0)
                    self[0x7b] = 1;
                S32_AT(self, 0x28) = a[0];
                S32_AT(self, 0x2c) = a[1];
            } else {
                a[0] += v[0];
                a[1] += v[2];
                S32_AT(self, 0x28) = a[0];
                S32_AT(self, 0x2c) = a[1];
            }
        }
    }
    func_02053460(PTR_AT(self, 0x44), dt);
    if (BITS(U32_AT(self, 4), 3, 1) != 0)
        func_02046880(self);
    self[0x7a] = 0;
    U32_AT(self, 0x38) &= ~8u;
}

/* 0x02047150
 * @difftest ptr:0x80 cases=3 */
void func_02047150(u8 *self)
{
    U32_AT(self, 0x38) |= 2;
}

/* what both destructors do */
static void Destroy(u8 *self)
{
    PTR_AT(self, 0) = data_020bfa38;
    func_02008130(S8_AT(self, 0x42), S16_AT(self, 0x40));
    func_02046bf4(self);
    PTR_AT(self, 0x4c) = data_020bfa8c;
    PTR_AT(self, 0x4c) = data_020bf760;
    func_020506c0(self);
}

/* 0x02047164: the platform's deleting destructor
 * @difftest $A=zero:0x120 @020deec0:32=$A $S=zero:0x80 $S stub=0x02008130,0x020506c0,0x0207fe44 cases=3 */
u8 *func_02047164(u8 *self)
{
    Destroy(self);
    func_0207fe44(self);
    return self;
}

/* 0x020471c0: the platform's destructor
 * @difftest $A=zero:0x120 @020deec0:32=$A $S=zero:0x80 $S stub=0x02008130,0x020506c0 cases=3 */
u8 *func_020471c0(u8 *self)
{
    Destroy(self);
    return self;
}

/* 0x02047214: the platform's constructor (actor kind `kind` at `pos`)
 * @difftest zero:0x80 int:0:0x130 ptr:8:4 stub=0x02050764,0x02046c2c cases=5 */
u8 *func_02047214(u8 *self, u32 kind, s32 *pos)
{
    u8 *d = self + 0x4c;
    u32 m1[2], m2[2];

    func_02050764(self, pos);
    PTR_AT(self, 0) = data_020bfa38;
    m2[0] = data_020bfa28[0];
    m2[1] = data_020bfa28[1];
    m1[0] = data_020bfa20[0];
    m1[1] = data_020bfa20[1];
    PTR_AT(d, 0) = data_020bf760;
    PTR_AT(d, 0) = data_020bfa8c;
    PTR_AT(d, 4) = self;
    U32_AT(d, 8) = m1[0];
    U32_AT(d, 0xc) = m1[1];
    U32_AT(d, 0x10) = m2[0];
    U32_AT(d, 0x14) = m2[1];
    S32_AT(self, 0x64) = pos[0];
    S32_AT(self, 0x68) = pos[1];
    S32_AT(self, 0x6c) = 0;
    S32_AT(self, 0x70) = 0;
    S32_AT(self, 0x74) = 0;
    self[0x78] = 0xff;
    self[0x79] = 0xff;
    self[0x7a] = 0;
    self[0x7b] = 0;
    self[0x7c] = 0;
    self[0x7d] = 0;
    func_02046c2c(self, kind);
    return self;
}

/* 0x02047304: the callback's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_02047304(u8 *self)
{
    PTR_AT(self, 0) = data_020bfa8c;
    PTR_AT(self, 0) = data_020bf760;
    return self;
}

/* an int made fx32 through the game's float routines (rounded away from 0
 * when `up`) */
static s32 Fx(s32 v, s32 up)
{
    float f = (float)SHL(v, 12);

    return (s32)(up ? 0.5f + f : f - 0.5f);
}

/* 0x02047320: create platform `id` (of map object list `k`)
 * @difftest int:0:4 int:0:2 cases=5 */
u8 *func_02047320(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    s32 v[2], t[2];
    u32 kind;
    u8 *p, *o;
    s32 r9, r8, sx, sz, w, sc;
    u32 off;

    v[0] = S16_AT(def, 4);
    v[1] = S16_AT(def, 6);
    func_02003b08(t, v, 0xc);
    v[0] = t[0];
    v[1] = t[1];
    kind = func_0200820c(U16_AT(def, 8), 0xa);
    p = func_0207fe74(0x80);
    if (p != NULL)
        p = func_02047214(p, kind, v);
    U32_AT(p, 0x38) = (U32_AT(p, 0x38) & ~0x20u) | ((def[0x11] & 1) << 5);
    U32_AT(p, 0x38) = (U32_AT(p, 0x38) & ~0x40u) | ((def[0x12] & 1) << 6);
    func_02046288(p, def[0x13]);
    func_02046280(p, def[0x14]);
    func_02046274(p, def[0xf]);
    func_02046268(p, def[0x10]);
    U32_AT(p, 0x38) = (U32_AT(p, 0x38) & ~0x800000u) | ((S8_AT(def, 0x15) & 1) << 23);
    if (def[0x10] == 0)
        U32_AT(p, 0x38) |= 0x4000;
    else
        U32_AT(p, 0x38) &= ~0x4000u;
    if (kind == 0x12f)
        func_02046274(p, 0x3c);
    else if (kind == 0x130)
        func_02046274(p, 0x20);
    else if (kind == 0x121)
        func_02046274(p, 0x64);
    off = kind * 0x48;
    r9 = Fx(S32_AT(data_020c3b0c + off, 0x3c), S32_AT(data_020c3b0c + off, 0x3c) > 0);
    r8 = Fx(S32_AT(data_020c3b0c + off, 0x38), S32_AT(data_020c3b0c + off, 0x38) > 0);
    sx = Fx(U16_AT(def, 0xa), U16_AT(def, 0xa) != 0);
    sz = Fx(U16_AT(def, 0xc), U16_AT(def, 0xc) != 0);
    sx = func_0208f070(sx, 0x64000);
    sz = func_0208f070(sz, 0x64000);
    r8 = func_0208f070(r8, sx);
    w = func_0208f070(r9, sz);
    r8 >>= 12;
    o = PTR_AT(p, 0x44);
    sc = S32_AT(data_020c3b0c + off, 0x2c);
    S32_AT(o, 0x1c) = FxMul(sc, sx);
    S32_AT(o, 0x20) = sc;
    S32_AT(o, 0x24) = FxMul(sc, sz);
    o[0x8c] = 1;
    w >>= 12;
    S32_AT(p, 0x10) = -w;
    S32_AT(p, 0x14) = -r8;
    S32_AT(p, 0x18) = w;
    S32_AT(p, 0x1c) = r8;
    {
        fx32 r = func_02001ef4(def[0xe]);
        void (*m)(void *, fx32, void *) = VCALL(p, 0x40, void (*)(void *, fx32, void *));

        m(p, r, (void *)m);
    }
    return p;
}

/* 0x02047690: the callback's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_02047690(u8 *self)
{
    PTR_AT(self, 0) = data_020bfa8c;
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

/* 0x020476c0: call the callback's first member function
 * @difftest $D=zero:0x18 @$D+8:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+8:32=8 @$D+0xc:32=pick:1,9 $D cases=5 */
void func_020476c0(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 8) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 8);
}

/* 0x02047720: call the callback's second member function
 * @difftest $D=zero:0x18 @$D+0x10:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+0x10:32=8 @$D+0x14:32=pick:1,9 $D cases=5 */
void func_02047720(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 0x10) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 0x10);
}
