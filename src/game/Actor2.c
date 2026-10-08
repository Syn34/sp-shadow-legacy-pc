/*
 * Actor base class, part 2: kill/spawn, per-frame update and sprite drawing.
 * ARM9 main, 0x020044a0 - 0x02005f20 (27 functions).
 *
 * Source file "Actor.cpp". Actors live in a pool per screen (func_020061a0,
 * 0x114 bytes each) and move between per-screen lists: free (data_020d9f18),
 * active (func_02006200), to draw (func_02006220) and drawn as sprites
 * (data_020d9f30). data_020df0fc is the current screen (0 or 1).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define A_FLAGS110(a) U32_AT(a, 0x110)

/* this file's own functions, called before their definition */
void func_020057e4(void *dst, const void *src);
void func_020057f8(u8 *anim, s32 frame, u32 flags, const s32 *pos, u32 tile, const u32 *tmpl, u8 *obj);
void func_02005af0(s32 *out, const s32 *x, const s32 *y);
s32 func_02005b04(void *a);
s32 func_02005b1c(void *a);
void func_02005b34(void *dst, const void *src);
u32 *func_02005b48(void *obj);
s32 func_02005b50(void *a);
void func_02005b58(void *model, const s32 *pos);
void func_02005b7c(s32 *out, const s32 *x, const s32 *y, const s32 *z);
void func_02005b98(s32 *out, const void *src);
u32 func_02005bb4(void);
void func_02005be4(s32 *v, const s32 *add);
void func_02005c1c(s32 *out, const s32 *a, const s32 *b);
void func_02005c50(s32 *v, const s32 *scale);
s32 func_02005c88(void *a);
void func_02005ca0(void *model, s32 v);
s32 func_02005ca8(void *a);
void func_02005cb0(s32 *v);
void *func_02005cc0(void *obj);
void *func_02005cc8(void *a);
void *func_02005d78(void);

/* std::vector<T*>::push_back(*value) */
static inline void VecPush(PtrVec *v, void **value)
{
    if ((u32)v->count < (u32)v->cap) {
        v->count++;
        v->items[v->count - 1] = *value;
    } else {
        func_020062f0(v, value, 0);
    }
}

/* round to the nearest integer, halves away from zero (v << sh as float) */
static s32 RoundShl(s32 v, s32 sh)
{
    float f = (float)SHL(v, sh);
    return (s32)(v > 0 ? 0.5f + f : f - 0.5f);
}

/* OAM manager of the current screen */
#define OAM_ID() U8_AT(data_020df598, data_020df0fc * 0x13ec + 0x13e9)

/* 0x020044a0: kill an actor and return it to the free list */
void func_020044a0(void *actor)
{
    void *self = actor;
    PtrVec *list;
    void **end, **it;
    u32 mode;

    if (data_020deebc[2] != 0 && func_02016f24(data_020deebc) != NULL && U32_AT(self, 0xc) != 0) {
        if (U32_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) == (u32)self) {
            u32 v = data_020deebc[2];
            U32_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = v;
        }
    }
    if (PTR_AT(self, 0xec) != NULL)
        func_020044a0(PTR_AT(self, 0xec));
    if (PTR_AT(self, 0x98) != NULL) {
        ((void (*)(void *))PTR_AT(self, 0x98))(self);
        PTR_AT(self, 0x98) = NULL;
    }
    func_0201aa88(self);
    U32_AT(self, 0x8) = 0xffff;
    U32_AT(self, 0xc) = 0xffff;
    mode = BITS(U32_AT(self, 0xa8), 8, 2);
    if (mode == 1 || mode == 3)
        func_0200809c(self);
    if (PTR_AT(self, 0x44) != NULL)
        func_0205047c(self);
    else
        func_02003740(self);
    if (PTR_AT(self, 0xe8) != NULL) {
        func_0207ff14(PTR_AT(self, 0xe8), data_020bca78, 0x3b9);
        PTR_AT(self, 0xe8) = NULL;
    }
    if (BITS(U32_AT(self, 0x38), 0, 1))
        func_02008130(S8_AT(self, 0x42), S16_AT(self, 0x40));
    func_02050680(self);

    U8_AT(self, 0x108) = 0;
    U8_AT(self, 0x109) = 0;
    U8_AT(self, 0x10a) = 0;
    U8_AT(self, 0x10b) = 0;
    U32_AT(self, 0x94) = 0;
    U32_AT(self, 0xec) = 0;
    U32_AT(self, 0xa0) = 0;
    U32_AT(self, 0xa4) = 0;
    S32_AT(self, 0x4c) = -1;
    U32_AT(self, 0x54) = 0;
    U32_AT(self, 0x58) = 0;
    U32_AT(self, 0x5c) = 0;
    U32_AT(self, 0x60) = 0;
    U32_AT(self, 0x24) = 0;
    U32_AT(self, 0x6c) = 0;

    /* remove from the active (or other) list; erase(find(...)) */
    if (BITS(A_FLAGS110(self), 1, 1)) {
        list = func_02006200();
        end = list->items + list->count;
        for (it = func_02006200()->items; it != end; it++)
            if (*it == self)
                break;
        func_02006240(func_02006200(), it, 0);
    } else {
        list = func_020061e0();
        end = list->items + list->count;
        for (it = func_020061e0()->items; it != end; it++)
            if (*it == self)
                break;
        func_02006240(func_020061e0(), it, 0);
    }
    A_FLAGS110(self) = 0;
    VecPush(&data_020d9f18[data_020df0fc], &self);
}

