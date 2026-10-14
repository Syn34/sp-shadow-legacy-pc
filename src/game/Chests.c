/*
 * Chests: chest actors (kind 6), the drop each one keeps ready, and the
 * map from chests to drops.
 * ARM9 main, 0x0203f518 - 0x02040088 (10 functions).
 *
 * A chest is created from a map object (func_0203fd88); its drop (a
 * 0x24-byte spawner holding the pickups, func_02054f6c) is made at once
 * and kept in the map data_020ebb54 (a red-black tree keyed by the chest
 * actor, header node at data_020ebb58). Chest kind 2 is locked: touching
 * it with a key (A button) uses one of the inventory's keys
 * (data_020def74 + 0x12b). Other chests are pushed aside by kind-6 actors
 * that come close and open when the hero is near and attacking.
 * Actor callbacks: +0x94 per frame (func_0203fc78), +0x98 removal
 * (func_0203fd7c), +0x9c collision (func_0203f780 / func_0203f854).
 *
 * Testing: with fake actors; heap and card functions stubbed where the
 * set-up allocates.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const char data_020bf058[];   /* source file name for the heap */
extern const char data_020bf068[], data_020bf098[], data_020bf0d0[];   /* debug messages */
extern const fx32 data_020bee80;
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */
extern u8 data_020d9f9c[];
extern u8 data_020def74[];
extern s32 data_020ebb50;            /* push radius (squared) */
extern u8 data_020ebb54[];           /* the chests' drops: a map */
extern u8 data_020ebb58[];           /* (its header node) */
extern u8 data_020ebb6c[];           /* the locked chest tutorial message */
extern const fx32 g_FxTwoPi, g_FxPi_2;

void func_0207fe28(const char *file, s32 line);
void func_0207fe24(void);
void *func_0207fe74(u32 size);
void *func_02054f6c(void *p, s32 *pos);
u8 *func_020400e4(void *map, void *key);
void func_020401d4(u8 **it, void *map, void *key);
s32 func_02040178(void *map, void *key);
void func_02040230(u8 **it);
void func_0204029c(void *map);
void func_02040088(void);
void func_0203f168(void *o, void *pos, u32 kind);
void func_0203f470(void *o, void *pos, u32 kind, u32 a, u16 b);
u32 func_020186ec(void);
u32 func_02019650(void);
u8 *func_02011f34(void);
void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
void func_02072418(void *obj, u32 a, u32 fbits);
void *func_02009394(void);
void func_0205f6b0(void *snd, void *msg);
s32 func_0205f598(void *snd, void *msg);
s32 func_0208f30c(s32 x, s32 y);
void func_020156f4(s32 *out, const s32 *in, s32 sh);
void func_02053648(void *obj, u32 kind, void *x, u32 a, u32 b);
void func_02053460(void *obj, u32 dt);
u8 *func_02008994(s32 i, s32 k);
u8 *func_02005d5c(s32 type);
s32 func_0200820c(u32 id, s32 kind);
void func_02003c74(void *actor, s32 x, s32 y);
s32 func_0201a0a0(s32 *xz, void *obj);
fx32 func_02001ef4(u32 dir);

void func_0203f518(u8 *who, u8 *def);
void func_0203f780(u8 *self, u8 *other);
void func_0203f854(u8 *self, u8 *other);
void func_0203fc10(u8 *self, u32 kind);
void func_0203fc78(u8 *self);
void func_0203fd08(u8 *key);
void func_0203fd7c(u8 *key);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define KIND(a) (data_020c3b0c + S32_AT(a, 8) * 0x48)

static inline s32 Wrap(s32 a, s32 lo, s32 hi, s32 turn)
{
    while (a < lo)
        a += turn;
    while (a > hi)
        a -= turn;
    return a;
}

