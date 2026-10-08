/*
 * Motion.cpp, part 1: the BG collision map (block types, heights, the
 * ground height under an actor) and the per-screen OAM buffers with their
 * affine parameter slots.
 * ARM9 main, 0x02019f58 - 0x0201a790 (14 functions).
 *
 * Collision: data_020df578 is the header (+2/+4 u16 width/height in 8-px
 * tiles, +6 one height byte per 512-px block), data_020df588 the u16 block
 * map (32-px blocks; low 12 bits = pattern), data_020df57c the 4x4-cell
 * patterns (one type byte per cell).
 * OAM: data_020df598 + screen * 0x13ec: +0 current buffer (OAM copy at +0,
 * second copy at +0x400), +4 previous, +8/+0x808 the two buffers, +0x1308
 * affine parameters (6 bytes each: s16 x/y scale, u16 angle), +0x13c8 32
 * slot-used flags, +0x13e8 slots in use, +0x13e9 sprites in use.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const u32 data_020b5780[2];   /* an unused OAM entry */
extern const char data_020bda10[];   /* "Motion.cpp" */
extern u8 *data_020df578;            /* collision header */
extern u8 *data_020df57c;            /* collision patterns */
extern u8 *data_020df580;            /* block map file */
extern s32 data_020df584;            /* collision header file */
extern u16 *data_020df588;           /* block map */
extern u8 data_020df598[];           /* OAM buffers */
extern u8 *data_020df0b4;            /* frame state */

void func_0200fdb4(const u8 *src, void *dst);
u32 func_0200feec(const u8 *src);
void func_020129ac(s32 id);
s32 func_0201993c(s32 a, s32 b);
s32 func_020199a0(s32 a);
s32 func_020199c0(s32 a);

/* this file's own functions, called before their definition */
void func_0201a50c(const u8 *params, u16 *out, s32 n, s32 stride);

#define OAM_STATE() (data_020df598 + data_020df0fc * 0x13ec)
#define OAM_BUF() ((u8 *)PTR_AT(OAM_STATE(), 0))

/* 0x02019f58: collision type at a pixel position (0 outside the map)
 * @difftest $P=ptr:8:4 @$P+0:32=int:0:0x400 @$P+4:32=int:0:0x400 $P */
u32 func_02019f58(const s32 *p)
{
    u8 *hdr = data_020df578;
    u32 x = p[0], w = U16_AT(hdr, 2) << 3, y;
    if (x > w)
        return 0;
    y = p[1];
    if (y > (u32)(U16_AT(hdr, 4) << 3))
        return 0;
    {
        s32 blk = ((s32)y >> 5) * ((s32)w >> 5) + ((s32)x >> 5);
        u32 v = data_020df588[blk];
        u32 cell = (((s32)x >> 3) & 3) | (((s32)y >> 1) & 0xc);
        return data_020df57c[(u16)(cell + ((v << 20) >> 16))];
    }
}

/* 0x0201a000: block at a pixel position
 * @difftest int:0:0x400 int:0:0x400 */
u32 func_0201a000(s32 x, s32 y)
{
    s32 size[3];
    func_0201c428(size);
    return data_020df588[(y >> 5) * (SHL(size[0], 3) >> 5) + (x >> 5)];
}

/* 0x0201a050: set the block at a pixel position
 * @difftest int:0:0x400 int:0:0x400 u16 */
void func_0201a050(s32 x, s32 y, u32 v)
{
    s32 size[3];
    func_0201c428(size);
    data_020df588[(y >> 5) * (size[0] >> 5) + (x >> 5)] = v;
}

/* 0x0201a0a0: ground height at pos (the top of the platform the actor
 * stands on, when inside it)
 * @difftest $L=ptr:0x30:4 @$L+0x10:32=int:-20:0 @$L+0x14:32=int:-20:0 @$L+0x18:32=int:0:20 @$L+0x1c:32=int:0:20 @$L+0x28:32=int:0:0x40000 @$L+0x2c:32=int:0:0x40000 $A=ptr:0xb0:4 @$A+0xa4:32=$L @$A+0x38:32=pick:0,0x20000 $P=ptr:8:4 @$P+0:32=int:0:0x400000 @$P+4:32=int:0:0x400000 $P $A cases=150 */
s32 func_0201a0a0(s32 *pos, void *actor)
{
    u8 *hdr = data_020df578;
    s32 x = pos[0], y = pos[1], h;
    u8 *pl;

    h = SHL(hdr[6 + (y >> 19) * U16_AT(hdr, 2) + (x >> 19)], 18);
    if (actor == NULL)
        return h;
    if ((s8)BITS(U32_AT(actor, 0x38), 17, 1) == 0)
        return h;
    pl = PTR_AT(actor, 0xa4);
    if (pl == NULL)
        return h;
    {
        s32 px = S32_AT(pl, 0x28), py = S32_AT(pl, 0x2c);
        s32 xs = x >> 16, ys = y >> 16;
        if (xs > S32_AT(pl, 0x14) + (px >> 12) && xs < S32_AT(pl, 0x1c) + (px >> 12) &&
            ys < S32_AT(pl, 0x18) + (py >> 12) && ys > S32_AT(pl, 0x10) + (py >> 12))
            return S32_AT(actor, 0x68);
    }
    return h;
}

/* 0x0201a190: free the collision map ("motionBGCollideExit") */
void func_0201a190(void)
{
    func_020129ac(data_020df584);
    func_0207ff14(data_020df57c, data_020bda10, 0x7b);
    data_020df57c = NULL;
    func_0207ff14(data_020df580, data_020bda10, 0x7e);
    data_020df580 = NULL;
    data_020df588 = NULL;
}

