/*
 * Song.cpp, part 1: loading a song for the music player from the sound
 * data file: its patterns, samples and instruments (sharing note maps and
 * envelopes between instruments that use the same ones).
 * ARM9 main, 0x02026758 - 0x0202752c (3 functions).
 *
 * `hdr` is the sound data file's header: tables of offsets (+0x04
 * instruments, +0x08 samples, +0x0c patterns, +0x10 sample data, +0x14
 * envelopes, +0x18/+0x1c note maps, +0x20/+0x24 envelope times and
 * values). With `alloc` 0 everything goes into the song memory pool,
 * data_020e2ba8 + 0x04 (base) / + 0x14 (used), otherwise into the heap.
 * The player's +0x31/+0x32/+0x33 are the instrument, sample and pattern
 * counts (see SeqPlayer1.c for the rest of its layout).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const char data_020bde68[];   /* "Song.cpp" */
extern u8 data_020e2ba4[];
extern u8 data_020e2ba8[];

u32 func_020803ac(void *file);                       /* tell */
void func_020803d0(void *file, u32 pos, u32 whence); /* seek */

#define PLAYER() ((u8 *)PTR_AT(data_020e2ba4, 0))
#define POOL_NEXT() (U32_AT(data_020e2ba8, 4) + U32_AT(data_020e2ba8, 0x14))
#define POOL_USE(n) (U32_AT(data_020e2ba8, 0x14) += (n))

/* go to the entry `i` of an offset table of the file */
static void Seek(void *file, u32 table, u32 i)
{
    u32 off;
    func_020803d0(file, table + i * 4, 0);
    func_02080418(&off, 4, 1, file);
    func_020803d0(file, off, 0);
}

/* 0x02026758: load the patterns */
void func_02026758(void *file, u8 *hdr, s32 alloc)
{
    u16 idx[0x194 / 2];
    u32 off, len;
    s32 i;

    func_02080418(idx, PLAYER()[0x33] << 1, 1, file);
    if (alloc != 0) {
        PTR_AT(PLAYER(), 0xc) = func_0207ff70(PLAYER()[0x33] << 2, data_020bde68, 0x652);
        U32_AT(PLAYER(), 0x10) = 0;
    } else {
        U32_AT(PLAYER(), 0xc) = POOL_NEXT();
        U32_AT(data_020e2ba8, 0x14) = U32_AT(data_020e2ba8, 0x14) + (PLAYER()[0x33] << 2);
    }
    for (i = 0; i < PLAYER()[0x33]; i++) {
        func_020803d0(file, U32_AT(hdr, 0xc) + idx[i] * 4, 0);
        func_02080418(&off, 4, 1, file);
        func_020803d0(file, off, 0);
        func_02080418(&len, 4, 1, file);
        if (alloc != 0) {
            ((void **)PTR_AT(PLAYER(), 0xc))[i] = func_0207ff70(len + 1, data_020bde68, 0x669);
        } else {
            ((u32 *)PTR_AT(PLAYER(), 0xc))[i] = POOL_NEXT();
            U32_AT(data_020e2ba8, 0x14) = U32_AT(data_020e2ba8, 0x14) + ((len + 4) & ~3u);
        }
        func_02080418(((void **)PTR_AT(PLAYER(), 0xc))[i], len + 1, 1, file);
    }
}

