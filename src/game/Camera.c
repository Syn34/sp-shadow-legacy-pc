/*
 * The 2D camera that follows an actor over the scrolling map, its screen
 * shake, and the 3D camera placed to match it.
 * ARM9 main, 0x0201f004 - 0x0201ff6c (37 functions).
 *
 * The camera, data_020e22c4:
 *   +0x00/+0x04 position (fx32, the centre of the view), +0x08 distance to
 *   the target when a move starts, +0x0c move speed, +0x10/+0x14 current and
 *   wanted height of the target, +0x18/+0x1c/+0x20/+0x24 the bounds
 *   (min y, min x, max y, max x), +0x28 the target actor (its position is
 *   at +0x28), +0x2c the previous target, +0x30 mode (1 moving to the
 *   target, 2 following it, 3 off, 4 shaking), +0x34/+0x38 shake offsets,
 *   +0x3c shake time left, +0x40 u16 shake phase.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020df2b8[];           /* 3D camera matrices */
extern u8 data_020e22c4[];           /* the camera */

int rand(void);
void MI_Copy64B(const void *src, void *dest);
void func_02002bd8(fx32 *out, const fx32 *vec2, const fx32 *len);
void func_02005b98(s32 *out, const void *src);
void func_02005c1c(s32 *out, const s32 *a, const s32 *b);
void func_020150a0(s32 *out, const s32 *a, const s32 *b);
void func_02018370(s32 *out);
void func_02019280(void);
void func_020540cc(void *cam3d, s32 v);
void func_020540d4(void *cam3d, const s32 *pos);
void func_02053dfc(void *cam3d, void *out);
s32 func_020a4e00(s32 num, s32 den);    /* signed division */

void *func_0201f354(u8 *dst, const u8 *src);
void func_0201f744(s32 *out);
void func_0201f9ac(u8 *c);
void func_0201fa3c(u8 *c, s32 dt);
u32 func_0201fbbc(u8 *c);
void func_0201fe28(u8 *c);
void func_0201fe58(u8 *c, s32 miny, s32 minx, s32 maxy, s32 maxx);
void func_0201fea0(u8 *c, const s32 *pos);
s32 *func_0201fef4(u8 *c);
u8 *func_0201fef8(u8 *c);
void func_0201ff00(u8 *c, void *actor);
s32 *func_0201ff44(u8 *c);
void func_0201ff50(u8 *c, s32 v);
s32 func_0201ff58(u8 *c);
void func_0201ff60(u8 *c, s32 mode);

#define CAM data_020e22c4
#define LEVEL() (data_020c3b60 + (func_020436f0(data_020bf6a0)[0xf] >> 1) * 0x60)

/* t << 12 rounded the way the game does it */
static s32 RoundI(s32 t)
{
    float f = (float)SHL(t, 12);
    return (s32)(t > 0 ? 0.5f + f : f - 0.5f);
}

/* 0x0201f004: place the 3D camera to match the 2D camera */
void func_0201f004(void)
{
    s32 pos[2], size[2], sh[2], c1[2], c2[2], c3[2], c4[2], v[3], m1[3], m2[3];
    s32 base, h, hi, y, r, z;
    u8 *cam = func_0201f490();

    if (cam == NULL)
        return;
    {
        u8 *c = func_0201f490();
        pos[0] = S32_AT(c, 0x28);
        pos[1] = S32_AT(c, 0x2c);
    }
    func_0201c428(size);
    func_02003b08(sh, pos, 4);
    base = S32_AT(cam, 0x24);
    h = (base - func_0201a0a0(sh, NULL)) >> 4;
    hi = h >> 12;
    y = size[1] - (pos[1] >> 12) + hi;
    if (S32_AT(LEVEL(), 0x38) > 0)
        r = (s32)(0.5f + (float)SHL(S32_AT(LEVEL(), 0x38), 12));
    else
        r = (s32)((float)SHL(S32_AT(LEVEL(), 0x38), 12) - 0.5f);
    z = (S32_AT(cam, 0x2c) >> 4) - r;
    func_02005b98(m1, data_020df1bc);
    z = z + m1[2] - 0x1d000;
    v[0] = pos[0];
    v[1] = RoundI(y);
    v[2] = z;
    v[0] = RoundI(v[0] >> 12);
    v[1] = RoundI(v[1] >> 12);
    func_0201f744(c1);
    if ((c1[0] >> 12) > 0) {
        func_0201f744(c2);
        v[0] = (s32)(0.5f + (float)SHL(c2[0] >> 12, 12));
    } else {
        func_0201f744(c3);
        v[0] = (s32)((float)SHL(c3[0] >> 12, 12) - 0.5f);
    }
    func_0201f744(c4);
    v[1] = RoundI(hi + (size[1] - (c4[1] >> 12)));
    func_02005b98(m2, data_020df1bc);
    S32_AT(data_020df1bc, 4) = v[0];
    S32_AT(data_020df1bc, 8) = v[1];
    S32_AT(data_020df1bc, 0xc) = m2[2];
    U8_AT(data_020df1bc, 0x8c) = 1;
    func_020540d4(data_020df1bc, v);
    func_020540cc(data_020df1bc, r);
    func_02053dfc(data_020df1bc, data_020df2b8);
    func_0201f354((u8 *)func_020062e0() + 4, data_020df2b8);
}

