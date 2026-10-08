/*
 * SoundDS.cpp, part 1: direct sounds (loading a sample from the sound data
 * file, IMA ADPCM decoding), the software mixer's per-frame step and its
 * click removal, and direct-sound handles.
 * Also pausing, resuming and starting direct sounds.
 * ARM9 main, 0x02020b9c - 0x02021b50 (16 functions).
 *
 * A direct-sound handle is slot | id << 8 (-1: none); slots are the
 * 0x38-byte entries at data_020e26c0 (see Sound1.c), +0x08 u16 id.
 * File 0x91e holds the samples: a header whose +0x08 and +0x10 point to
 * offset tables of 0x24-byte sample records and of the sample data.
 * data_020e2314 holds 0x18-byte software channels: +0x00 samples (bit 31:
 * 16-bit, address >> 1), +0x04 position (16.16), +0x0c volume, +0x10 end,
 * +0x14 loop length. data_020e2b14 + 0x84/0x86 are the left/right offsets
 * faded out after a sound stops, +0x7c the free hardware channels.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct {
    u32 w[3];
} SePan;                             /* 12-byte argument passed by value */

extern const char data_020bde3c[];   /* "SoundDS.cpp" */
extern const u8 data_020b580c[];     /* IMA ADPCM index changes (s8) */
extern const u8 data_020b581c[];     /* IMA ADPCM step sizes (u16) */
extern u8 data_020e2314[];
extern u8 data_020e2614[];
extern u8 data_020e295c[];           /* slot 15 - 0xac */
extern u8 data_020e2a08[];           /* slot 15 */
extern u8 data_020e26c0[];
extern u8 data_020e26c8[];
extern u8 data_020e2ae4[];
extern u8 data_020e2b10[];
extern u8 data_020e2b14[];
extern u8 data_020e2b40[];
extern u8 data_020e2b70[];

void func_01ff8000(u32 n, u32 a, u32 b);   /* the mixer (ITCM, hand-written) */
void func_020803d0(void *file, u32 pos, u32 whence);   /* seek */
void func_02020780(u32 h);
void func_020208c8(u32 h);
void func_020209cc(u8 *w);
void func_020955a0(u32 mask, u32 a, u32 b, u32 c);   /* stop channels */
void func_02095610(u32 mask, u32 a, u32 b, u32 c);   /* start channels */
u32 func_020958f0(void);
void func_02095930(u32 v);
void func_02020afc(u8 *w, SePan pan);
void func_02021c1c(void *a, void *b, u32 len);
u32 func_02027810(void);

void func_02021048(s16 *buf, s32 n);
u32 func_0202112c(void);
void func_02021160(void **p);
u32 func_020211e8(u32 i);
void func_02021308(u32 ch);
void func_02021890(u32 h, s32 mode);
void func_02021204(u8 *dst, const u8 *src, u32 n, u32 pcm16);
u32 func_02021540(u32 h);

#define SLOT(h) (data_020e26c0 + ((h) & 0xff) * 0x38)

/* 0x02020b9c: load sample `idx` of the sound data file into direct sound
 * `w` and start it (on a hardware channel if `tryhw` and it fits) */