/* 0x02026938: load the samples */
void func_02026938(void *file, u8 *hdr, s32 alloc)
{
    u16 idx[0xc8 / 2];
    u8 rec[0x24];
    u32 pos, size;
    s32 i;

    func_02080418(idx, PLAYER()[0x32] << 1, 1, file);
    pos = func_020803ac(file);
    if (alloc != 0) {
        PTR_AT(PLAYER(), 0) = func_0207ff70((PLAYER()[0x32] + 1) * 0x24, data_020bde68, 0x5e4);
    } else {
        U32_AT(PLAYER(), 0) = POOL_NEXT();
        U32_AT(data_020e2ba8, 0x14) =
            U32_AT(data_020e2ba8, 0x14) + (((PLAYER()[0x32] + 1) * 0x24 + 3) & ~3u);
    }
    MI_CpuFill8(PTR_AT(PLAYER(), 0), 0, (PLAYER()[0x32] + 1) * 0x24);
    for (i = 1; i < PLAYER()[0x32] + 1; i++) {
        u8 *s = (u8 *)PTR_AT(PLAYER(), 0) + i * 0x24;
        u32 k;
        if (idx[i - 1] == 0xffff) {
            s[4] = 0;
            s[5] = 0;
            U32_AT(s, 8) = 0;
            U32_AT(s, 0) = 0;
            continue;
        }
        Seek(file, U32_AT(hdr, 8), idx[i - 1]);
        func_02080418(rec, 0x24, 1, file);
        s[4] = rec[4];
        s[5] = rec[5];
        s[6] = rec[6];
        s[7] = rec[7];
        for (k = 8; k < 0x20; k += 4)
            U32_AT(s, k) = U32_AT(rec, k);
        s[0x20] = rec[0x20];
        s[0x21] = rec[0x21];
        s[0x22] = rec[0x22];
        s[0x23] = rec[0x23];
        Seek(file, U32_AT(hdr, 0x10), U16_AT(rec, 0));
        if (s[5] & 2) {
            size = ((U32_AT(s, 8) + 1) * 2 + 3) & ~3u;
            if (alloc != 0) {
                PTR_AT(s, 0) = func_0207ff70(size, data_020bde68, 0x61f);
            } else {
                U32_AT(s, 0) = POOL_NEXT();
                POOL_USE((size + 3) & ~3u);
            }
            func_02080418(PTR_AT(s, 0), size, 1, file);
            U32_AT(s, 0) = (U32_AT(s, 0) >> 1) | 0x80000000;
        } else {
            size = (U32_AT(s, 8) + 4) & ~3u;
            if (alloc != 0) {
                PTR_AT(s, 0) = func_0207ff70(size, data_020bde68, 0x62b);
            } else {
                U32_AT(s, 0) = POOL_NEXT();
                POOL_USE((size + 3) & ~3u);
            }
            func_02080418(PTR_AT(s, 0), size, 1, file);
        }
    }
    func_020803d0(file, pos, 0);
}

