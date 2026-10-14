/*
 * Pausing the world, and a marker actor (model kind 0x33) that stays at a
 * fixed point next to the level (data_020df1bc).
 * ARM9 main, 0x0204378c - 0x02043d68 (23 functions).
 *
 * func_0204378c / func_020437c4 stop and restart the actors, the models
 * and the particles of a screen (data_020ebba0: 1 while stopped).
 *
 * The marker (vtable data_020bf6d0, an actor): its per-frame update puts
 * its model 0xa000 (0x19000 outside a level) behind the point and
 * registers its callback object (+0x4c) with func_02019670's list. A
 * callback object (vtables data_020bf724 / data_020bf760) holds +4 an
 * object and +8 / +0x10 two member function pointers {function, this
 * adjustment * 2 + virtual}.
 *
 * Testing: with fake objects; the actor and model set-up stubbed where it
 * needs a real actor.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const u32 data_020bf6ac[2], data_020bf6b4[2];   /* the member functions */
extern u8 data_020bf6d0[];           /* the marker's vtable */
extern u8 data_020bf724[], data_020bf760[];            /* the callbacks' vtables */
extern const char data_020bf788[];   /* source file name for the heap */
extern s8 data_020df10c;             /* inside a level */
extern u8 data_020df1bc[];           /* the marker's point */
extern s8 data_020ebba0;             /* the world is stopped */

void func_02004008(s32 screen);
void func_02003f8c(s32 screen);
void func_0205932c(void *mgr);
void func_020593a0(void *mgr);
void func_02050a6c(void *mgr);
void func_02050ae0(void *mgr);
void *func_020062a0(void);
void *func_020062b0(void);
void *func_02019670(void);
void func_020062f0(void *vec, void **value, u32 tag);
void func_0207fe28(const char *file, s32 line);
void func_0207fe24(void);
void func_0207fe44(void *p);
void func_020504b8(void *actor);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
void func_02053460(void *obj, u32 dt);
void func_02017abc(void *list, void *item);
void *func_020506c0(void *actor);
void func_0205092c(void *actor);
void func_02005b98(s32 *out, const void *src);
void *func_02016f24(void *self);

void func_02043844(u8 *self);
void func_02043848(u8 *self, u32 kind);
void func_02043cc8(u8 *list, void *item);

/* 0x0204378c: restart the world on screen `screen`
 * @difftest int:0:1 stub=0x02004008,0x0205932c,0x02050a6c cases=3 */
void func_0204378c(s32 screen)
{
    func_02004008(screen);
    func_0205932c(func_020062a0());
    func_02050a6c(func_020062b0());
    data_020ebba0 = 0;
}

/* 0x020437c4: stop the world on screen `screen`
 * @difftest int:0:1 stub=0x02003f8c,0x020593a0,0x02050ae0 cases=3 */
void func_020437c4(s32 screen)
{
    func_02003f8c(screen);
    func_020593a0(func_020062a0());
    func_02050ae0(func_020062b0());
    data_020ebba0 = 1;
}

/* 0x020437fc: is the world stopped?
 * @difftest @020ebba0:8=pick:0,1,0xff cases=3 */
s32 func_020437fc(void)
{
    return data_020ebba0;
}

/* 0x0204380c
 * @difftest cases=3 */
u32 func_0204380c(void)
{
    return 0x19;
}

/* 0x02043814
 * @difftest cases=3 */
u32 func_02043814(void)
{
    return 0;
}

/* 0x0204381c
 * @difftest cases=3 */
void func_0204381c(void)
{
}

/* 0x02043820: register the marker's callback
 * @difftest zero:0x70 cases=5 */
s32 func_02043820(u8 *self)
{
    func_02043cc8((u8 *)func_02019670() + 4, self + 0x4c);
    return 1;
}

/* 0x02043844
 * @difftest ptr:0x70 cases=3 */
void func_02043844(u8 *self)
{
}

/* 0x02043848: set up a marker with model kind `kind`
 * @difftest $O=zero:0x200 $S=zero:0x70 @$S+0x44:32=$O @$S+0x38:32=int:0:0xffffffff $S int:0:0x40 stub=0x020504b8,0x02053648,0x02017abc cases=10 */
