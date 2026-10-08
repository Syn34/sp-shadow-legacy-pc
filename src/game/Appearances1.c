/*
 * Sprite affine helpers and the level "appearances" tables, part 1.
 * ARM9 main, 0x02007efc - 0x02008b04 (26 functions).
 *
 * The first four functions (OBJ scaling/rotation) still use actor fields and
 * may belong to the end of Actor.cpp; the rest builds and reads the tables of
 * level objects ("appearances", Appearances.cpp is referenced a little later
 * at 0x02009324). The tables are relocatable blobs of u16 offsets:
 *   table + U16(table, 2 + 2*i) -> entry i
 * data_020d9f88 holds flags of the current level (bit 0: an extra group 0 is
 * prepended), data_020d9f8c/90/94/98 point to the built tables.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u16 data_020d9f88;
extern u8 *data_020d9f8c;
extern u8 *data_020d9f90;
extern u8 *data_020d9f94;
extern u8 *data_020d9f98;
extern u8 data_020c5b8c[];
extern u8 *data_020c3b0c;            /* scene nodes, 0x48 bytes each */
extern u8 data_020e08a0[], data_020e08a2[], data_020e08a4[];   /* OBJ affine params, per screen */
extern const u8 data_020aac08[];     /* record length by record type */
extern const u8 data_020aac70[], data_020aac71[];   /* OBJ {width, height} by shape, size */
extern const char data_020bcbfc[];
extern void *(*data_020bcc1c[])(u32 group, u32 index);   /* spawn function by object type */

s32 func_02019620(void);
void *func_02009394(void);
void func_0205f6e4(void *obj, u32 v);
u32 func_0201a6fc(void);
void func_0201a660(u32 idx);
void func_0201a790(u32 *attr, u32 idx);
u32 func_0201a7b0(u32 *attr);
void func_020506a0(void *actor, s32 group, s32 index);
void *func_02007d9c(void *actor);
void *func_02007d78(void *actor);

/* this file's own functions, called before their definition */
void func_02007f68(void *a, s32 sx, s32 sy, s32 rot);
u32 func_020081d8(void);
s32 func_0200820c(u32 id, s32 kind);
u32 func_02008278(u32 v);
u32 func_020082b4(u32 v);
u8 *func_020082d0(u8 *tbl, u8 *dst, u32 i);
u8 *func_02008454(u8 *tbl, u8 *dst, u32 i);
void func_02008614(const u8 *src, u8 *dst, u32 n);
u8 *func_02008640(u8 *tbl, u8 *dst, u32 i);

#define ENTRY(t, i) ((t) + U16_AT(t, 2 + (i) * 2))

/* 0x02007efc: step the actor's scale animation
 * @difftest $A=ptr:0x114:4 @$A+0xa8:32=pick:0x100,0x300 $A */
void func_02007efc(void *a)
{
    U16_AT(a, 0xda)--;
    S32_AT(a, 0xc8) += S32_AT(a, 0xd0);
    S32_AT(a, 0xcc) += S32_AT(a, 0xd4);
    func_02007f68(a, SHL(S32_AT(a, 0xc8), 8) >> 16, SHL(S32_AT(a, 0xcc), 8) >> 16, S16_AT(a, 0xd8));
}

/* 0x02007f68: scale/rotate the actor's sprite with an OBJ affine slot
 * @difftest $A=ptr:0x114:4 @$A+0xa8:32=pick:0x100,0x300 $A int:0x100:0x30000 int:0x100:0x30000 s16 */