/* 0x02026cdc: load the instruments */
void func_02026cdc(void *file, u8 *hdr, s32 alloc)
{
    u16 idx[100], mapA[100], mapB[100], envid[100][3];
    u8 rec[0x16], eh[0xa];
    u32 pos;
    s32 i, j, k;

    func_02080418(idx, PLAYER()[0x31] << 1, 1, file);
    pos = func_020803ac(file);
    for (i = 0; i < 100; i++) {
        mapA[i] = 0xffff;
        mapB[i] = 0xffff;
        for (k = 0; k < 3; k++)
            envid[i][k] = 0xffff;
    }
    if (alloc != 0) {
        PTR_AT(PLAYER(), 4) = func_0207ff70((PLAYER()[0x31] + 1) << 5, data_020bde68, 0x4fe);
    } else {
        U32_AT(PLAYER(), 4) = POOL_NEXT();
        U32_AT(data_020e2ba8, 0x14) =
            U32_AT(data_020e2ba8, 0x14) + ((((PLAYER()[0x31] + 1) << 5) + 3) & ~3u);
    }
    MI_CpuFill8(PTR_AT(PLAYER(), 4), 0, (PLAYER()[0x31] + 1) << 5);
    for (i = 1; i < PLAYER()[0x31] + 1; i++) {
        u8 *in;
        Seek(file, U32_AT(hdr, 4), idx[i - 1]);
        func_02080418(rec, 0x16, 1, file);
        in = (u8 *)PTR_AT(PLAYER(), 4) + i * 0x20;
        in[0] = rec[0];
        in[1] = rec[1];
        in[2] = rec[2];
        U16_AT(in, 4) = U16_AT(rec, 4);
        in[6] = rec[6];
        in[7] = rec[7];
        in[8] = rec[8];
        in[9] = rec[9];
        in[0xa] = rec[0xa];
        in[0xb] = rec[0xb];
        mapA[i] = U16_AT(rec, 0xc);
        mapB[i] = U16_AT(rec, 0xe);
        for (k = 0; k < 3; k++)
            envid[i][k] = U16_AT(rec, 0x10 + k * 2);

        U32_AT(in, 0xc) = 0;
        for (j = i - 1; j >= 1; j--) {
            if (mapA[i] == mapA[j]) {
                U32_AT(in, 0xc) = U32_AT((u8 *)PTR_AT(PLAYER(), 4) + j * 0x20, 0xc);
                break;
            }
        }
        if (U32_AT(in, 0xc) == 0) {
            in[3] |= 1;
            if (alloc != 0) {
                PTR_AT(in, 0xc) = func_0207ff70(0x78, data_020bde68, 0x53f);
            } else {
                U32_AT(in, 0xc) = POOL_NEXT();
                U32_AT(data_020e2ba8, 0x14) = U32_AT(data_020e2ba8, 0x14) + 0x78;
            }
            Seek(file, U32_AT(hdr, 0x18), mapA[i]);
            func_02080418(PTR_AT(in, 0xc), 0x78, 1, file);
        }

        U32_AT(in, 0x10) = 0;
        for (j = i - 1; j >= 1; j--) {
            if (mapB[i] == mapB[j]) {
                U32_AT(in, 0x10) = U32_AT((u8 *)PTR_AT(PLAYER(), 4) + j * 0x20, 0x10);
                break;
            }
        }
        if (U32_AT(in, 0x10) == 0) {
            in[3] |= 2;
            if (alloc != 0) {
                PTR_AT(in, 0x10) = func_0207ff70(0x78, data_020bde68, 0x561);
            } else {
                U32_AT(in, 0x10) = POOL_NEXT();
                U32_AT(data_020e2ba8, 0x14) = U32_AT(data_020e2ba8, 0x14) + 0x78;
            }
            Seek(file, U32_AT(hdr, 0x1c), mapB[i]);
            func_02080418(PTR_AT(in, 0x10), 0x78, 1, file);
        }

        for (k = 2; k >= 0; k--) {
            u8 *env;
            U32_AT(in, 0x14 + k * 4) = 0;
            if (envid[i][k] == 0xffff)
                continue;
            for (j = i - 1; j >= 1; j--) {
                s32 m;
                for (m = 2; m >= 0; m--) {
                    if (envid[i][k] == envid[j][m]) {
                        U32_AT(in, 0x14 + k * 4) =
                            U32_AT((u8 *)PTR_AT(PLAYER(), 4) + j * 0x20 + m * 4, 0x14);
                        m = 0;
                        j = 0;
                    }
                }
            }
            if (U32_AT(in, 0x14 + k * 4) != 0)
                continue;
            in[3] |= 4 << k;
            Seek(file, U32_AT(hdr, 0x14), envid[i][k]);
            func_02080418(eh, 0xa, 1, file);
            if (alloc != 0) {
                PTR_AT(in, 0x14 + k * 4) = func_0207ff70((eh[1] + 1) * 3 + 0x10, data_020bde68, 0x599);
            } else {
                U32_AT(in, 0x14 + k * 4) = POOL_NEXT();
                U32_AT(data_020e2ba8, 0x14) =
                    U32_AT(data_020e2ba8, 0x14) + (((eh[1] + 1) * 3 + 0x13) & ~3u);
            }
            env = PTR_AT(in, 0x14 + k * 4);
            env[8] = eh[0];
            env[9] = eh[1];
            env[0xa] = eh[2];
            env[0xb] = eh[3];
            env[0xc] = eh[4];
            env[0xd] = eh[5];
            PTR_AT(env, 0) = env + 0x10;
            PTR_AT(env, 4) = (u8 *)PTR_AT(env, 0) + env[9] * 2 + 2;
            Seek(file, U32_AT(hdr, 0x20), U16_AT(eh, 6));
            func_02080418(PTR_AT(env, 0), env[9] << 1, 1, file);
            ((u16 *)PTR_AT(env, 0))[env[9]] = ((u16 *)PTR_AT(env, 0))[env[9] - 1] + 1;
            Seek(file, U32_AT(hdr, 0x24), U16_AT(eh, 8));
            func_02080418(PTR_AT(env, 4), env[9], 1, file);
            ((s8 *)PTR_AT(env, 4))[env[9]] = ((s8 *)PTR_AT(env, 4))[env[9] - 1];
        }
    }
    func_020803d0(file, pos, 0);
}
