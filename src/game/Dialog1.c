/*
 * Last script commands, data decompression by header, and the start of the
 * dialogue box logic.
 * ARM9 main, 0x0200fb74 - 0x02010210 (12 functions).
 *
 * Compressed blobs start with a u32 header: bits 4-7 type (0 raw, 1 LZ,
 * 2 Huffman, 3 RL, 4/6 game-specific callbacks; bit 3 of the type = 16-bit
 * delta filter afterwards), bits 8-31 size. Dialogue texts are 0x2c-byte
 * records at data_020c3b64 (+4 id, +8 next, +0x14 has choice, +0x18/+0x1c
 * next for yes/no).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcb94[];           /* RTTI: CActor */
extern s8 data_020da9c4;
extern void (*data_020daa4c)(const void *src, void *dst, u32 *size);
extern void (*data_020daa50)(const void *src, void *dst, u32 *size);
extern u8 data_020daac0[];           /* dialogue state */
extern u8 data_020dabf4[], data_020dac6c[], data_020dace4[];
extern void *data_020ebb9c;

void *func_0200917c(s32 group, s32 index);
void func_02000df4(const void *src, void *dst, u32 *size);
void func_02001d0c(const void *src, void *dst, u32 *size);
void func_02010210(void);
void func_02010c0c(void);
void func_02010e64(void);
void func_02011bb8(u32 v);
u32 func_02011d98(void);
u32 func_02011dd0(void);
s32 func_02011e08(void);
s32 func_02011e74(void);
s32 func_02011eac(void);
void func_02011eec(u32 idx);
u32 func_02011efc(void);
s32 func_02011f14(void);
void func_02020440(u32 v);
s32 func_0203ded8(void);
void func_02095044(const void *src, void *dst);
void func_0209511c(const void *src, void *dst);
void func_020951f4(const void *src, void *dst);   /* Thumb */
void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */

/* this file's own functions, called before their definition */
void func_0200fcd0(u16 *buf, u32 size);
void func_0200fe9c(u16 *buf, u32 size);
u32 func_0200ff24(s32 id);
u32 func_02010108(void);

#define LOOKUP(cmd) ((u8 *)func_0200917c((s8)(cmd)[4], (cmd)[5]))
#define TEXT(i) (data_020c3b64 + (i) * 0x2c)

static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

static s32 RoundShl(s32 v, s32 sh)
{
    float f = (float)SHL(v, sh);
    return (s32)(v > 0 ? 0.5f + f : f - 0.5f);
}

/* 0x0200fb74: set an actor's speed
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200fb74(u8 *cmd)
{
    u8 *a0 = LOOKUP(cmd), *a = a0;
    if (a0 != NULL)
        a = func_020a6efc(a0, data_020bcb88, data_020bcb94, -1);
    if (a0 == NULL)
        return 1;
    if (S32_AT(a, 0xc) == 0) {
        s32 v = S16_AT(cmd, 6) * 0x28000;
        S32_AT(a, 0x50) = v / 2;
        S32_AT(a, 0x50) = S32_AT(a, 0x50) >> 4;
    } else {
        S32_AT(a, 0x50) = RoundShl(S16_AT(cmd, 6), 12);
    }
    return 1;
}

/* 0x0200fc38: call an object's virtual slot 13 and wait until it is gone
 * @difftest @020da9c4:8=pick:0,1 $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:6 $C cases=60 */
s32 func_0200fc38(u8 *cmd)
{
    u8 *a = LOOKUP(cmd);
    if (data_020da9c4 == 0) {
        if (a != NULL) {
            data_020da9c4 = 1;
            VCallR1(a, 0x34);
        }
    } else {
        data_020da9c4 = a != NULL;
    }
    return (s8)(data_020da9c4 == 0);
}

/* 0x0200fcc0
 * @difftest */
void *func_0200fcc0(void) { return data_020ebb9c; }

/* 0x0200fcd0: 16-bit delta filter */
void func_0200fcd0(u16 *buf, u32 size) { func_0200fe9c(buf, size); }

/* unpack a blob by its header with the given type-2 decoder */
static u32 Unpack(const u8 *src, void *dst, void (*huff)(const void *, void *))
{
    u32 hdr = U32_AT(src, 0), size;
    size = hdr >> 8;
    switch (BITS(hdr, 4, 4) & ~8u) {
    case 0:
        MI_CpuCopy8(src + 4, dst, U32_AT(src, 0) >> 8);
        break;
    case 1:
        func_02095044(src, dst);
        break;
    case 2:
        if (huff != NULL)
            huff(src, dst);
        break;
    case 3:
        func_020951f4(src, dst);
        break;
    case 4:
        data_020daa4c(src, dst, &size);
        break;
    case 6:
        data_020daa50(src + 4, dst, &size);
        break;
    }
    if (BITS(U32_AT(src, 0), 4, 4) & 8)
        func_0200fcd0(dst, size);
    return U32_AT(src, 0) >> 8;
}

