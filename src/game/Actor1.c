/*
 * Actor base class, part 1: animation scripts, movement and actor lists.
 * ARM9 main, 0x020030d0 - 0x020044a0 (39 functions).
 *
 * Source file "Actor.cpp" (allocation debug info). Actor fields used here:
 *   +0x04  flags (bit 2: active in layer updates)
 *   +0x24/+0x64/+0x6c  depth, +0x28/+0x2c screen position (pixels << 12)
 *   +0x54/+0x58, +0x5c/+0x60  position (fx32, pixels << 16), +0x70/+0x74 velocity
 *   +0xa8  flags (bit 13: second screen, bit 28/29: mirrored X/Y)
 *   +0xb0  frame timer, +0xb1 frame time, +0xb2 shown frame, +0xb3 frame
 *   +0xb8  animation resource, +0xbc script handle (-1: none),
 *   +0xc0  script position, +0xc4 script base
 *   +0xdc/+0xde  palette slot, +0xe8 loaded animation data
 *   +0x110 flags (bit 5: animated, bit 6: finished, bit 7: survives killAll,
 *          bit 11: frame rectangle dirty)
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define A_FLAGS110(a) U32_AT(a, 0x110)

/* frame record of the current frame inside the animation data */
static inline u8 *FrameEntry(u8 *anim, void *a)
{
    return anim + 0xc + U16_AT(anim, 0xc + U8_AT(a, 0xb3) * 2);
}

/* recompute the actor's frame rectangle from the animation data, mark it dirty */
static void UpdateFrameRect(u8 *a)
{
    u8 *anim = func_02003958(a);
    if (anim != NULL) {
        u8 *ent = FrameEntry(anim, a);
        s32 rect[4], bottom, left, top, right;
        func_02003720(rect);
        right = S16_AT(ent, 6) + U8_AT(ent, 2);
        top = S16_AT(ent, 8);
        left = S16_AT(ent, 6);
        bottom = S16_AT(ent, 8) + U8_AT(ent, 3);
        func_020036e8(rect, &bottom, &left, &top, &right);
        func_020036c4(a, rect);
        func_020036c0(rect);
    }
    *func_02003738(a) |= 0x800;
}

/* script opcodes 0xfb/0xfc: nudge the position by a signed byte, rounded */
static s32 ScriptNudge(u32 raw, u32 mirrored)
{
    s32 v = mirrored ? (s8)-raw : (s8)raw;
    float f = (float)SHL(v, 16);
    return (s32)(v > 0 ? 0.5f + f : f - 0.5f);
}

/* 0x020030d0: advance the actor's animation by one tick
 * @difftest ptr:0x120:4
 * @difftest $N=ptr:0x100:4 $A=ptr:0x120:4 @$A+0xe8:32=$N @$A+0xbc:32=-1 @$A+0xb0:8=1 @$A+0x110:32=pick:0x20,0x820,0x60 $A */
