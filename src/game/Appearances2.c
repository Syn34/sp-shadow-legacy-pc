/*
 * Level object tables ("appearances"), part 2: saving/loading object state,
 * loading the tables of a level, and card backup wrappers.
 * ARM9 main, 0x02008b04 - 0x02009548 (24 functions).
 *
 * Source file "Appearances.cpp" (func_02009324). The last functions
 * (0x020093a4 on) lock the card bus around backup memory access and may
 * belong to the next file.
 *
 * Stream functions (save data): func_02051370 write byte, func_020513d0
 * write bytes, func_02051430 read byte, func_02051464 read bytes,
 * func_02051308 tell, func_02051318 seek(pos, whence), func_02051300 error.
 * A stream with +0x1c == 0 is being written.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 *data_020d9f8c;
extern u8 *data_020d9f90;
extern u8 *data_020d9f94;
extern u8 *data_020d9f98;
extern u8 data_020d9f9c[];           /* [0]: level; [0..0x1ff] object flags + 0x30 bytes (saved) */
extern u32 data_020da1dc;            /* card lock id */
extern void *data_020f62b4;
extern const char data_020bcbfc[];
extern const char data_020bcc0c[];   /* "Appearances.cpp" */
extern const char data_020bcc64[], data_020bcc90[];
extern void *(*data_020bcc1c[])(u32 group, u32 index);

void func_02051370(void *stream, u32 v);
void func_020513d0(void *stream, const void *buf, u32 n);
s32 func_02051430(void *stream);
void func_02051464(void *stream, void *buf, u32 n);
s32 func_02051300(void *stream);
u32 func_02051308(void *stream);
void func_02051318(void *stream, s32 pos, s32 whence);
void *func_0207fe5c(u32 size);
void func_0207fe2c(void *p);
void func_020506a0(void *actor, s32 group, s32 index);
void *func_02007d9c(void *actor);
void *func_02007d78(void *actor);
u32 func_020081d8(void);
s32 func_020089dc(s32 i);
void func_02008a30(s32 i);
u8 *func_02008804(u8 *src);
u8 *func_020086f0(u8 *a, u8 *b, u8 *dst);
u8 *func_020084e4(u8 *a, u8 *b, u8 *dst);
u8 *func_0200833c(u8 *src, u8 *tbl, u8 *dst);
void func_0202068c(void);
void func_020206c8(void);
u32 func_02091f00(void);
void func_02091f58(u32 id);
void func_0209c10c(u32 id);
void func_0209c11c(u32 id);
s32 func_0209c16c(void);
void func_0209c750(u32 v);
u32 func_0209c7f8(u32 a, u32 c, u32 b, u32 d, u32 e, u32 f);
u32 func_0209c8cc(u32 a, u32 c, u32 b, u32 d, u32 e, u32 f);
void func_0209cbd0(void);

/* this file's own functions, called before their definition */
void func_02008b48(void *stream);
void func_02008ba4(void *stream);
void func_02008c40(void *stream, s32 full);
void func_02008d2c(s32 group, void *stream, s32 full);
void func_02008f90(u8 *rec, s32 group, s32 index, void *stream);
s32 func_02009044(s32 type, const u8 *data);
void func_020090f8(u8 *rec, void *stream);
void func_020093a4(void);
void func_020093e8(void);

#define ENTRY(t, i) ((t) + U16_AT(t, 2 + (i) * 2))
#define GROUP_COUNT(g) U16_AT(data_020d9f90, U16_AT(data_020d9f90, (g) * 2 + 2))

/* 0x02008b04: save or load the object flags */
void func_02008b04(void *stream)
{
    if (S32_AT(stream, 0x1c) == 0)
        func_02008ba4(stream);
    else
        func_02008b48(stream);
}

/* 0x02008b48: load the object flags
 * @difftest $D=ptr:0x200:4 $S=zero:0x20 @$S:32=$D @$S+8:32=$D @$S+0x1c:32=1 $S cases=50 */
void func_02008b48(void *stream)
{
    u8 *p = data_020d9f9c;
    s32 i;
    for (i = 0; i < 0x200; i++)
        *p++ = func_02051430(stream) == 1 ? 1 : 0;
    func_02051464(stream, data_020d9f9c + i, 0x30);
}

/* 0x02008ba4: save the object flags */
void func_02008ba4(void *stream)
{
    u8 *p = data_020d9f9c;
    s32 i;
    for (i = 0; i < 0x200; i++)
        func_02051370(stream, *p++ != 0 ? 1 : 0);
    func_020513d0(stream, data_020d9f9c + i, 0x30);
}

