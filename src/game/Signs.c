/*
 * Message objects: map objects of actor class 5 (signs and the like) that
 * show a message (+0x108) when the hero is next to them, either at once
 * or (+0x109) when he presses A. They can ride on a platform (+0xa4).
 * ARM9 main, 0x02043d68 - 0x02044594 (5 functions).
 *
 * Actor callbacks: +0x94 per frame (func_02044024), +0x98 removal
 * (func_02044134), +0x9c collision (func_02043d68). func_02044428 places
 * an actor on the ground and draws its model when it is in view.
 *
 * Testing: with fake actors; the message system, the model animation
 * and the camera stubbed where they need real objects.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcef0[];           /* RTTI */
extern const char data_020bf798[], data_020bf7b4[];   /* debug messages */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */

s32 func_020437fc(void);
s32 func_02011bc8(void);
u32 func_02019b98(u32 v);
u8 *func_02011f34(void);
void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
void func_02072418(void *obj, u32 a, u32 fbits);
void *func_02009394(void);
void func_0205f6e4(void *obj, u32 v);
void *func_02016f24(void *self);
void *func_020a6efc(void *p, void *from, void *to, s32 v);
void func_020150a0(s32 *out, const s32 *a, const s32 *b);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
void func_02005c1c(s32 *out, const s32 *a, const s32 *b);
void func_02053460(void *obj, u32 dt);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
void func_02053440(void *model);
u8 *func_02008994(s32 i, s32 k);
u8 *func_02005d5c(s32 type);
s32 func_0200820c(u32 id, s32 kind);
void func_02003c74(void *actor, s32 x, s32 y);
s32 func_0201a0a0(s32 *xz, void *obj);
fx32 func_02001ef4(u32 dir);
void func_0201c17c(s32 *out, s32 *pos);
void func_020040f4(u8 *dst, const u8 *src);
void func_02082ce8(void *cam);
void *func_020062c0(void);
void *func_020062e0(void);
void func_02017abc(void *list, void *item);

void func_02043d68(u8 *self, u8 *other);
void func_02044024(u8 *self);
void func_02044134(void);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define KIND(a) (data_020c3b0c + S32_AT(a, 8) * 0x48)
#define LINKS(a) ((u8 *)PTR_AT(PTR_AT(a, 0x44), 0xac))

/* bit 6 of the actor's appearance flags */
static s32 Shown(u8 *self)
{
    u8 *(*m)(void *, void *) = VCALL(self, 0x38, u8 *(*)(void *, void *));

    return (s8)BITS(U32_AT(m(self, (void *)m), 0x24), 6, 1);
}

/* whether the object can be used now */
static s32 Usable(u8 *self)
{
    if (Shown(self) == 0 || self[0x10c] != 0) {
        if (Shown(self) != 0)
            return 0;
    }
    return 1;
}

/* 0x02043d68: an object touched: show its message to the hero
 * @difftest $T=zero:0x40 @$T+0x38:32=0x020343a8 $C=zero:0x10 $B=zero:0x100 @$B+0xac:32=$C $S=zero:0x120 @$S+0:32=$T @$S+0x44:32=$B @$S+8:32=pick:0xa,0xf8,0x105,0x46,3 @$S+0x24:32=int:-0x80000:0x80000 @$S+0x108:8=pick:0,7 @$S+0x109:8=pick:0,1 @$S+0x10c:8=pick:0,1 $O=zero:0x60 @$O+8:32=1 @$O+0xc:32=pick:0,0,0,4 @$O+0x24:32=int:-0x80000:0x80000 @$O+0x38:32=pick:0,0x100 @020ebba0:8=pick:0,0,1 @020deec4:32=$O $S $O stub=0x02011bc8:0,0x02019b98:3,0x02072418,0x0205f6e4,0x02081d34:1 cases=200
 * @difftest $T=zero:0x40 @$T+0x38:32=0x020343a8 $C=zero:0x10 $B=zero:0x100 @$B+0xac:32=$C $S=zero:0x120 @$S+0:32=$T @$S+0x44:32=$B @$S+8:32=pick:0xb,0xf9 @$S+0x108:8=pick:0,7 @$S+0x109:8=1 $O=zero:0x60 @$O+8:32=2 @$O+0x38:32=0x100 @020deec4:32=$O $S $O stub=0x02011bc8:0,0x02019b98:3,0x02072418,0x0205f6e4,0x02081d34:0 cases=30 */