void func_020030d0(u8 *a)
{
    if (!BITS(A_FLAGS110(a), 5, 1) || U8_AT(a, 0xb0) == 0)
        return;
    U8_AT(a, 0xb0)--;
    if (U8_AT(a, 0xb0) != 0)
        return;

    if (S32_AT(a, 0xbc) == -1) {
        u8 *anim;
        U8_AT(a, 0xb3)++;
        anim = func_02003958(a);
        if (U8_AT(a, 0xb3) >= U16_AT(anim, 6)) {
            U8_AT(a, 0xb3) = 0;
            A_FLAGS110(a) |= 0x40;
        }
        if (U16_AT(anim, 6) - 1 == U8_AT(a, 0xb3) || U16_AT(anim, 6) == 1)
            A_FLAGS110(a) |= 0x40;
        U8_AT(a, 0xb0) = U8_AT(a, 0xb1);
    } else {
        u8 *script, *p;
        s32 more;

        if (BITS(A_FLAGS110(a), 6, 1))
            return;
        S32_AT(a, 0xc0)++;
        func_020036a4(a, 0);
        func_0207fe28(data_020bca78, 0x845);
        script = func_02012a64(S32_AT(a, 0xbc));
        p = script + S32_AT(a, 0xc0) * 2;
        func_0207fe24();
        func_02003958(a);

        more = 1;
        do {
            u32 op = p[0];
            if (op < 0xf1) {
                U8_AT(a, 0xb3) = op;
                U8_AT(a, 0xb0) = p[1];
                more = 0;
                continue;
            }
            switch (op) {
            case 0xf5:
            case 0xf6:
                S32_AT(a, 0xc0)++;
                p = script + S32_AT(a, 0xc0) * 2;
                break;
            case 0xf9:
                func_020039d0(a, p[1] << 16);
                S32_AT(a, 0xc0)++;
                p = script + S32_AT(a, 0xc0) * 2;
                break;
            case 0xfa:
                func_020039e8(a, p[1] << 16);
                S32_AT(a, 0xc0)++;
                p = script + S32_AT(a, 0xc0) * 2;
                break;
            case 0xfb:
                S32_AT(a, 0x60) += ScriptNudge(p[1], BITS(U32_AT(a, 0xa8), 29, 1));
                S32_AT(a, 0xc0)++;
                p = script + S32_AT(a, 0xc0) * 2;
                break;
            case 0xfc:
                S32_AT(a, 0x5c) += ScriptNudge(p[1], BITS(U32_AT(a, 0xa8), 28, 1));
                S32_AT(a, 0xc0)++;
                p = script + S32_AT(a, 0xc0) * 2;
                break;
            case 0xfd:
                S32_AT(a, 0xc0) = S32_AT(a, 0xc4) + p[1];
                p = script + S32_AT(a, 0xc0) * 2;
                break;
            case 0xfe:
                U8_AT(a, 0xb0) = 1;
                more = 0;
                break;
            case 0xff:
                func_020036a4(a, 1);
                more = 0;
                break;
            default: /* 0xf1-0xf4, 0xf7, 0xf8: no effect (the original spins) */
                break;
            }
        } while (more == 1);
        func_020129ac(S32_AT(a, 0xbc));
    }

    if (U8_AT(a, 0xb2) == U8_AT(a, 0xb3) && !BITS(*func_02003738(a), 11, 1))
        return;
    UpdateFrameRect(a);
}

/* 0x020036a4: set or clear the "animation finished" flag
 * @difftest ptr:0x120:4 u32 */
void func_020036a4(void *a, s32 v)
{
    SET_BITS(A_FLAGS110(a), 6, 1, v);
}

/* 0x020036c0: rectangle destructor (empty)
 * @difftest ptr:16:4 */
void func_020036c0(s32 *rect) { (void)rect; }

/* 0x020036c4: store the frame rectangle
 * @difftest ptr:0x40:4 ptr:16:4 */
void func_020036c4(void *a, const s32 *rect)
{
    S32_AT(a, 0x10) = rect[0];
    S32_AT(a, 0x14) = rect[1];
    S32_AT(a, 0x18) = rect[2];
    S32_AT(a, 0x1c) = rect[3];
}

/* 0x020036e8: rectangle constructor from four values
 * @difftest ptr:16:4 ptr:4:4 ptr:4:4 ptr:4:4 ptr:4:4 */
void func_020036e8(s32 *rect, const s32 *a, const s32 *b, const s32 *c, const s32 *d)
{
    s32 vb = *b, vc = *c, vd = *d, va = *a;
    rect[0] = va;
    rect[1] = vb;
    rect[2] = vc;
    rect[3] = vd;
}

/* 0x02003720: empty rectangle
 * @difftest ptr:16:4 */
void func_02003720(s32 *rect)
{
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = 0;
    rect[3] = 0;
}

/* 0x02003738: address of the +0x110 flag word
 * @difftest u32 */
u32 *func_02003738(void *a)
{
    return (u32 *)((u8 *)a + 0x110);
}

/* 0x02003740: release the actor's palette slot
 * @difftest $A=ptr:0x120:4 @$A+0xde:16=pick:0xffff,3,7 @$A+0xdc:16=pick:0,1,2 $A cases=60 */
void func_02003740(void *a)
{
    if (U16_AT(a, 0xde) == 0xffff)
        return;
    if (U16_AT(a, 0xdc) != 0)
        func_020291ec(U16_AT(a, 0xde), U16_AT(a, 0xdc), BITS(U32_AT(a, 0xa8), 13, 1));
    U16_AT(a, 0xde) = 0xffff;
    U16_AT(a, 0xdc) = 0;
}