/* 0x02008c04: save the state of all object groups (when writing) */
void func_02008c04(void *stream, s32 full)
{
    if (S32_AT(stream, 0x1c) == 0)
        func_02008c40(stream, full);
}

/* 0x02008c40: write the object groups with an offset table */
void func_02008c40(void *stream, s32 full)
{
    u32 start, end, i;
    u16 *offs;
    func_020513d0(stream, data_020d9f90, 2);
    start = (u16)func_02051308(stream);
    func_02051318(stream, func_020081d8() * 2, 1);
    offs = func_0207fe5c(func_020081d8() * 2);
    for (i = 0; i < func_020081d8(); i++) {
        offs[i] = func_02051308(stream);
        func_02008d2c(i & 0xff, stream, full);
    }
    end = (u16)func_02051308(stream);
    func_02051318(stream, start, 0);
    func_020513d0(stream, offs, func_020081d8() * 2);
    func_02051318(stream, end, 0);
    func_0207fe2c(offs);
}

/* 0x02008d2c: write the state of the objects of one group */
void func_02008d2c(s32 group, void *stream, s32 full)
{
    s32 g = (s8)group;
    u32 k = 0;
    if ((s32)GROUP_COUNT(g) > 0) {
        do {
            u8 *e = ENTRY(data_020d9f90, g);
            u8 *rec = e + U16_AT(e, k * 2 + 2);
            if (func_02009044(S32_AT(rec, 8), rec + 0x10)) {
                if (PTR_AT(rec, 0) != NULL || full != 0) {
                    func_02051370(stream, 1);
                    if (full != 0) {
                        func_020090f8(rec, stream);
                    } else {
                        void *a = PTR_AT(rec, 0);
                        VCALL(a, 0x1c, void (*)(void *, void *))(a, stream);
                    }
                } else {
                    func_02051370(stream, 0);
                }
            }
            k = (k + 1) & 0xff;
        } while ((s32)k < (s32)GROUP_COUNT(g));
    }
    if (func_02051300(stream))
        func_02051318(stream, 1, 1);
}

/* 0x02008e54
 * @difftest u32 */
void func_02008e54(void *stream) { (void)stream; }

/* 0x02008e58: load the objects of one group from the save */
void func_02008e58(void *stream, s32 group)
{
    u16 n, off;
    s32 g;
    u32 k = 0;
    func_02051464(stream, &n, 2);
    func_02051318(stream, group << 1, 1);
    func_02051464(stream, &off, 2);
    func_02051318(stream, off, 0);
    g = (s8)group;
    if ((s32)GROUP_COUNT(g) > 0) {
        do {
            u8 *e = ENTRY(data_020d9f90, g);
            u8 *rec = e + U16_AT(e, k * 2 + 2);
            u32 load = 1;
            if (func_02009044(S32_AT(rec, 8), rec + 0x10) && func_02051430(stream) == 0)
                load = 0;
            if (load)
                func_02008f90(rec, group, k, stream);
            k = (k + 1) & 0xff;
        } while ((s32)k < (s32)GROUP_COUNT(g));
    }
    if (func_02051300(stream))
        func_02051318(stream, 1, 1);
}

/* 0x02008f90: spawn the actor of an object and load its state */
void func_02008f90(u8 *rec, s32 group, s32 index, void *stream)
{
    u32 proto[0x114 / 4], tmp[2];
    s32 type;
    func_02007d9c(proto);
    tmp[0] = 0;
    tmp[1] = 0;
    (void)tmp;
    type = S32_AT(rec, 8);
    func_02004490(data_020bcbfc, type);
    PTR_AT(rec, 0) = data_020bcc1c[type](group, index);
    if (PTR_AT(rec, 0) != NULL) {
        func_020506a0(PTR_AT(rec, 0), (s8)group, index);
        if (func_02009044(S32_AT(rec, 8), rec + 0x10)) {
            void *a = PTR_AT(rec, 0);
            VCALL(a, 0x1c, void (*)(void *, void *))(a, stream);
        }
    }
    func_02007d78(proto);
}

/* 0x02009044: does an object of this type keep state in the save
 * @difftest int:0:20 ptr:0x20 */
s32 func_02009044(s32 type, const u8 *data)
{
    switch ((u32)type) {
    case 0: return 1;
    case 2: return (s8)data[0x10];
    case 4: return (s8)data[0x1b];
    case 5: return (s8)data[0x8];
    case 6: return (s8)data[0x19];
    case 7: return (s8)data[0x7];
    case 10: return (s8)data[0x6];
    case 12: return (s8)data[0x6];
    case 13: return (s8)data[0x2];
    case 14: return (s8)data[0x4];
    case 17: return (s8)data[0xe];
    default: return 0;
    }
}

