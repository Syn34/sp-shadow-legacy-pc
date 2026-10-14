/*
 * Particle effect data: a std::vector of frames, each a std::vector of
 * 0x20-byte particle keys {particle, +4, x, y, z, angle, scale x, scale y},
 * loaded from a file of {frame count, then per frame: key count, keys}.
 * ARM9 main, 0x02051e08 - 0x02052c14 (22 functions).
 *
 * Vectors are {data, size, capacity}; growing goes through a split buffer
 * {data, size, capacity, &vector capacity, offset}.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb38[];           /* the exceptions' type */
extern const char data_020c0580[], data_020c059c[], data_020c05b4[], data_020c05cc[],
    data_020c05e8[];                 /* messages, source file name */

typedef struct {
    u32 w[8];
} Key;

typedef struct {
    u8 *data;
    u32 size;
    u32 cap;
    u32 *capref;
    u32 off;
} SplitBuf;

void *func_0207fe74(u32 size);
void func_0207fe44(void *p);
void func_0209e90c(void *e, const char *msg);
void func_0209dcb8(void);
void func_02006ec8(void);
void func_0207fe28(const char *file, s32 line);
void func_0207fe24(void);
void func_02080488(void *file, u32 id);
void func_02080458(void *file);
void *func_0209e7d8(void *dst, s32 c, u32 n);
void *func_0209e83c(void *dst, const void *src, u32 n);
s64 func_020a4c50(s64 a, s64 b);

void func_0205221c(u8 *v, u32 n, u32 tag);
void func_02052230(u8 *v, u32 n, const Key *key, u32 tag);
u8 *func_02052338(u8 *e);
void func_02052388(u8 *e, u32 n, u32 tag);
void func_020523e0(u8 *e, u32 n, u32 tag);
u8 *func_0205247c(u8 *v);
void func_020524cc(u8 *v, u32 n, const Key *key);
u32 func_020526c0(u8 *v, u32 n);
void func_02052798(u8 *e, u32 n);
u32 func_02052904(u8 *e, u32 n);
SplitBuf *func_020529e0(SplitBuf *b);
SplitBuf *func_02052a00(SplitBuf *b);
SplitBuf *func_02052a58(SplitBuf *b);
void func_02052aa8(SplitBuf *b, u32 n, u32 tag);
SplitBuf *func_02052b04(SplitBuf *b);
void func_02052b54(SplitBuf *b, u32 n, u32 tag);
void func_02052ba8(SplitBuf *b, u32 n, u32 tag);
void func_02052c00(SplitBuf *b, u32 n, u32 tag);

static void CopyKey(u32 *d, const u32 *s)
{
    d[0] = s[0];
    d[1] = s[1];
    d[2] = s[2];
    d[3] = s[3];
    d[4] = s[4];
    d[5] = s[5];
    d[6] = s[6];
    d[7] = s[7];
}

/* 0x02051e08: read the `n` keys of frame `k` from `file`
 * @difftest $R=zero:0x80 $E=zero:0xc @$E:32=$R @$R:32=$R+0x20 $E ptr:0x48:4 pick:0 int:0:3 stub=0x02080418:1 cases=10
 * @difftest $R=zero:0x80 $E=zero:0xc @$E:32=$R @$R:32=$R+0x20 $E ptr:0x48:4 pick:0 int:0:3 stub=0x02080418:0 cases=5 */
void func_02051e08(u8 *e, void *file, u32 k, s32 n)
{
    u32 b[8];
    s32 j;

    b[2] = 0;
    b[3] = 0;
    b[4] = 0;
    for (j = 0; j < n; j++) {
        if (func_02080418(b, 0x20, 1, file) == 1) {
            s32 v, r;
            u32 *kf;

            kf = (u32 *)((u8 *)PTR_AT(PTR_AT(e, 0), k * 0xc) + j * 0x20);
            kf[0] = b[0];
            kf[1] = b[1];
            kf[2] = b[2];
            kf[3] = b[3];
            kf[4] = b[4];
            kf[6] = b[6];
            kf[7] = b[7];
            v = b[5];
            r = (s32)(v > 0 ? 0.5f + (float)SHL(v, 12) : (float)SHL(v, 12) - 0.5f);
            kf[5] = (u32)func_020a4c50((s64)r * 0x3244, 0xb4000);
        }
    }
}

/* 0x02051f80: load effect file `id`
 * @difftest zero:0xc u32 stub=0x02080488,0x02080418:0,0x02080458 cases=3 */
