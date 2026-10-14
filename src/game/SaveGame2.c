/*
 * Saving and loading games (part 2): the sections of the save stream, the
 * options block and the slot headers.
 * ARM9 main, 0x02042f60 - 0x0204378c (25 functions).
 *
 * The stream (data_020ebbc8, +0x1c set while loading) holds in turn: the
 * levels visited (a count byte, then one bit per level), the game state
 * (func_02008b04) and the inventory (func_0205b274), each of the latter
 * two after its 16-bit length, then the levels' data. The options block
 * (data_020ebbb8, 0x10 bytes: +8 / +9 the volumes, +0xe checksum) is valid
 * when it sums to 0 and starts like data_020b6894. data_020ebba8 counts
 * the used slots.
 *
 * Testing: with the card routines stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const u16 data_020b68a4[], data_020b68d8[];   /* per difficulty */
extern const u8 data_020b6894[];     /* the options block's signature / defaults */
extern u8 data_020def74[];           /* the inventory */
extern u8 data_020ebba4[];           /* level and area to save */
extern s32 data_020ebba8;            /* used slots */
extern u8 data_020ebbb8[];           /* options block (0x10 bytes) */
extern u8 data_020ebbc8[];           /* the save stream */
extern u8 data_020ebc40[];           /* the slot headers, 0x20 bytes each */

void func_02051318(void *stream, s32 pos, s32 whence);
void func_02051464(void *stream, void *buf, u32 n);
void func_020513d0(void *stream, const void *buf, u32 n);
u32 func_02051308(void *stream);
s32 func_02051300(void *stream);
void func_02051370(void *stream, s32 v);
void func_02051550(void *stream, u32 size);
void func_02008b04(void *stream);
void func_0200925c(s32 handle);
void func_0205b274(void *inv, void *stream, s32 a);
s32 func_02080bd0(const void *a, const void *b);
void *func_0207fe5c(u32 size);
void func_0207fe2c(void *p);
s32 func_0201af68(u32 slot, void *hdr);
s32 func_0201af88(u32 slot, void *hdr);
s32 func_0201afc0(void *opts);
void func_0201afd8(void);
s32 func_0201b254(void);
u32 func_0201b264(void);
void func_02020504(u32 v);
void func_020204d8(u32 v);
void func_02042b6c(u32 slot, u32 level);
s32 func_02042b24(void);

void func_02042ffc(void);
void func_020430b8(void);
void func_0204314c(void);
void func_02043280(void);
void func_020432b0(void);
void func_020433c0(void);
void func_020433fc(void);
s32 func_020434d0(const u16 *opts);
void func_020435f8(void);

#define STREAM data_020ebbc8
#define SLOT() func_020436f0(data_020bf6a0)
#define LEVEL(h) ((u32)(h)[0xf] >> 1)

static inline u16 Sum16(const u16 *p, s32 n)
{
    u16 s = 0;

    while (n > 0) {
        s += *p++;
        n--;
    }
    return s;
}

/* 0x02042f60: the levels visited: write them (save != 0), mark the
 * current one (not loading) or check them (loading)
 * @difftest pick:0,1 @020ebbe4:32=pick:0,1 @020bf6a0:32=int:0:2 stub=0x02042b6c,0x0200925c cases=10 */
void func_02042f60(s32 save)
{
    if (save != 0) {
        func_0204314c();
        return;
    }
    if (S32_AT(STREAM, 0x1c) == 0) {
        func_020430b8();
        return;
    }
    func_02042ffc();
}

/* 0x02042fc4: skip the levels visited
 * @difftest cases=3 */
void func_02042fc4(void)
{
    u8 n = 0;

    func_02051464(STREAM, &n, 1);
    func_02051318(STREAM, n, 1);
}

/* 0x02042ffc: check the levels visited (none: start a new game)
 * @difftest @020bf6a0:32=int:0:2 stub=0x02042b6c,0x0200925c cases=5 */