/* 0x02003798: scale a 2D fx32 vector
 * @difftest ptr:8:4 ptr:8:4 ptr:4:4 */
void func_02003798(s32 *out, const s32 *in, const s32 *scale)
{
    s32 s = *scale, y = in[1], x = in[0];
    out[0] = FxMul(x, s);
    out[1] = FxMul(y, s);
}

/* 0x020037ec: pixel data of the current animation frame */
void *func_020037ec(void *a)
{
    u8 *ent = FrameEntry(func_02003958(a), a);
    u8 *base;
    func_0207fe28(data_020bca78, 0x77f);
    base = func_02012a64(*(s32 *)PTR_AT(a, 0xb8));
    func_0207fe24();
    return base + U16_AT(ent, 4);
}

/* 0x0200384c: size of the current animation frame
 * @difftest $N=ptr:0x100:4 $A=ptr:0x120:4 @$A+0xe8:32=$N $A ptr:8:4 */
void func_0200384c(void *a, u32 *out)
{
    u8 *ent = FrameEntry(func_02003958(a), a);
    out[0] = U8_AT(ent, 2);
    out[1] = U8_AT(ent, 3);
}

/* 0x02003890: actor position in pixels, adjusted for the camera */
void func_02003890(void *a, s32 *out)
{
    s32 tmp[2];
    func_020038d4(tmp, (s32 *)((u8 *)a + 0x54), 4);
    out[0] = tmp[0];
    out[1] = tmp[1];
    func_0201f6c8(out);
}

/* shift the magnitude right, keeping the sign (rounds toward zero) */
static inline s32 ShrMag(s32 v, s32 sh)
{
    u32 m = v < 0 ? -(u32)v : (u32)v;
    s32 r = (s32)m >> sh;
    return v < 0 ? -r : r;
}

/* 0x020038d4: 2D vector >> shift, rounding toward zero
 * @difftest ptr:8:4 ptr:8:4 int:0:32 */
void func_020038d4(s32 *out, const s32 *in, s32 sh)
{
    s32 x = ShrMag(in[0], sh);
    s32 y = ShrMag(in[1], sh);
    out[0] = x;
    out[1] = y;
}

/* 0x02003958: animation data, loaded from the archive on first use */
void *func_02003958(void *a)
{
    if (PTR_AT(a, 0xe8) == NULL) {
        u32 file[0x44 / 4];
        u32 size;
        func_02080488(file, U32_AT(PTR_AT(a, 0xb8), 4));
        size = func_02080388(file);
        PTR_AT(a, 0xe8) = func_0207ff70(size, data_020bca78, 0x724);
        func_02080418(PTR_AT(a, 0xe8), size, 1, file);
        func_02080458(file);
    }
    return PTR_AT(a, 0xe8);
}

/* 0x020039d0: set the Y velocity (negated when mirrored)
 * @difftest ptr:0xb0:4 s32 */
void func_020039d0(void *a, s32 v)
{
    S32_AT(a, 0x74) = BITS(U32_AT(a, 0xa8), 29, 1) ? -v : v;
}

/* 0x020039e8: set the X velocity (negated when mirrored)
 * @difftest ptr:0xb0:4 s32 */
void func_020039e8(void *a, s32 v)
{
    S32_AT(a, 0x70) = BITS(U32_AT(a, 0xa8), 28, 1) ? -v : v;
}

/* 0x02003a00: set the velocity from the heading of the actor's model
 * @difftest $M=ptr:0x20:4 $A=ptr:0x80:4 @$A+0x44:32=$M $A s32 */
void func_02003a00(void *a, s32 speed)
{
    s32 dir[2], s, out[2], res[2];
    dir[1] = FX_COS_RAD(S32_AT(PTR_AT(a, 0x44), 0x14));
    s = speed >> 4;
    dir[0] = FX_SIN_RAD(S32_AT(PTR_AT(a, 0x44), 0x14));
    func_02003798(out, dir, &s);
    dir[0] = out[0];
    dir[1] = out[1];
    func_02003b08(res, dir, 4);
    dir[0] = res[0];
    dir[1] = res[1];
    S32_AT(a, 0x70) = res[0];
    S32_AT(a, 0x74) = dir[1];
}