void func_02051f80(u8 *e, u32 id)
{
    u8 file[0x48];
    u32 cnt, n, i;

    func_02080488(file, id);
    func_02080418(&cnt, 4, 1, file);
    func_02004490(data_020c0580, cnt);
    func_0207fe28(data_020c059c, 0x2a);
    if (cnt > U32_AT(e, 4))
        func_020523e0(e, cnt - U32_AT(e, 4), 0);
    else if (cnt < U32_AT(e, 4))
        func_02052388(e, U32_AT(e, 4) - cnt, 0);
    func_0207fe24();
    func_02004490(data_020c05b4, cnt);
    i = 0;
    if (func_02080418(&n, 4, 1, file) == 1) {
        do {
            if (n != 0) {
                u8 *row;

                func_0207fe28(data_020c059c, 0x35);
                row = (u8 *)PTR_AT(e, 0) + i * 0xc;
                if (n > U32_AT(row, 4)) {
                    /* (the original leaves the other words as they are on
                     * the stack; they are read from the file next) */
                    Key key;
                    u32 w;

                    for (w = 0; w < 8; w++)
                        key.w[w] = 0;
                    func_02052230(row, n - U32_AT(row, 4), &key, 0);
                } else if (n < U32_AT(row, 4)) {
                    func_0205221c(row, U32_AT(row, 4) - n, 0);
                }
                func_0207fe24();
                func_02051e08(e, file, i, n);
            }
            i++;
        } while (func_02080418(&n, 4, 1, file) == 1);
    }
    func_02080458(file);
}

/* 0x0205216c: the effect data's destructor
 * @difftest $E=zero:0xc $E stub=0x0207fe44 cases=3
 * @difftest $R=zero:0x18 @$R+4:32=2 @$R+8:32=4 @$R+0xc:32=$R @$R+0x10:32=1 $E=zero:0xc @$E:32=$R @$E+4:32=2 $E stub=0x0207fe44 cases=3 */
u8 *func_0205216c(u8 *e)
{
    u32 i;

    for (i = 0; i < U32_AT(e, 4); i++) {
        u8 *row = (u8 *)PTR_AT(e, 0) + i * 0xc;

        func_0205221c(row, U32_AT(row, 4), 0);
    }
    func_02052388(e, U32_AT(e, 4), 0);
    func_02052338(e);
    return e;
}

/* 0x02052208: a vector's constructor
 * @difftest ptr:0xc:4 cases=3 */
u8 *func_02052208(u8 *v)
{
    U32_AT(v, 0) = 0;
    U32_AT(v, 4) = 0;
    U32_AT(v, 8) = 0;
    return v;
}

/* 0x0205221c: drop the last `n` keys
 * @difftest ptr:0xc:4 int:0:4 cases=5 */
void func_0205221c(u8 *v, u32 n, u32 tag)
{
    U32_AT(v, 4) -= n;
}

/* 0x02052230: append `n` copies of `*key`
 * @difftest $D=ptr:0x80:4 $V=zero:0xc @$V:32=$D @$V+4:32=int:0:3 @$V+8:32=4 $V int:0:4 ptr:0x20:4 stub=0x020526c0:0,0x020524cc cases=20 */
void func_02052230(u8 *v, u32 n, const Key *key, u32 tag)
{
    u32 cap = U32_AT(v, 8);
    u32 *p;

    if (n > cap || U32_AT(v, 4) > cap - n) {
        func_020526c0(v, U32_AT(v, 4) + n - cap);
        func_020524cc(v, n, key);
        return;
    }
    p = (u32 *)((u8 *)PTR_AT(v, 0) + U32_AT(v, 4) * 0x20);
    for (; n != 0; n--) {
        if (p != NULL)
            CopyKey(p, key->w);
        U32_AT(v, 4)++;
        p += 8;
    }
}

/* 0x02052338: the frame vector's destructor
 * @difftest $E=zero:0xc $E stub=0x0207fe44 cases=3
 * @difftest $R=zero:0x18 @$R:32=4 @$R+4:32=2 $E=zero:0xc @$E:32=$R @$E+4:32=2 $E stub=0x0207fe44 cases=3 */
u8 *func_02052338(u8 *e)
{
    func_02052388(e, U32_AT(e, 4), 0);
    if (PTR_AT(e, 0) != NULL)
        func_0207fe44(PTR_AT(e, 0));
    return e;
}

/* 0x02052388: destroy the last `n` frames
 * @difftest $R=zero:0x18 @$R:32=pick:0,4 @$R+0xc:32=pick:0,4 $E=zero:0xc @$E:32=$R @$E+4:32=2 $E int:0:3 stub=0x0207fe44 cases=10 */
void func_02052388(u8 *e, u32 n, u32 tag)
{
    u8 *p = (u8 *)PTR_AT(e, 0) + U32_AT(e, 4) * 0xc;

    U32_AT(e, 4) -= n;
    for (; n != 0; n--) {
        p -= 0xc;
        func_0205247c(p);
    }
}