/* 0x0201f354: copy the 3D camera state (the matrices with MI_Copy64B; the
 * 0x24 bytes after each matrix are not copied)
 * @difftest ptr:0x2c0:4 ptr:0x2c0:4 */
void *func_0201f354(u8 *dst, const u8 *src)
{
    u32 i;
    for (i = 0; i < 0x18; i += 4)
        U32_AT(dst, i) = U32_AT(src, i);
    MI_Copy64B(src + 0x18, dst + 0x18);
    MI_Copy64B(src + 0x7c, dst + 0x7c);
    MI_Copy64B(src + 0xe0, dst + 0xe0);
    MI_Copy64B(src + 0x144, dst + 0x144);
    MI_Copy64B(src + 0x1a8, dst + 0x1a8);
    MI_Copy64B(src + 0x20c, dst + 0x20c);
    for (i = 0x270; i < 0x284; i += 4)
        U32_AT(dst, i) = U32_AT(src, i);
    U8_AT(dst, 0x284) = U8_AT(src, 0x284);
    for (i = 0x288; i < 0x2c0; i += 4)
        U32_AT(dst, i) = U32_AT(src, i);
    return dst;
}

/* 0x0201f490: the actor the camera follows
 * @difftest cases=5 */
void *func_0201f490(void) { return func_0201fef8(CAM); }

/* 0x0201f4a4: per-frame camera update */
void func_0201f4a4(void)
{
    switch (func_0201ff58(CAM)) {
    case 1:
        func_0201fbbc(CAM);
        break;
    case 2:
        func_0201f9ac(CAM);
        break;
    case 4:
        func_0201fa3c(CAM, func_020822c0(func_020062d0()));
        break;
    }
}

/* 0x0201f524: follow an actor, jumping to it
 * @difftest ptr:0x30:4 @04000006:16=0x8c @020df0fc:32=0 @020e20a4:32=0 @020e20f4:32=0 @020e1fa8:32=1 @020e2000:32=1 @020e1f18:32=0 cases=10 */
void func_0201f524(void *actor)
{
    func_0201ff60(CAM, 2);
    func_0201ff00(CAM, actor);
    func_0201fea0(CAM, (s32 *)((u8 *)actor + 0x28));
    func_0201f4a4();
    func_02019280();
}

/* 0x0201f564: start moving to an actor */
void func_0201f564(void *actor)
{
    s32 d[2], *tg;

    func_0201ff60(CAM, 1);
    func_0201ff00(CAM, actor);
    S32_AT(CAM, 0xc) = 0x5000;
    tg = func_0201ff44(CAM);
    func_020150a0(d, tg, func_0201fef4(CAM));
    func_0201ff50(CAM, func_0208eff8(FxMul(d[0], d[0]) + FxMul(d[1], d[1])));
}

/* 0x0201f60c: top-left corner of the view in pixels
 * @difftest ptr:8:4 */
void func_0201f60c(s32 *out)
{
    s32 off[2];

    off[0] = 0;
    off[1] = 0;
    func_02018370(off);
    out[0] = func_0201fef4(CAM)[0] >> 12;
    out[1] = func_0201fef4(CAM)[1] >> 12;
    out[1] = out[1] - (S32_AT(CAM, 0x10) >> 16);
    out[0] = out[0] + off[0];
    out[1] = out[1] + off[1];
}