void func_02007f68(void *a, s32 sx, s32 sy, s32 rot)
{
    u32 idx, off;
    s32 ix, iy;

    S32_AT(a, 0xc8) = sx;
    S32_AT(a, 0xcc) = sy;
    S16_AT(a, 0xd8) = rot;
    idx = BITS(U32_AT(a, 0xa8), 8, 2);
    if (idx == 1 || idx == 3) {
        idx = func_0201a7b0((u32 *)((u8 *)a + 0xa8));
    } else {
        idx = func_0201a6fc();
        func_0201a790((u32 *)((u8 *)a + 0xa8), idx);
    }
    ix = func_0208f070(0x1000, sx >> 4);
    iy = func_0208f070(0x1000, sy >> 4);
    if (ix == 0)
        ix = 1;
    if (iy == 0)
        iy = 1;
    off = data_020df0fc * 0x13ec + idx * 6;
    S16_AT(data_020e08a0, off) = ix >> 4;
    S16_AT(data_020e08a2, off) = iy >> 4;
    S16_AT(data_020e08a4, off) = rot;
    U32_AT(a, 0xa8) = (U32_AT(a, 0xa8) & ~0x300u) | 0x100;
    if (sx > 0x10000 || sy > 0x10000)
        U32_AT(a, 0xa8) = (U32_AT(a, 0xa8) & ~0x300u) | 0x300;
}

/* 0x0200809c: release the actor's OBJ affine slot */
void func_0200809c(void *a)
{
    u32 mode = BITS(U32_AT(a, 0xa8), 8, 2), idx;
    if (mode != 1 && mode != 3)
        return;
    U32_AT(a, 0xa8) &= ~0x300u;
    idx = func_0201a7b0((u32 *)((u8 *)a + 0xa8));
    func_0201a790((u32 *)((u8 *)a + 0xa8), 0);
    func_0201a660(idx);
}

/* 0x02008104: OBJ shape/size -> width, height in pixels
 * @difftest int:0:3 int:0:4 ptr:8:4 */
void func_02008104(u32 shape, u32 size, s32 *dim)
{
    dim[0] = data_020aac70[shape * 16 + size * 4];
    dim[1] = data_020aac71[shape * 16 + size * 4];
}

/* 0x02008130: an object of the level was removed: play its sound */
void func_02008130(s32 group, s32 index)
{
    u8 *e = ENTRY(data_020d9f90, group);
    e = ENTRY(e, index);
    if (S32_AT(e, 8) == 4 && func_02019620() == 0) {
        func_0205f6e4(func_02009394(), U8_AT(e, 0x28));
    } else if (S32_AT(e, 8) == 5 && func_02019620() == 0) {
        s32 k = func_0200820c(U16_AT(e, 0x10), 2);
        if (k == 10 || k == 11 || k == 0x46)
            func_0205f6e4(func_02009394(), U8_AT(e, 0x15));
    }
    U32_AT(e, 0) = 0;
}

/* 0x020081d8: number of object groups
 * @difftest */
u32 func_020081d8(void) { return U16_AT(data_020d9f90, 0); }

/* 0x020081ec: number of objects in a group
 * @difftest int:0:2 */
u32 func_020081ec(s32 group)
{
    u8 *t = data_020d9f90;
    return U16_AT(t, U16_AT(t + (s8)group * 2, 2));
}

/* 0x0200820c: index of the id-th scene node of kind `kind` (0: none)
 * @difftest int:0:8 int:0:8 */
s32 func_0200820c(u32 id, s32 kind)
{
    u32 n = U32_AT(data_020c5b8c, 0x884), cnt = 0, i = 2;
    if (n > 2) {
        u8 *p = data_020c3b0c + 0x90;
        do {
            if (kind == S32_AT(p, 0x14)) {
                if (id == cnt)
                    return i;
                cnt++;
            }
            i++;
            p += 0x48;
        } while (i < n);
    }
    return 0;
}

/* 0x02008278: round up to a multiple of 4 (u16)
 * @difftest u32 */
u32 func_02008278(u32 v)
{
    u32 r = (u16)(v & 3), add = r;
    if (r == 1)
        add = 3;
    else if (r == 2)
        add = 2;
    else if (r == 3)
        add = 1;
    return (u16)(v + add);
}

