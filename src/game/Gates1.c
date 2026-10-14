/*
 * Level objects with an effect (part 1): doors that open with enough
 * eggs, save spots, level exits and other animated objects of actor class
 * 0xc, each with an optional particle effect.
 * ARM9 main, 0x02047780 - 0x02048628 (17 functions).
 *
 * The object (0x290 bytes, vtable data_020bfb1c) is a model with an actor
 * at +0x90 (vtable data_020bfb58; this-adjusting entry points at
 * 0x02049910..), a draw callback at +0xdc, an animation at +0xf4 and a
 * second one at +0x1d0. +0x274 the particle effect (0x3c68 bytes,
 * func_02058b2c), +0x278 the object's type (0-0xc), +0x27c its state,
 * +0x280 the map object kind, +0x284 brightness, +0x288 / +0x28a two
 * messages, +0x28c the area (save spots) or the height to lift the hero.
 *
 * Testing: with fake objects; the messages, the sounds, the effects, the
 * height map and the camera stubbed or faked where they need real data.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020b6cc4;
extern const fx32 data_020bfac4;
extern const s32 data_020bfad8[];    /* push speed */
extern const u8 data_020bfb9c[];     /* name of the effect's node */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */
extern s32 data_020ebca8;            /* fading speed */

void func_0204ed80(void *anim, u32 id, u32 a);
void func_02020440(u32 v);
void *func_02009394(void);
void func_0205f6e4(void *obj, u32 v);
s32 func_0208f070(s32 num, s32 den);
void func_0201c17c(s32 *out, s32 *pos);
void *func_0207fe74(u32 size);
void *func_02058b2c(void *e, u32 kind, void *cam);
u8 *func_0204f220(void *anim, const void *name);
void *func_02016f24(void *self);
s32 func_0202df68(const s32 *v);
void func_0202dea4(s32 *v);
void func_0202ded8(s32 *v, const s32 *s);
void func_020297d4(s32 *out, const s32 *v, const s32 *s);
s32 func_0204a80c(u32 counter);
u32 func_02049124(u8 *self);
void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
u8 *func_02011f34(void);
void func_02072418(void *obj, u32 a, u32 fbits);
void func_0201146c(void);
void func_02011664(u32 text, u32 v);
void *func_020062e0(void);
void *func_020062c0(void);
void *func_020062b0(void);
void func_020040f4(u8 *dst, const u8 *src);
void func_02082ce8(void *cam);
void *func_02019670(void);
void func_02043cc8(u8 *list, void *item);
void func_02017abc(void *list, void *item);
void func_02071804(void *anim, u32 kind, void *actor);

u32 func_020477e0(u8 *self);
void func_02047a10(u8 *self);
void func_02047b08(u8 *self);
void func_02047c94(s32 *out, const u8 *m);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define HERO_ACTOR() ((u8 *)PTR_AT(data_020deebc, 4))
#define ACTOR(self) ((self) != NULL ? (self) + 0x90 : NULL)
#define EFFECT(self) ((u8 *)PTR_AT(self, 0x274))

/* 0x02047780: turn the object to `h`
 * @difftest zero:0x290 int:0:0x6487 cases=3 */
void func_02047780(u8 *self, fx32 h)
{
    s32 x = S32_AT(self, 0x10), z = S32_AT(self, 0x18);

    S32_AT(self, 0x10) = x;
    S32_AT(self, 0x14) = h;
    S32_AT(self, 0x18) = z;
    self[0x8c] = 1;
}

/* 0x020477a8: the object's direction
 * @difftest ptr:0x290 cases=3 */
fx32 func_020477a8(u8 *self)
{
    return S32_AT(self, 0x14);
}

/* 0x020477b0: the second animation
 * @difftest ptr:0x290 cases=3 */
u8 *func_020477b0(u8 *self)
{
    return self + 0x1d0;
}

/* 0x020477b8: set the second message
 * @difftest zero:0x290 int:-3:0x100 cases=5 */
