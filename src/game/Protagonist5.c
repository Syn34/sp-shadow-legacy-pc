/*
 * CProtagonist, part 5: saved and restored hero positions, the breath
 * emitters (+0x40 fire, +0x44 ice), the camera matrix, the alternative
 * camera, set-up and the IManager list helpers.
 * ARM9 main, 0x02016f30 - 0x02017b58 (21 functions).
 *
 * CProtagonist fields used here (see Protagonist3.c/Protagonist4.c):
 *   +0x24/+0x28/+0x2c saved position, +0x30/+0x34 checkpoint, +0x3c camera
 *   tilt, +0x40/+0x44 breath emitters, +0x48 spare camera.
 * Cameras: +0x10..+0x18 angles, +0x1c..+0x24 scale, +0x28 4x4 matrix,
 * +0x8c matrix dirty.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020bd084, data_020bd088;
extern const char data_020bd128[], data_020bd154[];   /* "CProtagonist::KillEmitters() - ..." */
extern const char data_020bd180[], data_020bd1a8[];   /* "heroInit: Set up ... emitter" */
extern const char data_020bd20c[];   /* "e_IManager.h" */
extern u8 *data_020c3b0c;            /* object kinds, 0x48 bytes each */
extern u8 data_020d9f9c[];           /* script variables */
extern s8 data_020df10c;
extern u8 data_020dedfc[], data_020dee2c[], data_020dee5c[], data_020dee8c[];

void MI_Copy64B(const void *src, void *dest);
void func_020038d4(s32 *out, const s32 *in, s32 shift);
void func_02003be8(void *actor);
void func_02005b98(s32 *out, const void *src);
s32 func_02019f58(s32 *tile);
void *func_02051c38(void *e, u32 v);
void *func_02051d78(void *e);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
void *func_0205382c(void *e);
void func_020550ac(void *e, u32 v);
void func_020551b4(void *e);
void func_020557ec(void *e, void *emitter);
void func_02055ad8(void *e, u32 v, u32 w);
void *func_02055eb0(void *e, u32 v);
void func_020573c4(void *e, u32 v);
void func_02058900(void *e, u32 v, s32 *pos, s32 *rot);
void *func_02058b2c(void *e, u32 kind, void *cam);
void func_02064714(void *view);
void *func_0207fe74(u32 size);       /* operator new */
void func_0208eacc(const s32 *a, const s32 *b, s32 *ab);   /* MTX_Concat44 */
void func_0208edf0(s32 *dst, const s32 *src, fx32 x, fx32 y, fx32 z);   /* MTX_Scale44 */

/* this file's own functions, called before their definition */
void func_020172e8(s32 *m, const fx32 *angle);
void func_020173a4(s32 *m, const fx32 *angle);
void func_02017460(s32 *m, const fx32 *angle);
void func_0201751c(s32 *out, const u8 *cam);
void func_02017538(s32 *out, const s32 *rot, const s32 *m);
void func_02017550(s32 *out, const u8 *cam);

#define HERO(self) ((u8 *)PTR_AT(self, 4))

static inline void VCall(void *obj, u32 off, u32 v)
{
    VCALL(obj, off, void (*)(void *, u32))(obj, v);
}

static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

/* 0x02016f30: point the camera at the target angle
 * @difftest $C=ptr:0x90:4 $O=ptr:0x48:4 @$O+0x44:32=$C $T=ptr:0x60:4 @$T+8:32=$O $T */
void func_02016f30(u8 *self)
{
    s32 t = S32_AT(self, 0x10);
    u8 *c = func_02016f24(self);
    s32 z = S32_AT(c, 0x18), x = S32_AT(c, 0x10);
    S32_AT(c, 0x10) = x;
    S32_AT(c, 0x14) = t;
    S32_AT(c, 0x18) = z;
    c[0x8c] = 1;
}

/* 0x02016f68: put the hero back at the saved position
 * @difftest $H=ptr:0x120:4 $W=ptr:0xa0:4 @$H+0x44:32=$W $T=ptr:0x40:4 @$T+4:32=$H @$T+0x24:32=int:0:0x400000 @$T+0x28:32=int:0:0x400000 $T cases=60 */