/* shift the magnitude left, keeping the sign */
static inline s32 ShlMag(s32 v, s32 sh, s32 neg)
{
    u32 m = v < 0 ? -(u32)v : (u32)v;
    s32 r = (s32)(m << sh);
    return neg ? -r : r;
}

/* 0x02003b08: 2D vector << shift, keeping signs (out may alias in)
 * @difftest ptr:8:4 ptr:8:4 int:0:32
 * @difftest $V=ptr:8:4 $V $V int:0:16 */
void func_02003b08(s32 *out, const s32 *in, s32 sh)
{
    s32 neg = in[0] < 0;
    out[0] = 0;
    out[1] = 0;
    out[0] = ShlMag(in[0], sh, neg);
    neg = in[1] < 0;
    out[1] = ShlMag(in[1], sh, neg);
}

/* 0x02003b98: place the actor at pixel coordinates
 * @difftest ptr:0x80:4 s16 s16 */
void func_02003b98(void *a, s32 x, s32 y)
{
    s32 tmp[2];
    S32_AT(a, 0x54) = SHL(x, 16);
    S32_AT(a, 0x58) = SHL(y, 16);
    S32_AT(a, 0x5c) = SHL(x, 16);
    S32_AT(a, 0x60) = SHL(y, 16);
    func_020038d4(tmp, (s32 *)((u8 *)a + 0x54), 4);
    S32_AT(a, 0x28) = tmp[0];
    S32_AT(a, 0x2c) = tmp[1];
}

/* 0x02003be8: reset the position from the spawn point (+0x30)
 * @difftest ptr:0x80:4 */
void func_02003be8(void *a)
{
    s32 t[2], u[2];
    func_02003b08(t, (s32 *)((u8 *)a + 0x30), 4);
    S32_AT(a, 0x28) = S32_AT(a, 0x30);
    S32_AT(a, 0x2c) = S32_AT(a, 0x34);
    S32_AT(a, 0x54) = t[0];
    S32_AT(a, 0x58) = t[1];
    S32_AT(a, 0x5c) = t[0];
    S32_AT(a, 0x60) = t[1];
    func_020038d4(u, t, 4);
    S32_AT(a, 0x28) = u[0];
    S32_AT(a, 0x2c) = u[1];
    S32_AT(a, 0x24) = S32_AT(a, 0x64);
    S32_AT(a, 0x6c) = S32_AT(a, 0x24);
}

/* 0x02003c74: place the actor at an fx32 position
 * @difftest ptr:0x80:4 s32 s32 */
void func_02003c74(void *a, s32 x, s32 y)
{
    s32 tmp[2];
    S32_AT(a, 0x54) = x;
    S32_AT(a, 0x58) = y;
    S32_AT(a, 0x5c) = x;
    S32_AT(a, 0x60) = y;
    func_020038d4(tmp, (s32 *)((u8 *)a + 0x54), 4);
    S32_AT(a, 0x28) = tmp[0];
    S32_AT(a, 0x2c) = tmp[1];
}

/* 0x02003cbc: show animation frame `frame`
 * @difftest $N=ptr:0x100:4 $A=ptr:0x120:4 @$A+0xe8:32=$N @$A+0xb3:8=int:0:3 @$A+0xb2:8=int:0:3 $A int:0:3
 * @difftest $N=ptr:0x100:4 $A=ptr:0x120:4 @$A+0xe8:32=$N $A u32 */
void func_02003cbc(void *a, s32 frame)
{
    if (U8_AT(a, 0xb3) != frame || BITS(A_FLAGS110(a), 11, 1)) {
        U8_AT(a, 0xb3) = frame;
        U8_AT(a, 0xb0) = U8_AT(a, 0xb1);
        if (U8_AT(a, 0xb2) != U8_AT(a, 0xb3) || BITS(*func_02003738(a), 11, 1))
            UpdateFrameRect(a);
    }
    A_FLAGS110(a) &= ~0x40u;
}

/* 0x02003ddc: switch to another animation resource, from frame 0
 * @difftest $N=ptr:0x100:4 $A=ptr:0x120:4 @$A+0xe8:32=$N $A ptr:16:4 */
