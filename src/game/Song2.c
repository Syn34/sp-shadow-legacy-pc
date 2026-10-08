/*
 * Song.cpp, part 2: songs of the music player: loading a song's header,
 * freeing it, stealing a hardware channel, the per-tick driver, tempo,
 * volume fades, jumping to an order position, pausing, resuming and
 * starting songs; plus two text helpers that live in the same file
 * (number formatting and a string lookup).
 * ARM9 main, 0x0202752c - 0x02028a50 (19 functions).
 *
 * data_020e2ba8: +0x00 row callback, +0x04/+0x14 song memory pool base and
 * used bytes, +0x08 the three players (= data_020e2bb0[3]: 0 music, 1
 * jingle, 2 extra). A player is 0x103c bytes (layout in SeqPlayer1.c);
 * here also +0x14 row pointer, +0x18/+0x1c tick length, +0x20 song, +0x22
 * rows left, +0x24 mixing rate, +0x26/+0x27 tracks and mixer voices,
 * +0x28 state (0 stopped, 1 playing, 2 paused), +0x29 flags, +0x2a/+0x2e
 * tick counter and speed, +0x2b/+0x35 order position and next, +0x2d
 * tempo, +0x2f bit 7 loaded into the heap, +0x30 order length, +0x34
 * pattern break, +0x36 pattern delay, +0x37 fine delay, +0xd3c channel
 * registers saved while paused (0x18 bytes per channel).
 * Channel `i` is owned by data_020e2614[0x55c + i] (= data_020e2b70[i]:
 * a player, 3 free, 5 reserved).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define REG16(a) (*(vu16 *)(a))
#define REG32(a) (*(vu32 *)(a))
#define REG_DIVCNT 0x04000280
#define REG_DIV_NUMER 0x04000290
#define REG_DIV_DENOM 0x04000298
#define REG_DIV_RESULT 0x040002a0

extern const char data_020bde68[];   /* "Song.cpp" */
extern const char data_020bde74[];   /* the sound data file */
extern u8 data_020e2314[];
extern u8 data_020e2614[];
extern u8 data_020e2b70[];
extern u8 data_020e2ba4[];
extern u8 data_020e2ba8[];
extern u8 data_020e2bb0[];
extern u8 data_020e2bcc[];           /* thousands separator */
extern u8 data_020e2bd0[];           /* 0x400-byte string buffer */

u32 func_020803ac(void *file);
void func_020803d0(void *file, u32 pos, u32 whence);
void func_020804ac(void *file, const char *path);
s32 func_02017b5c(u32 index, u8 *out, u32 maxlen);

void func_02022ecc(u32 v);
void func_02022f68(u32 t);
void func_0202268c(u32 n);
void func_02022748(u32 rate);
void func_020231c0(void);
void func_02023648(void);
void func_020236b4(void);
void func_02023828(u32 v);
void func_02023d60(u32 v, u32 mode);
void func_02023fac(void);
void func_020241b8(void);
void func_0202627c(void);
u32 func_020265c8(void);
void func_02026758(void *file, u8 *hdr, s32 alloc);
void func_02026938(void *file, u8 *hdr, s32 alloc);
void func_02026cdc(void *file, u8 *hdr, s32 alloc);
void func_02027acc(u32 id);
void func_02027b9c(u32 id);
void func_02027cec(u32 id, u32 pos, u32 row);
void func_02027ec4(u32 id);
void func_020281bc(u32 id);
void func_02028330(u32 id);

#define D data_020e2614
#define PLAYER() ((u8 *)PTR_AT(data_020e2ba4, 0))
#define SET_PLAYER(p) (PTR_AT(data_020e2ba4, 0) = (p))
#define SONG(id) ((u8 *)PTR_AT(data_020e2bb0, (id) * 4))
#define VOICE(p, v) ((p) + 0x93c + (v) * 0x20)
#define CHAN(v) (data_020e2314 + (v) * 0x18)
#define SAVED(p, v) ((p) + 0xd3c + (v) * 0x18)
#define ROWP() (*(u8 **)(PLAYER() + 0x14))
#define POOL_NEXT() (U32_AT(data_020e2ba8, 4) + U32_AT(data_020e2ba8, 0x14))