void func_02016f68(u8 *self)
{
    s32 tile[2], pos[2];

    S32_AT(HERO(self), 0x54) = S32_AT(self, 0x24);
    S32_AT(HERO(self), 0x58) = S32_AT(self, 0x28);
    S32_AT(HERO(self), 0x5c) = S32_AT(self, 0x24);
    S32_AT(HERO(self), 0x60) = S32_AT(self, 0x28);
    func_020038d4(tile, (s32 *)(self + 0x24), 0x10);
    if ((u32)(func_02019f58(tile) - 2) <= 1) {
        func_02003be8(HERO(self));
        return;
    }
    func_020038d4(pos, (s32 *)(HERO(self) + 0x54), 4);
    S32_AT(HERO(self), 0x28) = pos[0];
    S32_AT(HERO(self), 0x2c) = pos[1];
    S32_AT(HERO(self), 0x24) = S32_AT(self, 0x2c);
    S32_AT(HERO(self), 0x6c) = S32_AT(self, 0x2c);
}

/* 0x02017028: save the hero's position
 * @difftest $H=ptr:0x80:4 $T=ptr:0x40:4 @$T+4:32=$H $T */
void func_02017028(u8 *self)
{
    u8 *h = HERO(self);
    S32_AT(self, 0x24) = S32_AT(h, 0x54);
    S32_AT(self, 0x28) = S32_AT(h, 0x58);
    S32_AT(self, 0x2c) = S32_AT(HERO(self), 0x24);
}

/* 0x02017050: save the hero's position as the checkpoint
 * @difftest $H=ptr:0x80:4 $T=ptr:0x40:4 @$T+4:32=$H $T */
void func_02017050(u8 *self)
{
    u8 *h = HERO(self);
    S32_AT(self, 0x30) = S32_AT(h, 0x54);
    S32_AT(self, 0x34) = S32_AT(h, 0x58);
}

/* 0x0201706c: run the active breath emitter
 * @difftest $F=zero:0x3c00 $I=zero:0x3c00 @$F+0:32=pick:0x020c0c64 @$I+0:32=pick:0x020c0c64 @$F+0x3bc4:8=pick:0,1 @$I+0x3bc4:8=pick:0,1 $T=ptr:0x50:4 @$T+0x40:32=$F @$T+0x44:32=$I $T cases=30 */
void func_0201706c(u8 *self)
{
    u8 *e = PTR_AT(self, 0x40);
    if ((s8)e[0x3bc4] != 0) {
        VCall(e, 0xc, func_020822c0(func_020062d0()));
        VCallR1(PTR_AT(self, 0x40), 0x14);
        return;
    }
    e = PTR_AT(self, 0x44);
    if ((s8)e[0x3bc4] == 0)
        return;
    VCall(e, 0xc, func_020822c0(func_020062d0()));
    VCallR1(PTR_AT(self, 0x44), 0x14);
}

/* 0x02017118: give the active breath emitter a matrix
 * @difftest $F=zero:0x3c00 $I=zero:0x3c00 @$F+0x3bc4:8=pick:0,1 @$I+0x3bc4:8=pick:0,1 $T=ptr:0x50:4 @$T+0x40:32=$F @$T+0x44:32=$I $T ptr:0x40:4 */
void func_02017118(u8 *self, const s32 *m)
{
    u8 *e = PTR_AT(self, 0x40);
    if ((s8)e[0x3bc4] != 0) {
        MI_Copy64B(m, e + 0x28);
        e[0x8c] = 0;
        return;
    }
    e = PTR_AT(self, 0x44);
    if ((s8)e[0x3bc4] == 0)
        return;
    MI_Copy64B(m, e + 0x28);
    e[0x8c] = 0;
}

/* 0x02017180: build the camera matrix (scale, tilt, rotation, position)
 * @difftest $T=ptr:0x60:4 @$T+0x3c:32=pick:0,0x1000,-0x3000 $T cases=60 */