void func_02003ddc(void *a, void *anim)
{
    if (PTR_AT(a, 0xb8) != anim) {
        PTR_AT(a, 0xb8) = anim;
        A_FLAGS110(a) |= 0x800;
    }
    func_02003cbc(a, 0);
}

/* 0x02003e08: start an animation (frame time from the resource) */
void func_02003e08(void *a, void *anim)
{
    U8_AT(a, 0xb1) = U8_AT(anim, 0xc);
    func_02003ddc(a, anim);
    func_02002228(a, anim);
}

/* 0x02003e3c: allocate a palette slot
 * @difftest ptr:0x120:4 int:0:16 */
void func_02003e3c(void *a, s32 v)
{
    U16_AT(a, 0xde) = func_02029284(v, U16_AT(a, 0xdc), BITS(U32_AT(a, 0xa8), 13, 1));
}

/* 0x02003e68: notify the actors of `layer` whose id is in the index range of `group`
 * @difftest int:0:8 int:0:2 cases=40 */
void func_02003e68(s32 group, s32 layer)
{
    PtrVec *list = &data_020d9ea0[layer];
    void **it;
    for (it = list->items; it != list->items + list->count; it++) {
        u8 *actor = *it;
        s32 id;
        u8 *range, *rec;
        u32 i;
        if (PTR_AT(actor, 0x44) == NULL || !BITS(U32_AT(actor, 4), 2, 1))
            continue;
        id = S32_AT(actor, 8);
        if (id == 1)
            continue;
        range = data_020c3b68 + group * 0xc;
        i = U32_AT(range, 4);
        if (i > U32_AT(range, 8))
            continue;
        rec = data_020c3b64 + i * 0x2c;
        do {
            if (S32_AT(rec, 0xc) == id) {
                VCALL(actor, 0xc, void (*)(void *, s32))(actor, 0);
                break;
            }
            i++;
            rec += 0x2c;
        } while (i <= U32_AT(range, 8));
    }
}

/* 0x02003f70: set or clear the "active" flag
 * @difftest ptr:8:4 u32 */
void func_02003f70(void *a, s32 v)
{
    SET_BITS(U32_AT(a, 4), 2, 1, v);
}

/* 0x02003f8c: notify(1) every actor of a layer that has a model
 * @difftest int:0:2 cases=10 */
void func_02003f8c(s32 layer)
{
    PtrVec *list = &data_020d9ea0[layer];
    void **it;
    for (it = list->items; it != list->items + list->count; it++) {
        u8 *actor = *it;
        if (PTR_AT(actor, 0x44) != NULL)
            VCALL(actor, 0xc, void (*)(void *, s32))(actor, 1);
    }
}

/* 0x02004008: notify(0) every actor of a layer
 * @difftest int:0:2 cases=10 */
void func_02004008(s32 layer)
{
    PtrVec *list = &data_020d9ea0[layer];
    void **it;
    for (it = list->items; it != list->items + list->count; it++) {
        u8 *actor = *it;
        VCALL(actor, 0xc, void (*)(void *, s32))(actor, 0);
    }
}

/* 0x02004078: notify(0) every actor of a layer with +0xa4 set
 * @difftest int:0:2 cases=10 */
void func_02004078(s32 layer)
{
    PtrVec *list = &data_020d9ea0[layer];
    void **it;
    for (it = list->items; it != list->items + list->count; it++) {
        u8 *actor = *it;
        if (PTR_AT(actor, 0xa4) != NULL)
            VCALL(actor, 0xc, void (*)(void *, s32))(actor, 0);
    }
}

static inline void CopyWords(void *dst, const void *src, u32 n)
{
    u32 i;
    for (i = 0; i < n; i++)
        ((u32 *)dst)[i] = ((const u32 *)src)[i];
}

void func_020041e0(void *dst, const void *src);
void func_020041f4(void *dst, const void *src);
void *func_02004218(void *dst, const void *src);
void func_02004278(void *dst, const void *src);
void func_02004294(void *dst, const void *src);

/* 0x020040f4: copy actor setup data (the source has a 4-byte header)
 * @difftest ptr:0x2c0:4 ptr:0x2c4:4 */