/* 0x02004864: on both screens, refresh the palette of actors with a new frame
 * and empty the sprite list */
void func_02004864(void)
{
    u32 saved = data_020df0fc;
    u32 i;
    for (i = 0; i < 2; i++) {
        PtrVec *v;
        void **it;
        data_020df0fc = i;
        if (U8_AT(data_020df0b4 + i, 0x38) != 1)
            continue;
        v = &data_020d9f30[i];
        for (it = v->items; it != v->items + v->count; it++) {
            u8 *a = *it;
            if (BITS(A_FLAGS110(a), 11, 1)) {
                A_FLAGS110(a) &= ~0x800u;
                U32_AT(a, 0xac) = (U32_AT(a, 0xac) & ~0x3ffu) | (U16_AT(a, 0xde) & 0x3ff);
            }
            v = &data_020d9f30[data_020df0fc];
        }
        v->count = 0;
        func_02028f04();
    }
    data_020df0fc = saved;
}

/* write the actor's sprite(s) to OAM. Returns 0: not visible, 1: tiled
 * sprites, 2: single sprite or no VRAM slot (gets drawn as a sprite later) */
static u32 DrawActorSprite(u8 *a)
{
    s32 scr[2], pos[2], unused[2];
    u32 attr[2], attr2[2];
    u32 flags, n;
    s32 k;

    func_02005cb0(scr);
    func_02005cb0(pos);
    func_02005cb0(unused);
    func_020057e4(attr, a + 0xa8);
    func_02005b34(pos, a + 0x5c);
    flags = (U8_AT(a, 0xe1) & 0x80) ? 2 : 0;
    if (!func_02005b1c(a)) {
        func_02002164(unused);
        func_02002164(pos);
        func_02002164(scr);
        return 0;
    }
    if (U16_AT(a, 0xde) == 0xffff) {
        func_02002164(unused);
        func_02002164(pos);
        func_02002164(scr);
        return 2;
    }
    if (func_02005b04(a)) {
        s32 px, py;
        py = pos[1] >> 16;
        px = pos[0] >> 16;
        func_02005af0(scr, &px, &py);
    } else {
        func_0201f6c8(pos);
        func_02005b34(scr, pos);
    }
    U32_AT(a, 0xa8) = (U32_AT(a, 0xa8) & 0xfe00ffff) | ((scr[0] & 0x1ff) << 16);
    U8_AT(a, 0xa8) = scr[1];
    if (U8_AT(a, 0xe0) == 0) {
        n = 1;
        func_020057e4(attr2, attr);
        if (U8_AT(a, 0xe1) & 0x20)
            flags |= 1;
        else
            flags &= ~1u;
        func_020057f8(func_02003958(a), U8_AT(a, 0xb3), flags & 0xff, scr, U16_AT(a, 0xde), attr2, a);
        n++;
    } else {
        U32_AT(a, 0xac) = (U32_AT(a, 0xac) & ~0x3ffu) | (U16_AT(a, 0xde) & 0x3ff);
        for (k = U16_AT(a, 0xb4); k > 0;) {
            u32 ac;
            if (U8_AT(a, 0xe1) & 0x20)
                func_0201a390(OAM_ID());
            func_0201a7c8(OAM_ID(), (u32 *)(a + 0xa8));
            k--;
            U32_AT(a, 0xa8) = (U32_AT(a, 0xa8) & 0xfe00ffff) | (((BITS(U32_AT(a, 0xa8), 16, 9) + 0x20) & 0x1ff) << 16);
            ac = U32_AT(a, 0xac);
            U32_AT(a, 0xac) = (ac & ~0x3ffu) | (((ac & 0x3ff) + (U32_AT(a, 0xa8) >> 30) * 4) & 0x3ff);
        }
        n = 1;
    }
    func_02002164(unused);
    func_02002164(pos);
    func_02002164(scr);
    return n & 0xff;
}

