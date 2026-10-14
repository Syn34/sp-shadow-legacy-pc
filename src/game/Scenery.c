/*
 * Animated scenery: objects that play an animation and show in one of the
 * hero's forms (+0x184: 0 one, 1 the other, 2 both but dimmed), and a
 * looping kind of it (map objects with variants 0x812 / 0x80c / 0x80d).
 * ARM9 main, 0x0204b36c - 0x0204c148 (31 functions).
 *
 * The object (0x190 bytes, vtable data_020bfdf4; looping kind
 * data_020bfec8) is a model with an actor at +0x90 (vtable data_020bfe18
 * / data_020bfeec; this-adjusting entry points at 0x0204be40.. and
 * 0x0204c118..), an animation at +0xdc, a draw callback at +0x168
 * (vtables data_020bfe64 / data_020bf760). +0x180 the state (0 hidden,
 * 1 shown, 2 fading, 3 done), +0x188 dimmed, +0x18c the frame of the
 * looping kind.
 *
 * Testing: with fake objects; the animations, the lights and the camera
 * stubbed or faked where they need real data.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020bfd58, data_020bfd5c, data_020bfd60, data_020bfd64, data_020bfd68,
    data_020bfd74, data_020bfd78, data_020bfd7c, data_020bfd80, data_020bfd84, data_020bfd8c,
    data_020bfd90, data_020bfd94, data_020bfd98;          /* boxes */
extern const u32 data_020bfd9c[2], data_020bfda4[2], data_020bfdac[2], data_020bfdb4[2];
extern u8 data_020bfdf4[], data_020bfe18[], data_020bfe64[], data_020bf760[];
extern u8 data_020bfec8[], data_020bfeec[];          /* vtables */
extern u8 data_020ebd04[], data_020ebd1c[];

s32 func_0201b650(u32 screen);
void func_02020440(u32 v);
void *func_020062b0(void);
void *func_020062c0(void);
void *func_020062e0(void);
void func_02017abc(void *list, void *item);
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
void func_02080e84(void *settings);
u32 func_020186ec(void);
void func_0201c17c(s32 *out, s32 *pos);
void func_020040f4(u8 *dst, const u8 *src);
void func_02082ce8(void *cam);
void *func_02019670(void);
void func_02043cc8(u8 *list, void *item);
void func_02008130(s32 group, s32 index);
void func_0205eef0(void *anim);
void func_0205ef34(void *anim, u32 a, u32 b);
void func_0205ef8c(void *anim, u32 a);
void func_0205ed6c(void *anim, s32 frame);
s32 func_0205ed14(void *anim);
void *func_020506c0(void *actor);
void func_0205b784(void *self);
void func_0205b7c8(void *self, s32 *box, void *a, void *b);
void func_02050890(void *actor, s32 *pos, s32 *box);
void func_0207fe44(void *p);
void *func_0207fe74(u32 size);
u8 *func_02008994(s32 i, s32 k);
void func_02003b08(s32 *out, const s32 *in, s32 shift);
s32 func_0201a0a0(s32 *xz, void *obj);

void func_0204b36c(u8 *self);
void func_0204b370(u8 *self);
void func_0204b3b4(u8 *self);
void func_0204b3e8(u8 *self);
void func_0204b4d4(u8 *self, s32 dt);
void func_0204b65c(u8 *self);
void func_0204b82c(u8 *self);
u8 *func_0204b90c(u8 *self);
u8 *func_0204b974(u8 *self);
u8 *func_0204b9e4(u8 *self);
u8 *func_0204bbe0(u8 *self, u32 a, s32 *pos);
void func_0204beb4(u8 *self, s32 dt);
u8 *func_0204bf08(u8 *self);
u8 *func_0204bf40(u8 *self);
u8 *func_0204bf70(u8 *self, u32 a, s32 *pos);

#define ACTOR(self) ((self) != NULL ? (self) + 0x90 : NULL)

/* is the object shown in the hero's current form? */
static s32 Shown(u8 *self)
{
    if (BITS(U32_AT(self, 0xc8), 6, 1) != 0 && func_020186ec() == 1)
        return 1;
    if (BITS(U32_AT(self, 0xc8), 5, 1) == 0)
        return 0;
    return func_020186ec() == 0;
}

/* 0x0204b36c
 * @difftest ptr:0x190 cases=3 */
void func_0204b36c(u8 *self)
{
}