void func_020040f4(u8 *dst, const u8 *src)
{
    func_02004294(dst, src + 4);
    func_02004278(dst + 0xc, src + 0x10);
    func_02004218(dst + 0x18, src + 0x1c);
    func_02004218(dst + 0x7c, src + 0x80);
    func_02004218(dst + 0xe0, src + 0xe4);
    func_02004218(dst + 0x144, src + 0x148);
    func_02004218(dst + 0x1a8, src + 0x1ac);
    func_02004218(dst + 0x20c, src + 0x210);
    S32_AT(dst, 0x270) = S32_AT(src, 0x274);
    S32_AT(dst, 0x274) = S32_AT(src, 0x278);
    S32_AT(dst, 0x278) = S32_AT(src, 0x27c);
    S32_AT(dst, 0x27c) = S32_AT(src, 0x280);
    S32_AT(dst, 0x280) = S32_AT(src, 0x284);
    S8_AT(dst, 0x284) = S8_AT(src, 0x288);
    func_020041f4(dst + 0x288, src + 0x28c);
    func_020041e0(dst + 0x298, src + 0x29c);
    func_020041f4(dst + 0x2a0, src + 0x2a4);
    func_020041e0(dst + 0x2b0, src + 0x2b4);
    S32_AT(dst, 0x2b8) = S32_AT(src, 0x2bc);
    S32_AT(dst, 0x2bc) = S32_AT(src, 0x2c0);
}

/* 0x020041e0: copy 8 bytes
 * @difftest ptr:8:4 ptr:8:4 */
void func_020041e0(void *dst, const void *src) { CopyWords(dst, src, 2); }

/* 0x020041f4: copy 16 bytes
 * @difftest ptr:16:4 ptr:16:4 */
void func_020041f4(void *dst, const void *src) { CopyWords(dst, src, 4); }

/* 0x02004218: copy 0x64 bytes, return dst
 * @difftest ptr:0x64:4 ptr:0x64:4 */
void *func_02004218(void *dst, const void *src)
{
    CopyWords(dst, src, 0x64 / 4);
    return dst;
}

/* 0x02004278: copy 12 bytes
 * @difftest ptr:12:4 ptr:12:4 */
void func_02004278(void *dst, const void *src) { CopyWords(dst, src, 3); }

/* 0x02004294: copy 12 bytes
 * @difftest ptr:12:4 ptr:12:4 */
void func_02004294(void *dst, const void *src) { CopyWords(dst, src, 3); }

/* 0x020042b0: move `item` from one actor list to another
 * @difftest $X=u32 $I=ptr:0x40:4 @$I+8:32=$X $L=ptr:12:4 @$L:32=$I @$L+4:32=int:0:8 $J=ptr:0x40:4 $O=ptr:12:4 @$O:32=$J @$O+4:32=int:0:4 @$O+8:32=pick:4,8 $X $L $O */
void func_020042b0(void *item, PtrVec *list, PtrVec *other)
{
    void **end = list->items + list->count;
    void **it;
    for (it = list->items; it != end; it++)
        if (*it == item)
            break;
    if (it == end)
        return;
    if ((u32)other->count < (u32)other->cap) {
        other->count++;
        other->items[other->count - 1] = *it;
    } else {
        func_020062f0(other, it, 0);
    }
    func_02006240(list, it, 0);
}

/* 0x020043c8: actorKillAll - kill every actor of a list (except persistent ones) */
void func_020043c8(PtrVec *list)
{
    s32 i = 0, n = list->count;
    if (n <= 0)
        return;
    do {
        u8 *actor = list->items[i];
        void (*kill)(void *) = PTR_AT(actor, 0x98);
        if (kill != NULL) {
            kill(actor);
            PTR_AT(actor, 0x98) = NULL;
        }
        func_02004490(data_020bca84);
        if (PTR_AT(actor, 0x44) != NULL)
            func_0205047c(actor);
        func_02004490(data_020bcaac);
        if (BITS(A_FLAGS110(actor), 7, 1)) {
            i++;
        } else {
            func_020044a0(actor);
            func_02004490(data_020bcad0);
            n = list->count;
        }
    } while (i < n);
}

/* 0x02004490: debug printf (compiled out)
 * @difftest u32 u32 u32 u32 */
s32 func_02004490(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}