/* 0x020082b4: round up to even, times 2 (u16)
 * @difftest u32 */
u32 func_020082b4(u32 v)
{
    if (v & 1)
        v = (u16)(v + 1);
    return (u16)(v << 1);
}

/* 0x020082d0: copy the variable-length records of entry i (up to the
 * terminating type-0 record); returns the end of the copy
 * @difftest $T=ptr:0x80:4 @$T+2:16=4 @$T+4:32=pick:0,1,2 @$T+8:32=0 $T ptr:0x80:4 pick:0 cases=100 */
u8 *func_020082d0(u8 *tbl, u8 *dst, u32 i)
{
    u8 *p = ENTRY(tbl, i);
    u8 *start;
    do {
        u32 len, k = 0;
        start = p;
        len = data_020aac08[U32_AT(p, 0)];
        if ((s32)len > 0) {
            do {
                *dst++ = *p++;
                k = (k + 1) & 0xff;
            } while ((s32)k < (s32)len);
        }
    } while (U32_AT(start, 0) != 0);
    return dst;
}

/* 0x0200833c: build table 94 from a list of record entries; returns the end */
u8 *func_0200833c(u8 *src, u8 *tbl, u8 *dst)
{
    u32 n = U16_AT(tbl, U16_AT(tbl, 2)) & 0xff, sz, i;
    if (data_020d9f88 & 1)
        U16_AT(data_020d9f94, 0) = n + 1;
    else
        U16_AT(data_020d9f94, 0) = n;
    sz = func_020082b4((u16)(U16_AT(data_020d9f94, 0) + 1));
    dst += sz;
    if (data_020d9f88 & 1) {
        U16_AT(data_020d9f94, 2) = sz;
        dst = func_020082d0(src + U16_AT(src, 2), dst, 0);
    }
    tbl += U16_AT(tbl, 2);
    for (i = 0; (s32)i < (s32)n; i++) {
        u8 *h = data_020d9f94;
        u32 off = (u16)(dst - h);
        if (data_020d9f88 & 1)
            U16_AT(h, i * 2 + 4) = off;
        else
            U16_AT(h, i * 2 + 2) = off;
        dst = func_020082d0(tbl, dst, i & 0xff);
    }
    return dst;
}

/* 0x02008454: copy entry i: count + count * {s16 x, s16 y, u8 a, b, c, d}
 * @difftest $T=ptr:0x80:4 @$T+2:16=4 @$T+4:32=int:0:8 $T ptr:0x80:4 pick:0 cases=100 */
u8 *func_02008454(u8 *tbl, u8 *dst, u32 i)
{
    u8 *e = ENTRY(tbl, i);
    u32 n = U32_AT(e, 0) & 0xff, k = 0;
    U32_AT(dst, 0) = n;
    if (n != 0) {
        do {
            u8 *s = e + k * 8, *d = dst + k * 8;
            S16_AT(d, 4) = S16_AT(s, 4);
            S16_AT(d, 6) = S16_AT(s, 6);
            U8_AT(d, 8) = U8_AT(s, 8);
            U8_AT(d, 9) = U8_AT(s, 9);
            U8_AT(d, 10) = U8_AT(s, 10);
            U8_AT(d, 11) = U8_AT(s, 11);
            k = (k + 1) & 0xff;
        } while (k < n);
    }
    return dst + (u16)(n * 8 + 4);
}