void func_02043848(u8 *self, u32 kind)
{
    u8 *o;

    S32_AT(self, 0xc) = -1;
    S32_AT(self, 8) = kind;
    U32_AT(self, 0x38) = (U32_AT(self, 0x38) & ~1u) | 1;
    U32_AT(self, 0x38) &= ~4u;
    U32_AT(self, 4) |= 8;
    U32_AT(self, 0x38) |= 0x800000;
    func_020504b8(self);
    func_02053648(PTR_AT(self, 0x44), kind, self, 0, 0);
    o = PTR_AT(self, 0x44);
    S32_AT(o, 4) = 0;
    S32_AT(o, 8) = 0;
    S32_AT(o, 0xc) = 0;
    o[0x8c] = 1;
    o = PTR_AT(self, 0x44);
    S32_AT(o, 0x1c) = 0x1800;
    S32_AT(o, 0x20) = 0x1800;
    S32_AT(o, 0x24) = 0x1800;
    o[0x8c] = 1;
    o = PTR_AT(self, 0x44);
    S32_AT(o, 0x10) = g_FxZero;
    S32_AT(o, 0x14) = g_FxZero;
    S32_AT(o, 0x18) = g_FxZero;
    o[0x8c] = 1;
    func_02017abc((u8 *)func_020062b0() + 4, self);
    U32_AT(self, 0x38) |= 0x2000;
}

/* 0x0204393c: the model's method +0xc
 * @difftest $T=zero:0x20 @$T+0xc:32=0x020343a8 $O=zero:0x10 @$O+0:32=$T $S=zero:0x70 @$S+0x44:32=$O $S cases=3 */
void func_0204393c(u8 *self)
{
    u8 *o = PTR_AT(self, 0x44);
    void (*m)(void *, void *) = VCALL(o, 0xc, void (*)(void *, void *));

    m(o, (void *)m);
}

/* 0x02043960: the model's method +8
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x10 @$O+0:32=$T $S=zero:0x70 @$S+0x44:32=$O $S cases=3 */
void func_02043960(u8 *self)
{
    u8 *o = PTR_AT(self, 0x44);
    void (*m)(void *, void *) = VCALL(o, 8, void (*)(void *, void *));

    m(o, (void *)m);
}

/* 0x02043984: the marker's per-frame update
 * @difftest $O=zero:0x200 $S=zero:0x70 @$S+0x44:32=$O @$S+4:32=pick:0,4 @020df10c:8=pick:0,1 $S int:0:0x2000 stub=0x02053460 cases=20 */
void func_02043984(u8 *self, u32 dt)
{
    if ((s8)BITS(U32_AT(self, 4), 2, 1) == 0) {
        s32 a[3], b[3];
        u8 *o;

        func_02005b98(a, func_02016f24(data_020deebc));
        func_02005b98(b, data_020df1bc);
        if (data_020df10c != 0)
            a[2] = b[2] - 0xa000;
        else
            a[2] = b[2] - 0x19000;
        a[0] = b[0];
        a[1] = b[1];
        o = PTR_AT(self, 0x44);
        S32_AT(o, 4) = a[0];
        S32_AT(o, 8) = a[1];
        S32_AT(o, 0xc) = a[2];
        o[0x8c] = 1;
        func_02053460(PTR_AT(self, 0x44), dt);
    }
    func_02043820(self);
}

/* 0x02043a50: the marker's deleting destructor
 * @difftest zero:0x70 stub=0x020506c0,0x0207fe44 cases=3 */
u8 *func_02043a50(u8 *self)
{
    PTR_AT(self, 0) = data_020bf6d0;
    func_02043844(self);
    PTR_AT(self, 0x4c) = data_020bf724;
    PTR_AT(self, 0x4c) = data_020bf760;
    func_020506c0(self);
    func_0207fe44(self);
    return self;
}

/* 0x02043a9c: the marker's destructor
 * @difftest zero:0x70 stub=0x020506c0 cases=3 */
u8 *func_02043a9c(u8 *self)
{
    PTR_AT(self, 0) = data_020bf6d0;
    func_02043844(self);
    PTR_AT(self, 0x4c) = data_020bf724;
    PTR_AT(self, 0x4c) = data_020bf760;
    func_020506c0(self);
    return self;
}