/* 0x0202752c: load song `song` into the current player (`alloc`: heap) */
void func_0202752c(u32 song, s32 alloc)
{
    u32 off;
    u8 hdr[0x28];
    u8 info[0xc];
    u8 vols[0x40];
    u8 pans[0x40];
    u32 file[0x48 / 4];
    s32 i;

    U16_AT(PLAYER(), 0x20) = song;
    func_020804ac(file, data_020bde74);
    func_02080418(hdr, 0x28, 1, file);
    func_020803d0(file, U32_AT(hdr, 0) + song * 4, 0);
    func_02080418(&off, 4, 1, file);
    func_020803d0(file, off, 0);
    func_02080418(info, 0xc, 1, file);
    for (i = 0; i < 8; i++)
        PLAYER()[0x2c + i] = info[i];
    U16_AT(PLAYER(), 0x24) = U16_AT(info, 8);
    PLAYER()[0x26] = info[0xa];
    PLAYER()[0x27] = info[0xb];
    PLAYER()[0x38] = 0x80;
    PLAYER()[0x3a] = 0;
    PLAYER()[0x39] = 0;
    func_02080418(vols, PLAYER()[0x26], 1, file);
    func_02080418(pans, PLAYER()[0x26], 1, file);
    for (i = 0; i < PLAYER()[0x26]; i++) {
        PLAYER()[0x42 + i * 0x24] = vols[i];
        PLAYER()[0x43 + i * 0x24] = pans[i];
    }
    if (alloc != 0) {
        PTR_AT(PLAYER(), 8) = func_0207ff70(PLAYER()[0x30], data_020bde68, 0x4c4);
    } else {
        U32_AT(PLAYER(), 8) = POOL_NEXT();
        U32_AT(data_020e2ba8, 0x14) = U32_AT(data_020e2ba8, 0x14) + ((PLAYER()[0x30] + 3) & ~3u);
    }
    func_02080418(PTR_AT(PLAYER(), 8), PLAYER()[0x30], 1, file);
    func_02026cdc(file, hdr, alloc);
    func_02026938(file, hdr, alloc);
    func_02026758(file, hdr, alloc);
    func_02080458(file);
}

/* 0x020277c0: free a sample's data (bit 31: stored as address / 2)
 * @difftest $X=ptr:4:4 @$X:32=pick:0,0x80000000 $X cases=20 */
void func_020277c0(u32 *p)
{
    if (*p & 0x80000000)
        func_0207ff14((void *)(*p << 1), data_020bde68, 0x471);
    else
        func_0207ff14((void *)*p, data_020bde68, 0x475);
    *p = 0;
}

/* 0x02027810: find a channel for a sound effect: a free one, else the
 * quietest released music voice (which is released for good); 0xff: none */
u32 func_02027810(void)
{
    u8 *saved = PLAYER();
    u8 *bv = NULL;
    u32 best = 0xff;
    s32 i;

    for (i = D[0x10] - 1; i >= 0; i--) {
        u32 own = D[0x55c + i];
        u8 *e;
        if (own == 5)
            continue;
        if (own == 3) {
            best = i;
            break;
        }
        SET_PLAYER(SONG(own));
        e = VOICE(PLAYER(), i);
        if (!(e[0xb] & 2))
            continue;
        if (best == 0xff || e[6] < bv[6]) {
            best = i;
            bv = e;
        }
    }
    if (best != 0xff && data_020e2b70[best] != 3) {
        SET_PLAYER(SONG(data_020e2b70[best]));
        func_02023d60(best & 0xff, 0);
        data_020e2b70[best] = 3;
    }
    SET_PLAYER(saved);
    return best & 0xff;
}

/* 0x02027918: set a song's volume (`step` 0), or fade it from `vol` (0xff:
 * the current volume) to `target` by `step` per tick
 * @difftest $P=ptr:0x40:4 @020e2bb0:32=$P @020e2bb4:32=0 int:0:2 pick:0x40,0xff u8 pick:0,1,4 cases=40 */
void func_02027918(u32 id, u32 vol, u32 target, u32 step)
{
    u8 *p = SONG(id);

    if (p == NULL)
        return;
    SET_PLAYER(p);
    if (step != 0) {
        if (vol != 0xff)
            p[0x38] = vol;
        PLAYER()[0x39] = target;
        PLAYER()[0x3a] = step;
    } else {
        p[0x38] = target;
    }
    SET_PLAYER(NULL);
}