/* 0x02004978: per-frame update of all active actors of the current screen:
 * run their update, project positions, refresh sprite VRAM and draw */
void func_02004978(void)
{
    PtrVec *actors = func_02006200();
    PtrVec *v;
    void *cur;
    void **it;
    u32 i;
    s32 k;

    v = func_02006220();
    v->count = 0;
    data_020d9f30[data_020df0fc].count = 0;
    v = func_020061c0();
    v->count = 0;
    if (data_020df0fc == 0)
        func_0200c110(data_020e1f50, func_020822c0(func_020062d0()));

    for (k = actors->count - 1; k >= 0; k--) {
        cur = actors->items[k];
        VCALL(cur, 0x8, void (*)(void *, s32))(cur, 1);
    }

    if (data_020df0fc == 0) {
        u32 t = func_020822c0(func_020062d0());
        void *o = func_020062a0();
        VCALL(o, 0x8, void (*)(void *, u32))(o, t);
        o = func_020062b0();
        VCALL(o, 0x8, void (*)(void *, u32))(o, t);
        data_020deebc[8] &= ~4u;
        o = func_020062c0();
        VCALL(o, 0x8, void (*)(void *, u32))(o, t);
    }
    if (data_020df0fc == 0 && data_020df17c == 1)
        func_02018ba4();
    if (BITS(data_020deebc[8], 4, 1)) {
        u8 *p = (u8 *)data_020deebc[2];
        if (BITS(U32_AT(p, 0x38), 17, 1))
            U32_AT(p, 0x38) &= ~0x20000u;
        else
            U32_AT(p, 0x38) |= 0x20000;
    }

    /* place every model in the 3D scene */
    for (i = 0; i < (u32)actors->count; i++) {
        u8 *a = actors->items[i];
        s32 cam[2], d[2], cam2[2], pos[2], sh[2], scale, y, z, out[3], tmp[3];
        s32 h, ground, base;
        float lvl;

        cur = a;
        if (func_02005cc8(a) == NULL)
            continue;
        func_020041e0(cam, func_02005cc0(func_0201f490()));
        func_02005cb0(d);
        func_020041e0(pos, a + 0x54);
        h = func_02005ca8(a);
        h = (h - (s32)func_0201a0a0(pos, a)) >> 4;
        func_02002164(pos);
        func_02005ca0(func_02005cc8(a), h);
        if (func_02005c88(a)) {
            h = (func_02005ca8(a) - S32_AT(a, 0x68)) >> 4;
            func_02005ca0(func_02005cc8(a), h);
        }
        func_0201c428(cam2);
        func_02003b08(sh, cam, 4);
        ground = func_02005ca8(a);
        ground -= (s32)func_0201a0a0(sh, NULL);
        func_02002164(sh);
        d[0] = (S32_AT(a, 0x54) >> 4) - cam[0];
        d[1] = (S32_AT(a, 0x58) >> 4) - cam[1];
        scale = func_0200be9c(data_020e1f50);
        func_02005c50(d, &scale);
        func_02005be4(d, cam);
        ground = (cam2[1] - (d[1] >> 12)) + (ground >> 16);

        if (S32_AT(data_020c3b60 + func_02005bb4() * 0x60, 0x38) > 0)
            lvl = 0.5f + (float)SHL(S32_AT(data_020c3b60 + func_02005bb4() * 0x60, 0x38), 12);
        else
            lvl = (float)SHL(S32_AT(data_020c3b60 + func_02005bb4() * 0x60, 0x38), 12) - 0.5f;
        base = func_02052f10(func_02005cc8(a)) + 0x1d000;
        h = (S32_AT(a, 0x58) >> 4) - (s32)lvl;
        func_02005b98(tmp, data_020df1bc);
        z = h + tmp[2] - base;
        func_020022e4(tmp);
        y = RoundShl(ground, 12);
        func_02005b7c(out, d, &y, &z);
        if (func_02005c88(a))
            out[2] += S32_AT(a, 0x68) >> 4;
        out[0] = RoundShl(out[0] >> 12, 12);
        out[1] = RoundShl(out[1] >> 12, 12);
        func_02005b58(func_02005cc8(a), out);
        if (func_02005b50(a) == 0 && BITS(*func_02005b48(data_020deebc), 5, 1))
            func_02017180(data_020deebc);
        func_020022e4(out);
        func_02002164(cam2);
        func_02002164(d);
        func_02002164(cam);
    }

    if (data_020df0fc == 0)
        func_0201f004();

    /* commit positions */
    for (i = 0; i < (u32)actors->count; i++) {
        u8 *a = actors->items[i];
        s32 t[2];
        cur = a;
        S32_AT(a, 0x54) = S32_AT(a, 0x5c);
        S32_AT(a, 0x58) = S32_AT(a, 0x60);
        S32_AT(a, 0x24) = S32_AT(a, 0x6c);
        func_020038d4(t, (s32 *)(a + 0x54), 4);
        S32_AT(a, 0x28) = t[0];
        S32_AT(a, 0x2c) = t[1];
    }

    /* collect the visible 2D actors */
    for (i = 0; i < (u32)actors->count; i++) {
        u8 *a = actors->items[i];
        cur = a;
        if (PTR_AT(a, 0x44) != NULL) {
            func_02007708(a);
            continue;
        }
        if (!BITS(U32_AT(a, 4), 3, 1) || !BITS(U32_AT(a, 0x38), 13, 1) || BITS(A_FLAGS110(a), 3, 1))
            continue;
        VecPush(func_02006220(), &cur);
    }

    if (BITS(data_020deebc[8], 4, 1)) {
        u8 *p;
        u32 f = data_020deebc[8] & ~0x10u;
        p = (u8 *)data_020deebc[2];
        data_020deebc[8] = f;
        if (BITS(U32_AT(p, 0x38), 17, 1))
            U32_AT(p, 0x38) &= ~0x20000u;
        else
            U32_AT(p, 0x38) |= 0x20000;
    }

    /* upload changed frames to sprite VRAM and draw */
    it = func_02006220()->items;
    v = func_02006220();
    if (it == v->items + v->count)
        return;
    do {
        u8 *a = *it;
        u32 upload = 1, result;
        u32 size[2];

        cur = a;
        size[0] = 0;
        size[1] = 0;
        if (BITS(A_FLAGS110(a), 11, 1) || U16_AT(a, 0xde) == 0xffff) {
            u32 n, slot;
            func_0200384c(a, size);
            n = (u16)(size[0] * size[1]);
            if (U16_AT(a, 0xde) != 0xffff)
                func_020291ec(U16_AT(a, 0xde), U16_AT(a, 0xdc), BITS(U32_AT(a, 0xa8), 13, 1));
            slot = (u16)func_02029284(0, n, BITS(U32_AT(a, 0xa8), 13, 1));
            if (slot == 0xffff) {
                upload = 0;
                n = 0;
            }
            U16_AT(a, 0xde) = slot;
            U32_AT(a, 0xac) = (U32_AT(a, 0xac) & ~0x3ffu) | (slot & 0x3ff);
            U16_AT(a, 0xdc) = n;
            if (upload == 1) {
                func_0202926c(a, func_020037ec(a));
                func_020129ac(*(s32 *)PTR_AT(a, 0xb8));
            }
            A_FLAGS110(a) |= 0x800;
        }
        U8_AT(a, 0xb2) = U8_AT(a, 0xb3);
        result = DrawActorSprite(a);
        if (U8_AT(a, 0xe1) & 0x10) {
            /* mirrored copy */
            U32_AT(a, 0xa8) |= 0x10000000;
            DrawActorSprite(a);
            U32_AT(a, 0xa8) &= ~0x10000000u;
        }
        if (result == 2)
            VecPush(&data_020d9f30[data_020df0fc], &cur);
        it++;
        v = func_02006220();
    } while (it != v->items + v->count);
}

