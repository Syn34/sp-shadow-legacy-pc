/*
 * Camera helpers: keeping a view (+0 position, +0xc angles, +0x18 its
 * matrix, +0x270.. the settings it was made for, +0x280 dirty flags,
 * +0x284 has bounds, +0x2a0 the bounds) in step with the object it
 * follows, and the camera target objects (vtable data_020c0870, 0xfc
 * bytes) that look at a point.
 * ARM9 main, 0x02053dfc - 0x02054468 (13 functions).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020b6cb8, data_020b6cbc, data_020b6cc4, data_020b6ce0;
extern const fx32 data_020c081c, data_020c0820, data_020c0824, data_020c0828, data_020c082c,
    data_020c0830, data_020c0834, data_020c0838, data_020c083c, data_020c0840, data_020c0844,
    data_020c0848, data_020c084c;
extern u8 data_020c0870[];           /* vtable */
extern const fx32 data_020f6e58[], data_020f6e7c[];
extern u8 data_020f6ea0[];

void func_02005b98(s32 *out, const void *src);
void func_0201751c(s32 *out, const void *obj);
void func_02002978(s32 *mtx, const fx32 *angles);
void func_02005c1c(s32 *out, const s32 *a, const s32 *b);
void func_02002bd8(fx32 *out, const fx32 *vec2, const fx32 *len);
void func_02002164(void *v);
void func_020036e8(s32 *rect, const s32 *a, const s32 *b, const s32 *c, const s32 *d);
void func_0202a104(s32 *out, const s32 *a, const s32 *b);
s32 func_0208f30c(s32 x, s32 y);
s32 func_0208eff8(s32 v);
s32 func_0202df68(const s32 *v);
void func_0202ded8(s32 *v, const s32 *s);
void func_0202dea4(s32 *v);
void func_0205b784(void *self);
void func_0205b7c8(void *self, s32 *box, void *a, void *b);
void func_0207fe44(void *p);

void func_02053f58(s32 *d, const s32 *s);
s8 func_02053f74(const s32 *a, const s32 *b);
void func_02053fc8(s32 *rect, s32 dx, s32 dz);
void func_02054018(s32 *v, const fx32 *a);
s8 func_02054050(u8 *cam);
void func_0205405c(s32 *d, const s32 *s);
s8 func_02054078(const s32 *a, const s32 *b);

/* 0x02053dfc: follow object `obj`: its position (moving the bounds with
 * it), angles and settings
 * @difftest $O=ptr:0x100:4 $C=ptr:0x2c0:4 @$C+0x284:8=pick:0,1 $O $C cases=10
 * @difftest $O=ptr:0x100:4 $C=ptr:0x2c0:4 @$C+0x284:8=pick:0,1 @$C+0:32=0 @$C+4:32=0 @$C+8:32=0 @$O+4:32=0 @$O+8:32=0 @$O+0xc:32=0 $O $C cases=5 */
void func_02053dfc(u8 *obj, u8 *cam)
{
    s32 p[3], r[3];

    func_02005b98(p, obj);
    if (func_02054078((s32 *)cam, p)) {
        func_0205405c((s32 *)cam, p);
        U32_AT(cam, 0x280) |= 8;
        if (func_02054050(cam)) {
            s32 v[2], a, b;

            func_02005c1c(v, (s32 *)(cam + 0x298), (s32 *)(cam + 0x2b0));
            func_02054018(v, &data_020c081c);
            a = -v[0];
            b = -v[1];
            func_020036e8((s32 *)(cam + 0x2a0), &b, &a, &v[1], &v[0]);
            func_02053fc8((s32 *)(cam + 0x2a0), p[0], p[1]);
            func_02002164(v);
        }
    }
    func_0201751c(r, obj);
    if (func_02053f74(r, (s32 *)(cam + 0xc))) {
        func_02053f58((s32 *)(cam + 0xc), r);
        func_02002978((s32 *)(cam + 0x18), r);
        S32_AT(cam, 0x280) = -1;
    }
    if (U32_AT(cam, 0x270) != U32_AT(obj, 0x94) || U32_AT(cam, 0x274) != U32_AT(obj, 0x98)) {
        U32_AT(cam, 0x270) = U32_AT(obj, 0x94);
        U32_AT(cam, 0x274) = U32_AT(obj, 0x98);
        S32_AT(cam, 0x280) = -1;
    }
    if (U32_AT(obj, 0x90) != U32_AT(cam, 0x278)) {
        U32_AT(cam, 0x278) = U32_AT(obj, 0x90);
        S32_AT(cam, 0x280) = -1;
    }
}