void func_020477b8(u8 *self, s32 v)
{
    if (v <= 0)
        v = 0;
    U16_AT(self, 0x28a) = v;
}

/* 0x020477cc: set the first message
 * @difftest zero:0x290 int:-3:0x100 cases=5 */
void func_020477cc(u8 *self, s32 v)
{
    if (v <= 0)
        v = 0;
    U16_AT(self, 0x288) = v;
}

/* 0x020477e0: the object's type
 * @difftest ptr:0x290 cases=3 */
u32 func_020477e0(u8 *self)
{
    return U32_AT(self, 0x278);
}

/* 0x020477e8: set the object's type
 * @difftest zero:0x290 int:0:0xd cases=3 */
void func_020477e8(u8 *self, u32 v)
{
    U32_AT(self, 0x278) = v;
}

/* 0x020477f0: close
 * @difftest zero:0x290 stub=0x0204ed80 cases=3
 * @difftest $S=zero:0x290 @$S+0x98:32=0xe6 $S stub=0x0204ed80 cases=3 */
void func_020477f0(u8 *self)
{
    if (S32_AT(self, 0x98) == 0xe6)
        func_0204ed80(self + 0xf4, 0x1bb, 0);
    S32_AT(self, 0x27c) = 1;
}

/* 0x02047828: open
 * @difftest $S=zero:0x290 @$S+0x98:32=pick:0xe6,3 @$S+0x278:32=int:0:0xd $S stub=0x0204ed80,0x02020440,0x02047b08 cases=30 */
void func_02047828(u8 *self)
{
    if (S32_AT(self, 0x98) == 0xe6)
        func_0204ed80(self + 0xf4, 0x1bc, 0);
    if (func_020477e0(self) == 8 || func_020477e0(self) == 9) {
        S32_AT(self, 0x27c) = 1;
        func_02020440(0xd8);
        return;
    }
    if (func_020477e0(self) == 1 || func_020477e0(self) == 2) {
        func_02047b08(self);
        S32_AT(self, 0x27c) = 2;
        U32_AT(self, 0xc8) |= 0x200;
        return;
    }
    S32_AT(self, 0x27c) = 2;
}

/* 0x020478dc: triggered (a switch or a script)
 * @difftest $V=zero:0x10 $E=zero:0x3c68 @$E+0:32=$V $S=zero:0x290 @$S+0x278:32=pick:0,1,2,3 @$S+0x27c:32=pick:1,2 @$S+0x274:32=0 @$S+0x288:16=pick:-1,4 @$S+0x28a:16=pick:-1,5 $S stub=0x0205f6e4,0x02047b08 cases=40
 * @difftest $V=zero:0x10 $E=zero:0x3c68 @$E+0:32=$V $S=zero:0x290 @$S+0x278:32=pick:0,1,2 @$S+0x27c:32=2 @$S+0x274:32=$E @$S+0x288:16=4 @$S+0x28a:16=5 $S stub=0x0205f6e4,0x02047b08 cases=10 */
void func_020478dc(u8 *self)
{
    s32 m;
    u8 *e;

    switch (U32_AT(self, 0x278)) {
    case 0:
        m = S16_AT(self, 0x288);
        if (m != -1)
            func_0205f6e4(func_02009394(), m);
        func_02047b08(self);
        data_020ebca8 = func_0208f070(0x1000, 0x2000);
        e = EFFECT(self);
        if (e != NULL) {
            S32_AT(e, 0x3bc8) = 0x3000;
            U8_AT(e, 0x3bc6) = 1;
        }
        S32_AT(self, 0x27c) = 2;
        return;
    case 1:
    case 2:
        if (S32_AT(self, 0x27c) != 2) {
            m = S16_AT(self, 0x288);
            if (m != -1)
                func_0205f6e4(func_02009394(), m);
            func_02047b08(self);
            S32_AT(self, 0x27c) = 2;
            return;
        }
        e = EFFECT(self);
        if (e != NULL)
            U8_AT(e, 0x3bc5) = 1;
        m = S16_AT(self, 0x28a);
        if (m == -1)
            return;
        func_0205f6e4(func_02009394(), m);
        return;
    }
}