/* 0x020090f8: default state of an object without an actor */
void func_020090f8(u8 *rec, void *stream)
{
    switch (S32_AT(rec, 8)) {
    case 5:
        func_02051370(stream, 0);
        break;
    case 0xe: {
        u8 b = rec[0x10];
        func_020513d0(stream, &b, 1);
        break;
    }
    case 0x11:
        func_02051370(stream, 0);
        break;
    }
}

/* 0x0200917c: the actor spawned for object k of group i (group 0, k >= 255: Spyro)
 * @difftest int:0:2 int:0:3
 * @difftest pick:0 int:250:300 */
void *func_0200917c(s32 i, s32 k)
{
    u8 *e;
    if (i == 0 && k >= 0xff)
        return (void *)data_020deebc[2];
    e = ENTRY(data_020d9f90, i);
    return PTR_AT(e, U16_AT(e, k * 2 + 2));
}

/* 0x020091c0: spawn the actors of a group unless it is always loaded
 * @difftest int:0:3 cases=10 */
void func_020091c0(s32 i)
{
    if (func_020089dc(i))
        return;
    func_02008a30(i);
}

/* 0x020091e8: find a record {.., u8 a, u8 b} in table 98
 * @difftest u8 u8
 * @difftest int:0:4 int:0:4 */
void *func_020091e8(u32 a, u32 b)
{
    u8 *h = data_020d9f98, *res = NULL;
    u32 k = 0;
    if ((s32)U16_AT(h, 2) > 0) {
        do {
            u8 *r = h + k * 8;
            if (a == r[8] && b == r[9]) {
                res = h + 4 + k * 8;
                break;
            }
            k = (k + 1) & 0xff;
        } while ((s32)k < (s32)U16_AT(h, 2));
    }
    return res;
}

/* 0x0200925c: build the object tables of the current level from a resource */
void func_0200925c(s32 handle)
{
    u8 *res = func_02012a64(handle), *hdr, *a, *b;
    u32 idx;
    MI_CpuFill8(data_020d9f98, 0, 0x2d00);
    hdr = res + U16_AT(res, 0);
    idx = hdr[data_020d9f9c[0] + 1];
    a = res + U16_AT(hdr, 0xc);
    b = res + U16_AT(hdr + idx * 8, 0xc);
    data_020d9f90 = func_02008804(res);
    data_020d9f8c = func_020086f0(a, b, data_020d9f90);
    data_020d9f94 = func_020084e4(a, b, data_020d9f8c);
    func_0200833c(a, b, data_020d9f94);
    func_020129ac(handle);
}

/* 0x02009324: allocate the table buffer */
void func_02009324(void)
{
    data_020d9f9c[0] = 0;
    data_020d9f98 = func_0207ff70(0x2d00, data_020bcc0c, 0x89);
}

/* 0x02009368: clear the object flags
 * @difftest */
void func_02009368(void) { MI_CpuFill8(data_020d9f9c, 0, 0x240); }

/* 0x02009384
 * @difftest */
u8 *func_02009384(void) { return data_020d9f94; }

/* 0x02009394: sound player
 * @difftest */
void *func_02009394(void) { return data_020f62b4; }

/* 0x020093a4: unlock the card bus */
void func_020093a4(void)
{
    func_0209c10c((u16)data_020da1dc);
    func_02091f58((u16)data_020da1dc);
    func_0202068c();
}

/* 0x020093e8: lock the card bus */
void func_020093e8(void)
{
    u32 id;
    func_020206c8();
    id = func_02091f00();
    data_020da1dc = id;
    func_0209c11c((u16)id);
}

/* 0x0200941c: card backup access with retries */
s32 func_0200941c(u32 a, u32 b, u32 c)
{
    s32 tries = 2;
    u32 r;
    func_020093e8();
    for (;;) {
        r = (u16)func_0209c7f8(a, c, b, 0, 0, 0);
        if (r != 0)
            break;
        if (tries-- <= 0)
            break;
    }
    func_020093a4();
    return (s8)r;
}

/* 0x02009488: card backup access with retries */
s32 func_02009488(u32 a, u32 b, u32 c)
{
    s32 tries = 2;
    u32 r;
    func_020093e8();
    for (;;) {
        r = (u16)func_0209c8cc(a, c, b, 0, 0, 0);
        if (r != 0)
            break;
        if (tries-- <= 0)
            break;
    }
    func_020093a4();
    return (s8)r;
}

/* 0x020094f4: card backup set-up */
void func_020094f4(void)
{
    func_02004490(data_020bcc64);
    func_02004490(data_020bcc90, 0x40);
    func_0209cbd0();
    if (func_0209c16c())
        func_0209c750(0xd01);
}