s32 func_02020b9c(u32 idx, u8 *w, s32 tryhw, SePan pan)
{
    u32 off, size, conv;
    u8 rec[0x24], hdr[0x28], file[0x44], *buf;
    s32 hw = 1;

    func_02080488(file, 0x91e);
    func_02080418(hdr, 0x14, 1, file);
    func_020803d0(file, U32_AT(hdr, 8) + idx * 4, 0);
    func_02080418(&off, 4, 1, file);
    func_020803d0(file, off, 0);
    func_02080418(rec, 0x24, 1, file);
    U16_AT(w, 8) = idx;
    w[0xb] &= ~0xfe;
    w[4] = rec[4];
    w[5] = rec[5];
    w[6] = rec[6];
    w[7] = rec[7];
    U32_AT(w, 0xc) = U32_AT(rec, 8);
    U32_AT(w, 0x10) = U32_AT(rec, 0xc);
    U32_AT(w, 0x14) = U32_AT(rec, 0x10);
    U32_AT(w, 0x18) = U32_AT(rec, 0x14);
    if (w[7] & 0x80)
        w[7] &= ~0x80;
    else
        w[7] = 0x20;
    if (tryhw == 0 || U16_AT(data_020e2b14, 0x7c) == 0) {
        hw = 0;
    } else if (w[5] & 0x10) {
        u32 m = (w[5] & 8) ? 7 : (w[5] & 2) ? 1 : 3;
        if ((U32_AT(w, 0x10) & m) || (U32_AT(w, 0x14) & m))
            hw = 0;
    }
    if (!hw) {
        w[0xb] &= ~1;
        w[0xa] = func_02027810();
        if (w[0xa] == 0xff)
            return 0;
    } else {
        w[0xb] = (w[0xb] & ~1) | 1;
        w[0xa] = func_0202112c();
        if (w[0xa] == 0xff)
            return 0;
    }
    func_020803d0(file, U32_AT(hdr, 0x10) + U16_AT(rec, 0) * 4, 0);
    func_02080418(&off, 4, 1, file);
    func_020803d0(file, off, 0);
    if (U32_AT(w, 0xc) == 0)
        return 0;
    conv = 0;
    PTR_AT(w, 0) = NULL;
    if (w[5] & 8) {
        u32 len = U32_AT(w, 0xc);
        size = (((len + 8) & ~7u) >> 1) + 4;
        if (!(w[0xb] & 1) || (u8)pan.w[0] != 7) {
            u32 n = len + 1;
            conv = 1;
            w[5] &= ~8;
            if (w[5] & 2)
                n <<= 1;
            PTR_AT(w, 0) = func_0207ff70(n, data_020bde3c, 0xadd);
            if (PTR_AT(w, 0) == NULL)
                return 0;
        }
    } else if (!(w[5] & 2)) {
        size = (U32_AT(w, 0xc) + 4) & ~3u;
    } else {
        size = ((U32_AT(w, 0xc) + 1) * 2 + 3) & ~3u;
    }
    buf = func_0207ff70(size, data_020bde3c, 0xaf4);
    if (buf == NULL) {
        if (PTR_AT(w, 0) != NULL)
            func_0207ff14(PTR_AT(w, 0), data_020bde3c, 0xafb);
        return 0;
    }
    func_02080418(buf, size, 1, file);
    if (conv == 1) {
        func_02021204(PTR_AT(w, 0), buf, U32_AT(w, 0xc) + 1, (w[5] & 2) ? 1 : 0);
        func_0207ff14(buf, data_020bde3c, 0xb0d);
    } else {
        PTR_AT(w, 0) = buf;
    }
    func_02020afc(w, pan);
    if (!(w[0xb] & 1))
        data_020e2b70[w[0xa]] = 5;
    else
        U16_AT(data_020e2b14, 0x7c) &= ~(1 << w[0xa]);
    func_02080458(file);
    return 1;
}

/* 0x02020fb0: mix the next `n` samples */
void func_02020fb0(u32 n)
{
    u32 base = U32_AT(data_020e2614, 0), pos = U32_AT(data_020e2614, 4);
    s16 *out = (s16 *)(base + pos * 2);

    func_01ff8000(n, pos, base);
    if (U32_AT(data_020e2614, 0x584) != 0)
        func_02021048(out, n >= 0x20 ? 0x20 : n);
    if (data_020e2614[0x4d0] == 7)
        return;
    func_02021c1c(data_020e2ae4, data_020e2b10, n);
    func_02021c1c(data_020e2ae4, data_020e2b40, n);
}

static s32 Clamp16(s32 v)
{
    if (v > 0x7fff)
        return 0x7fff;
    if (v < -0x8000)
        return -0x8000;
    return v;
}

/* 0x02021048: fade the left/right offsets out over `n` mixed samples
 * @difftest ptr:0x1080:2 int:0:0x21 @020e2b98:16=s16 @020e2b9a:16=s16 */
void func_02021048(s16 *buf, s32 n)
{
    u8 *m = data_020e2b14;
    s32 dl = S16_AT(m, 0x84) / 32, dr = S16_AT(m, 0x86) / 32, i;

    for (i = n; i > 0; i--) {
        s16 l = S16_AT(m, 0x84), r = S16_AT(m, 0x86);
        S16_AT(m, 0x84) = l - dl;
        S16_AT(m, 0x86) = r - dr;
        buf[0] = Clamp16(buf[0] + S16_AT(m, 0x84));
        buf[0x800] = Clamp16(buf[0x800] + S16_AT(m, 0x86));
        buf++;
    }
    if (n == 0x20)
        U32_AT(data_020e2614, 0x584) = 0;
}