/* 0x0201f69c: set the camera bounds from the map area (pixels)
 * @difftest s32 s32 s32 s32 */
void func_0201f69c(s32 minx, s32 miny, s32 maxx, s32 maxy)
{
    func_0201fe58(CAM, miny, minx, maxy, maxx);
}

/* 0x0201f6c8: world position to screen position (in place)
 * @difftest ptr:8:4 */
void func_0201f6c8(s32 *pos)
{
    s32 c[2], d[2], r[2], *cp = func_0201fef4(CAM);

    c[0] = cp[0];
    c[1] = cp[1];
    d[0] = pos[0] - c[0];
    d[1] = c[1] - pos[1];
    func_020038d4(r, d, 12);
    pos[0] = r[0] + 0x80;
    pos[1] = r[1] + 0x60;
}

/* 0x0201f744: camera position
 * @difftest ptr:8:4 */
void func_0201f744(s32 *out)
{
    s32 *p = func_0201fef4(CAM);
    out[0] = p[0];
    out[1] = p[1];
}

/* 0x0201f770: set the move speed
 * @difftest s32 */
void func_0201f770(s32 v) { S32_AT(CAM, 0xc) = v; }

/* a random 2..4 */
static s32 Rnd(void) { return (s32)(((u32)rand() & 0x7fff) % 3) + 2; }

/* 0x0201f780: shake the camera for `time`
 * @difftest s32 */
void func_0201f780(s32 time)
{
    s32 v;

    func_0201ff60(CAM, 4);
    if (Rnd() > 0)
        v = (s32)(0.5f + (float)SHL(Rnd(), 12));
    else
        v = (s32)((float)SHL(Rnd(), 12) - 0.5f);
    S32_AT(CAM, 0x38) = v;
    if (Rnd() > 0)
        v = (s32)(0.5f + (float)SHL(Rnd(), 12));
    else
        v = (s32)((float)SHL(Rnd(), 12) - 0.5f);
    S32_AT(CAM, 0x34) = v;
    rand();
    S32_AT(CAM, 0x38) = -S32_AT(CAM, 0x38);
    rand();
    S32_AT(CAM, 0x3c) = time;
    S32_AT(CAM, 0x34) = -S32_AT(CAM, 0x34);
}

/* 0x0201f940: switch the target without moving */
void func_0201f940(void *actor)
{
    func_0201ff00(CAM, actor);
    func_0201fea0(CAM, (s32 *)((u8 *)actor + 0x28));
}

/* 0x0201f96c: camera mode
 * @difftest cases=5 */
s32 func_0201f96c(void) { return func_0201ff58(CAM); }

/* 0x0201f980: set the camera mode
 * @difftest int:0:5 */
void func_0201f980(s32 mode) { func_0201ff60(CAM, mode); }

/* 0x0201f998: reset the camera
 * @difftest cases=5 */
void func_0201f998(void) { func_0201fe28(CAM); }

/* 0x0201f9ac: follow the target
 * @difftest $C=ptr:0x44:4 $A=ptr:0x30:4 @$C+0x28:32=$A $C
 * @difftest $C=ptr:0x44:4 @$C+0x28:32=0 $C */
void func_0201f9ac(u8 *c)
{
    if (func_0201fef8(c) == NULL)
        return;
    func_0201fea0(c, func_0201ff44(c));
    S32_AT(c, 0x10) = S32_AT(func_0201fef8(c), 0x24);
    S32_AT(c, 0x14) = S32_AT(c, 0x10);
    if (S32_AT(c, 0) < S32_AT(c, 0x1c))
        S32_AT(c, 0) = S32_AT(c, 0x1c);
    else if (S32_AT(c, 0) > S32_AT(c, 0x24))
        S32_AT(c, 0) = S32_AT(c, 0x24);
    if (S32_AT(c, 4) < S32_AT(c, 0x18))
        S32_AT(c, 4) = S32_AT(c, 0x18);
    else if (S32_AT(c, 4) > S32_AT(c, 0x20))
        S32_AT(c, 4) = S32_AT(c, 0x20);
}