/* 0x0204b370: fade away (with a sound when the screen is not fading)
 * @difftest zero:0x190 stub=0x0201b650:0,0x02020440 cases=3
 * @difftest zero:0x190 stub=0x0201b650:1,0x02020440 cases=3 */
void func_0204b370(u8 *self)
{
    S32_AT(self, 0x180) = 2;
    if (func_0201b650(data_020df0fc) != 0)
        return;
    func_02020440(0x56);
}

/* 0x0204b3b4: register the actor (not drawn yet)
 * @difftest zero:0x190 stub=0x02017abc cases=3 */
void func_0204b3b4(u8 *self)
{
    U32_AT(self, 0xc8) &= ~4u;
    func_02017abc((u8 *)func_020062b0() + 4, ACTOR(self));
}

/* 0x0204b3e8: draw the object
 * @difftest $T=zero:0x10 @$T+4:32=0x020343a8 $S=zero:0x190 @$S+0xdc:32=$T @$S+0x8c:8=pick:0,1 @$S+0xcc:32=int:0:0x20 $S stub=0x02080e84 cases=5 */
void func_0204b3e8(u8 *self)
{
    u8 *g;

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
    {
        u8 *a = self + 0xdc;
        void (*m)(void *, u32, void *) = VCALL(a, 4, void (*)(void *, u32, void *));

        m(a, U32_AT(self, 0xcc), (void *)m);
    }
}

/* 0x0204b4b4: the animation's first method
 * @difftest $T=zero:0x10 @$T+0:32=0x020343a8 $S=zero:0x190 @$S+0xdc:32=$T $S cases=3 */
void func_0204b4b4(u8 *self)
{
    u8 *a = self + 0xdc;
    void (*m)(void *, void *) = VCALL(a, 0, void (*)(void *, void *));

    m(a, (void *)m);
}

/* 0x0204b4d4: per-frame update
 * @difftest $S=zero:0x190 @$S+0x180:32=int:0:5 @$S+0xc8:32=pick:0,0x20,0x40,0x60 @$S+0xcc:32=int:0:4 $S int:0:4 stub=0x020186ec:0,0x0204b65c cases=40
 * @difftest $S=zero:0x190 @$S+0x180:32=int:0:4 @$S+0xc8:32=pick:0x20,0x40 @$S+0xcc:32=int:0:4 $S int:0:4 stub=0x020186ec:1,0x0204b65c cases=20 */
void func_0204b4d4(u8 *self, s32 dt)
{
    switch (S32_AT(self, 0x180)) {
    case 0:
        if (!Shown(self))
            return;
        S32_AT(self, 0x180) = 1;
        U32_AT(self, 0xc8) |= 4;
        func_0204b65c(self);
        return;
    case 1:
        if (Shown(self)) {
            func_0204b65c(self);
            return;
        }
        U32_AT(self, 0xc8) &= ~4u;
        S32_AT(self, 0x180) = 0;
        return;
    case 2:
        if (Shown(self)) {
            if (U32_AT(self, 0xcc) <= 1) {
                S32_AT(self, 0x180) = 3;
                return;
            }
            U32_AT(self, 0xcc)--;
            func_0204b65c(self);
            return;
        }
        S32_AT(self, 0x180) = 3;
        return;
    case 3:
        U32_AT(self, 0xc8) |= 2;
        return;
    }
}

/* 0x0204b65c: place the object and draw it if in view
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 $S=zero:0x190 @$S+0x94:32=pick:0,8 @$S+0xc8:32=pick:0,4 @$S+0x180:32=int:0:4 @$S+0x184:32=int:0:3 @$S+0xb8:32=int:-0x10000:0x10000 @$S+0xbc:32=int:-0x10000:0x10000 @$S+0xa0:32=int:-0x10:0 @$S+0xa4:32=int:-0x10:0 @$S+0xa8:32=int:0:0x10 @$S+0xac:32=int:0:0x10 $S stub=0x0201f490:0x020deebc,0x020186ec:0,0x02043cc8,0x02017abc cases=30 */
void func_0204b65c(u8 *self)
{
    s32 xz[2], p[3], b[4];
    u32 cam[0x2c0 / 4];
    s32 vis;

    if (BITS(U32_AT(self, 0x94), 3, 1) == 0)
        return;
    func_0204b82c(self);
    xz[0] = S32_AT(self, 0xb8);
    xz[1] = S32_AT(self, 0xbc);
    func_0201c17c(p, xz);
    S32_AT(self, 4) = p[0];
    S32_AT(self, 8) = p[1];
    S32_AT(self, 0xc) = p[2];
    self[0x8c] = 1;
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
        return;
    }
    func_02043cc8((u8 *)func_02019670() + 4, self + 0x168);
    if (BITS(U32_AT(self, 0xc8), 2, 1) != 0)
        func_02017abc((u8 *)func_020062c0() + 4, ACTOR(self));
    U32_AT(self, 0xc8) |= 0x2000;
}