/* 0x020057e4: copy 8 bytes
 * @difftest ptr:8:4 ptr:8:4 */
void func_020057e4(void *dst, const void *src)
{
    u32 a = ((const u32 *)src)[0];
    ((u32 *)dst)[0] = a;
    ((u32 *)dst)[1] = ((const u32 *)src)[1];
}

#define SEXT9(x) (SHL(x, 23) >> 23)

/* half of a size, rounded toward zero */
static inline s32 Half(s32 v) { return (v + (s32)((u32)v >> 31)) >> 1; }

/* scale an offset by a 16.16 factor, rounding toward zero */
static inline s32 ScaleOfs(s32 v, s32 s)
{
    s32 t = v * s;
    return t >= 0 ? t >> 16 : -((-t) >> 16);
}

/* 0x020057f8: write the OAM entries of one animation frame (up to 31 parts)
 * @difftest $N=ptr:0x200:4 @$N+0xa:8=int:0:4 @$N+0xb:8=int:0:4 @$N+0xc:16=int:0:0x40 $O=ptr:0xd0:4 $N int:0:2 u8 ptr:8:4 u16 ptr:8:4 $O cases=200 */
void func_020057f8(u8 *anim, s32 frame, u32 flags, const s32 *pos, u32 tile, const u32 *tmpl, u8 *obj)
{
    s32 dim[2];
    u8 *ent, *parts;
    u32 count, flip, vflip;
    s32 sx, sy;

    dim[0] = 0;
    dim[1] = 0;
    ent = anim + 0xc + U16_AT(anim, 0xc + frame * 2);
    count = ent[0] & 0x1f;
    parts = ent + 0xa + U8_AT(anim, 0xb) * 0xa + U8_AT(anim, 0xa) * 2;
    sx = S32_AT(obj, 0xc8);
    sy = S32_AT(obj, 0xcc);
    if (count == 0)
        return;
    flip = flags & 3;
    vflip = flags & 2;
    tile = (u16)tile;
    do {
        u32 part, attr[2], a0;
        s32 x, y;
        u32 mode;

        MI_CpuCopy8(parts, &part, 4);
        attr[0] = tmpl[0];
        attr[1] = (tmpl[1] & ~0x3ffu) | ((tile + (part >> 22)) & 0x3ff);
        a0 = (attr[0] & ~0xc0000000u) | (BITS(part, 18, 2) << 30);
        a0 = (a0 & ~0xc000u) | (BITS(part, 20, 2) << 14);
        attr[0] = a0;
        func_02008104(BITS(a0, 14, 2), a0 >> 30, dim);

        a0 = attr[0];
        mode = BITS(a0, 8, 2);
        if (mode == 0) {
            if (BITS(a0, 28, 1) == 1)
                x = -(dim[0] + SEXT9(part));
            else
                x = SEXT9(part);
        } else {
            x = ScaleOfs(SEXT9(part) + Half(dim[0]), sx);
            x -= mode == 3 ? dim[0] : Half(dim[0]);
        }
        a0 = (a0 & 0xfe00ffff) | (((pos[0] + x) & 0x1ff) << 16);
        attr[0] = a0;

        mode = BITS(a0, 8, 2);
        if (mode == 0) {
            if (BITS(a0, 29, 1) == 1) {
                y = -(dim[1] + SEXT9(part >> 9));
            } else if (vflip == 0) {
                y = SEXT9(part >> 9);
            } else {
                attr[0] = a0 | 0x20000000;
                y = -(dim[1] + SEXT9(part >> 9));
                attr[1] &= ~0xc00u;
            }
        } else {
            y = ScaleOfs(SEXT9(part >> 9) + Half(dim[1]), sy);
            y -= mode == 3 ? dim[1] : Half(dim[1]);
        }
        parts += 4;
        y += pos[1];
        if ((s32)(y + 0x40) >= 0 && y + 0x40 <= 0x100) {
            ((u8 *)attr)[0] = y;
            func_0201a7c8(OAM_ID(), attr);
            if (flip != 0)
                func_0201a390(OAM_ID() - 1);
        }
    } while (--count != 0);
}