/* make the object's effect of type `type` at `pos` */
static void MakeEffect(u8 *self, u32 type, const s32 *pos)
{
    u8 *e = func_0207fe74(0x3c68);

    if (e != NULL)
        e = func_02058b2c(e, type, NULL);
    PTR_AT(self, 0x274) = e;
    if (EFFECT(self) == NULL)
        return;
    e = EFFECT(self);
    S32_AT(e, 4) = pos[0];
    S32_AT(e, 8) = pos[1];
    S32_AT(e, 0xc) = pos[2];
    e[0x8c] = 1;
    e = EFFECT(self);
    U8_AT(e, 0x3bc4) = 1;
    U8_AT(e, 0x3bc5) = 0;
    Put64(e + 0x3c58, func_020822c8(func_020062d0()));
}

/* the point above the object's foot on the ground */
static void Above(u8 *self, s32 *pos)
{
    s32 xz[2], p[3];

    xz[0] = S32_AT(self, 0xb8);
    xz[1] = S32_AT(self, 0xbc);
    func_0201c17c(p, xz);
    pos[0] = p[0];
    pos[1] = p[1];
    pos[2] = p[2] + 0xf000;
}

/* 0x02047a10: the opening effect
 * @difftest zero:0x290 stub=0x0201c17c,0x02058b2c,0x0207fe74:0 cases=3 */
void func_02047a10(u8 *self)
{
    s32 pos[3];

    pos[0] = 0;
    pos[1] = 0;
    pos[2] = 0;
    Above(self, pos);
    MakeEffect(self, 9, pos);
}

/* 0x02047b08: the object's effect (at its node for type 1)
 * @difftest $S=zero:0x290 @$S+0x278:32=pick:0,2 $S stub=0x0201c17c,0x02058b2c,0x0207fe74:0 cases=3 */
void func_02047b08(u8 *self)
{
    s32 pos[3];

    pos[0] = 0;
    pos[1] = 0;
    pos[2] = 0;
    if (U32_AT(self, 0x278) == 1) {
        u8 *m = func_0204f220(self + 0xf4, data_020bfb9c);
        s32 t[3];

        func_02047c94(t, m);
        pos[0] = t[0];
        pos[1] = t[1];
        pos[2] = t[2];
    } else {
        Above(self, pos);
    }
    MakeEffect(self, 6, pos);
}

/* 0x02047c94: a node's position (from its matrix)
 * @difftest ptr:12:4 ptr:0x64:4 cases=3 */
void func_02047c94(s32 *out, const u8 *m)
{
    out[0] = S32_AT(m, 0x30);
    out[1] = S32_AT(m, 0x34);
    out[2] = S32_AT(m, 0x38);
}

/* the actor's appearance (its method +0x28) */
static u8 *Look(u8 *self)
{
    u8 *(*m)(void *, void *) = VCALL(self, 0x28, u8 *(*)(void *, void *));

    return m(self, (void *)m);
}

static void Message(s32 m)
{
    func_0205f6e4(func_02009394(), m);
}