/* 0x0204b82c: dim the object in the form it isn't meant for
 * @difftest $S=zero:0x190 @$S+0x180:32=int:0:4 @$S+0x184:32=int:0:4 @$S+0x188:8=pick:0,1 $S stub=0x020186ec:0 cases=20
 * @difftest $S=zero:0x190 @$S+0x180:32=int:0:4 @$S+0x184:32=int:0:4 @$S+0x188:8=pick:0,1 $S stub=0x020186ec:1 cases=20 */
void func_0204b82c(u8 *self)
{
    if ((u32)(S32_AT(self, 0x180) - 2) <= 1)
        return;
    if (func_020186ec() == 1) {
        switch (S32_AT(self, 0x184)) {
        case 0:
            self[0x188] = 1;
            break;
        case 1:
        case 2:
            self[0x188] = 0;
            break;
        }
    } else {
        switch (S32_AT(self, 0x184)) {
        case 0:
        case 2:
            self[0x188] = 0;
            break;
        case 1:
            self[0x188] = 1;
            break;
        }
    }
    if (S8_AT(self, 0x188) != 0) {
        U32_AT(self, 0xcc) = 0xf;
        U32_AT(self, 0xc8) &= ~4u;
        return;
    }
    U32_AT(self, 0xcc) = 0x1f;
    U32_AT(self, 0xc8) |= 4;
}

/* what the destructors do */
static void Destroy(u8 *self)
{
    PTR_AT(self, 0) = data_020bfdf4;
    PTR_AT(self, 0x90) = data_020bfe18;
    func_02008130(S8_AT(self, 0xd2), S16_AT(self, 0xd0));
    PTR_AT(self, 0x168) = data_020bfe64;
    PTR_AT(self, 0x168) = data_020bf760;
    func_0205eef0(self + 0xdc);
    func_020506c0(self + 0x90);
    func_0205b784(self);
}

/* 0x0204b90c: the object's destructor
 * @difftest zero:0x190 stub=0x02008130,0x0205eef0,0x020506c0,0x0205b784 cases=3 */
u8 *func_0204b90c(u8 *self)
{
    Destroy(self);
    return self;
}

/* 0x0204b974: the object's deleting destructor
 * @difftest zero:0x190 stub=0x02008130,0x0205eef0,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_0204b974(u8 *self)
{
    Destroy(self);
    func_0207fe44(self);
    return self;
}

/* 0x0204b9e4: the object's destructor (another copy)
 * @difftest zero:0x190 stub=0x02008130,0x0205eef0,0x020506c0,0x0205b784 cases=3 */
u8 *func_0204b9e4(u8 *self)
{
    Destroy(self);
    return self;
}

/* 0x0204ba4c: the draw callback's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_0204ba4c(u8 *self)
{
    PTR_AT(self, 0) = data_020bfe64;
    PTR_AT(self, 0) = data_020bf760;
    return self;
}

/* the common end of the constructors */
static void Finish(u8 *self, const u32 *m1, const u32 *m2)
{
    u8 *d = self + 0x168;
    u32 a[2], b[2];

    b[0] = m2[0];
    b[1] = m2[1];
    a[0] = m1[0];
    a[1] = m1[1];
    PTR_AT(d, 0) = data_020bf760;
    PTR_AT(d, 0) = data_020bfe64;
    PTR_AT(d, 4) = self;
    U32_AT(d, 8) = a[0];
    U32_AT(d, 0xc) = a[1];
    U32_AT(d, 0x10) = b[0];
    U32_AT(d, 0x14) = b[1];
    S32_AT(self, 0x180) = 0;
    U32_AT(self, 0x94) |= 8;
    func_0204b3b4(self);
}