/* 0x02053f58: copy a vector
 * @difftest ptr:0xc:4 ptr:0xc:4 cases=3 */
void func_02053f58(s32 *d, const s32 *s)
{
    d[0] = s[0];
    d[1] = s[1];
    d[2] = s[2];
}

/* 0x02053f74: whether two vectors differ
 * @difftest $A=ptr:0xc:4 @$A:32=int:0:2 @$A+4:32=int:0:2 @$A+8:32=int:0:2 $B=ptr:0xc:4 @$B:32=int:0:2 @$B+4:32=int:0:2 @$B+8:32=int:0:2 $A $B cases=30 */
s8 func_02053f74(const s32 *a, const s32 *b)
{
    return !(a[0] == b[0] && a[1] == b[1] && a[2] == b[2]);
}

/* 0x02053fc8: move a rectangle {z0, x0, z1, x1} by (dx, dz)
 * @difftest ptr:0x10:4 s32 s32 cases=5 */
void func_02053fc8(s32 *rect, s32 dx, s32 dz)
{
    rect[1] += dx;
    rect[3] += dx;
    rect[0] += dz;
    rect[2] += dz;
}

/* 0x02054018: turn a 2D vector
 * @difftest ptr:8:4 $A=ptr:4:4 @$A:32=int:0:0x6488 $A cases=10 */
void func_02054018(s32 *v, const fx32 *a)
{
    s32 t[2];

    func_02002bd8(t, v, a);
    v[0] = t[0];
    v[1] = t[1];
}

/* 0x02054050: whether the view has bounds
 * @difftest ptr:0x2c0:4 cases=3 */
s8 func_02054050(u8 *cam)
{
    return S8_AT(cam, 0x284);
}

/* 0x0205405c: copy a vector
 * @difftest ptr:0xc:4 ptr:0xc:4 cases=3 */
void func_0205405c(s32 *d, const s32 *s)
{
    d[0] = s[0];
    d[1] = s[1];
    d[2] = s[2];
}

/* 0x02054078: whether two vectors differ
 * @difftest $A=ptr:0xc:4 @$A:32=int:0:2 @$A+4:32=int:0:2 @$A+8:32=int:0:2 $B=ptr:0xc:4 @$B:32=int:0:2 @$B+4:32=int:0:2 @$B+8:32=int:0:2 $A $B cases=30 */
s8 func_02054078(const s32 *a, const s32 *b)
{
    return !(b[0] == a[0] && b[1] == a[1] && b[2] == a[2]);
}

/* 0x020540cc: set the distance
 * @difftest ptr:0xfc:4 s32 cases=3 */
void func_020540cc(u8 *self, s32 d)
{
    S32_AT(self, 0x98) = d;
}

/* turn the object to look along `d` */
static void LookAlong(u8 *self, const s32 *d)
{
    s32 yaw = func_0208f30c(d[0], d[2]);
    s32 len = func_0208eff8(FxMul(d[0], d[0]) + FxMul(d[2], d[2]));

    S32_AT(self, 0x10) = -func_0208f30c(d[1], len);
    S32_AT(self, 0x14) = yaw;
    S32_AT(self, 0x18) = g_FxZero;
    self[0x8c] = 1;
}