/* 0x020084e4: build table 8c (paths) from two sources; returns the end */
u8 *func_020084e4(u8 *a, u8 *b, u8 *dst)
{
    u32 flag = data_020d9f88 & 1, base = 0, total, sz, skip, k, an;
    u8 *ap = NULL, *bp;
    u32 boff = U16_AT(b, 0), bn;

    if (flag) {
        u32 aoff = U16_AT(a, 0);
        ap = a + aoff;
        base = U16_AT(a, aoff) & 0xff;
    }
    bn = U16_AT(b, boff);
    bp = b + boff;
    total = (base + bn) & 0xff;
    U16_AT(data_020d9f8c, 0) = total;
    sz = func_020082b4((u16)(total + 1));
    dst += sz;
    if (data_020d9f88 & 1) {
        an = U16_AT(ap, 0);
        for (k = 0; (s32)k < (s32)an;) {
            u8 *h = data_020d9f8c;
            U16_AT(h, k * 2 + 2) = (u16)(dst - h);
            dst = func_02008454(ap, dst, k);
            k = (k + 1) & 0xff;
            an = U16_AT(ap, 0);
        }
        skip = an & 0xff;
    } else {
        skip = 0;
    }
    for (k = 0; (s32)k < (s32)U16_AT(bp, 0);) {
        u8 *h = data_020d9f8c;
        U16_AT(h, (k + skip) * 2 + 2) = (u16)(dst - h);
        dst = func_02008454(bp, dst, k);
        k = (k + 1) & 0xff;
    }
    return dst;
}

/* 0x02008614: copy n bytes to dst + 8
 * @difftest ptr:0x100 ptr:0x110 int:0:256 */
void func_02008614(const u8 *src, u8 *dst, u32 n)
{
    u32 k = 0;
    dst += 8;
    if (n == 0)
        return;
    do {
        *dst++ = *src++;
        k = (k + 1) & 0xff;
    } while (k < n);
}

/* 0x02008640: copy entry i: n byte strings with an offset table
 * @difftest $T=ptr:0x200:4 @$T+2:16=4 @$T+4:16=int:0:6 $T ptr:0x800:4 pick:0 cases=100 */
u8 *func_02008640(u8 *tbl, u8 *dst, u32 i)
{
    u8 *e = ENTRY(tbl, i), *src, *p = dst;
    u32 n = U16_AT(e, 0) & 0xff, step, k;

    U16_AT(dst, 0) = n;
    src = e + func_02008278((u16)(n + 2));
    step = func_020082b4((u16)(n + 1));
    for (k = 0; k < n; k = (k + 1) & 0xff) {
        u32 len;
        p += step;
        U16_AT(dst, k * 2 + 2) = (u16)(p - dst);
        len = e[k + 2];
        func_02008614(src, p, len & 0xff);
        src += len;
        step = (u16)(len + 8);
    }
    return p + step;
}

/* 0x020086f0: build table 90 (object groups); returns the end */
u8 *func_020086f0(u8 *a, u8 *b, u8 *dst)
{
    u32 n, sz, k;
    data_020d9f88 = U16_AT(b, 4);
    n = U16_AT(b, 8) & 0xff;
    if (data_020d9f88 & 1)
        U16_AT(data_020d9f90, 0) = n + 1;
    else
        U16_AT(data_020d9f90, 0) = n;
    sz = func_020082b4((u16)(func_020081d8() + 1));
    dst += sz;
    if (data_020d9f88 & 1) {
        U16_AT(data_020d9f90, 2) = sz;
        dst = func_02008640(a + 8, dst, 0);
    }
    b += 8;
    for (k = 0; k < n; k = (k + 1) & 0xff) {
        u8 *h = data_020d9f90;
        u32 off = (u16)(dst - h);
        if (data_020d9f88 & 1)
            U16_AT(h, (k + 1) * 2 + 2) = off;
        else
            U16_AT(h, k * 2 + 2) = off;
        dst = func_02008640(b, dst, k);
    }
    return dst;
}

/* 0x02008804: build table 98 from a list of {s16, s16, u8 x4} records; returns its end
 * @difftest $S=ptr:0x100:4 @$S+2:16=int:0:16 $S */