/* make `who`'s drop spawner and file it in the map */
static void *NewDrop(u8 *who, u8 **key, s32 *pos, s32 line1, s32 line2)
{
    void *d;

    pos[0] = S32_AT(who, 0x28);
    pos[1] = S32_AT(who, 0x2c);
    func_0207fe28(data_020bf058, line1);
    d = func_0207fe74(0x24);
    if (d != NULL)
        d = func_02054f6c(d, pos);
    func_0207fe24();
    if (d == NULL)
        return NULL;
    func_0207fe28(data_020bf058, line2);
    PTR_AT(func_020400e4(data_020ebb54, key), 4) = d;
    func_0207fe24();
    return d;
}

/* call an object's method 1 (its deleting destructor), address in r1 */
static void Delete(u8 *o)
{
    void (*m)(void *, void *) = VCALL(o, 4, void (*)(void *, void *));

    m(o, (void *)m);
}

/* 0x0203f518: the drop of a chest (or other container)
 * @difftest $W=zero:0x40 $D=zero:0x20 @$D+0x1c:8=int:0:6 @$D+0xa:16=int:0:0x20 @$D+0x10:16=int:0:0x20 @$D+0x16:16=int:0:0x20 $W $D stub=0x0207fe74:0 cases=20
 * @difftest $W=zero:0x40 $D=zero:0x20 @$D+0x1c:8=int:0:6 @$D+0xa:16=int:0:0x20 @$D+0x10:16=int:0:0x20 @$D+0x16:16=int:0:0x20 $W $D stub=0x02054e64 cases=20 */
void func_0203f518(u8 *who, u8 *def)
{
    s32 pos[2];
    u8 *d = NewDrop(who, &who, pos, 0x1b8, 0x1be);

    if (d == NULL)
        return;
    func_0203f168(d, pos, def[0x1c]);
    func_0203f470(d, pos, U16_AT(def, 0xa), U16_AT(def, 0xc), U16_AT(def, 0xe));
    func_0203f470(d, pos, U16_AT(def, 0x10), U16_AT(def, 0x12), U16_AT(def, 0xe));
    func_0203f470(d, pos, U16_AT(def, 0x16), U16_AT(def, 0x18), U16_AT(def, 0x1a));
}

/* 0x0203f620: the drop of a boss
 * @difftest $W=zero:0x40 $D=zero:0x20 $W $D stub=0x0207fe74:0 cases=10 */
void func_0203f620(u8 *who, u8 *def)
{
    s32 pos[2];
    u8 *d = NewDrop(who, &who, pos, 0x192, 0x198);

    if (d == NULL)
        return;
    func_0203f168(d, pos, def[0x1c]);
    func_0203f470(d, pos, U16_AT(def, 0xc), U16_AT(def, 0xc), U16_AT(def, 0x10));
    func_0203f470(d, pos, U16_AT(def, 0x12), U16_AT(def, 0x12), U16_AT(def, 0x10));
    func_0203f470(d, pos, U16_AT(def, 0x18), U16_AT(def, 0x18), U16_AT(def, 0x1c));
    if (func_020186ec() != 0 || func_02019650() == 0x1f)
        func_0203f470(d, pos, 0xc, 0, 0);
    else
        func_0203f470(d, pos, 0xb, 0, 0);
}

/* 0x0203f780: a locked chest touched by the hero: A uses a key
 * @difftest $S=zero:0x40 $O=zero:0x20 @$O+0xc:32=pick:0,0,1 $K=zero:0x80 @020f6f30:32=$K @$K+0x5c:32=pick:0,1 @020da11b:8=pick:0,1 @020df09f:8=int:0:2 $S $O stub=0x02072418,0x0205f6b0,0x0205f598:0 cases=40 */