void func_02017180(void *obj)
{
    u8 *self = obj, *cam = func_02016f24(data_020deebc);
    s32 pos[3], sc[3], rot[3], m[16], r[16], out[16];

    func_02005b98(pos, cam);
    pos[1] += 0x5000;
    func_0208ea6c(m);
    func_02017550(sc, cam);
    func_0208edf0(m, m, sc[0], sc[1], sc[2]);
    if (S32_AT(self, 0x3c) != 0) {
        u32 idx = FxRadToIdx(S32_AT(self, 0x3c)) >> 4;
        s32 sn = FX_SinCosTable_[idx * 2], cs = FX_SinCosTable_[idx * 2 + 1];
        func_0208ea6c(r);
        r[5] = cs;
        r[6] = sn;
        r[9] = -sn;
        r[10] = cs;
        func_02017538(out, r, m);
        MI_Copy64B(out, m);
    }
    func_0201751c(rot, cam);
    func_02017460(m, &rot[1]);
    func_020173a4(m, &rot[0]);
    func_020172e8(m, &rot[2]);
    m[12] = pos[0];
    m[13] = pos[1];
    m[14] = pos[2];
    MI_Copy64B(m, cam + 0x28);
    cam[0x8c] = 0;
}

/* 0x020172e8: m = m * rotation about z
 * @difftest ptr:0x40:4 $A=ptr:4:4 @$A+0:32=pick:0,0x1000,-0x2000,0x3243,0x6000 $A */
void func_020172e8(s32 *m, const fx32 *angle)
{
    s32 r[16], out[16], sn, cs;
    u32 idx;
    if (*angle == 0)
        return;
    idx = FxRadToIdx(*angle) >> 4;
    sn = FX_SinCosTable_[idx * 2];
    cs = FX_SinCosTable_[idx * 2 + 1];
    func_0208ea6c(r);
    r[0] = cs;
    r[1] = sn;
    r[4] = -sn;
    r[5] = cs;
    func_02017538(out, r, m);
    MI_Copy64B(out, m);
}

/* 0x020173a4: m = m * rotation about x
 * @difftest ptr:0x40:4 $A=ptr:4:4 @$A+0:32=pick:0,0x1000,-0x2000,0x3243,0x6000 $A */
void func_020173a4(s32 *m, const fx32 *angle)
{
    s32 r[16], out[16], sn, cs;
    u32 idx;
    if (*angle == 0)
        return;
    idx = FxRadToIdx(*angle) >> 4;
    sn = FX_SinCosTable_[idx * 2];
    cs = FX_SinCosTable_[idx * 2 + 1];
    func_0208ea6c(r);
    r[5] = cs;
    r[6] = sn;
    r[9] = -sn;
    r[10] = cs;
    func_02017538(out, r, m);
    MI_Copy64B(out, m);
}

/* 0x02017460: m = m * rotation about y
 * @difftest ptr:0x40:4 $A=ptr:4:4 @$A+0:32=pick:0,0x1000,-0x2000,0x3243,0x6000 $A */
void func_02017460(s32 *m, const fx32 *angle)
{
    s32 r[16], out[16], sn, cs;
    u32 idx;
    if (*angle == 0)
        return;
    idx = FxRadToIdx(*angle) >> 4;
    sn = FX_SinCosTable_[idx * 2];
    cs = FX_SinCosTable_[idx * 2 + 1];
    func_0208ea6c(r);
    r[0] = cs;
    r[2] = -sn;
    r[8] = sn;
    r[10] = cs;
    func_02017538(out, r, m);
    MI_Copy64B(out, m);
}

/* 0x0201751c: a camera's angles
 * @difftest ptr:12:4 ptr:0x30:4 */
void func_0201751c(s32 *out, const u8 *cam)
{
    out[0] = S32_AT(cam, 0x10);
    out[1] = S32_AT(cam, 0x14);
    out[2] = S32_AT(cam, 0x18);
}

/* 0x02017538: out = m * rot
 * @difftest ptr:0x40:4 ptr:0x40:4 ptr:0x40:4 */
void func_02017538(s32 *out, const s32 *rot, const s32 *m)
{
    func_0208eacc(m, rot, out);
}