/* 0x020523e0: append `n` empty frames
 * @difftest $R=ptr:0x30:4 $E=zero:0xc @$E:32=$R @$E+4:32=int:0:3 @$E+8:32=4 $E int:0:3 stub=0x02052904:0,0x02052798 cases=20 */
void func_020523e0(u8 *e, u32 n, u32 tag)
{
    u32 cap = U32_AT(e, 8);

    if (n > cap || U32_AT(e, 4) > cap - n) {
        func_02052904(e, U32_AT(e, 4) + n - cap);
        func_02052798(e, n);
        return;
    }
    func_0209e7d8((u8 *)PTR_AT(e, 0) + U32_AT(e, 4) * 0xc, 0, n * 0xc);
    U32_AT(e, 4) += n;
}

/* 0x0205247c: a frame's destructor
 * @difftest $V=ptr:0xc:4 @$V:32=pick:0,4 $V stub=0x0207fe44 cases=5 */
u8 *func_0205247c(u8 *v)
{
    func_0205221c(v, U32_AT(v, 4), 0);
    if (PTR_AT(v, 0) != NULL)
        func_0207fe44(PTR_AT(v, 0));
    return v;
}

/* 0x020524cc: append `n` copies of `*key` into a bigger buffer
 * @difftest $D=ptr:0x80:4 $V=zero:0xc @$V:32=$D @$V+4:32=int:0:4 @$V+8:32=4 $V int:1:4 ptr:0x20:4 stub=0x0207fe44 cases=20 */
void func_020524cc(u8 *v, u32 n, const Key *key)
{
    SplitBuf b;
    u32 *p, *src, *dst, k, t;
    u8 *td;

    b.data = NULL;
    b.size = 0;
    b.cap = 0;
    b.off = 0;
    b.capref = (u32 *)(v + 8);
    func_02052b54(&b, func_020526c0(v, U32_AT(v, 4) + n - U32_AT(v, 8)), 0);
    b.off = U32_AT(v, 4);
    p = (u32 *)(b.data + b.off * 0x20 + b.size * 0x20);
    for (k = n; k != 0; k--) {
        if (p != NULL)
            CopyKey(p, key->w);
        b.size++;
        p += 8;
    }
    src = (u32 *)((u8 *)PTR_AT(v, 0) + U32_AT(v, 4) * 0x20);
    dst = (u32 *)(b.data + b.off * 0x20);
    while (src > (u32 *)PTR_AT(v, 0)) {
        src -= 8;
        dst -= 8;
        if (dst != NULL)
            CopyKey(dst, src);
        b.off--;
        b.size++;
    }
    t = *b.capref;
    *b.capref = b.cap;
    b.cap = t;
    td = PTR_AT(v, 0);
    PTR_AT(v, 0) = b.data;
    b.data = td;
    t = U32_AT(v, 4);
    U32_AT(v, 4) = b.size;
    b.size = t;
    func_020529e0(&b);
}

/* the capacity to grow to: `cap` + max(`n`, half or 3/5 of it) */
static u32 Grow(u32 cap, u32 n, u32 max, u32 lo, u32 hi)
{
    u32 g;

    if (n > max - cap)
        func_02006ec8();
    if (cap < lo) {
        g = (cap + 1) * 3 / 5;
        return cap + (g < n ? n : g);
    }
    if (cap < hi) {
        g = (cap + 1) >> 1;
        return cap + (g < n ? n : g);
    }
    return max;
}

/* 0x020526c0: the capacity to grow a key vector to for `n` more
 * @difftest $V=zero:0xc @$V+8:32=pick:0,1,5,100,0x2aaaaa9,0x2aaaaaa,0x5555553,0x5555554,0x7ffffff $V int:0:0x200 stub=0x02006ec8 cases=40 */
u32 func_020526c0(u8 *v, u32 n)
{
    return Grow(U32_AT(v, 8), n, 0x7ffffff, 0x2aaaaaa, 0x5555554);
}

/* 0x02052798: append `n` empty frames into a bigger buffer
 * @difftest $R=ptr:0x30:4 $E=zero:0xc @$E:32=$R @$E+4:32=int:0:4 @$E+8:32=4 $E int:1:3 stub=0x0207fe44 cases=20 */
void func_02052798(u8 *e, u32 n)
{
    SplitBuf b;
    u32 t, cnt;
    u8 *td;

    b.data = NULL;
    b.size = 0;
    b.cap = 0;
    b.off = 0;
    b.capref = (u32 *)(e + 8);
    func_02052aa8(&b, func_02052904(e, U32_AT(e, 4) + n - U32_AT(e, 8)), 0);
    b.off = U32_AT(e, 4);
    func_0209e7d8(b.data + b.off * 0xc + b.size * 0xc, 0, n * 0xc);
    b.size += n;
    cnt = (s32)(((u8 *)PTR_AT(e, 0) + U32_AT(e, 4) * 0xc) - (u8 *)PTR_AT(e, 0)) / 0xc;
    b.off -= cnt;
    func_0209e83c(b.data + b.off * 0xc, PTR_AT(e, 0), cnt * 0xc);
    b.size += cnt;
    U32_AT(e, 4) = 0;
    t = *b.capref;
    *b.capref = b.cap;
    b.cap = t;
    td = PTR_AT(e, 0);
    PTR_AT(e, 0) = b.data;
    b.data = td;
    t = U32_AT(e, 4);
    U32_AT(e, 4) = b.size;
    b.size = t;
    func_02052a00(&b);
}