void func_0203f780(u8 *self, u8 *other)
{
    s32 ok;

    if (S32_AT(other, 0xc) != 0)
        return;
    func_02072418(func_02011f34(), 3, 0x45000000);   /* 2048.0f */
    ((u8 *)data_020deebc)[0x1f] = 1;
    if (data_020d9f9c[0x17f] == 0) {
        data_020d9f9c[0x17f] = 1;
        func_0205f6b0(func_02009394(), data_020ebb6c);
        return;
    }
    if (func_0205f598(func_02009394(), data_020ebb6c) != 0)
        return;
    if (func_02081d34(func_02011f44(), 0xa) == 0)
        return;
    if (data_020def74[0x12b] == 0) {
        ok = 0;
    } else {
        data_020def74[0x12b]--;
        ok = 1;
    }
    if (ok)
        U32_AT(self, 0x38) |= 0x400;
}

/* 0x0203f854: a chest touched: kind-6 actors push it aside, the hero
 * opens it with an attack
 * @difftest $A=zero:0x20 $B=zero:0x20 @$A+4:32=int:-0x8000:0x8000 @$A+8:32=int:-0x8000:0x8000 @$B+4:32=int:-0x8000:0x8000 @$B+8:32=int:-0x8000:0x8000 @$B+0x14:32=int:0:0x6487 $S=zero:0x120 @$S+0x44:32=$B @$S+0x24:32=int:-0x100000:0x100000 $O=zero:0x60 @$O+0xc:32=pick:6,6,0 @$O+0x44:32=$A @$O+8:32=pick:1,1,2 @$O+0x24:32=int:-0x100000:0x100000 @$O+0x38:32=pick:0,0x100 $H=zero:0x60 @$H+0x24:32=int:-0x100000:0x100000 @$H+0x38:32=pick:0,0x100 @020deec4:32=$H @020ebb50:32=int:0:0x4000000 @020deed4:32=pick:8,0x10,0x20 $S $O cases=200 */
void func_0203f854(u8 *self, u8 *other)
{
    if (S32_AT(other, 0xc) == 6) {
        u8 *a, *b;
        s32 v[3], out[3], dx, dy, dz, ang, h, diff, dir;
        u32 idx;

        if (PTR_AT(self, 0xa0) != NULL && PTR_AT(self, 0xa0) == other)
            return;
        a = PTR_AT(other, 0x44);
        b = PTR_AT(self, 0x44);
        dz = S32_AT(b, 8) - S32_AT(a, 8);
        dy = data_020bee80;
        dx = S32_AT(a, 4) - S32_AT(b, 4);
        v[0] = dx;
        v[1] = dy;
        v[2] = dz;
        if (FxMul(dz, dz) + FxMul(dx, dx) + FxMul(dy, dy) > data_020ebb50)
            return;
        ang = func_0208f30c(dx, dz);
        h = S32_AT(PTR_AT(self, 0x44), 0x14);
        ang = Wrap(ang, g_FxZero, g_FxTwoPi, g_FxTwoPi);
        diff = ang - Wrap(h, g_FxZero, g_FxTwoPi, g_FxTwoPi);
        diff = Wrap(diff, -g_FxPi, g_FxPi, g_FxTwoPi);
        if (diff < g_FxZero)
            dir = Wrap(g_FxPi_2 + h, g_FxZero, g_FxTwoPi, g_FxTwoPi);
        else
            dir = Wrap(h - g_FxPi_2, g_FxZero, g_FxTwoPi, g_FxTwoPi);
        idx = FxRadToIdx(dir) >> 4;
        v[0] = FxMul(FX_SinCosTable_[idx * 2], 0x1000);
        v[1] = FxMul(FX_SinCosTable_[idx * 2 + 1], 0x1000);
        func_020156f4(out, v, 4);
        S32_AT(self, 0x100) = out[0];
        S32_AT(self, 0x104) = out[1];
        return;
    }
    if (S32_AT(other, 0xc) != 0)
        return;
    if (S32_AT(other, 8) != 1)
        other = HERO();
    {
        s32 d = (S32_AT(other, 0x24) - S32_AT(self, 0x24)) >> 4;
        if (d < 0)
            d = -d;
        if (d > 0xc000)
            return;
    }
    U32_AT(self, 0x38) |= 8;
    if ((s8)BITS(U32_AT(other, 0x38), 8, 1) == 0)
        return;
    U32_AT(self, 0x38) |= 0x400;
    if (func_020186ec() != 1)
        return;
    if (S32_AT(data_020deebc, 0x18) == 8 || S32_AT(data_020deebc, 0x18) == 0x10)
        U32_AT(self, 0x38) &= ~0x400u;
}