/* 0x02005af0: 2D vector from two values
 * @difftest ptr:8:4 ptr:4:4 ptr:4:4 */
void func_02005af0(s32 *out, const s32 *x, const s32 *y)
{
    out[0] = *x;
    out[1] = *y;
}

/* 0x02005b04: +0x110 bit 2 (position is in screen space)
 * @difftest ptr:0x120:4 */
s32 func_02005b04(void *a) { return (s8)BITS(A_FLAGS110(a), 2, 1); }

/* 0x02005b1c: +0x38 bit 13 (visible)
 * @difftest ptr:0x40:4 */
s32 func_02005b1c(void *a) { return (s8)BITS(U32_AT(a, 0x38), 13, 1); }

/* 0x02005b34: copy a 2D vector
 * @difftest ptr:8:4 ptr:8:4 */
void func_02005b34(void *dst, const void *src)
{
    u32 a = ((const u32 *)src)[0];
    ((u32 *)dst)[0] = a;
    ((u32 *)dst)[1] = ((const u32 *)src)[1];
}

/* 0x02005b48
 * @difftest u32 */
u32 *func_02005b48(void *obj) { return (u32 *)((u8 *)obj + 0x20); }

/* 0x02005b50
 * @difftest ptr:0x10:4 */
s32 func_02005b50(void *a) { return S32_AT(a, 0xc); }