/* 0x0202112c: highest free hardware channel (0xff: none)
 * @difftest @020e2b90:16=pick:0,1,0x8000,0x1234,0x700 */
u32 func_0202112c(void)
{
    u32 mask = U16_AT(data_020e2b14, 0x7c);
    s32 i;
    for (i = 15; i >= 0; i--) {
        if (mask & (1u << i))
            return i & 0xff;
    }
    return 0xff;
}

/* 0x02021160: free the samples of a direct sound
 * @difftest zero:4 */
void func_02021160(void **p)
{
    if (*p == NULL)
        return;
    func_0207ff14(*p, data_020bde3c, 0x99f);
    *p = NULL;
}

/* 0x0202119c: fill the noise table
 * @difftest $N=ptr:0x168 @020e261c:32=$N */
void func_0202119c(void)
{
    u32 x = 0x69946cf3;
    s32 i;
    for (i = 0; i < 0x168; i++) {
        x = x * 0x10dcd + 0x12d687;
        ((u8 *)PTR_AT(data_020e2614, 8))[i] = x & 0x3f;
    }
}

/* 0x020211e8: handle of direct-sound slot `i`
 * @difftest int:0:16 */
u32 func_020211e8(u32 i) { return i | (U16_AT(data_020e26c8, i * 0x38) << 8); }

/* 0x02021204: decode `n` IMA ADPCM samples (to 8 or 16 bits)
 * @difftest ptr:0x400:2 bytes:0x104:0 int:0:0x200 pick:0,1 */
void func_02021204(u8 *dst, const u8 *src, u32 n, u32 pcm16)
{
    s32 pred = S16_AT(src, 0), idx = src[2] & 0x7f, step = ((const u16 *)data_020b581c)[idx];
    const u32 *p = (const u32 *)(src + 4);
    u32 i, word = 0;

    if (n == 0)
        return;
    for (i = 0; i < n; i++) {
        u32 nib;
        s32 d;
        if ((i & 7) == 0)
            word = *p++;
        nib = word & 0xf;
        d = step >> 3;
        if (nib & 1)
            d += step >> 2;
        if (nib & 2)
            d += step >> 1;
        if (nib & 4)
            d += step;
        if (nib & 8)
            d = -d;
        pred += d;
        word >>= 4;
        if (pred >= 0x8000)
            pred = 0x7fff;
        else if (pred < -0x8000)
            pred = -0x8000;
        if (pcm16 != 0)
            ((s16 *)dst)[i] = pred;
        else
            dst[i] = pred >> 8;
        idx += ((const s8 *)data_020b580c)[nib];
        if (idx < 0)
            idx = 0;
        else if (idx > 0x58)
            idx = 0x58;
        step = ((const u16 *)data_020b581c)[idx];
    }
}

/* 0x02021308: take the last sample of a stopped software channel into the
 * left/right offsets, so it fades out instead of clicking
 * @difftest $B=ptr:0x200:2 @020e2314:32=$B @020e2318:32=int:0:0xff0000 @020e2320:16=u16 @020e2b98:16=s16 @020e2b9a:16=s16 pick:0 */
void func_02021308(u32 ch)
{
    u8 *c = data_020e2314 + ch * 0x18;
    u32 data = U32_AT(c, 0);
    s32 s, v, t;

    if (data == 0)
        return;
    if (!(data & 0x80000000)) {
        v = U16_AT(c, 0xc);
        s = ((const s8 *)data)[S32_AT(c, 4) >> 16];
    } else {
        v = (u32)(U16_AT(c, 0xc) << 8) >> 16;
        s = ((const s16 *)(data << 1))[S32_AT(c, 4) >> 16];
    }
    v = s * v;
    t = (S16_AT(data_020e2b14, 0x84) + v) >> 7;
    if (t < -0x8000)
        S16_AT(data_020e2b14, 0x84) = -0x8000;
    else if (t >= 0x8000)
        S16_AT(data_020e2b14, 0x84) = 0x7fff;
    else
        S16_AT(data_020e2b14, 0x84) = t;
    t = (S16_AT(data_020e2b14, 0x86) + v) >> 7;
    if (t < -0x8000)
        S16_AT(data_020e2b14, 0x86) = -0x8000;
    else if (t >= 0x8000)
        S16_AT(data_020e2b14, 0x86) = 0x7fff;
    else
        S16_AT(data_020e2b14, 0x86) = t;
    U32_AT(c, 0) = 0;
}

