/*
 * A bit/byte stream over a buffer (0x24 bytes): +0 data, +4 end,
 * +8 current byte, +0xc current bit (0-7), +0xe size, +0x14 bytes read,
 * +0x18 owns the buffer, +0x20 written to.
 * ARM9 main, 0x02051300 - 0x02051634 (14 functions).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const char data_020c04b8[];   /* source file name */

void MI_CpuCopy8(const void *src, void *dest, u32 size);
void MI_CpuFill8(void *dest, u8 data, u32 size);
void *func_0207ff48(u32 size, const char *file, s32 line);
void func_0207ff14(void *ptr, const char *file, s32 line);

void func_02051318(u8 *bs, s32 off, u32 whence);
void func_020514cc(u8 *bs);

#define CUR(bs) (*(u8 **)((bs) + 8))

/* 0x02051300: the current bit
 * @difftest ptr:0x24:4 cases=3 */
u8 func_02051300(u8 *bs)
{
    return bs[0xc];
}

/* 0x02051308: bytes from the start to the current one
 * @difftest ptr:0x24:4 cases=3 */
s32 func_02051308(u8 *bs)
{
    return (u8 *)PTR_AT(bs, 8) - (u8 *)PTR_AT(bs, 0);
}

/* 0x02051318: seek by `off` bytes from the start (0), here (1) or the end (2)
 * @difftest ptr:0x24:4 int:-0x100:0x100 int:0:4 cases=20 */
void func_02051318(u8 *bs, s32 off, u32 whence)
{
    bs[0xc] = 0;
    if (whence == 0)
        PTR_AT(bs, 8) = (u8 *)PTR_AT(bs, 0) + off;
    else if (whence == 1)
        PTR_AT(bs, 8) = (u8 *)PTR_AT(bs, 8) + off;
    else if (whence == 2)
        PTR_AT(bs, 8) = (u8 *)PTR_AT(bs, 4) + off;
}

/* 0x02051368: the size
 * @difftest ptr:0x24:4 cases=3 */
u16 func_02051368(u8 *bs)
{
    return U16_AT(bs, 0xe);
}

/* 0x02051370: write bit `bit`
 * @difftest $D=ptr:4:4 $B=zero:0x24 @$B+8:32=$D @$B+0xc:8=int:0:8 $B pick:0,1,2 cases=20 */
void func_02051370(u8 *bs, u32 bit)
{
    if (bit == 1)
        *CUR(bs) |= 1 << bs[0xc];
    else
        *CUR(bs) &= ~(1 << bs[0xc]);
    func_020514cc(bs);
    bs[0x20] = 1;
}

/* 0x020513d0: write `n` bytes
 * @difftest $D=ptr:0x20:4 $B=zero:0x24 @$B+8:32=$D @$B+0xc:8=pick:0,3 $B ptr:8 int:0:8 cases=20 */
void func_020513d0(u8 *bs, const void *src, u32 n)
{
    if (n == 0)
        return;
    if (bs[0xc] != 0)
        func_02051318(bs, 1, 1);
    MI_CpuCopy8(src, PTR_AT(bs, 8), n);
    PTR_AT(bs, 8) = (u8 *)PTR_AT(bs, 8) + n;
    bs[0x20] = 1;
}

/* 0x02051430: read a bit
 * @difftest $D=ptr:4:4 $B=zero:0x24 @$B+8:32=$D @$B+0xc:8=int:0:8 $B cases=20 */
s32 func_02051430(u8 *bs)
{
    u8 b = *CUR(bs) & (1 << bs[0xc]);

    func_020514cc(bs);
    return b != 0;
}

/* 0x02051464: read `n` bytes
 * @difftest $D=ptr:0x20:4 $B=zero:0x24 @$B:32=$D @$B+8:32=$D+4 @$B+0xc:8=pick:0,3 $B ptr:8 int:0:8 cases=20 */
void func_02051464(u8 *bs, void *dst, u32 n)
{
    if (n == 0)
        return;
    if (bs[0xc] != 0)
        func_02051318(bs, 1, 1);
    MI_CpuCopy8(PTR_AT(bs, 8), dst, n);
    PTR_AT(bs, 8) = (u8 *)PTR_AT(bs, 8) + n;
    S32_AT(bs, 0x14) = (u8 *)PTR_AT(bs, 8) - (u8 *)PTR_AT(bs, 0);
}

/* 0x020514cc: to the next bit
 * @difftest ptr:0x24:4 cases=20 */
void func_020514cc(u8 *bs)
{
    bs[0xc]++;
    bs[0xc] &= 7;
    if (bs[0xc] == 0)
        PTR_AT(bs, 8) = (u8 *)PTR_AT(bs, 8) + 1;
}

/* 0x02051504: clear the buffer and go back to the start
 * @difftest $D=ptr:0x20:4 $B=ptr:0x24:4 @$B:32=$D @$B+0xe:16=int:0:0x20 $B cases=5 */
void func_02051504(u8 *bs)
{
    MI_CpuFill8(PTR_AT(bs, 0), 0, U16_AT(bs, 0xe));
    PTR_AT(bs, 8) = PTR_AT(bs, 0);
    bs[0xc] = 0;
    U32_AT(bs, 0x10) = 0;
    U32_AT(bs, 0x14) = 0;
    U32_AT(bs, 0x1c) = 0;
    U16_AT(bs, 0x22) = 0;
    bs[0x20] = 0;
}

/* 0x02051548: the buffer
 * @difftest ptr:0x24:4 cases=3 */
void *func_02051548(u8 *bs)
{
    return PTR_AT(bs, 0);
}

/* 0x02051550: constructor: a new buffer of `size` bytes
 * @difftest ptr:0x24:4 int:0:0x100 stub=0x0207ff48:0x1234 cases=3 */
void func_02051550(u8 *bs, u32 size)
{
    PTR_AT(bs, 0) = func_0207ff48(size, data_020c04b8, 0x33);
    PTR_AT(bs, 4) = (u8 *)PTR_AT(bs, 0) + size;
    PTR_AT(bs, 8) = PTR_AT(bs, 0);
    bs[0xc] = 0;
    U16_AT(bs, 0xe) = size;
    U32_AT(bs, 0x10) = 0;
    U32_AT(bs, 0x14) = 0;
    U16_AT(bs, 0x22) = 0;
    bs[0x20] = 0;
    U32_AT(bs, 0x1c) = 0;
    bs[0x18] = 1;
}

/* 0x020515c0: destructor
 * @difftest $B=ptr:0x24:4 @$B+0x18:8=pick:0,1 $B stub=0x0207ff14 cases=5 */
u8 *func_020515c0(u8 *bs)
{
    if (S8_AT(bs, 0x18) != 0) {
        func_0207ff14(PTR_AT(bs, 0), data_020c04b8, 0x22);
        PTR_AT(bs, 0) = NULL;
    }
    return bs;
}

/* 0x020515fc: constructor: no buffer
 * @difftest ptr:0x24:4 cases=3 */
u8 *func_020515fc(u8 *bs)
{
    U32_AT(bs, 0) = 0;
    U32_AT(bs, 4) = 0;
    U32_AT(bs, 8) = 0;
    bs[0xc] = 0;
    U16_AT(bs, 0xe) = 0;
    U32_AT(bs, 0x10) = 0;
    U32_AT(bs, 0x14) = 0;
    bs[0x18] = 0;
    U32_AT(bs, 0x1c) = 0;
    bs[0x20] = 1;
    U16_AT(bs, 0x22) = 0;
    return bs;
}