void func_02042ffc(void)
{
    u8 n = 0;

    func_02051464(STREAM, &n, 1);
    if (n == 0) {
        func_02042b6c(0, LEVEL(SLOT()));
        func_0200925c(S32_AT(data_020c3b60 + LEVEL(SLOT()) * 0x60, 0x18));
        func_02051318(STREAM, 0, 0);
        func_02051464(STREAM, &n, 1);
    }
    func_02051318(STREAM, n, 1);
}

/* 0x020430b8: mark the current level as visited
 * @difftest @020bf6a0:32=int:0:2 cases=5 */
void func_020430b8(void)
{
    u8 n = 0;
    u8 *p;

    func_02051464(STREAM, &n, 1);
    p = func_0207fe5c(n);
    func_02051464(STREAM, p, n);
    p[0] |= 1 << LEVEL(SLOT());
    func_02051318(STREAM, 1, 0);
    func_020513d0(STREAM, p, n);
    func_0207fe2c(p);
}

/* 0x0204314c: write the levels visited (only the current one)
 * @difftest @020bf6a0:32=int:0:2 @020ebc4f:8=int:0:0x52 cases=10 */
void func_0204314c(void)
{
    u8 n = 6;
    s32 i;

    func_020513d0(STREAM, &n, 1);
    for (i = 0; i < 0x29; i++) {
        if ((u32)i == LEVEL(SLOT()))
            func_02051370(STREAM, 1);
        else
            func_02051370(STREAM, 0);
    }
    if (func_02051300(STREAM) == 0)
        return;
    func_02051318(STREAM, 1, 1);
}

/* 0x020431fc: the game state: write it, or read it while loading
 * @difftest @020ebbe4:32=pick:0,1 stub=0x02008b04 cases=5 */
void func_020431fc(void)
{
    if (S32_AT(STREAM, 0x1c) == 0) {
        func_020432b0();
        return;
    }
    func_02043280();
}

/* 0x02043248: skip the game state
 * @difftest cases=3 */
void func_02043248(void)
{
    u16 n = 0;

    func_02051464(STREAM, &n, 2);
    func_02051318(STREAM, n, 1);
}

/* 0x02043280: read the game state
 * @difftest stub=0x02008b04 cases=3 */
void func_02043280(void)
{
    u16 n;

    func_02051464(STREAM, &n, 2);
    func_02008b04(STREAM);
}

/* 0x020432b0: write the game state after its length
 * @difftest stub=0x02008b04 cases=3 */
void func_020432b0(void)
{
    u16 start = func_02051308(STREAM);
    u16 base, len;

    func_02051318(STREAM, 2, 1);
    base = func_02051308(STREAM);
    func_02008b04(STREAM);
    len = func_02051308(STREAM) - base;
    func_02051318(STREAM, start, 0);
    func_020513d0(STREAM, &len, 2);
    func_02051318(STREAM, len, 1);
}

/* 0x0204333c: the inventory: write it, or read it while loading
 * @difftest @020ebbe4:32=pick:0,1 stub=0x0205b274 cases=5 */
void func_0204333c(void)
{
    if (S32_AT(STREAM, 0x1c) == 0) {
        func_020433fc();
        return;
    }
    func_020433c0();
}

/* 0x02043388: skip the inventory
 * @difftest cases=3 */
void func_02043388(void)
{
    u16 n = 0;

    func_02051464(STREAM, &n, 2);
    func_02051318(STREAM, n, 1);
}

/* 0x020433c0: read the inventory
 * @difftest stub=0x0205b274 cases=3 */
void func_020433c0(void)
{
    u16 n;

    func_02051464(STREAM, &n, 2);
    func_0205b274(data_020def74, STREAM, 0);
}

/* 0x020433fc: write the inventory after its length
 * @difftest stub=0x0205b274 cases=3 */
void func_020433fc(void)
{
    u16 start = func_02051308(STREAM);
    u16 base, len;

    func_02051318(STREAM, 2, 1);
    base = func_02051308(STREAM);
    func_0205b274(data_020def74, STREAM, 0);
    len = func_02051308(STREAM) - base;
    func_02051318(STREAM, start, 0);
    func_020513d0(STREAM, &len, 2);
    func_02051318(STREAM, len, 1);
}