/* 0x02052904: the capacity to grow the frame vector to for `n` more
 * @difftest $V=zero:0xc @$V+8:32=pick:0,1,5,100,0x71c71c6,0x71c71c7,0xe38e38d,0xe38e38e,0x15555555 $V int:0:0x200 stub=0x02006ec8 cases=40 */
u32 func_02052904(u8 *e, u32 n)
{
    return Grow(U32_AT(e, 8), n, 0x15555555, 0x71c71c7, 0xe38e38e);
}

/* 0x020529e0: a key split buffer's destructor (it owns no keys)
 * @difftest $B=zero:0x14 @$B:32=pick:0,4 @$B+4:32=int:0:4 $B stub=0x0207fe44 cases=5 */
SplitBuf *func_020529e0(SplitBuf *b)
{
    b->size = 0;
    func_02052b04(b);
    return b;
}

/* 0x02052a00: a frame split buffer's destructor
 * @difftest $R=zero:0x24 $B=zero:0x14 @$B:32=$R @$B+4:32=int:0:3 $B stub=0x0207fe44 cases=5 */
SplitBuf *func_02052a00(SplitBuf *b)
{
    u8 *p0 = b->data + b->off * 0xc;
    u8 *p = p0 + b->size * 0xc;

    while (p > p0) {
        p -= 0xc;
        func_0205247c(p);
    }
    b->size = 0;
    func_02052a58(b);
    return b;
}

/* 0x02052a58: free a frame split buffer
 * @difftest $R=zero:0x24 $B=zero:0x14 @$B:32=$R @$B+4:32=int:0:3 $B stub=0x0207fe44 cases=5 */
SplitBuf *func_02052a58(SplitBuf *b)
{
    func_02052ba8(b, b->size, 0);
    if (b->data != NULL)
        func_0207fe44(b->data);
    return b;
}

/* 0x02052aa8: give a frame split buffer room for `n` frames
 * @difftest zero:0x14 int:0:8 stub=0x0207fe74:0x1234 cases=5 */
void func_02052aa8(SplitBuf *b, u32 n, u32 tag)
{
    u8 *p;

    if (n > 0x15555555)
        func_02006ec8();
    p = func_0207fe74(n * 0xc);
    if (p == NULL) {
        func_0209e90c(data_020bcb38, data_020c05e8);
        func_0209dcb8();
    }
    b->data = p;
    b->cap = n;
}

/* 0x02052b04: free a key split buffer
 * @difftest $B=zero:0x14 @$B:32=pick:0,4 @$B+4:32=int:0:4 $B stub=0x0207fe44 cases=5 */
SplitBuf *func_02052b04(SplitBuf *b)
{
    func_02052c00(b, b->size, 0);
    if (b->data != NULL)
        func_0207fe44(b->data);
    return b;
}

/* 0x02052b54: give a key split buffer room for `n` keys
 * @difftest zero:0x14 int:0:8 stub=0x0207fe74:0x1234 cases=5 */
void func_02052b54(SplitBuf *b, u32 n, u32 tag)
{
    u8 *p;

    if (n > 0x7ffffff)
        func_02006ec8();
    p = func_0207fe74(n << 5);
    if (p == NULL) {
        func_0209e90c(data_020bcb38, data_020c05cc);
        func_0209dcb8();
    }
    b->data = p;
    b->cap = n;
}

/* 0x02052ba8: destroy the last `n` frames of a split buffer
 * @difftest $R=zero:0x18 @$R:32=pick:0,4 @$R+0xc:32=pick:0,4 $B=zero:0x14 @$B:32=$R @$B+4:32=2 $B int:0:3 stub=0x0207fe44 cases=10 */
void func_02052ba8(SplitBuf *b, u32 n, u32 tag)
{
    u8 *p = b->data + b->size * 0xc;

    b->size -= n;
    for (; n != 0; n--) {
        p -= 0xc;
        func_0205247c(p);
    }
}

/* 0x02052c00: drop the last `n` keys of a split buffer
 * @difftest ptr:0x14:4 int:0:4 cases=5 */
void func_02052c00(SplitBuf *b, u32 n, u32 tag)
{
    b->size -= n;
}