/* 0x02027974: per-tick update of song `id` */
void func_02027974(s32 id)
{
    u8 *p = SONG(id);

    if (p == NULL)
        return;
    SET_PLAYER(p);
    if (p[0x28] != 1)
        return;
    func_020236b4();
    if (PLAYER()[0x37]-- != 0) {
        func_020241b8();
        goto tail;
    }
    PLAYER()[0x37] = 0;
    if (--PLAYER()[0x2a] != 0) {
        func_020241b8();
        goto tail;
    }
    PLAYER()[0x2a] = PLAYER()[0x2e];
    if (PLAYER()[0x36]-- == 0) {
        PLAYER()[0x36] = 0;
        if (U16_AT(PLAYER(), 0x22)-- == 0) {
            if (func_020265c8() == 1)
                return;
            U16_AT(PLAYER(), 0x22)--;
        }
        func_0202627c();
    }
    if (!(PLAYER()[0x2f] & 0x10))
        func_020241b8();
tail:
    func_020231c0();
    func_02023fac();
}

/* 0x02027acc: refresh the volume and pan of song `id`'s channels (4: all) */
void func_02027acc(u32 id)
{
    u8 *p;
    s32 i;

    if (id == 4) {
        for (i = 0; i < 3; i++)
            func_02027acc(i);
        return;
    }
    p = SONG(id);
    if (p == NULL)
        return;
    SET_PLAYER(p);
    for (i = D[0x10] - 1; i >= 0; i--) {
        if (PLAYER()[0x3b] == D[0x55c + i] && U32_AT(CHAN(i), 0) != 0)
            func_02023828(i & 0xff);
    }
}

/* 0x02027b9c: recompute the tick length of song `id` from its tempo (4: all)
 * @difftest $P=ptr:0x40:4 @020e2bb0:32=$P @020e2bb4:32=0 @020e2bb8:32=$P @$P+0x2d:8=int:0x20:0x100 pick:0,1,2,4 cases=40 */
void func_02027b9c(u32 id)
{
    u8 *p;
    s32 i, q;

    if (id == 4) {
        for (i = 0; i < 3; i++)
            func_02027b9c(i);
        return;
    }
    p = SONG(id);
    if (p == NULL)
        return;
    SET_PLAYER(p);
    q = (s32)(p[0x2d] << 1) / 5;
    REG16(REG_DIVCNT) = 0;
    REG32(REG_DIV_NUMER) = U16_AT(D, 0x12) << 15;
    REG32(REG_DIV_DENOM) = q;
    REG32(REG_DIV_DENOM + 4) = 0;
    while (REG16(REG_DIVCNT) & 0x8000)
        ;
    U32_AT(PLAYER(), 0x18) = REG32(REG_DIV_RESULT) << 1;
    U32_AT(PLAYER(), 0x1c) = U32_AT(PLAYER(), 0x18);
}

/* 0x02027c94: set the tempo of song `id` (clamped to 0x20-0xff)
 * @difftest $P=ptr:0x40:4 @020e2bb0:32=$P @020e2bb4:32=0 int:0:2 pick:0,0x1f,0x20,0x7d,0xff,0x100,0x12345 cases=40 */
void func_02027c94(u32 id, u32 tempo)
{
    u8 *p = SONG(id);

    if (p == NULL)
        return;
    SET_PLAYER(p);
    if (tempo < 0x20)
        tempo = 0x20;
    else if (tempo > 0xff)
        tempo = 0xff;
    p[0x2d] = tempo;
    func_02027b9c(id);
}

/* 0x02027cec: jump song `id` to order position `pos`, row `row` */
void func_02027cec(u32 id, u32 pos, u32 row)
{
    u8 *p = SONG(id);
    u8 *pat;
    u32 n;

    if (p == NULL)
        return;
    SET_PLAYER(p);
    p[0x2b] = pos;
    pat = ((u8 **)PTR_AT(PLAYER(), 0xc))[((u8 *)PTR_AT(PLAYER(), 8))[PLAYER()[0x2b]]];
    U16_AT(PLAYER(), 0x22) = pat[0] + 1;
    PTR_AT(PLAYER(), 0x14) = pat + 1;
    for (n = 0; n < row; n++) {
        u8 b;
        while ((b = *ROWP()) != 0) {
            ROWP() += 1;
            switch (b >> 6) {
            case 1:
                ROWP() += 1;
                break;
            case 2:
                if (*ROWP() & 0x80)
                    ROWP() += 1;
                ROWP() += 2;
                break;
            case 3: {
                u8 f = *ROWP();
                ROWP() += 2;
                if (f & 0x20)
                    ROWP() += 1;
                if (f & 0x40)
                    ROWP() += 1;
                if (f & 0x80)
                    ROWP() += 1;
                break;
            }
            }
        }
        ROWP() += 1;
        U16_AT(PLAYER(), 0x22)--;
    }
    PLAYER()[0x2a] = PLAYER()[0x2e];
    PLAYER()[0x34] = 0;
    PLAYER()[0x35] = PLAYER()[0x2b] + 1;
    PLAYER()[0x36] = 0;
    PLAYER()[0x37] = 0;
}