/* 0x0201a20c: load the collision map from three files */
void func_0201a20c(s32 patterns, s32 blocks, s32 header)
{
    u8 *src, *buf;

    func_0207fe28(data_020bda10, 0x55);
    data_020df578 = func_02012a64(header);
    func_0207fe24();
    data_020df584 = header;

    src = func_02012a64(patterns);
    buf = func_0207ff70(func_0200feec(src), data_020bda10, 0x5f);
    data_020df57c = buf;
    func_0200fdb4(src, buf);
    func_020129ac(patterns);

    src = func_02012a64(blocks);
    buf = func_0207ff70(func_0200feec(src), data_020bda10, 0x69);
    data_020df580 = buf;
    data_020df588 = (u16 *)(buf + 4);
    func_0200fdb4(src, buf);
    func_020129ac(blocks);
}

static inline void HideEntry(u8 *e)
{
    U32_AT(e, 0) = (U32_AT(e, 0) & ~0x300u) | 0x200;
}

/* 0x0201a2e0: hide the sprites not used this frame
 * @difftest $B=zero:0x800 $S=zero:0x40 @020df0b4:32=$S @020df598:32=$B @020e0981:8=int:0:0x81 @020df0fc:32=pick:0 */
void func_0201a2e0(void)
{
    u32 i = OAM_STATE()[0x13e9];
    for (; i < 0x80; i++) {
        HideEntry(OAM_BUF() + i * 8);
        HideEntry(OAM_BUF() + 0x400 + i * 8);
    }
    data_020df0b4[data_020df0fc + 0x38] = 1;
}

/* 0x0201a390: hide sprite i of the second copy
 * @difftest $B=zero:0x800 @020df598:32=$B @020df0fc:32=pick:0 int:0:0x80 */
void func_0201a390(u32 i) { HideEntry(OAM_BUF() + 0x400 + i * 8); }

/* 0x0201a3cc: free all affine slots
 * @difftest @020df0fc:32=int:0:1 */
void func_0201a3cc(void)
{
    u8 *s;
    u32 i;
    OAM_STATE()[0x13e8] = 0;
    s = OAM_STATE();
    for (i = 0; i < 0x20; i++)
        s[0x13c8 + i] = 0;
}

/* 0x0201a420: clear the OAM buffer and upload it */
void func_0201a420(void)
{
    u32 v0 = data_020b5780[0], v1 = data_020b5780[1], i;
    for (i = 0; i < 0x80; i++) {
        U32_AT(OAM_BUF(), i * 8) = v0;
        U32_AT(OAM_BUF() + i * 8, 4) = v1;
        U32_AT(OAM_BUF() + 0x400, i * 8) = v0;
        U32_AT(OAM_BUF() + 0x400 + i * 8, 4) = v1;
    }
    func_0207fed8(3, (void *)(0x07000000 + (data_020df0fc << 10)), OAM_BUF(), 0x400, 0x20);
    OAM_STATE()[0x13e9] = 0;
}

/* 0x0201a50c: write n affine matrices (rotation and scale) into OAM
 * @difftest ptr:0x30:2 zero:0x100 int:0:6 pick:2,8 */
void func_0201a50c(const u8 *params, u16 *out, s32 n, s32 stride)
{
    s32 i;
    for (i = 0; i < n; i++, params += 6) {
        u32 a = (U16_AT(params, 4) >> 8) & 0xff;
        s32 sx = S16_AT(params, 0), sy = S16_AT(params, 2);
        *out = func_0201993c(func_020199a0(a), sx);
        out = (u16 *)((u8 *)out + stride);
        *out = func_0201993c(func_020199c0(a), sx);
        out = (u16 *)((u8 *)out + stride);
        *out = func_0201993c((s16)-func_020199c0(a), sy);
        out = (u16 *)((u8 *)out + stride);
        *out = func_0201993c(func_020199a0(a), sy);
        out = (u16 *)((u8 *)out + stride);
    }
}

/* 0x0201a5c4: write the affine matrices into both OAM copies */
void func_0201a5c4(void)
{
    u8 *s = OAM_STATE();
    if (s[0x13e8] == 0)
        return;
    func_0201a50c(s + 0x1308, (u16 *)((u8 *)PTR_AT(s, 0) + 6), s[0x13e8], 8);
    s = OAM_STATE();
    func_0201a50c(s + 0x1308, (u16 *)((u8 *)PTR_AT(s, 0) + 0x400 + 6), s[0x13e8], 8);
}

/* 0x0201a660: free affine slot i (and trailing free slots)
 * @difftest @020df0fc:32=int:0:1 @020e0980:8=int:0:0x20 int:0:0x20 */
void func_0201a660(u32 i)
{
    u8 *s = OAM_STATE();
    s[0x13c8 + i] = 0;
    if (i != (u32)(s[0x13e8] - 1))
        return;
    while (s[0x13e8] != 0 && s[0x13c8 + i] == 0) {
        i = (u16)(i - 1);
        s[0x13e8]--;
    }
}

/* 0x0201a6fc: take a free affine slot; 0x20 when none is free
 * @difftest @020df0fc:32=int:0:1 @020e0980:8=int:0:0x20 */
u32 func_0201a6fc(void)
{
    u8 *s = OAM_STATE();
    u16 i = 0, k = 0x20;
    do {
        if (s[0x13c8 + i] != 1) {
            s[0x13c8 + i] = 1;
            if (i >= s[0x13e8])
                s[0x13e8] = s[0x13e8] + 1;
            return i;
        }
        i++;
    } while (--k != 0);
    return i;
}