/* 0x0203fc10: give a chest its model (actor kind `kind`)
 * @difftest $O=zero:0x200 $S=zero:0x60 @$S+0x44:32=$O $S int:0:3 stub=0x02053648 cases=5 */
void func_0203fc10(u8 *self, u32 kind)
{
    u8 *o;

    func_02053648(PTR_AT(self, 0x44), kind, self, 0, 0);
    func_02004490(data_020bf068);
    o = PTR_AT(self, 0x44);
    U32_AT(o, 0x18c) |= 2;
    o = PTR_AT(self, 0x44);
    S32_AT(o, 4) = 0;
    S32_AT(o, 8) = 0;
    S32_AT(o, 0xc) = 0;
    o[0x8c] = 1;
}

/* 0x0203fc78: a chest's per-frame update (its model's animation)
 * @difftest $O=zero:0x200 $S=zero:0x60 @$S+0x44:32=$O $S stub=0x02053460 cases=5 */
void func_0203fc78(u8 *self)
{
    u8 *o = PTR_AT(self, 0x44);

    func_02053460(o, func_020822c0(func_020062d0()));
}

/* 0x0203fc9c: delete every drop and empty the map
 * @difftest @020ebb54:32=0 @020ebb58:32=0 @020ebb5c:32=0x020ebb58 cases=3
 * @difftest $U=zero:0x10 @$U+4:32=0x020343a8 $V=zero:0x10 @$V+0:32=$U $N=zero:0x14 @$N+8:32=0x020ebb58 @$N+0xc:32=0x1234 @$N+0x10:32=$V @020ebb54:32=1 @020ebb58:32=$N @020ebb5c:32=$N stub=0x0207fe44 cases=3
 * @difftest $N=zero:0x14 @$N+8:32=0x020ebb58 @$N+0xc:32=0x1234 @020ebb54:32=1 @020ebb58:32=$N @020ebb5c:32=$N stub=0x0207fe44 cases=3 */
void func_0203fc9c(void)
{
    u8 *it = PTR_AT(data_020ebb54, 8);

    while (it != data_020ebb58) {
        u8 *d = PTR_AT(it, 0x10);
        if (d != NULL)
            Delete(d);
        func_02040230(&it);
    }
    func_0204029c(data_020ebb54);
}

/* 0x0203fd08: a chest goes: delete its drop
 * @difftest $U=zero:0x10 @$U+4:32=0x020343a8 $V=zero:0x10 @$V+0:32=$U $N=zero:0x14 @$N+8:32=0x020ebb58 @$N+0xc:32=0x1234 @$N+0x10:32=$V @020ebb54:32=1 @020ebb58:32=$N @020ebb5c:32=$N @020ebb60:32=0 pick:0x1234,0x999 stub=0x0207fe44 cases=10
 * @difftest @020ebb54:32=0 @020ebb58:32=0 @020ebb5c:32=0x020ebb58 @020ebb60:32=0 pick:0x1234,0x999 cases=5 */
void func_0203fd08(u8 *key)
{
    u8 *it;

    func_020401d4(&it, data_020ebb54, &key);
    if (it == data_020ebb58)
        return;
    if (PTR_AT(it, 0x10) != NULL)
        Delete(PTR_AT(it, 0x10));
    func_02040178(data_020ebb54, &key);
}

/* 0x0203fd7c: a chest's removal callback
 * @difftest @020ebb54:32=0 @020ebb58:32=0 @020ebb5c:32=0x020ebb58 @020ebb60:32=0 pick:0x1234,0x999 cases=5 */