/* 0x02027ec4: stop song `id` and free it if it was loaded into the heap
 * (4: all); stopping the jingle resumes the music */
void func_02027ec4(u32 id)
{
    u8 *p;
    s32 i, k;

    if (id == 4) {
        for (i = 0; i < 3; i++)
            func_02027ec4(i);
        return;
    }
    p = SONG(id);
    if (p == NULL)
        return;
    SET_PLAYER(p);
    if ((u8)(p[0x28] - 1) <= 1) {
        func_02023648();
        PLAYER()[0x28] = 0;
    }
    if (PLAYER()[0x2f] & 0x80) {
        for (i = 1; i < PLAYER()[0x31] + 1; i++) {
            u8 *in = (u8 *)PTR_AT(PLAYER(), 4) + i * 0x20;
            if (in[3] & 1)
                func_0207ff14(PTR_AT(in, 0xc), data_020bde68, 0x287);
            if (in[3] & 2)
                func_0207ff14(PTR_AT(in, 0x10), data_020bde68, 0x28c);
            for (k = 0; k < 3; k++) {
                if (in[3] & (4 << k))
                    func_0207ff14(PTR_AT(in, 0x14 + k * 4), data_020bde68, 0x295);
            }
        }
        for (i = 1; i < PLAYER()[0x32] + 1; i++) {
            u8 *s = (u8 *)PTR_AT(PLAYER(), 0) + i * 0x24;
            if (U32_AT(s, 0) != 0)
                func_020277c0((u32 *)s);
        }
        for (i = 0; i < PLAYER()[0x33]; i++)
            func_0207ff14(((void **)PTR_AT(PLAYER(), 0xc))[i], data_020bde68, 0x2a6);
        func_0207ff14(PTR_AT(PLAYER(), 8), data_020bde68, 0x2a9);
        U32_AT(PLAYER(), 8) = 0;
        func_0207ff14(PTR_AT(PLAYER(), 4), data_020bde68, 0x2ac);
        U32_AT(PLAYER(), 4) = 0;
        func_0207ff14(PTR_AT(PLAYER(), 0), data_020bde68, 0x2af);
        U32_AT(PLAYER(), 0) = 0;
        func_0207ff14(PTR_AT(PLAYER(), 0xc), data_020bde68, 0x2b2);
        U32_AT(PLAYER(), 0xc) = 0;
        func_0207ff14(SONG(id), data_020bde68, 0x2b5);
        PTR_AT(data_020e2bb0, id * 4) = NULL;
        SET_PLAYER(NULL);
    }
    if (id == 1)
        func_020281bc(0);
}