/* 0x02005b58: set a model's position and mark it dirty
 * @difftest ptr:0x90:4 ptr:12:4 */
void func_02005b58(void *model, const s32 *pos)
{
    S32_AT(model, 0x4) = pos[0];
    S32_AT(model, 0x8) = pos[1];
    S32_AT(model, 0xc) = pos[2];
    U8_AT(model, 0x8c) = 1;
}

/* 0x02005b7c: 3D vector from three values
 * @difftest ptr:12:4 ptr:4:4 ptr:4:4 ptr:4:4 */
void func_02005b7c(s32 *out, const s32 *x, const s32 *y, const s32 *z)
{
    out[0] = *x;
    out[1] = *y;
    out[2] = *z;
}

/* 0x02005b98: 3D vector from +4
 * @difftest ptr:12:4 ptr:16:4 */
void func_02005b98(s32 *out, const void *src)
{
    out[0] = S32_AT(src, 4);
    out[1] = S32_AT(src, 8);
    out[2] = S32_AT(src, 0xc);
}

/* 0x02005bb4: current level's record index
 * @difftest */
u32 func_02005bb4(void)
{
    return U8_AT(func_020436f0(data_020bf6a0), 0xf) >> 1;
}

/* 0x02005be4: v += add (2D)
 * @difftest ptr:8:4 ptr:8:4
 * @difftest $V=ptr:8:4 $V $V */
void func_02005be4(s32 *v, const s32 *add)
{
    s32 t[2];
    func_02005c1c(t, v, add);
    v[0] = t[0];
    v[1] = t[1];
}

/* 0x02005c1c: out = a + b (2D)
 * @difftest ptr:8:4 ptr:8:4 ptr:8:4
 * @difftest $V=ptr:8:4 $V $V ptr:8:4 */