/* 0x02047cb0: touched by the hero
 * @difftest $Y=zero:0x10 @020def5c:32=$Y $T=zero:0x40 @$T+0x28:32=0x020343a8 @$T+0x38:32=0x020343a8 $H=zero:0x80 @$H+0:32=$T @$H+0x64:16=pick:0x21,3 $A=zero:0x120 @020deec0:32=$A @020deec4:32=$H $S=zero:0x290 @$S+0:32=$T @$S+0x24:32=pick:0,0x80 @$S+0x98:32=pick:3,0xe6 @$S+0x278:32=pick:5,8,9,1 @$S+0x27c:32=pick:0,1,2 @$S+0x280:32=pick:5,6,7 @$S+0x28c:8=int:0:4 @$S+0x288:16=4 @$S+0x28a:16=5 $O=zero:0x60 @$O+8:32=pick:1,1,2 @$O+0xc:32=pick:0,0,1 @$O+0x38:32=pick:0,0x100 $S $O stub=0x02016f24:0x020deeb0,0x0204a80c:2,0x02020440,0x0201f490:0x020deebc,0x0207fe74:0,0x0205f6e4,0x02081d34:1,0x02072418,0x0201146c,0x02011664 cases=200
 * @difftest $T=zero:0x40 @$T+0x28:32=0x020343a8 @$T+0x38:32=0x020343a8 $H=zero:0x80 @$H+0:32=$T $A=zero:0x120 @020deec0:32=$A @020deec4:32=$H $S=zero:0x290 @$S+0:32=$T @$S+0x24:32=0x80 @$S+0x278:32=pick:5,8,9 @$S+0x27c:32=pick:0,1 @$S+0xb8:32=int:-0x10000:0x10000 @$S+0xbc:32=int:-0x10000:0x10000 $Y=zero:0x10 @$Y+4:32=$S+0x90 @020def5c:32=$Y $O=zero:0x60 @$O+8:32=1 @$O+0x28:32=int:-0x10000:0x10000 @$O+0x2c:32=int:-0x10000:0x10000 $S $O stub=0x02016f24:0x020deeb0,0x0204a80c:0,0x0205f6e4,0x02081d34:0,0x02072418 cases=30 */
void func_02047cb0(u8 *self, u8 *other)
{
    if (S32_AT(other, 0xc) != 0)
        return;
    if (S32_AT(other, 8) != 1)
        return;
    U32_AT(self, 0xc8) |= 8;
    if (PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) == ACTOR(self)
        && (s8)BITS(U32_AT(Look(self), 0x24), 7, 1) != 0
        && PTR_AT(HERO_ACTOR(), 0xa4) != ACTOR(self)) {
        s32 a[2], b[2], v[3], t[3], len;

        a[0] = S32_AT(self, 0xb8);
        a[1] = S32_AT(self, 0xbc);
        b[0] = S32_AT(other, 0x28);
        b[1] = S32_AT(other, 0x2c);
        v[0] = b[0] - a[0];
        v[1] = data_020bfac4;
        v[2] = b[1] - a[1];
        len = func_0202df68(v);
        if (len > 1)
            func_0202ded8(v, &len);
        else
            func_0202dea4(v);
        func_020297d4(t, v, data_020bfad8);
        S32_AT(HERO_ACTOR(), 0x100) = t[0];
        S32_AT(HERO_ACTOR(), 0x104) = t[2];
    }
    if (S32_AT(self, 0x98) == 0xe6) {
        float f;

        U32_AT(self, 0xc8) |= 8;
        if (S32_AT(self, 0x27c) != 2)
            return;
        data_020deebc[0x20 / 4] |= 8;
        f = (float)(s32)((u32)self[0x28c] << 16);
        S32_AT(HERO_ACTOR(), 0x68) = (s32)(self[0x28c] != 0 ? 0.5f + f : f - 0.5f);
        return;
    }
    if (func_020477e0(self) == 5 && S32_AT(self, 0x27c) == 1) {
        u8 *(*m)(void *, void *) = VCALL(HERO(), 0x38, u8 *(*)(void *, void *));
        u32 st = U16_AT(m(HERO(), (void *)m), 0x64);
        s32 n = func_0204a80c(7);
        u32 attack = (s8)BITS(U32_AT(other, 0x38), 8, 1);
        s32 ok = 0;

        if (attack != 0 && st == 0x21) {
            switch (func_02049124(self)) {
            case 5:
                if (n >= 3)
                    ok = 1;
                break;
            case 6:
                if (n >= 2)
                    ok = 1;
                break;
            case 7:
                if (n >= 1)
                    ok = 1;
                break;
            }
            if (ok != 0) {
                func_02020440(0x5e);
                func_02047a10(self);
                S32_AT(self, 0x27c) = 2;
                Message(S16_AT(self, 0x28a));
            }
        }
        if (PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) != ACTOR(self))
            return;
        if ((s8)BITS(U32_AT(Look(self), 0x24), 7, 1) == 0)
            return;
        U32_AT(HERO(), 0x38) &= ~0x200u;
        {
            u8 *h = HERO();
            PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = h;
        }
        return;
    }
    if (func_020477e0(self) == 8) {
        if (S32_AT(self, 0x27c) != 1 && func_02081d34(func_02011f44(), 0xa) != 0) {
            u8 area;

            S32_AT(self, 0x27c) = 1;
            func_02072418(func_02011f34(), 0, 0x45000000);
            Message(S16_AT(self, 0x288));
            area = self[0x28c];
            func_020436f0(data_020bf6a0)[0x10] = area;
            return;
        }
        func_02072418(func_02011f34(), 8, 0x45000000);
        return;
    }
    if (func_020477e0(self) != 9)
        return;
    if (S32_AT(self, 0x27c) != 1 && func_02081d34(func_02011f44(), 0xa) != 0) {
        S32_AT(self, 0x27c) = 1;
        func_02072418(func_02011f34(), 0, 0x45000000);
        if (BITS(data_020deebc[0x20 / 4], 5, 1) != 0) {
            Message(S16_AT(self, 0x288));
            return;
        }
        Message(S16_AT(self, 0x28a));
        func_0201146c();
        func_02011664(0x3e9, 1);
        return;
    }
    func_02072418(func_02011f34(), 8, 0x45000000);
}