/* 0x020281bc: resume paused song `id`: give its channels back (those
 * taken meanwhile are dropped from their voices)
 * @difftest $P=ptr:0x1040:4 @020e2bb0:32=$P @020e2bb4:32=$P @020e2bb8:32=$P @020e2624:8=int:1:17 @020e2bbc:32=0 @$P+0x28:8=pick:2,2,1 @$P+0x3b:8=3 @020e2b70:8=pick:0,1,2,3,5 @020e2b71:8=pick:0,1,2,3,5 @020e2b72:8=pick:0,1,2,3,5 @020e2b73:8=pick:0,1,2,3,5 @020e2b74:8=pick:0,1,2,3,5 @020e2b75:8=pick:0,1,2,3,5 @020e2b76:8=pick:0,1,2,3,5 @020e2b77:8=pick:0,1,2,3,5 @020e2b78:8=pick:0,1,2,3,5 @020e2b79:8=pick:0,1,2,3,5 @020e2b7a:8=pick:0,1,2,3,5 @020e2b7b:8=pick:0,1,2,3,5 @020e2b7c:8=pick:0,1,2,3,5 @020e2b7d:8=pick:0,1,2,3,5 @020e2b7e:8=pick:0,1,2,3,5 @020e2b7f:8=pick:0,1,2,3,5 @$P+0x946:8=pick:0,1,0xff @$P+0x966:8=pick:0,1,0xff @$P+0x986:8=pick:0,1,0xff @$P+0x9a6:8=pick:0,1,0xff @$P+0x9c6:8=pick:0,1,0xff @$P+0x9e6:8=pick:0,1,0xff @$P+0xa06:8=pick:0,1,0xff @$P+0xa26:8=pick:0,1,0xff @$P+0xa46:8=pick:0,1,0xff @$P+0xa66:8=pick:0,1,0xff @$P+0xa86:8=pick:0,1,0xff @$P+0xaa6:8=pick:0,1,0xff @$P+0xac6:8=pick:0,1,0xff @$P+0xae6:8=pick:0,1,0xff @$P+0xb06:8=pick:0,1,0xff @$P+0xb26:8=pick:0,1,0xff @$P+0xd3c:32=pick:0,1 @$P+0xd6c:32=pick:0,1 @$P+0xd9c:32=pick:0,1 @$P+0xdcc:32=pick:0,1 @$P+0xdfc:32=pick:0,1 @$P+0xe2c:32=pick:0,1 @$P+0xe5c:32=pick:0,1 @$P+0xe8c:32=pick:0,1 int:0:3 cases=100 */
void func_020281bc(u32 id)
{
    u8 *p = SONG(id);
    s32 i;

    if (p == NULL)
        return;
    SET_PLAYER(p);
    if (p[0x28] != 2)
        return;
    for (i = D[0x10] - 1; i >= 0; i--) {
        u8 *P = PLAYER();
        if (U32_AT(SAVED(P, i), 0) == 0)
            continue;
        if (D[0x55c + i] == 3) {
            s32 k;
            for (k = 0; k < 0x18; k += 4)
                U32_AT(CHAN(i), k) = U32_AT(SAVED(P, i), k);
            D[0x55c + i] = PLAYER()[0x3b];
        } else {
            u32 t = VOICE(P, i)[0xa];
            if (t != 0xff) {
                u8 *tr = P + t * 0x24;
                if (i == tr[0x44])
                    tr[0x44] = 0xff;
                VOICE(PLAYER(), i)[0xa] = 0xff;
            }
        }
        U32_AT(SAVED(PLAYER(), i), 0) = 0;
    }
    func_02027b9c(PLAYER()[0x3b]);
    func_02027acc(PLAYER()[0x3b]);
    PLAYER()[0x28] = 1;
}

/* 0x02028330: pause song `id`: save and free its channels
 * @difftest $P=ptr:0x1040:4 @020e2bb0:32=$P @020e2bb4:32=$P @020e2bb8:32=$P @020e2624:8=int:1:17 @$P+0x28:8=pick:1,1,0 @$P+0x3b:8=pick:0,1,2 @020e2b70:8=pick:0,1,2,3,5 @020e2b71:8=pick:0,1,2,3,5 @020e2b72:8=pick:0,1,2,3,5 @020e2b73:8=pick:0,1,2,3,5 @020e2b74:8=pick:0,1,2,3,5 @020e2b75:8=pick:0,1,2,3,5 @020e2b76:8=pick:0,1,2,3,5 @020e2b77:8=pick:0,1,2,3,5 @020e2b78:8=pick:0,1,2,3,5 @020e2b79:8=pick:0,1,2,3,5 @020e2b7a:8=pick:0,1,2,3,5 @020e2b7b:8=pick:0,1,2,3,5 @020e2b7c:8=pick:0,1,2,3,5 @020e2b7d:8=pick:0,1,2,3,5 @020e2b7e:8=pick:0,1,2,3,5 @020e2b7f:8=pick:0,1,2,3,5 int:0:3 cases=100 */
void func_02028330(u32 id)
{
    u8 *p = SONG(id);
    s32 i;

    if (p == NULL)
        return;
    SET_PLAYER(p);
    if (p[0x28] != 1)
        return;
    for (i = D[0x10] - 1; i >= 0; i--) {
        u8 *P = PLAYER();
        if (P[0x3b] == D[0x55c + i] && U32_AT(CHAN(i), 0) != 0) {
            s32 k;
            for (k = 0; k < 0x18; k += 4)
                U32_AT(SAVED(P, i), k) = U32_AT(CHAN(i), k);
            U32_AT(CHAN(i), 0) = 0;
            D[0x55c + i] = 3;
        } else {
            U32_AT(SAVED(P, i), 0) = 0;
        }
    }
    PLAYER()[0x28] = 2;
}