/* 0x020213e0: set the sample rate of a direct sound (negative: backwards) */
void func_020213e0(u32 h, s32 rate)
{
    u8 *c;

    if (!func_02021540(h))
        return;
    c = SLOT(h);
    if (!(c[0xb] & 1) && (U32_AT(c, 0x18) & 0x80000000) != ((u32)rate & 0x80000000)) {
        u8 *s = data_020e2314 + c[0xa] * 0x18;
        u32 a, b;
        if (rate < 0) {
            b = 0;
            if (!(c[5] & 0x10)) {
                a = U32_AT(c, 0xc);
            } else {
                S32_AT(s, 0x14) = U32_AT(c, 0x10) - U32_AT(c, 0x14);
                a = U32_AT(c, 0x14);
            }
        } else {
            a = 0;
            if (!(c[5] & 0x10)) {
                b = U32_AT(c, 0xc);
            } else {
                S32_AT(s, 0x14) = U32_AT(c, 0x14) - U32_AT(c, 0x10);
                b = U32_AT(c, 0x14);
            }
        }
        if (c[5] & 2) {
            U32_AT(s, 0) = ((U32_AT(c, 0) >> 1) | 0x80000000) + a;
            U32_AT(s, 0x10) = ((U32_AT(c, 0) >> 1) | 0x80000000) + b;
        } else {
            U32_AT(s, 0) = U32_AT(c, 0) + a;
            U32_AT(s, 0x10) = U32_AT(c, 0) + b;
        }
    }
    S32_AT(c, 0x18) = rate;
    func_02020780(h);
}

/* 0x02021514: the sample id of a direct sound (0: none)
 * @difftest pick:-1,0,1,0x102,0x305 */
u32 func_02021514(u32 h)
{
    if (h == (u32)-1)
        return 0;
    return U16_AT(SLOT(h), 8);
}

/* 0x02021540: whether a direct-sound handle is still valid
 * @difftest pick:-1,0,1,0x102,0x305,0x10f */
u32 func_02021540(u32 h)
{
    u8 *c;
    if (h == (u32)-1)
        return 0;
    c = SLOT(h);
    if (U32_AT(c, 0) == 0)
        return 0;
    if (U16_AT(c, 8) != (h >> 8))
        return 0;
    if (((u8)c[0xb] >> 1) <= 1)
        return 1;
    return 0;
}

/* 0x0202159c: resume the paused direct sounds
 * @difftest $W=ptr:0x100:4 @020e2a08:32=$W @020e2a13:8=pick:2,3,0,4 @020e2a0d:8=pick:0,0x10,0x12,0x18 @020e2a12:8=int:0:16 @020e2b90:16=pick:0,0x8000,0x1 cases=60 */
void func_0202159c(void)
{
    u8 *w = data_020e2a08;
    s32 i;

    for (i = 15; i >= 0; i--, w -= 0x38) {
        if (U32_AT(w, 0) == 0)
            continue;
        if (((u8)w[0xb] >> 1) != 1)
            continue;
        w[0xb] &= ~0xfe;
        if ((w[0xb] & 1) == 1) {
            w[0xa] = func_0202112c();
            if (w[0xa] != 0xff) {
                U16_AT(data_020e2b14, 0x7c) &= ~(1 << w[0xa]);
                func_02095930(2);
                func_020209cc(w);
                func_02095610(1 << w[0xa], 0, 0, 0);
                U32_AT(w, 0x1c) = func_020958f0();
            } else {
                func_02021160((void **)w);
            }
        } else {
            w[0xa] = func_02027810();
            if (w[0xa] != 0xff) {
                u8 *s;
                u32 k;
                data_020e2614[0x55c + w[0xa]] = 5;
                s = data_020e2314 + w[0xa] * 0x18;
                for (k = 0; k < 0x18; k += 4)
                    U32_AT(s, k) = U32_AT(w, 0x20 + k);
                U32_AT(w, 0x20) = 0;
            } else {
                func_02021160((void **)w);
                U32_AT(w, 0x20) = 0;
            }
        }
    }
}