/* 0x0200fcdc: unpack a blob (no Huffman)
 * @difftest $S=ptr:0x80:4 @$S:32=pick:0x1000,0x2000,0x1008,0x800,0x1050 $S ptr:0x80:2 */
u32 func_0200fcdc(const u8 *src, void *dst)
{
    return Unpack(src, dst, NULL);
}

/* 0x0200fdb4: unpack a blob
 * @difftest $S=ptr:0x80:4 @$S:32=pick:0x1000,0x2000,0x1008,0x800,0x1050 $S ptr:0x80:2 */
u32 func_0200fdb4(const u8 *src, void *dst)
{
    return Unpack(src, dst, func_0209511c);
}

/* 0x0200fe9c: 16-bit delta decoding in place
 * @difftest ptr:0x80:2 int:0:0x80 */
void func_0200fe9c(u16 *buf, u32 size)
{
    u32 n = size >> 1, i;
    if (n <= 1)
        return;
    for (i = 1; i < n; i++)
        buf[i] = buf[i] + buf[i - 1];
}

/* 0x0200feec: unpacked size of a blob
 * @difftest ptr:4:4 */
u32 func_0200feec(const u8 *src) { return U32_AT(src, 0) >> 8; }

/* 0x0200fef8: install the game's decompression callbacks
 * @difftest */
void func_0200fef8(void)
{
    data_020daa4c = func_02001d0c;
    data_020daa50 = func_02000df4;
}

/* 0x0200ff24: index of the dialogue text with this id in the current range
 * @difftest s32
 * @difftest int:0:0x400 */
u32 func_0200ff24(s32 id)
{
    u32 i = func_02011dd0(), last = func_02011d98();
    if (i <= last) {
        u8 *r = TEXT(i);
        do {
            if (id == S32_AT(r, 4))
                return i;
            i++;
            r += 0x2c;
        } while (i <= last);
    }
    return i;
}

/* 0x0200ff90: advance the dialogue (button pressed) */
void func_0200ff90(void)
{
    u32 done = 0;
    func_02020440(0x4c);
    if (func_02011e74() != 1) {
        func_02010e64();
        func_02010c0c();
        data_020daac0[0x3d] = 0;
        done = 1;
    } else {
        if (func_02011f14() == 1 && ((s8)data_020daac0[0x40] == 1 || func_0203ded8() != 0)) {
            u8 *rec = TEXT(func_02011efc());
            func_02011eec((u16)func_0200ff24(S32_AT(rec, 0x1c)));
        } else {
            if (func_02011e08() == 1) {
                if (func_02011eac() == 1)
                    func_02020440(0xc7);
                else
                    func_02020440(0xc5);
            }
            func_02011eec(func_02010108());
        }
        if (func_02011efc() <= func_02011d98()) {
            func_02010210();
            done = 1;
        }
    }
    if (done) {
        func_02011bb8(1);
        if (PTR_AT(data_020dace4, 0x10) != NULL)
            U32_AT(PTR_AT(data_020dace4, 0x10), 4) &= ~8u;
        if (PTR_AT(data_020dac6c, 0x10) != NULL)
            U32_AT(PTR_AT(data_020dac6c, 0x10), 4) &= ~8u;
        if (PTR_AT(data_020dabf4, 0x10) != NULL)
            U32_AT(PTR_AT(data_020dabf4, 0x10), 4) &= ~8u;
    } else {
        func_02011bb8(4);
    }
}

/* 0x02010108: index of the text that follows the current one */
u32 func_02010108(void)
{
    if (S32_AT(TEXT(func_02011efc()), 0x14) != 0) {
        if (func_02011eac() == 1)
            return (u16)func_0200ff24(S32_AT(TEXT(func_02011efc()), 0x18));
        return (u16)func_0200ff24(S32_AT(TEXT(func_02011efc()), 0x1c));
    }
    if (S32_AT(TEXT(func_02011efc()), 8) == 0)
        return (u16)(U32_AT(data_020daac0, 4) + 1);
    return (u16)func_0200ff24(S32_AT(TEXT(func_02011efc()), 8));
}