/* 0x02028438: start song `song` in player `slot` (0 music, 1 jingle: pauses
 * the music, 2 extra) */
void func_02028438(u32 slot, u32 song, u32 flags)
{
    u8 *p;
    s32 i;

    if (slot != 0)
        flags |= 4;
    switch (slot) {
    case 0:
        func_02027ec4(0);
        func_02027ec4(1);
        break;
    case 1:
        func_02027ec4(1);
        func_02028330(0);
        break;
    case 2:
        func_02027ec4(2);
        break;
    }
    PTR_AT(data_020e2bb0, slot * 4) = func_0207ff70(0x103c, data_020bde68, 0x1c2);
    p = SONG(slot);
    SET_PLAYER(p);
    p[0x3b] = slot;
    for (i = 0; i < 0x40; i++)
        func_02022f68(i & 0xff);
    for (i = 0; i < 0x20; i++)
        func_02022ecc(i & 0xff);
    func_0202752c(song, 1);
    PLAYER()[0x29] = flags;
    PLAYER()[0x2f] |= 0x80;
    if (slot == 0) {
        func_0202268c(PLAYER()[0x27] != 0 ? PLAYER()[0x27] : D[0x11]);
        func_02022748(U16_AT(PLAYER(), 0x24) != 0 ? U16_AT(PLAYER(), 0x24) : 0x8000);
    }
    func_02027b9c(PLAYER()[0x3b]);
    func_02027cec(PLAYER()[0x3b], 0, 0);
    PLAYER()[0x28] = 1;
}

/* 0x020285cc: free the songs that have stopped */
void func_020285cc(void)
{
    s32 i;

    for (i = 2; i >= 0; i--) {
        u8 *p = SONG(i);
        if (p != NULL && p[0x28] == 0) {
            SET_PLAYER(p);
            func_02027ec4(i);
        }
    }
    SET_PLAYER(NULL);
}

/* 0x02028628: set the row callback
 * @difftest u32 cases=10 */
void func_02028628(u32 cb)
{
    U32_AT(data_020e2ba8, 0) = cb;
}

/* 0x02028638: reset the song state
 * @difftest cases=5 */
void func_02028638(void)
{
    MI_CpuFill8(data_020e2ba8, 0, 0x20);
    U32_AT(data_020e2ba8, 0x1c) = 0;
    SET_PLAYER(NULL);
}

/* one decimal digit of func_02028678 */
static inline char *Digit(char *s, s32 *n, s32 place, s32 *d, s32 sep)
{
    if (*n >= place || *d != -1) {
        *d = *n / place;
        *s++ = '0' + *d;
        *n -= *d * place;
        if (sep)
            *s++ = data_020e2bcc[0];
    }
    return s;
}

/* 0x02028678: format `n` with a sign and thousands separators
 * @difftest s32 ptr:24
 * @difftest int:-100000:100000 ptr:24
 * @difftest pick:0,1,-1,999,1000,-1000,1000000,999999999,1000000000,2147483647,-2147483647,-2147483648 ptr:24 cases=40 */
char *func_02028678(s32 n, char *buf)
{
    char *s = buf;
    s32 d = -1;

    if (n < 0) {
        *s++ = '-';
        n = -n;
    } else {
        *s++ = '+';
    }
    s = Digit(s, &n, 1000000000, &d, 1);
    s = Digit(s, &n, 100000000, &d, 0);
    s = Digit(s, &n, 10000000, &d, 0);
    s = Digit(s, &n, 1000000, &d, 1);
    s = Digit(s, &n, 100000, &d, 0);
    s = Digit(s, &n, 10000, &d, 0);
    s = Digit(s, &n, 1000, &d, 1);
    s = Digit(s, &n, 100, &d, 0);
    s = Digit(s, &n, 10, &d, 0);
    s[0] = '0' + n;
    s[1] = 0;
    return buf;
}

/* 0x02028a20: decode string `id` into a shared buffer (NULL on failure)
 * @difftest int:0:0x40 cases=40 */
const char *func_02028a20(u32 id)
{
    if (func_02017b5c(id, data_020e2bd0, 0x400) == 0)
        return (const char *)data_020e2bd0;
    return NULL;
}