/* 0x020540d4: look at `t`
 * @difftest $S=ptr:0xfc:4 @$S+4:32=int:-0x100000:0x100000 @$S+8:32=int:-0x100000:0x100000 @$S+0xc:32=int:-0x100000:0x100000 $T=ptr:0xc:4 @$T:32=int:-0x100000:0x100000 @$T+4:32=int:-0x100000:0x100000 @$T+8:32=int:-0x100000:0x100000 $S $T cases=30 */
void func_020540d4(u8 *self, const s32 *t)
{
    s32 p[3], d[3];

    S32_AT(self, 0x9c) = t[0];
    S32_AT(self, 0xa0) = t[1];
    S32_AT(self, 0xa4) = t[2];
    func_02005b98(p, self);
    func_0202a104(d, (s32 *)(self + 0x9c), p);
    LookAlong(self, d);
}

/* 0x0205419c: the camera target's deleting destructor
 * @difftest zero:0xfc stub=0x0205b784,0x0207fe44 cases=3 */
u8 *func_0205419c(u8 *self)
{
    PTR_AT(self, 0) = data_020c0870;
    func_0205b784(self);
    func_0207fe44(self);
    return self;
}

/* 0x020541c8: the camera target's destructor
 * @difftest zero:0xfc stub=0x0205b784 cases=3 */
u8 *func_020541c8(u8 *self)
{
    PTR_AT(self, 0) = data_020c0870;
    func_0205b784(self);
    return self;
}

/* 0x020541ec: the camera target's constructor
 * @difftest ptr:0xfc:4 stub=0x0205b7c8 cases=3 */
u8 *func_020541ec(u8 *self)
{
    s32 box[3], p[3], d[3], n;

    box[0] = data_020c084c;
    box[1] = data_020c0848;
    box[2] = data_020c0844;
    func_0205b7c8(self, box, (void *)data_020f6e58, data_020f6ea0);
    PTR_AT(self, 0) = data_020c0870;
    S32_AT(self, 0x90) = data_020b6cb8;
    S32_AT(self, 0x94) = 0;
    S32_AT(self, 0x98) = 0x500000;
    S32_AT(self, 0x9c) = data_020c0840;
    S32_AT(self, 0xa0) = data_020c083c;
    S32_AT(self, 0xa4) = data_020c0838;
    S32_AT(self, 0xa8) = 0;
    S32_AT(self, 0xac) = 0;
    S32_AT(self, 0xb0) = 0;
    S32_AT(self, 0xb4) = data_020f6e7c[0];
    S32_AT(self, 0xb8) = data_020f6e7c[1];
    S32_AT(self, 0xbc) = data_020f6e7c[2];
    S32_AT(self, 0xc0) = data_020c0820;
    S32_AT(self, 0xc4) = data_020c0834;
    S32_AT(self, 0xc8) = data_020c0830;
    S32_AT(self, 0xcc) = data_020f6e58[0];
    S32_AT(self, 0xd0) = data_020f6e58[1];
    S32_AT(self, 0xd4) = data_020f6e58[2];
    S32_AT(self, 0xd8) = data_020b6cbc;
    S32_AT(self, 0xdc) = 0;
    S32_AT(self, 0xe0) = 0x500000;
    S32_AT(self, 0xe4) = data_020b6ce0;
    S32_AT(self, 0xe8) = 0x32000;
    S32_AT(self, 0xec) = 0xc8000;
    S32_AT(self, 0xf0) = data_020b6cc4;
    S32_AT(self, 0xf4) = 0xc8000;
    S32_AT(self, 0xf8) = 0;
    func_02005b98(p, self);
    func_0202a104(d, (s32 *)(self + 0x9c), p);
    n = func_0202df68(d);
    if (n > 1)
        func_0202ded8(d, &n);
    else
        func_0202dea4(d);
    LookAlong(self, d);
    S32_AT(self, 0xa8) = data_020c082c;
    S32_AT(self, 0xac) = data_020c0828;
    S32_AT(self, 0xb0) = data_020c0824;
    return self;
}