/* 0x0204ba68: the object's constructor (animation `a` / `b`, at `pos`)
 * @difftest zero:0x190 u32 u32 ptr:8:4 stub=0x0205b7c8,0x02050890,0x0205ef34,0x02017abc cases=3 */
u8 *func_0204ba68(u8 *self, u32 a, u32 b, s32 *pos)
{
    s32 box3[3], box4[4];

    box3[0] = data_020bfd64;
    box3[1] = data_020bfd7c;
    box3[2] = data_020bfd8c;
    func_0205b7c8(self, box3, data_020ebd1c, data_020ebd04);
    box4[0] = data_020bfd80;
    box4[1] = data_020bfd58;
    box4[2] = data_020bfd78;
    box4[3] = data_020bfd74;
    func_02050890(self + 0x90, pos, box4);
    PTR_AT(self, 0) = data_020bfdf4;
    PTR_AT(self, 0x90) = data_020bfe18;
    func_0205ef34(self + 0xdc, a, b);
    Finish(self, data_020bfd9c, data_020bfdac);
    return self;
}

/* 0x0204bbe0: the object's constructor (animation `a`, at `pos`)
 * @difftest zero:0x190 u32 ptr:8:4 stub=0x0205b7c8,0x02050890,0x0205ef8c,0x02017abc cases=3 */
u8 *func_0204bbe0(u8 *self, u32 a, s32 *pos)
{
    s32 box3[3], box4[4];

    box3[0] = data_020bfd68;
    box3[1] = data_020bfd5c;
    box3[2] = data_020bfd98;
    func_0205b7c8(self, box3, data_020ebd1c, data_020ebd04);
    box4[0] = data_020bfd94;
    box4[1] = data_020bfd84;
    box4[2] = data_020bfd90;
    box4[3] = data_020bfd60;
    func_02050890(self + 0x90, pos, box4);
    PTR_AT(self, 0) = data_020bfdf4;
    PTR_AT(self, 0x90) = data_020bfe18;
    func_0205ef8c(self + 0xdc, a);
    Finish(self, data_020bfda4, data_020bfdb4);
    return self;
}

/* 0x0204bd50: the draw callback's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_0204bd50(u8 *self)
{
    PTR_AT(self, 0) = data_020bfe64;
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

/* 0x0204bd80: call the draw callback's first member function
 * @difftest $D=zero:0x18 @$D+8:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+8:32=8 @$D+0xc:32=pick:1,9 $D cases=5 */
void func_0204bd80(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 8) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 8);
}

/* 0x0204bde0: call the draw callback's second member function
 * @difftest $D=zero:0x18 @$D+0x10:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+0x10:32=8 @$D+0x14:32=pick:1,9 $D cases=5 */
void func_0204bde0(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 0x10) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 0x10);
}

/* The actor's this-adjusting entry points (the object is 0x90 before it). */

/* 0x0204be40
 * @difftest $S=zero:0x190 $S+0x90 stub=0x02008130,0x0205eef0,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_0204be40(u8 *a)
{
    return func_0204b974(a - 0x90);
}

/* 0x0204be50
 * @difftest $S=zero:0x190 $S+0x90 cases=3 */
void func_0204be50(u8 *a)
{
    func_0204b36c(a - 0x90);
}

/* 0x0204be60
 * @difftest $S=zero:0x190 $S+0x90 stub=0x0201b650:1 cases=3 */
void func_0204be60(u8 *a)
{
    func_0204b370(a - 0x90);
}

/* 0x0204be70
 * @difftest $S=zero:0x190 @$S+0x180:32=3 $S+0x90 int:0:4 cases=3 */
void func_0204be70(u8 *a, s32 dt)
{
    func_0204b4d4(a - 0x90, dt);
}

/* 0x0204be80
 * @difftest $S=zero:0x190 $S+0x90 stub=0x02008130,0x0205eef0,0x020506c0,0x0205b784 cases=3 */
u8 *func_0204be80(u8 *a)
{
    return func_0204b9e4(a - 0x90);
}

/* 0x0204be90: draw the looping kind at its frame
 * @difftest $T=zero:0x10 @$T+4:32=0x020343a8 $S=zero:0x190 @$S+0xdc:32=$T @$S+0x18c:32=int:0:0x100 $S stub=0x0205ed6c,0x02080e84 cases=3 */
void func_0204be90(u8 *self)
{
    func_0205ed6c(self + 0xdc, S32_AT(self, 0x18c));
    func_0204b3e8(self);
}