/* 0x0201fa3c: shake step; `dt` is the time since the last frame
 * @difftest $C=ptr:0x44:4 $A=ptr:0x30:4 @$C+0x28:32=$A @$C+0x3c:32=int:-2:40 @$C+0x40:16=int:0:3 @$C+0x34:32=int:-0x5000:0x5000 @$C+0x38:32=int:-0x5000:0x5000 $C int:0:8 */
void func_0201fa3c(u8 *c, s32 dt)
{
    s32 t[2], p[2], a[2], b[2], d[2], *tg, *cp;
    u32 k;

    S32_AT(c, 0x3c) -= dt;
    if (S32_AT(c, 0x3c) <= 0) {
        func_0201ff60(c, 2);
        return;
    }
    tg = func_0201ff44(c);
    t[0] = tg[0];
    t[1] = tg[1];
    k = U16_AT(c, 0x40);
    U16_AT(c, 0x40) = k + 1;
    if (k >= 1) {
        U16_AT(c, 0x40) = 0;
        S32_AT(c, 0x38) = -S32_AT(c, 0x38);
        S32_AT(c, 0x34) = -S32_AT(c, 0x34);
    }
    t[1] = S32_AT(c, 0x38) * U16_AT(c, 0x40) + t[1];
    t[0] = S32_AT(c, 0x34) * U16_AT(c, 0x40) + t[0];
    cp = func_0201fef4(c);
    p[0] = cp[0];
    p[1] = cp[1];
    func_020038d4(a, t, 12);
    func_020038d4(b, p, 12);
    p[0] = b[0];
    p[1] = b[1];
    func_020150a0(d, p, a);
    if (d[0] > 5 || d[1] > 5) {
        func_0201f9ac(c);
        func_0201ff60(c, 2);
    } else {
        func_0201fea0(c, t);
    }
}

/* 0x0201fbbc: move step towards the target; 1 when arrived
 * @difftest $C=ptr:0x44:4 $A=ptr:0x30:4 @$C+0x28:32=$A @$C+0xc:32=int:0:0x8000 @$C+0x18:32=int:-0x100000:0 @$C+0x1c:32=int:-0x100000:0 @$C+0x20:32=int:0:0x100000 @$C+0x24:32=int:0:0x100000 @$A+0x28:32=int:-0x80000:0x80000 @$A+0x2c:32=int:-0x80000:0x80000 $C */
u32 func_0201fbbc(u8 *c)
{
    s32 speed = S32_AT(c, 0xc), d[2], len, n[2], m[2], p[2], q[2], e[2], *tg, *cp;
    u32 done = 0;

    if (speed < 0x3000)
        speed = 0x3000;
    tg = func_0201ff44(c);
    func_020150a0(d, tg, func_0201fef4(c));
    if (FxMul(d[1], d[1]) + FxMul(d[0], d[0]) < 0x19000) {
        done = 1;
        goto end;
    }
    func_0208eff8(FxMul(d[1], d[1]) + FxMul(d[0], d[0]));
    len = func_0208eff8(FxMul(d[0], d[0]) + FxMul(d[1], d[1]));
    if (len <= 1) {
        d[1] = 0;
        d[0] = 0;
    } else {
        func_02002bd8(n, d, &len);
        d[0] = n[0];
        d[1] = n[1];
    }
    func_02003798(m, d, &speed);
    d[0] = m[0];
    d[1] = m[1];
    cp = func_0201fef4(c);
    p[0] = cp[0];
    p[1] = cp[1];
    func_02005c1c(q, p, d);
    func_0201fea0(c, q);
    {
        s32 want = S32_AT(func_0201fef8(c), 0x24);
        S32_AT(CAM, 0x10) += func_020a4e00(want - S32_AT(CAM, 0x10), S32_AT(c, 0xc) >> 12);
    }
    tg = func_0201ff44(c);
    func_020150a0(e, tg, func_0201fef4(c));
    if (FxMul(e[0], e[0]) + FxMul(e[1], e[1]) < 0x19000) {
        done = 1;
        goto end;
    }
    cp = func_0201fef4(c);
    if (p[0] == cp[0] && p[1] == cp[1])
        done = 1;
end:
    if (done) {
        S32_AT(c, 0x14) = S32_AT(func_0201fef8(c), 0x24);
        func_0201ff60(CAM, 2);
    }
    return done;
}