/* 0x02043ae0: the marker's constructor
 * @difftest zero:0x70 stub=0x0205092c,0x02043848 cases=3 */
u8 *func_02043ae0(u8 *self)
{
    u8 *d = self + 0x4c;
    u32 m1[2], m2[2];

    func_0205092c(self);
    PTR_AT(self, 0) = data_020bf6d0;
    m2[0] = data_020bf6ac[0];
    m2[1] = data_020bf6ac[1];
    m1[0] = data_020bf6b4[0];
    m1[1] = data_020bf6b4[1];
    PTR_AT(d, 0) = data_020bf760;
    PTR_AT(d, 0) = data_020bf724;
    PTR_AT(d, 4) = self;
    U32_AT(d, 8) = m1[0];
    U32_AT(d, 0xc) = m1[1];
    U32_AT(d, 0x10) = m2[0];
    U32_AT(d, 0x14) = m2[1];
    func_02043848(self, 0x33);
    return self;
}

/* 0x02043b88: a callback's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_02043b88(u8 *self)
{
    PTR_AT(self, 0) = data_020bf724;
    PTR_AT(self, 0) = data_020bf760;
    return self;
}

/* 0x02043ba4: a callback's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_02043ba4(u8 *self)
{
    PTR_AT(self, 0) = data_020bf724;
    PTR_AT(self, 0) = data_020bf760;
    func_0207fe44(self);
    return self;
}

/* 0x02043bd4: the callback base's destructor
 * @difftest zero:0x18 cases=3 */
u8 *func_02043bd4(u8 *self)
{
    PTR_AT(self, 0) = data_020bf760;
    return self;
}

/* 0x02043be4: the callback base's deleting destructor
 * @difftest zero:0x18 stub=0x0207fe44 cases=3 */
u8 *func_02043be4(u8 *self)
{
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

/* 0x02043c08: call a callback's first member function
 * @difftest $D=zero:0x18 @$D+8:32=pick:0,0x020343a8 $D cases=4
 * @difftest $O=zero:0x20 $D=zero:0x18 @$D+4:32=$O @$D+8:32=pick:0,0x020343a8 $D cases=4
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+8:32=8 @$D+0xc:32=pick:1,9 $D cases=5
 * @difftest $O=zero:0x20 $D=zero:0x18 @$D+4:32=$O @$D+8:32=0x020343a8 @$D+0xc:32=pick:0,8,-8 $D cases=5 */
void func_02043c08(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 8) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 8);
}

/* 0x02043c68: call a callback's second member function
 * @difftest $O=zero:0x20 $D=zero:0x18 @$D+4:32=$O @$D+0x10:32=pick:0,0x020343a8 @$D+0x14:32=pick:0,8 $D cases=10
 * @difftest $T=zero:0x20 @$T+8:32=0x020343a8 $O=zero:0x20 @$O+0:32=$T @$O+4:32=$T $D=zero:0x18 @$D+4:32=$O @$D+0x10:32=8 @$D+0x14:32=pick:1,9 $D cases=5 */
void func_02043c68(u8 *self)
{
    if (PTR_AT(self, 4) == NULL)
        return;
    if (PTR_AT(self, 0x10) == NULL)
        return;
    CallMember(PTR_AT(self, 4), self + 0x10);
}

/* 0x02043cc8: vector<callback *>::push_back
 * @difftest $L=ptr:0x20:4 $M=zero:0x10 @$M+4:32=$L @$M+8:32=int:0:3 @$M+0xc:32=int:3:8 $M u32 */
void func_02043cc8(u8 *list, void *item)
{
    func_0207fe28(data_020bf788, 0x58);
    if (U32_AT(list, 8) < U32_AT(list, 0xc)) {
        U32_AT(list, 8) += 1;
        ((void **)PTR_AT(list, 4))[U32_AT(list, 8) - 1] = item;
    } else {
        func_020062f0(list + 4, &item, 0);
    }
    func_0207fe24();
}

/* 0x02043d64
 * @difftest cases=3 */
void func_02043d64(void)
{
}