/* 0x0204beb4: the looping kind's update
 * @difftest $S=zero:0x190 @$S+0x18c:32=int:0:0x100 @$S+0x180:32=3 $S int:0:8 stub=0x0205ed14:0x80 cases=20 */
void func_0204beb4(u8 *self, s32 dt)
{
    S32_AT(self, 0x18c) = dt * 0x1e + S32_AT(self, 0x18c);
    if (S32_AT(self, 0x18c) > func_0205ed14(self + 0xdc))
        S32_AT(self, 0x18c) = 0;
    func_0204b4d4(self, dt);
}

/* 0x0204bf08: the looping kind's deleting destructor
 * @difftest zero:0x190 stub=0x02008130,0x0205eef0,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_0204bf08(u8 *self)
{
    PTR_AT(self, 0) = data_020bfec8;
    PTR_AT(self, 0x90) = data_020bfeec;
    func_0204b90c(self);
    func_0207fe44(self);
    return self;
}

/* 0x0204bf40: the looping kind's destructor
 * @difftest zero:0x190 stub=0x02008130,0x0205eef0,0x020506c0,0x0205b784 cases=3 */
u8 *func_0204bf40(u8 *self)
{
    PTR_AT(self, 0) = data_020bfec8;
    PTR_AT(self, 0x90) = data_020bfeec;
    func_0204b90c(self);
    return self;
}

/* 0x0204bf70: the looping kind's constructor
 * @difftest zero:0x190 u32 ptr:8:4 stub=0x0205b7c8,0x02050890,0x0205ef8c,0x02017abc cases=3 */
u8 *func_0204bf70(u8 *self, u32 a, s32 *pos)
{
    func_0204bbe0(self, a, pos);
    PTR_AT(self, 0) = data_020bfec8;
    PTR_AT(self, 0x90) = data_020bfeec;
    S32_AT(self, 0x18c) = 0;
    return self;
}

/* 0x0204bfa8: create looping scenery `id` (of map object list `k`)
 * @difftest $G=zero:0x200 @020df578:32=$G+0x80 int:0:4 int:0:2 stub=0x0207fe74:0 cases=5 */
u8 *func_0204bfa8(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    s32 v[2], t[2], u[2];
    u32 var;
    u8 *p = NULL;

    v[0] = S16_AT(def, 4);
    v[1] = S16_AT(def, 6);
    func_02003b08(t, v, 0xc);
    v[0] = t[0];
    v[1] = t[1];
    var = def[0xb];
    if (var == 0) {
        p = func_0207fe74(0x190);
        if (p != NULL)
            p = func_0204bf70(p, 0x812, v);
    } else if (var == 1) {
        p = func_0207fe74(0x190);
        if (p != NULL)
            p = func_0204bf70(p, 0x80c, v);
    } else if (var == 2) {
        p = func_0207fe74(0x190);
        if (p != NULL)
            p = func_0204bf70(p, 0x80d, v);
    }
    func_02003b08(t, v, 4);
    v[0] = t[0];
    v[1] = t[1];
    u[0] = t[0];
    u[1] = t[1];
    S32_AT(p, 0xb4) = func_0201a0a0(u, NULL);
    U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x20u) | ((def[8] & 1) << 5);
    U32_AT(p, 0xc8) = (U32_AT(p, 0xc8) & ~0x40u) | ((def[9] & 1) << 6);
    U32_AT(p, 0x184) = var;
    S32_AT(p, 0x9c) = 0xd;
    return ACTOR(p);
}

/* 0x0204c118
 * @difftest $S=zero:0x190 $S+0x90 stub=0x02008130,0x0205eef0,0x020506c0,0x0205b784,0x0207fe44 cases=3 */
u8 *func_0204c118(u8 *a)
{
    return func_0204bf08(a - 0x90);
}

/* 0x0204c128
 * @difftest $S=zero:0x190 @$S+0x180:32=3 $S+0x90 int:0:4 stub=0x0205ed14:0x80 cases=3 */
void func_0204c128(u8 *a, s32 dt)
{
    func_0204beb4(a - 0x90, dt);
}

/* 0x0204c138
 * @difftest $S=zero:0x190 $S+0x90 stub=0x02008130,0x0205eef0,0x020506c0,0x0205b784 cases=3 */
u8 *func_0204c138(u8 *a)
{
    return func_0204bf40(a - 0x90);
}