u8 *func_02008804(u8 *src)
{
    u8 *h = data_020d9f98;
    u32 k = 0, n0;
    U16_AT(h, 2) = U16_AT(src, 2);
    h = data_020d9f98;
    if (U16_AT(h, 2) != 0) {
        do {
            u8 *s = src + k * 8, *d = h + 4 + k * 8;
            S16_AT(d, 0) = S16_AT(s, 4);
            S16_AT(d, 2) = S16_AT(s, 6);
            U8_AT(d, 4) = U8_AT(s, 8);
            U8_AT(d, 5) = U8_AT(s, 9);
            U8_AT(d, 6) = U8_AT(s, 10);
            U8_AT(d, 7) = U8_AT(s, 11);
            k = (u16)(k + 1);
            h = data_020d9f98;
        } while (k < U16_AT(h, 2));
    }
    n0 = U16_AT(src, 0);
    U16_AT(h, 0) = n0;
    return data_020d9f98 + n0;
}

/* 0x020088a8: play the sound of point k of path i */
void func_020088a8(s32 i, s32 k)
{
    u8 *t = data_020d9f8c;
    void *snd = func_02009394();
    func_0205f6e4(snd, U8_AT(ENTRY(t, i) + k * 8, 8));
}

/* 0x020088e4: point k of path i, as fx32 (pixels << 16)
 * @difftest ptr:8:4 int:0:2 int:0:2 */
void func_020088e4(s32 *out, s32 i, s32 k)
{
    u8 *e;
    out[0] = 0;
    out[1] = 0;
    e = ENTRY(data_020d9f8c, i) + 4;
    out[0] = SHL(S16_AT(e, k * 8), 16);
    out[1] = SHL(S16_AT(e + k * 8, 2), 16);
}

/* 0x02008930: number of points of path i
 * @difftest int:0:2 */
u32 func_02008930(s32 i)
{
    return U32_AT(ENTRY(data_020d9f8c, i), 0) & 0xff;
}

/* 0x02008950: the actor spawned for object k of group i (group 0, k >= 255: Spyro)
 * @difftest int:0:2 int:0:3
 * @difftest pick:0 int:250:300 */
void *func_02008950(s32 i, s32 k)
{
    u8 *e;
    if (i == 0 && k >= 0xff)
        return (void *)data_020deebc[2];
    e = ENTRY(data_020d9f90, i);
    return PTR_AT(e, U16_AT(e, k * 2 + 2));
}

/* 0x02008994: data of object k of group i
 * @difftest int:0:2 int:0:3 */
u8 *func_02008994(s32 i, s32 k)
{
    u8 *e = ENTRY(data_020d9f90, i);
    return e + U16_AT(e, k * 2 + 2) + 8;
}

/* 0x020089c0: number of objects in group i
 * @difftest int:0:2 */
u32 func_020089c0(s32 i)
{
    u8 *t = data_020d9f90;
    return U16_AT(t, U16_AT(t, i * 2 + 2));
}

/* 0x020089dc: is i a group that is always loaded
 * @difftest int:-1:4 */
s32 func_020089dc(s32 i)
{
    if (data_020d9f88 & 1)
        return (s8)(i == 1 || i == 0);
    return (s8)(i == 0);
}

/* 0x02008a30: spawn the actors of object group i
 * @difftest int:0:3 cases=10 */
void func_02008a30(s32 i)
{
    u32 proto[0x114 / 4];
    u8 *e;
    s32 n, k;

    func_02007d9c(proto);
    e = ENTRY(data_020d9f90, i);
    n = U16_AT(e, 0) & 0xff;
    for (k = 0; k < n; k = (s16)(k + 1)) {
        u8 *rec = e + U16_AT(e, k * 2 + 2);
        if (U32_AT(rec, 0) == 0) {
            u32 type = (u16)U32_AT(rec, 8);
            func_02004490(data_020bcbfc, type);
            PTR_AT(rec, 0) = data_020bcc1c[type](i & 0xff, k & 0xff);
            if (PTR_AT(rec, 0) != NULL)
                func_020506a0(PTR_AT(rec, 0), i, k);
        }
    }
    func_02007d78(proto);
}