/* 0x02021708: pause the playing direct sounds
 * @difftest $W=ptr:0x100:4 @020e2a08:32=$W @020e2a13:8=pick:0,1,2 @020e2a0d:8=pick:0,0x10,0x12 @020e2a12:8=int:0:16 cases=60 */
void func_02021708(void)
{
    u8 *w = data_020e2a08, *p;
    u32 mask = 0, t;
    s32 i;

    for (i = 15; i >= 0; i--, w -= 0x38) {
        if (U32_AT(w, 0) == 0)
            continue;
        if ((u8)w[0xb] >> 1)
            continue;
        w[0xb] = (w[0xb] & ~0xfe) | 2;
        if ((w[0xb] & 1) == 1) {
            if (w[5] & 0x10) {
                mask |= 1 << w[0xa];
                U16_AT(data_020e2b14, 0x7c) |= 1 << w[0xa];
            } else {
                func_02021890(func_020211e8(i), 0);
            }
        } else {
            u8 *s = data_020e2314 + w[0xa] * 0x18;
            u32 k;
            for (k = 0; k < 0x18; k += 4)
                U32_AT(w, 0x20 + k) = U32_AT(s, k);
            U32_AT(data_020e2314, w[0xa] * 0x18) = 0;
            data_020e2614[0x55c + w[0xa]] = 3;
        }
    }
    if (mask == 0)
        return;
    func_02095930(1);
    func_020955a0(mask, 0, 0, 0);
    t = func_020958f0();
    p = data_020e295c;
    for (i = 15; i >= 0; i--, p -= 0x38) {
        if (mask & (1u << i))
            U32_AT(p, 0xc8) = t;
    }
}

/* 0x02021890: stop a direct sound (`mode` 1: pause) */
void func_02021890(u32 h, s32 mode)
{
    u8 *w = SLOT(h);

    if (func_02021540(h)) {
        u8 f = w[0xb];
        if ((f & 1) == 1) {
            if (((u8)f >> 1) == 0) {
                func_02095930(1);
                func_020955a0(1 << w[0xa], 0, 0, 0);
                U32_AT(w, 0x1c) = func_020958f0();
            }
            if (mode == 0)
                w[0xb] = (w[0xb] & ~0xfe) | 4;
            else if (mode == 1)
                w[0xb] = (w[0xb] & ~0xfe) | 6;
        } else {
            func_02021308(w[0xa]);
            data_020e2b70[w[0xa]] = 3;
            func_02021160((void **)w);
        }
    } else if (mode == 1) {
        if (((u8)w[0xb] >> 1) == 0)
            w[0xb] = (w[0xb] & ~0xfe) | 2;
    }
}

/* 0x020219d0: play sample `id` in a free direct-sound slot; its handle or -1 */
u32 func_020219d0(u32 id, s32 tryhw, SePan pan)
{
    u8 *p = data_020e295c, *w;
    u32 slot = 0xff, h;
    s32 i;

    for (i = 15; i >= 0; i--, p -= 0x38) {
        if (U32_AT(p, 0xac) == 0) {
            slot = i & 0xff;
            break;
        }
    }
    if (slot == 0xff)
        return (u32)-1;
    w = data_020e26c0 + slot * 0x38;
    if (!func_02020b9c(id, w, tryhw, pan))
        return (u32)-1;
    h = slot | (id << 8);
    if ((w[0xb] & 1) == 1) {
        func_020209cc(w);
        func_02095610(1 << w[0xa], 0, 0, 0);
        U32_AT(w, 0x1c) = func_020958f0();
    } else {
        u8 *s = data_020e2314 + w[0xa] * 0x18;
        if (w[5] & 2)
            U32_AT(s, 0) = (U32_AT(w, 0) >> 1) | 0x80000000;
        else
            U32_AT(s, 0) = U32_AT(w, 0);
        U32_AT(s, 4) = 0;
        if (!(w[5] & 0x10)) {
            U32_AT(s, 0x10) = U32_AT(s, 0) + U32_AT(w, 0xc);
            U32_AT(s, 0x14) = 0;
        } else {
            U32_AT(s, 0x10) = U32_AT(s, 0) + U32_AT(w, 0x14);
            U32_AT(s, 0x14) = U32_AT(w, 0x14) - U32_AT(w, 0x10);
        }
        func_020208c8(h);
        func_02020780(h);
    }
    return h;
}