/* 0x02017550: a camera's scale
 * @difftest ptr:12:4 ptr:0x30:4 */
void func_02017550(s32 *out, const u8 *cam)
{
    out[0] = S32_AT(cam, 0x1c);
    out[1] = S32_AT(cam, 0x20);
    out[2] = S32_AT(cam, 0x24);
}

/* 0x0201756c: swap to the spare camera and start its effect (not difftested:
 * creating the effect waits on hardware in the emulator) */
void func_0201756c(u8 *self)
{
    u8 *c = func_02016f24(self), *e, *f;
    s32 pos[3], rot[3];
    u32 bit;

    PTR_AT(PTR_AT(self, 8), 0x44) = PTR_AT(self, 0x48);
    PTR_AT(self, 0x48) = c;
    func_02064714(PTR_AT(PTR_AT(self, 0x48), 0xac));
    S32_AT(self, 0x3c) = 0;
    e = func_0207fe74(0x3c88);
    if (e != NULL)
        e = func_02051d78(e);
    if (e != NULL) {
        func_02051c38(e, 0x710);
        func_020573c4(e, 0x4df);
        func_02005b98(pos, PTR_AT(self, 0x48));
        func_0201751c(rot, PTR_AT(self, 0x48));
        func_02058900(e, 0, pos, rot);
        f = func_0207fe74(0xf0);
        if (f != NULL)
            f = func_02055eb0(f, 1);
        if (f != NULL) {
            func_02055ad8(f, 0xc2, 1);
            func_020557ec(f, e);
            func_020550ac(f, 0x191);
            func_020550ac(f, 0x192);
            func_020551b4(f);
        }
    }
    bit = BITS(data_020deebc[0x20 / 4], 5, 1) == 0;
    data_020deebc[0x20 / 4] = (data_020deebc[0x20 / 4] & ~0x20u) | ((bit & 1) << 5);
    data_020d9f9c[0x182] = BITS(data_020deebc[0x20 / 4], 5, 1);
}

/* 0x020176c8: create the spare camera */
void func_020176c8(u8 *self)
{
    u8 *c = func_0207fe74(0x1a8), *k;
    if (c != NULL)
        c = func_0205382c(c);
    PTR_AT(self, 0x48) = c;
    func_02053648(PTR_AT(self, 0x48), 0x110, PTR_AT(self, 8), 1, 0);
    k = data_020c3b0c + 0x110 * 0x48;
    c = PTR_AT(self, 0x48);
    S32_AT(c, 0x1c) = S32_AT(k, 0x2c);
    S32_AT(c, 0x20) = S32_AT(k, 0x2c);
    S32_AT(c, 0x24) = S32_AT(k, 0x2c);
    c[0x8c] = 1;
    k = data_020c3b0c + 0x110 * 0x48;
    c = PTR_AT(self, 0x48);
    U32_AT(c, 0x18c) = (U32_AT(c, 0x18c) & ~1u) | (U32_AT(k, 0x30) & 1);
    k = data_020c3b0c + 0x110 * 0x48;
    S32_AT(PTR_AT(self, 0x48), 0xa4) = S32_AT(k, 0x34);
    U32_AT(PTR_AT(self, 0x48), 0x18c) |= 2;
    PTR_AT(PTR_AT(PTR_AT(self, 0x48), 0xac), 4) = PTR_AT(self, 8);
}

/* 0x020177a8: delete the breath emitters and the spare camera */
void func_020177a8(u8 *self)
{
    if (PTR_AT(self, 0x40) != NULL) {
        if (PTR_AT(self, 0x40) != NULL)
            VCallR1(PTR_AT(self, 0x40), 4);
        PTR_AT(self, 0x40) = NULL;
        func_02004490(data_020bd128);
    }
    if (PTR_AT(self, 0x44) != NULL) {
        if (PTR_AT(self, 0x44) != NULL)
            VCallR1(PTR_AT(self, 0x44), 4);
        PTR_AT(self, 0x44) = NULL;
        func_02004490(data_020bd154);
    }
    if (PTR_AT(self, 0x48) == NULL)
        return;
    if (PTR_AT(self, 0x48) != NULL)
        VCallR1(PTR_AT(self, 0x48), 4);
    PTR_AT(self, 0x48) = NULL;
}

