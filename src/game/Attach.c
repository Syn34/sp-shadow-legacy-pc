/*
 * Models attached to a bone of another model (their animation base is
 * func_0205ef8c / func_0205eef0): +0x8c the matrix, +0xbc the position,
 * +0xf0 the model carrying them, +0xf4 set up, +0xf8 the bone's name.
 * ARM9 main, 0x02051068 - 0x02051300 (5 functions).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const fx32 data_020c04b0, data_020c04b4, g_FxPi_2;

void *func_020062e0(void);
void MI_Copy64B(const void *src, void *dest);
void func_02080e84(void *settings);
u8 *func_0204f220(void *model, const void *name);
void func_02047c94(s32 *out, const u8 *m);
void func_0208ea6c(void *mtx44);
void func_02002bbc(fx32 *out, const fx32 *x, const fx32 *y, const fx32 *z);
void func_02002978(s32 *mtx44, const fx32 *angles);
void func_02002974(void *self);
void func_0208edf0(s32 *dst, const s32 *src, fx32 x, fx32 y, fx32 z);
s32 func_0208eff8(s32 v);
void func_0205eef0(void *anim);
void func_0205ef8c(void *anim, u32 a, u32 b, u32 c);

void func_02051170(s32 *out, const s32 *m);

/* 0x02051068: draw with the attached matrix
 * @difftest $T=zero:8 @$T+4:32=0x02043814 $S=ptr:0x100:4 @$S:32=$T $S u32 stub=0x02080e84 cases=3 */
void func_02051068(u8 *self, u32 a)
{
    u8 *g = func_020062e0();

    MI_Copy64B(self + 0x8c, g + 0x3d8);
    U32_AT(g, 0x2cc) |= 4;
    func_02080e84(func_020062e0());
    (*(void (**)(void *, u32))(*(u8 **)self + 4))(self, a);
}

/* square of an fx32, rounded */
static s32 Sq(s32 v)
{
    return (s32)(((s64)v * v + 0x800) >> 12);
}

/* 0x020510bc: follow the bone: take its position and scale, upright
 * @difftest $S=zero:0x100 $M=ptr:0x60:4 @$S+0xf0:32=$M $S stub=0x0204f220:0x02000000 cases=5 */
void func_020510bc(u8 *self)
{
    s32 t[3], s[3];
    fx32 r[3];

    MI_Copy64B(func_0204f220(PTR_AT(self, 0xf0), PTR_AT(self, 0xf8)), self + 0x8c);
    func_02047c94(t, self + 0x8c);
    func_02051170(s, (s32 *)(self + 0x8c));
    func_0208ea6c(self + 0x8c);
    S32_AT(self, 0xbc) = t[0];
    S32_AT(self, 0xc0) = t[1];
    S32_AT(self, 0xc4) = t[2];
    func_02002bbc(r, &data_020c04b0, &g_FxPi_2, &data_020c04b4);
    func_02002978((s32 *)(self + 0x8c), r);
    func_02002974(r);
    func_0208edf0((s32 *)(self + 0x8c), (s32 *)(self + 0x8c), s[0], s[1], s[2]);
}

/* 0x02051170: the lengths of a matrix's first three rows (its scale)
 * @difftest ptr:0xc:4 $M=ptr:0x40:4 @$M:32=int:-0x4000:0x4000 @$M+4:32=int:-0x4000:0x4000 @$M+8:32=int:-0x4000:0x4000 @$M+0x10:32=int:-0x4000:0x4000 @$M+0x14:32=int:-0x4000:0x4000 @$M+0x18:32=int:-0x4000:0x4000 @$M+0x20:32=int:-0x4000:0x4000 @$M+0x24:32=int:-0x4000:0x4000 @$M+0x28:32=int:-0x4000:0x4000 $M cases=30 */
void func_02051170(s32 *out, const s32 *m)
{
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    out[0] = func_0208eff8(Sq(m[2]) + (Sq(m[0]) + Sq(m[1])));
    out[1] = func_0208eff8(Sq(m[6]) + (Sq(m[4]) + Sq(m[5])));
    out[2] = func_0208eff8(Sq(m[10]) + (Sq(m[8]) + Sq(m[9])));
}

/* 0x020512b0: destructor
 * @difftest ptr:0x100:4 stub=0x0205eef0 cases=3 */
u8 *func_020512b0(u8 *self)
{
    func_0205eef0(self);
    return self;
}

/* 0x020512c8: constructor: attached to bone `bone` of `model`
 * @difftest ptr:0x100:4 u32 u32 u32 stub=0x0205ef8c cases=3 */
u8 *func_020512c8(u8 *self, u32 a, void *model, const char *bone)
{
    func_0205ef8c(self, a, (u32)model, (u32)bone);
    self[0xf4] = 1;
    func_0208ea6c(self + 0x8c);
    PTR_AT(self, 0xf0) = model;
    PTR_AT(self, 0xf8) = (void *)bone;
    return self;
}