/* 0x0204817c: place the object and draw it if in view (1 if drawn)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0x290 @$S+0x94:32=pick:0,8 @$S+0xb8:32=int:-0x10000:0x10000 @$S+0xbc:32=int:-0x10000:0x10000 @$S+0xa0:32=int:-0x10:0 @$S+0xa4:32=int:-0x10:0 @$S+0xa8:32=int:0:0x10 @$S+0xac:32=int:0:0x10 @$S+0x278:32=pick:0,1,2 $S stub=0x0201f490:0x020deebc,0x02043cc8,0x02017abc cases=30 */
s32 func_0204817c(u8 *self)
{
    s32 xz[2], p[3], b[4];
    u32 cam[0x2c0 / 4];
    s32 vis;

    if (BITS(U32_AT(self, 0x94), 3, 1) == 0)
        return 0;
    xz[0] = S32_AT(self, 0xb8);
    xz[1] = S32_AT(self, 0xbc);
    func_0201c17c(p, xz);
    S32_AT(self, 4) = p[0];
    S32_AT(self, 8) = p[1];
    S32_AT(self, 0xc) = p[2];
    self[0x8c] = 1;
    if (func_02049124(self) != 5 && func_02049124(self) != 6)
        func_02049124(self);
    b[0] = S32_AT(self, 8) + SHL(S32_AT(self, 0xa0), 12);
    b[1] = S32_AT(self, 4) + SHL(S32_AT(self, 0xa4), 12);
    b[2] = S32_AT(self, 8) + SHL(S32_AT(self, 0xa8), 12);
    b[3] = S32_AT(self, 4) + SHL(S32_AT(self, 0xac), 12);
    func_020040f4((u8 *)cam, func_020062e0());
    vis = 0;
    if (S8_AT(cam, 0x284) != 0) {
        vis = S32_AT(cam, 0x2ac) >= b[1] && S32_AT(cam, 0x2a4) <= b[3]
              && S32_AT(cam, 0x2a0) <= b[2] && S32_AT(cam, 0x2a8) >= b[0];
    }
    func_02082ce8(cam);
    if (vis == 0) {
        U32_AT(self, 0xc8) &= ~0x2000u;
        return 0;
    }
    if (EFFECT(self) != NULL && (U32_AT(self, 0x278) == 2 || U32_AT(self, 0x278) == 0)) {
        s32 pos[3];
        u8 *e;

        Above(self, pos);
        e = EFFECT(self);
        S32_AT(e, 4) = pos[0];
        S32_AT(e, 8) = pos[1];
        S32_AT(e, 0xc) = pos[2];
        e[0x8c] = 1;
    }
    U32_AT(self, 0xc8) |= 0x2000;
    func_02043cc8((u8 *)func_02019670() + 4, self + 0xdc);
    func_02017abc((u8 *)func_020062c0() + 4, ACTOR(self));
    return 1;
}