void func_02005c1c(s32 *out, const s32 *a, const s32 *b)
{
    s32 a0 = a[0], b0 = b[0], a1 = a[1], b1 = b[1];
    out[0] = a0 + b0;
    out[1] = a1 + b1;
}

/* 0x02005c50: v *= scale (2D, fx32)
 * @difftest ptr:8:4 ptr:4:4 */
void func_02005c50(s32 *v, const s32 *scale)
{
    s32 t[2];
    func_02003798(t, v, scale);
    v[0] = t[0];
    v[1] = t[1];
}

/* 0x02005c88: +0x38 bit 17
 * @difftest ptr:0x40:4 */
s32 func_02005c88(void *a) { return (s8)BITS(U32_AT(a, 0x38), 17, 1); }

/* 0x02005ca0
 * @difftest ptr:0xa4:4 u32 */
void func_02005ca0(void *model, s32 v) { S32_AT(model, 0xa0) = v; }

/* 0x02005ca8
 * @difftest ptr:0x28:4 */
s32 func_02005ca8(void *a) { return S32_AT(a, 0x24); }

/* 0x02005cb0: zero 2D vector
 * @difftest ptr:8:4 */
void func_02005cb0(s32 *v)
{
    v[0] = 0;
    v[1] = 0;
}

/* 0x02005cc0
 * @difftest u32 */
void *func_02005cc0(void *obj) { return (u8 *)obj + 0x28; }

/* 0x02005cc8: the actor's 3D model
 * @difftest ptr:0x48:4 */
void *func_02005cc8(void *a) { return PTR_AT(a, 0x44); }

/* 0x02005cd0: spawn an actor of type `type` */
void *func_02005cd0(s32 type)
{
    u8 *a = func_02005d78();
    if (a == NULL)
        return NULL;
    S32_AT(a, 0xc) = type;
    PTR_AT(a, 0xec) = NULL;
    U32_AT(a, 0x38) &= ~0x8u;
    U32_AT(a, 0x38) &= ~0x400u;
    U32_AT(a, 0x38) &= ~0x800u;
    U32_AT(a, 0x38) |= 0x4000;
    U32_AT(a, 0x100) = 0;
    U32_AT(a, 0x104) = 0;
    U32_AT(a, 0x70) = 0;
    U32_AT(a, 0x74) = 0;
    U32_AT(a, 0x78) = 0;
    U8_AT(a, 0x108) = 0;
    U8_AT(a, 0x109) = 0;
    U8_AT(a, 0x10a) = 0;
    U8_AT(a, 0x10b) = 0;
    U8_AT(a, 0x10c) = 0;
    PTR_AT(a, 0xe8) = NULL;
    return a;
}

/* 0x02005d5c: spawn an actor and initialise its model */
void *func_02005d5c(s32 type)
{
    void *a = func_02005cd0(type);
    func_020504b8(a);
    return a;
}

/* 0x02005d78: take an actor from the free list and make it active */
void *func_02005d78(void)
{
    void *a = NULL;
    PtrVec *free = &data_020d9f18[data_020df0fc];
    PtrVec *pool;
    u8 *p;

    if (free->count != 0) {
        a = free->items[free->count - 1];
        U32_AT(a, 0x110) = 0;
        U32_AT(a, 0x38) = 0;
        U32_AT(a, 0x4) = 0;
        data_020d9f18[data_020df0fc].count--;
        U32_AT(a, 0xc) = 0xffff;
        U32_AT(a, 0x8) = 0xffff;
        U16_AT(a, 0xde) = 0xffff;
        U32_AT(a, 0x38) |= 0x10000;
        S32_AT(a, 0x4c) = 0;
        /* +0x4c = index in the pool */
        p = (u8 *)func_020061a0()->items;
        for (;;) {
            pool = func_020061a0();
            if (p == (u8 *)pool->items + pool->count * 0x114 || p == a)
                break;
            S32_AT(a, 0x4c)++;
            p += 0x114;
        }
        VecPush(func_02006200(), &a);
        A_FLAGS110(a) |= 2;
        A_FLAGS110(a) = (A_FLAGS110(a) & ~1u) | 1;
    }
    return a;
}