void func_02043d68(u8 *self, u8 *other)
{
    s32 d;

    if (S32_AT(other, 0xc) != 0)
        return;
    if (func_020437fc() != 0)
        return;
    if (S32_AT(other, 8) != 1)
        other = HERO();
    d = (S32_AT(other, 0x24) - S32_AT(self, 0x24)) >> 4;
    if (d < 0)
        d = -d;
    if (d >= 0x8000)
        return;
    if (S32_AT(self, 8) != 0x105)
        U32_AT(self, 0x38) |= 8;
    if (func_02011bc8() == 0 && BITS(U32_AT(data_020deebc, 0x20), 5, 1) == 0
        && (s8)BITS(U32_AT(HERO(), 0x38), 23, 1) != 0 && func_02019b98(1) != 0x15
        && self[0x108] != 0 && Usable(self)) {
        if ((u32)(S32_AT(self, 8) - 0xf8) <= 1)
            func_02072418(func_02011f34(), 0xa, 0x45000000);
        else
            func_02072418(func_02011f34(), 2, 0x45000000);
        if (self[0x109] != 0) {
            U8_AT(data_020deebc, 0x1f) = 1;
            PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = self;
            if (func_02081d34(func_02011f44(), 0xa) != 0)
                func_0205f6e4(func_02009394(), self[0x108]);
        } else {
            func_0205f6e4(func_02009394(), self[0x108]);
        }
    }
    if ((s8)BITS(U32_AT(other, 0x38), 8, 1) == 0)
        return;
    if (!Usable(self)) {
        if (S32_AT(self, 8) != 0xa && S32_AT(self, 8) != 0xb && S32_AT(self, 8) != 0x46)
            return;
    }
    U32_AT(self, 0x38) |= 0x400;
    PTR_AT(LINKS(self), 8) = other;
}

/* 0x02044024: per-frame update (riding a platform, the model's animation)
 * @difftest $S=zero:0x120 @$S+0x40:16=int:0:0x100 $S cases=3
 * @difftest $T=zero:0x40 @$T+0x24:32=0x02043814 $P=zero:0x40 @$P+0:32=$T $S=zero:0x120 @$S+0xa4:32=$P @$S+0x38:32=0x20000 $S cases=3
 * @difftest $T=zero:0x40 @$T+0x24:32=0x020343a8 $P=zero:0x40 @$P+0:32=$T $S=zero:0x120 @$S+0xa4:32=$P @$S+0x38:32=0x20000 $S cases=3
 * @difftest $O=zero:0x200 $S=zero:0x120 @$S+0x44:32=$O $S stub=0x02053460 cases=3 */
void func_02044024(u8 *self)
{
    u8 *o;

    if (PTR_AT(self, 0xa4) != NULL) {
        u8 *p = PTR_AT(self, 0xa4);
        s32 (*m)(void *, void *, void *) = VCALL(p, 0x24, s32 (*)(void *, void *, void *));

        if (m(p, self, (void *)m) == 0) {
            PTR_AT(self, 0xa4) = NULL;
            U32_AT(self, 0x38) &= ~0x20000u;
            func_02004490(data_020bf798, S16_AT(self, 0x40));
        } else {
            u8 *pl = PTR_AT(self, 0xa4);
            s32 d[2], s[2], n[2];

            if (pl != NULL)
                pl = func_020a6efc(pl, data_020bcb88, data_020bcef0, -1);
            func_020150a0(d, (s32 *)((u8 *)PTR_AT(self, 0xa4) + 0x28), (s32 *)((u32)pl + 0x64));
            func_02003b08(s, d, 4);
            d[0] = s[0];
            d[1] = s[1];
            func_02005c1c(n, (s32 *)(self + 0x5c), d);
            S32_AT(self, 0x5c) = n[0];
            S32_AT(self, 0x60) = n[1];
        }
    }
    o = PTR_AT(self, 0x44);
    if (o == NULL)
        return;
    func_02053460(o, func_020822c0(func_020062d0()));
}

/* 0x02044134: the removal callback
 * @difftest cases=3 */
void func_02044134(void)
{
}

/* 0x02044138: create message object `id` (of map object list `k`)
 * @difftest int:0:4 int:0:2 cases=5 */