/* 0x020483f8: delete the effect; the hero is off the object
 * @difftest $A=zero:0x120 @020deec0:32=$A $H=zero:0x80 @020deec4:32=$H $V=zero:0x10 @$V+4:32=0x020343a8 $E=zero:0x20 @$E+0:32=$V $S=zero:0x290 @$S+0x274:32=0 $S cases=2
 * @difftest $A=zero:0x120 @020deec0:32=$A $H=zero:0x80 @$H+0x38:32=0x20000 @020deec4:32=$H $V=zero:0x10 @$V+4:32=0x020343a8 $E=zero:0x20 @$E+0:32=$V $S=zero:0x290 @$S+0x274:32=$E @$A+0xa4:32=$S+0x90 $S cases=3 */
void func_020483f8(u8 *self)
{
    u8 *e = EFFECT(self);
    u8 *a;

    if (e != NULL) {
        if (e != NULL) {
            void (*m)(void *, void *) = VCALL(e, 4, void (*)(void *, void *));
            m(e, (void *)m);
        }
        PTR_AT(self, 0x274) = NULL;
    }
    a = HERO_ACTOR();
    if (PTR_AT(a, 0xa4) != ACTOR(self))
        return;
    PTR_AT(a, 0xa4) = NULL;
    U32_AT(HERO(), 0x38) &= ~0x20000u;
}

/* 0x02048474: set up an object of actor kind `kind`
 * @difftest zero:0x290 pick:3,0xe6,0xf3,0x111,0x13c,0x120 stub=0x02071804,0x0204ed80,0x02017abc cases=12 */
void func_02048474(u8 *self, u32 kind)
{
    s32 sc, w, d;

    sc = S32_AT(data_020c3b0c + kind * 0x48, 0x2c);
    S32_AT(self, 0x1c) = sc;
    S32_AT(self, 0x20) = sc;
    S32_AT(self, 0x24) = sc;
    self[0x8c] = 1;
    S32_AT(self, 4) = 0;
    S32_AT(self, 8) = 0;
    S32_AT(self, 0xc) = 0;
    self[0x8c] = 1;
    S32_AT(self, 0x10) = data_020b6cc4;
    S32_AT(self, 0x14) = g_FxZero;
    S32_AT(self, 0x18) = g_FxZero;
    self[0x8c] = 1;
    S32_AT(self, 0x9c) = 0xc;
    S32_AT(self, 0x98) = kind;
    func_02071804(self + 0x1d0, kind, ACTOR(self));
    U32_AT(self, 0xc8) = (U32_AT(self, 0xc8) & ~1u) | 1;
    U32_AT(self, 0xc8) |= 4;
    w = S32_AT(data_020c3b0c + kind * 0x48, 0x3c);
    d = S32_AT(data_020c3b0c + kind * 0x48, 0x38);
    S32_AT(self, 0xa0) = -w;
    S32_AT(self, 0xa4) = -d;
    S32_AT(self, 0xa8) = w;
    S32_AT(self, 0xac) = d;
    if (kind == 0xe6) {
        func_0204ed80(self + 0xf4, 0x1bc, 0);
        U32_AT(self, 0xc8) &= ~4u;
    } else if (kind == 0xf3 || kind == 0x111 || kind == 0x13c) {
        U32_AT(self, 0xc8) &= ~4u;
    } else if (kind == 0x120) {
        func_0204ed80(self + 0xf4, 0x284, 0);
        func_0204ed80(self + 0xf4, 0x285, 0);
    }
    func_02017abc((u8 *)func_020062b0() + 4, ACTOR(self));
}