/* 0x02043494: is a slot header valid (its checksum)?
 * @difftest ptr:0x20:2 cases=5
 * @difftest zero:0x20 cases=2 */
s32 func_02043494(const u16 *hdr)
{
    return Sum16(hdr, 0x10) == 0;
}

/* 0x020434d0: is the options block valid?
 * @difftest ptr:0x10:2 cases=5
 * @difftest zero:0x10 cases=2 */
s32 func_020434d0(const u16 *opts)
{
    if (Sum16(opts, 8) != 0)
        return 0;
    return func_02080bd0(opts, data_020b6894) == 0;
}

/* 0x02043540: start the save system (stream, options, slots)
 * @difftest stub=0x0201b254:0,0x0201afd8,0x0201b264:0x2000,0x0201afc0,0x0201afa8,0x0201af68,0x0201af88 cases=3
 * @difftest stub=0x0201b254:1,0x0201b264:0x1000,0x0201afc0,0x0201afa8,0x0201af68,0x0201af88 cases=3 */
void func_02043540(void)
{
    if (func_0201b254() == 0)
        func_0201afd8();
    func_02051550(STREAM, func_0201b264());
    func_0201afc0(data_020ebbb8);
    if (func_020434d0((const u16 *)data_020ebbb8) == 0) {
        u8 buf[0x20];
        u32 i;

        MI_CpuCopy8(data_020b6894, data_020ebbb8, 0x10);
        func_02042b24();
        MI_CpuFill8(buf, 0xf, 0x20);
        for (i = 0; i < 3; i++)
            func_0201af68(i, buf);
    }
    func_020435f8();
    func_02020504(data_020ebbb8[8]);
    func_020204d8(data_020ebbb8[9]);
}

/* 0x020435f8: read the slot headers and count the used slots
 * @difftest stub=0x0201af88 cases=3 */
void func_020435f8(void)
{
    s32 i;

    data_020ebba8 = 0;
    for (i = 0; i < 3; i++) {
        func_0201af88(i, func_020436f0(i));
        if (Sum16((const u16 *)func_020436f0(i), 0x10) == 0)
            data_020ebba8++;
    }
}

/* 0x02043680: is slot `slot` used?
 * @difftest int:0:2 cases=3
 * @difftest int:0:2 @020ebc40:16=int:0:0xffff cases=5 */
s32 func_02043680(u32 slot)
{
    return Sum16((const u16 *)func_020436f0(slot), 0x10) == 0;
}

/* 0x020436d0: the area to save
 * @difftest cases=3 */
u32 func_020436d0(void)
{
    return data_020ebba4[1];
}

/* 0x020436e0: the level to save
 * @difftest cases=3 */
u32 func_020436e0(void)
{
    return data_020ebba4[0];
}

/* 0x020436f0: a slot's header
 * @difftest int:0:2 cases=3 */
u8 *func_020436f0(u32 id)
{
    return data_020ebc40 + (id << 5);
}

/* 0x02043700
 * @difftest cases=3 */
u32 func_02043700(void)
{
    return 0x12c;
}

/* 0x02043708
 * @difftest cases=3 */
u32 func_02043708(void)
{
    return 0x12c;
}

/* 0x02043710: the current difficulty's value from data_020b68d8
 * @difftest @020bf6a0:32=int:0:2 cases=5 */
u16 func_02043710(void)
{
    return data_020b68d8[SLOT()[0x12]];
}

/* 0x02043748: the current difficulty's value from data_020b68a4
 * @difftest @020bf6a0:32=int:0:2 cases=5 */
u16 func_02043748(void)
{
    return data_020b68a4[SLOT()[0x12]];
}

/* 0x02043780: read the slot headers again
 * @difftest stub=0x0201af88 cases=3 */
void func_02043780(void)
{
    func_020435f8();
}