/* 0x0201784c: create the breath emitters */
void func_0201784c(u8 *self)
{
    u32 fire, ice;
    void *e;

    if (data_020df10c != 0) {
        fire = 10;
        ice = 11;
    } else {
        fire = 1;
        ice = 2;
    }
    if (PTR_AT(self, 0x40) == NULL) {
        e = func_0207fe74(0x3c68);
        if (e != NULL)
            e = func_02058b2c(e, fire, func_02016f24(self));
        PTR_AT(self, 0x40) = e;
        func_02004490(data_020bd180);
    }
    if (PTR_AT(self, 0x44) != NULL)
        return;
    e = func_0207fe74(0x3c68);
    if (e != NULL)
        e = func_02058b2c(e, ice, func_02016f24(self));
    PTR_AT(self, 0x44) = e;
    func_02004490(data_020bd1a8);
}

/* 0x02017920: CProtagonist constructor fields
 * @difftest ptr:0x60:4 */
void func_02017920(u8 *self)
{
    U16_AT(self, 0) = 0;
    self[2] = 0;
    U32_AT(self, 4) = 0;
    S32_AT(self, 0x10) = g_FxZero;
    S32_AT(self, 0x14) = g_FxZero;
    U32_AT(self, 0x18) = 0;
    self[0x1c] = 0;
    self[0x1f] = 0;
    U32_AT(self, 0x20) = 0;
    S32_AT(self, 0x24) = data_020bd088;
    S32_AT(self, 0x28) = data_020bd084;
    U32_AT(self, 0x38) = 0;
    self[0x55] = 0;
    self[0x57] = 0;
    self[0x56] = 0;
    self[0x58] = 0;
    U32_AT(self, 0x48) = 0;
}

static inline void InitSound(u8 *s, u16 id)
{
    U32_AT(s, 0) = 0x34;
    U16_AT(s, 4) = id;
    U32_AT(s, 0x18) = 0;
}

/* 0x02017998: reset for a new level
 * @difftest ptr:0x60:4 */
void func_02017998(u8 *self)
{
    U32_AT(self, 0x24) = 0;
    U32_AT(self, 0x28) = 0;
    U32_AT(self, 0x30) = 0;
    U32_AT(self, 0x34) = 0;
    U32_AT(self, 0x20) = 0;
    self[0x1d] = 0;
    self[0x1e] = 1;
    self[0x50] = 0;
    self[0x51] = 0x14;
    self[0x52] = 0xa;
    self[0x53] = 5;
    self[0x54] = 5;
    InitSound(data_020dedfc, 0x23e);
    InitSound(data_020dee2c, 0x292);
    InitSound(data_020dee5c, 0x3f7);
    InitSound(data_020dee8c, 0xa5);
}

/* 0x02017a58: is `item` in the manager's list
 * @difftest $L=ptr:0x20:4 $M=ptr:0x10:4 @$M+4:32=$L @$M+8:32=int:0:8 @$L+4:32=pick:5,6 @$L+0xc:32=pick:5,6 $M pick:5,6 */
s32 func_02017a58(u8 *mgr, void *item)
{
    void **p = PTR_AT(mgr, 4), **end = p + U32_AT(mgr, 8);
    while (p != end && *p != item)
        p++;
    return p != end;
}

/* 0x02017abc: add `item` to the manager's list
 * @difftest $L=ptr:0x20:4 $M=zero:0x10 @$M+4:32=$L @$M+8:32=int:0:3 @$M+0xc:32=int:3:8 $M u32 */
void func_02017abc(u8 *mgr, void *item)
{
    func_0207fe28(data_020bd20c, 0x58);
    if (U32_AT(mgr, 8) < U32_AT(mgr, 0xc)) {
        U32_AT(mgr, 8) += 1;
        ((void **)PTR_AT(mgr, 4))[U32_AT(mgr, 8) - 1] = item;
    } else {
        func_020062f0(mgr + 4, &item, 0);
    }
    func_0207fe24();
}