void func_0203fd7c(u8 *key)
{
    func_0203fd08(key);
}

/* 0x0203fd88: create chest `id` (of map object list `k`)
 * @difftest int:0:4 int:0:2 cases=5 */
u8 *func_0203fd88(s32 id, s32 k)
{
    u8 *def = func_02008994((s8)id, k);
    u32 kind = U16_AT(def, 8);
    u8 *a, *o;
    s32 t[2], w, d;
    fx32 r;

    func_02040088();
    a = func_02005d5c(S32_AT(def, 0));
    S32_AT(a, 0xc) = 6;
    S32_AT(a, 8) = func_0200820c(kind, 7);
    U32_AT(a, 4) |= 8;
    U32_AT(a, 0x38) &= ~0x40000u;
    U32_AT(a, 0x110) |= 0x100;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x20u) | ((def[0x1f] & 1) << 5);
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~0x40u) | ((def[0x20] & 1) << 6);
    {
        void (*m)(void *, u32, void *) = VCALL(a, 0xc, void (*)(void *, u32, void *));
        m(a, 0, (void *)m);
    }
    PTR_AT(a, 0x98) = (void *)func_0203fd7c;
    PTR_AT(a, 0x94) = (void *)func_0203fc78;
    if (kind == 2)
        PTR_AT(a, 0x9c) = (void *)func_0203f780;
    else
        PTR_AT(a, 0x9c) = (void *)func_0203f854;
    func_0203fc10(a, S32_AT(a, 8));
    S32_AT(PTR_AT(a, 0x44), 0xa4) = S32_AT(KIND(a), 0x34);
    o = PTR_AT(a, 0x44);
    U32_AT(o, 0x18c) = (U32_AT(o, 0x18c) & ~1u) | ((s8)S32_AT(KIND(a), 0x30) & 1);
    w = S32_AT(KIND(a), 0x3c);
    d = S32_AT(KIND(a), 0x38);
    S32_AT(a, 0x10) = -w;
    S32_AT(a, 0x14) = -d;
    S32_AT(a, 0x18) = w;
    S32_AT(a, 0x1c) = d;
    U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~4u) | ((S32_AT(KIND(a), 0x40) > 0) << 2);
    {
        s32 sc = S32_AT(KIND(a), 0x2c);
        o = PTR_AT(a, 0x44);
        S32_AT(o, 0x1c) = sc;
        S32_AT(o, 0x20) = sc;
        S32_AT(o, 0x24) = sc;
        o[0x8c] = 1;
    }
    func_02003c74(a, SHL(S16_AT(def, 4), 16), SHL(S16_AT(def, 6), 16));
    S32_AT(a, 0x30) = S32_AT(a, 0x28);
    S32_AT(a, 0x34) = S32_AT(a, 0x2c);
    t[0] = S32_AT(a, 0x54);
    t[1] = S32_AT(a, 0x58);
    S32_AT(a, 0x24) = func_0201a0a0(t, a);
    S32_AT(a, 0x6c) = S32_AT(a, 0x24);
    S32_AT(a, 0x64) = S32_AT(a, 0x24);
    S32_AT(a, 0x50) = 0x2000;
    a[0x10d] = def[0x1d];
    r = func_02001ef4(a[0x10d]);
    o = PTR_AT(a, 0x44);
    {
        s32 x = S32_AT(o, 0x10), z = S32_AT(o, 0x18);
        S32_AT(o, 0x10) = x;
        S32_AT(o, 0x14) = r;
        S32_AT(o, 0x18) = z;
        o[0x8c] = 1;
    }
    PTR_AT(PTR_AT(PTR_AT(a, 0x44), 0xac), 4) = PTR_AT(data_020deebc, 4);
    func_02004490(data_020bf098);
    S32_AT(PTR_AT(PTR_AT(a, 0x44), 0xac), 0xc) = S32_AT(KIND(a), 0x20);
    func_02004490(data_020bf0d0);
    func_0203f518(a, def);
    return a;
}