u8 *func_02044138(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    u32 kind = U16_AT(def, 8);
    u8 *a, *o;
    s32 t[2], w, d, sc;
    fx32 r;

    a = func_02005d5c(S32_AT(def, 0));
    S32_AT(a, 0xc) = 5;
    S32_AT(a, 8) = func_0200820c(kind, 2);
    U32_AT(a, 4) |= 8;
    U32_AT(a, 0x38) &= ~0x40000u;
    U32_AT(a, 0x110) |= 0x100;
    U32_AT(a, 0x38) &= ~0x10u;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x20u) | ((def[0xe] & 1) << 5);
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x40u) | ((def[0xf] & 1) << 6);
    {
        void (*m)(void *, u32, void *) = VCALL(a, 0xc, void (*)(void *, u32, void *));
        m(a, 0, (void *)m);
    }
    PTR_AT(a, 0x94) = (void *)func_02044024;
    PTR_AT(a, 0x98) = (void *)func_02044134;
    PTR_AT(a, 0x9c) = (void *)func_02043d68;
    w = S32_AT(KIND(a), 0x38);
    d = S32_AT(KIND(a), 0x3c);
    S32_AT(a, 0x10) = -d;
    S32_AT(a, 0x14) = -w;
    S32_AT(a, 0x18) = d;
    S32_AT(a, 0x1c) = w;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~4u) | ((S32_AT(KIND(a), 0x40) > 0) << 2);
    func_02053648(PTR_AT(a, 0x44), S32_AT(a, 8), a, 0, 0);
    o = PTR_AT(a, 0x44);
    U32_AT(o, 0x18c) |= 2;
    o = PTR_AT(a, 0x44);
    S32_AT(o, 4) = 0;
    S32_AT(o, 8) = 0;
    S32_AT(o, 0xc) = 0;
    o[0x8c] = 1;
    S32_AT(PTR_AT(a, 0x44), 0xa4) = S32_AT(KIND(a), 0x34);
    o = PTR_AT(a, 0x44);
    U32_AT(o, 0x18c) = (U32_AT(o, 0x18c) & ~1u) | ((s8)S32_AT(KIND(a), 0x30) & 1);
    sc = S32_AT(KIND(a), 0x2c);
    o = PTR_AT(a, 0x44);
    S32_AT(o, 0x1c) = sc;
    S32_AT(o, 0x20) = sc;
    S32_AT(o, 0x24) = sc;
    o[0x8c] = 1;
    func_02003c74(a, SHL(S16_AT(def, 4), 16), SHL(S16_AT(def, 6), 16));
    S32_AT(a, 0x30) = S32_AT(a, 0x28);
    S32_AT(a, 0x34) = S32_AT(a, 0x2c);
    t[0] = S32_AT(a, 0x54);
    t[1] = S32_AT(a, 0x58);
    S32_AT(a, 0x24) = func_0201a0a0(t, a);
    S32_AT(a, 0x6c) = S32_AT(a, 0x24);
    S32_AT(a, 0x64) = S32_AT(a, 0x24);
    a[0x10d] = def[0xc];
    r = func_02001ef4(a[0x10d]);
    o = PTR_AT(a, 0x44);
    {
        s32 x = S32_AT(o, 0x10), z = S32_AT(o, 0x18);

        S32_AT(o, 0x10) = x;
        S32_AT(o, 0x14) = r;
        S32_AT(o, 0x18) = z;
        o[0x8c] = 1;
    }
    PTR_AT(LINKS(a), 4) = HERO();
    func_02004490(data_020bf7b4, kind);
    return a;
}

/* 0x02044428: put an actor on the ground and draw its model if in view
 * @difftest $O=zero:0x200 $S=zero:0x120 @$S+0x44:32=$O @$S+0x28:32=int:-0x100000:0x100000 @$S+0x2c:32=int:-0x100000:0x100000 @$S+0x10:32=int:-0x10000:0 @$S+0x14:32=int:-0x10000:0 @$S+0x18:32=int:0:0x10000 @$S+0x1c:32=int:0:0x10000 @$S+0x38:32=pick:0,4 $S stub=0x0201f490:0x020deebc,0x02053440,0x02017abc cases=20 */
void func_02044428(u8 *self)
{
    s32 xz[2], p[3], b[4];
    u32 cam[0x2c0 / 4];
    u8 *o;
    s32 vis;

    xz[0] = S32_AT(self, 0x28);
    xz[1] = S32_AT(self, 0x2c);
    func_0201c17c(p, xz);
    o = PTR_AT(self, 0x44);
    S32_AT(o, 4) = p[0];
    S32_AT(o, 8) = p[1];
    S32_AT(o, 0xc) = p[2];
    o[0x8c] = 1;
    o = PTR_AT(self, 0x44);
    b[0] = S32_AT(self, 0x10) + S32_AT(o, 8);
    b[1] = S32_AT(self, 0x14) + S32_AT(o, 4);
    b[2] = S32_AT(self, 0x18) + S32_AT(o, 8);
    b[3] = S32_AT(self, 0x1c) + S32_AT(o, 4);
    func_020040f4((u8 *)cam, func_020062e0());
    vis = 0;
    if (S8_AT(cam, 0x284) != 0) {
        vis = S32_AT(cam, 0x2ac) >= b[1] && S32_AT(cam, 0x2a4) <= b[3]
              && S32_AT(cam, 0x2a0) <= b[2] && S32_AT(cam, 0x2a8) >= b[0];
    }
    func_02082ce8(cam);
    if (vis == 0)
        return;
    func_02053440(PTR_AT(self, 0x44));
    if (BITS(U32_AT(self, 0x38), 2, 1) == 0)
        return;
    func_02017abc((u8 *)func_020062c0() + 4, self);
}