/* 0x0201fe28: reset
 * @difftest ptr:0x44:4 */
void func_0201fe28(u8 *c)
{
    S32_AT(c, 4) = 0;
    S32_AT(c, 0) = S32_AT(c, 4);
    S32_AT(c, 0xc) = 0;
    S32_AT(c, 0x10) = 0;
    PTR_AT(c, 0x28) = NULL;
    PTR_AT(c, 0x2c) = NULL;
    func_0201ff60(c, 3);
}

/* 0x0201fe58: set the bounds for a map area in pixels (half a screen in)
 * @difftest ptr:0x44:4 s32 s32 s32 s32 */
void func_0201fe58(u8 *c, s32 miny, s32 minx, s32 maxy, s32 maxx)
{
    S32_AT(c, 0x18) = SHL(miny + 0x60, 12);
    S32_AT(c, 0x1c) = SHL(minx + 0x80, 12);
    S32_AT(c, 0x20) = SHL(maxy - 0x60, 12);
    S32_AT(c, 0x24) = SHL((s16)maxx - 0x80, 12);
}

/* 0x0201fea0: set the position, kept within the bounds
 * @difftest $C=ptr:0x44:4 @$C+0x18:32=int:-0x1000:0 @$C+0x1c:32=int:-0x1000:0 @$C+0x20:32=int:0:0x1000 @$C+0x24:32=int:0:0x1000 $P=ptr:8:4 @$P+0:32=int:-0x2000:0x2000 @$P+4:32=int:-0x2000:0x2000 $C $P */
void func_0201fea0(u8 *c, const s32 *pos)
{
    s32 v;
    S32_AT(c, 0) = pos[0];
    S32_AT(c, 4) = pos[1];
    v = S32_AT(c, 0);
    if (v < S32_AT(c, 0x1c))
        S32_AT(c, 0) = S32_AT(c, 0x1c);
    else if (v > S32_AT(c, 0x24))
        S32_AT(c, 0) = S32_AT(c, 0x24);
    v = S32_AT(c, 4);
    if (v < S32_AT(c, 0x18))
        S32_AT(c, 4) = S32_AT(c, 0x18);
    else if (v > S32_AT(c, 0x20))
        S32_AT(c, 4) = S32_AT(c, 0x20);
}

/* 0x0201fef4: the position (the camera itself)
 * @difftest u32 */
s32 *func_0201fef4(u8 *c) { return (s32 *)c; }

/* 0x0201fef8: the target
 * @difftest ptr:0x44:4 */
u8 *func_0201fef8(u8 *c) { return PTR_AT(c, 0x28); }

/* 0x0201ff00: set the target
 * @difftest $C=ptr:0x44:4 $A=ptr:0x30:4 @$C+0x30:32=pick:1,2 $C $A */
void func_0201ff00(u8 *c, void *actor)
{
    PTR_AT(c, 0x2c) = PTR_AT(c, 0x28);
    PTR_AT(c, 0x28) = actor;
    if (func_0201ff58(c) != 2)
        return;
    S32_AT(c, 0x14) = S32_AT(func_0201fef8(c), 0x24);
    S32_AT(c, 0x10) = S32_AT(c, 0x14);
}

/* 0x0201ff44: the target's position
 * @difftest ptr:0x44:4 */
s32 *func_0201ff44(u8 *c) { return (s32 *)((u8 *)PTR_AT(c, 0x28) + 0x28); }

/* 0x0201ff50: set the distance
 * @difftest ptr:0x44:4 s32 */
void func_0201ff50(u8 *c, s32 v) { S32_AT(c, 8) = v; }

/* 0x0201ff58: the mode
 * @difftest ptr:0x44:4 */
s32 func_0201ff58(u8 *c) { return S32_AT(c, 0x30); }

/* 0x0201ff60: set the mode
 * @difftest ptr:0x44:4 s32 */
void func_0201ff60(u8 *c, s32 mode) { S32_AT(c, 0x30) = mode; }

/* 0x0201ff68: empty
 * @difftest cases=5 */
void func_0201ff68(void) {}
