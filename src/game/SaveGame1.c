/*
 * Saving and loading games (part 1): the save slots and the save stream.
 * ARM9 main, 0x0204244c - 0x02042f60 (15 functions).
 *
 * There are three save slots; their 0x20-byte headers are data_020ebc40
 * (func_020436f0): +0 date (day, month, year, weekday), +4 / +6 two
 * values from the difficulty tables, +8, +0xc time (hour, minute, second),
 * +0xf bit 0 a flag and bits 1-7 the level, +0x10 the area, +0x12 the
 * difficulty, +0x14 eight bytes, +0x1c the data checksum and +0x1e the
 * header checksum (the 16-bit words of a valid header sum to 0).
 * data_020bf6a0 is the current slot. The game data is written to and read
 * from a bit stream (data_020ebbc8: +0 buffer, +8 position, +0xc bit,
 * +0xe size, +0x1c 1 while loading) and kept on the card with the
 * func_0201af.. routines. Level 0x19 is not a save point: the level and
 * area saved instead are data_020ebba4.
 *
 * Testing: with the card routines and the state switch stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const u8 data_020b688c[];     /* eight bytes for a new game */
extern s8 data_020df10c;             /* saving inside a level */
extern u8 data_020def74[];           /* the inventory */
extern u8 data_020ebba4[];           /* level and area to save */
extern u8 data_020ebbb8[];           /* options block (0x10 bytes) */
extern u8 data_020ebbc8[];           /* the save stream */
extern u16 data_020ebbec[];          /* each level's data position */

void func_02051318(void *stream, s32 pos, s32 whence);
void func_02051464(void *stream, void *buf, u32 n);
void func_020513d0(void *stream, const void *buf, u32 n);
u32 func_02051308(void *stream);
u16 func_02051368(void *stream);
u16 *func_02051548(void *stream);
void func_02051504(void *stream);
void func_0204a89c(void *stream, s32 save);
void func_0204a96c(void *stream);
void func_02008e58(void *stream, s32 group);
void func_02008c04(void *stream, s32 full);
void func_0200925c(s32 handle);
void func_0205b564(void *inv);
s32 func_0201af68(u32 slot, void *hdr);
s32 func_0201af08(u32 slot, const void *buf, u32 n);
s32 func_0201af88(u32 slot, void *hdr);
s32 func_0201af38(u32 slot, void *buf, u32 n);
s32 func_0201afa8(void *opts);
u32 func_02019650(void);
u32 func_02019630(void);
void func_020186dc(u32 v);
void func_02009368(void);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
void func_0203ee8c(s32 k);
void *func_0208226c(void *timer);
void *func_02082288(void *timer);

s32 func_02042604(void);
s32 func_020426a0(void);
s32 func_0204297c(u32 slot);
s32 func_02042a48(void);
s32 func_02042ac0(void);
void func_02042d00(void);
void func_02042dcc(s32 save);
void func_02042e28(void);
void func_02042e98(void);
void func_02042f60(s32 save);
void func_02042fc4(void);
void func_020431fc(void);
void func_0204333c(void);
void func_02043248(void);
void func_02043388(void);
s32 func_02043494(const u16 *hdr);
void func_020435f8(void);
u16 func_02043710(void);
u16 func_02043748(void);

#define STREAM data_020ebbc8
#define SLOT() func_020436f0(data_020bf6a0)
#define LEVEL(h) ((u32)(h)[0xf] >> 1)

static inline void SetLevel(u8 *h, u32 v)
{
    h[0xf] = (h[0xf] & 1) | ((v & 0x7f) << 1);
}

static inline u16 Sum16(const u16 *p, s32 n)
{
    u16 s = 0;

    while (n > 0) {
        s += *p++;
        n--;
    }
    return s;
}

static void SaveFailed(s32 k)
{
    func_0203ee8c(k);
    func_0201b770(2, 0x3f, 1);
    func_02019b08(0x25, 0);
    func_02019b08(0x26, 1);
}

/* 0x0204244c: erase a slot's header
 * @difftest int:0:2 stub=0x0201af68 cases=3 */
void func_0204244c(u32 slot)
{
    MI_CpuFill8(func_020436f0(slot), 0xf, 0x20);
    func_0201af68(slot, func_020436f0(slot));
}

/* 0x02042480: save the game (mode 1: at the current level and area)
 * @difftest pick:0,1 @020df10c:8=pick:0,1 @020bf6a0:32=int:0:2 stub=0x0209a19c,0x0201af68:1,0x0201af08,0x02019b08,0x0204a89c,0x02008c04 cases=20
 * @difftest pick:0,1 @020df10c:8=0 @020bf6a0:32=0 @020ebc4f:8=pick:0x32,0x33,4 stub=0x0209a19c,0x0201af68:0,0x0201af08,0x02019b08,0x0204a89c,0x02008c04 cases=20 */
void func_02042480(s32 mode)
{
    if (data_020df10c != 0) {
        S32_AT(STREAM, 0x1c) = 0;
        func_02051318(STREAM, 0, 0);
        func_02042fc4();
        func_020431fc();
        func_0204333c();
        func_0204a89c(STREAM, 0);
    } else {
        if (LEVEL(SLOT()) != 0x19) {
            data_020ebba4[0] = LEVEL(SLOT());
            data_020ebba4[1] = SLOT()[0x10];
        }
        S32_AT(STREAM, 0x1c) = 0;
        func_02051318(STREAM, 0, 0);
        func_02042f60(0);
        func_020431fc();
        func_0204333c();
        func_0204a89c(STREAM, 0);
        func_02042dcc(0);
    }
    func_02042d00();
    if (mode == 1) {
        u32 v = func_02019650();

        SetLevel(SLOT(), v & 0xff);
        v = func_02019630();
        SLOT()[0x10] = v;
    }
    if (func_02042604() != 0)
        return;
    SaveFailed(2);
}

/* 0x02042604: checksum the stream and write the slot (header, then data)
 * @difftest @020bf6a0:32=int:0:2 stub=0x0201af68:1,0x0201af08:0 cases=10
 * @difftest @020bf6a0:32=int:0:2 stub=0x0201af68:1,0x0201af08:1 cases=10
 * @difftest @020bf6a0:32=int:0:2 stub=0x0201af68:0,0x0201af08 cases=10 */
s32 func_02042604(void)
{
    u16 n = func_02051368(STREAM);
    s32 r;

    U16_AT(SLOT(), 0x1c) = Sum16(func_02051548(STREAM), n >> 1);
    r = func_020426a0();
    if (r != 1)
        return r;
    {
        u16 *buf = func_02051548(STREAM);
        return func_0201af08(data_020bf6a0, buf, func_02051368(STREAM));
    }
}

/* 0x020426a0: write the slot's header (with its checksum)
 * @difftest @020bf6a0:32=0 @020ebc4f:8=pick:0x32,0x33,4,5 @020ebc50:8=int:0:0xff @020ebba4:16=int:0:0xffff stub=0x0201af68:1 cases=20 */
s32 func_020426a0(void)
{
    u32 level = 0xff, area = 0xff;
    s32 r;
    u8 *h;

    if (LEVEL(SLOT()) == 0x19) {
        level = LEVEL(SLOT()) & 0xff;
        area = SLOT()[0x10];
        SetLevel(SLOT(), data_020ebba4[0]);
        SLOT()[0x10] = data_020ebba4[1];
    }
    U16_AT(SLOT(), 0x1e) = 0;
    h = SLOT();
    U16_AT(SLOT(), 0x1e) = -Sum16((const u16 *)h, 0x10);
    r = func_0201af68(data_020bf6a0, SLOT());
    if (level == 0x19) {
        SetLevel(SLOT(), level);
        SLOT()[0x10] = area;
    }
    return r;
}

/* 0x02042808: load the saved game's level data (object group `group`)
 * @difftest int:0:3 @020bf6a0:32=int:0:2 stub=0x02008e58,0x02042b6c,0x0200925c cases=10 */
void func_02042808(s32 group)
{
    u16 off = 0;

    S32_AT(STREAM, 0x1c) = 1;
    func_02051318(STREAM, 0, 0);
    func_02042f60(0);
    func_02043248();
    func_02043388();
    func_0204a96c(STREAM);
    func_02051318(STREAM, LEVEL(SLOT()) << 1, 1);
    func_02051464(STREAM, &off, 2);
    func_02051318(STREAM, off, 0);
    func_02008e58(STREAM, group);
}

/* 0x020428ac: load the current slot (back to the title if it fails)
 * @difftest @020bf6a0:32=int:0:2 @020ebc4f:8=int:0:0x40 stub=0x0209a19c,0x0201af88:0,0x0201af38:1,0x0201af68,0x02019b08,0x020186dc,0x02042fc4,0x020431fc,0x0204333c,0x0204a89c cases=30 */
void func_020428ac(void)
{
    s8 f = (s8)(SLOT()[0xf] & 1);

    if (func_0204297c(data_020bf6a0) == 0) {
        SaveFailed(0);
        return;
    }
    SLOT()[0xf] = (SLOT()[0xf] & ~1) | (f & 1);
    func_020186dc((s8)(SLOT()[0xf] & 1) != 0);
}

/* 0x0204297c: read slot `slot` from the card and start loading it
 * @difftest int:0:2 stub=0x0209a19c,0x0201af88:1,0x0201af38:1,0x0201af68,0x020186dc,0x02042fc4,0x020431fc,0x0204333c,0x0204a89c cases=30 */
s32 func_0204297c(u32 slot)
{
    data_020bf6a0 = slot;
    if (func_02042ac0() != 0) {
        if (func_02043494((const u16 *)SLOT()) == 0)
            return 0;
        if (func_02042a48() == 0)
            return 0;
    }
    func_020186dc((s8)(SLOT()[0xf] & 1) != 0);
    func_02042d00();
    func_020426a0();
    S32_AT(STREAM, 0x1c) = 1;
    func_02051318(STREAM, 0, 0);
    func_02042fc4();
    func_020431fc();
    func_0204333c();
    func_0204a89c(STREAM, 0);
    return 1;
}

/* 0x02042a48: does the stream match the slot's data checksum?
 * @difftest @020bf6a0:32=int:0:2 cases=5
 * @difftest @020bf6a0:32=0 @020ebc5c:16=int:0:0xffff cases=5 */
s32 func_02042a48(void)
{
    u16 n = func_02051368(STREAM);
    u16 s = Sum16(func_02051548(STREAM), n >> 1);

    return s == U16_AT(SLOT(), 0x1c);
}

/* 0x02042ac0: read the current slot's header and data from the card
 * @difftest @020bf6a0:32=int:0:2 stub=0x0201af88:1,0x0201af38:0 cases=5
 * @difftest @020bf6a0:32=int:0:2 stub=0x0201af88:2,0x0201af38:1 cases=5
 * @difftest @020bf6a0:32=int:0:2 stub=0x0201af88:1,0x0201af38:1 cases=5 */
s32 func_02042ac0(void)
{
    s32 r = func_0201af88(data_020bf6a0, SLOT());
    u16 *buf;

    if (r != 1)
        return r;
    buf = func_02051548(STREAM);
    return func_0201af38(data_020bf6a0, buf, func_02051368(STREAM));
}

/* 0x02042b24: write the options block (with its checksum)
 * @difftest stub=0x0201afa8:1 cases=3 */
s32 func_02042b24(void)
{
    U16_AT(data_020ebbb8, 0xe) = 0;
    U16_AT(data_020ebbb8, 0xe) = -Sum16((const u16 *)data_020ebbb8, 8);
    return func_0201afa8(data_020ebbb8);
}

/* 0x02042b6c: start a new game in slot `slot` at level `level`
 * @difftest int:0:2 int:0:0x28 stub=0x0209a19c,0x0201af88,0x0201af68:1,0x0201af08:1,0x02019b08,0x02009368,0x0205b564,0x0204a89c,0x02008c04,0x0200925c,0x020186dc cases=20 */
void func_02042b6c(u32 slot, u32 level)
{
    u8 *h;

    data_020bf6a0 = slot;
    func_02051504(STREAM);
    func_02042d00();
    SLOT()[0xf] &= ~1;
    SetLevel(SLOT(), level & 0xff);
    U32_AT(SLOT(), 8) = 0;
    SLOT()[0x12] = 0;
    SLOT()[0x10] = 0;
    SLOT()[0x11] = 0;
    h = SLOT();
    U16_AT(h, 4) = func_02043748();
    h = SLOT();
    U16_AT(h, 6) = func_02043710();
    MI_CpuCopy8(data_020b688c, SLOT() + 0x14, 8);
    func_020186dc(0);
    func_02009368();
    func_0205b564(data_020def74);
    func_02042f60(1);
    func_020431fc();
    func_0204333c();
    func_0204a89c(STREAM, 1);
    func_02042dcc(1);
    if (func_02042604() == 0) {
        SaveFailed(2);
        return;
    }
    func_020435f8();
}

/* 0x02042d00: stamp the slot with the date and time
 * @difftest @020bf6a0:32=int:0:2 stub=0x0209a19c cases=5 */
void func_02042d00(void)
{
    u32 t[3], d[4];
    const u32 *p;

    p = func_02082288(func_020062d0());
    t[0] = p[0];
    t[1] = p[1];
    t[2] = p[2];
    SLOT()[0xc] = t[0];
    SLOT()[0xd] = t[1];
    SLOT()[0xe] = t[2];
    p = func_0208226c(func_020062d0());
    d[0] = p[0];
    d[1] = p[1];
    d[2] = p[2];
    d[3] = p[3];
    SLOT()[2] = d[2];
    SLOT()[1] = d[1];
    SLOT()[0] = d[0];
    SLOT()[3] = d[3];
}

/* 0x02042dcc: the levels' data: write all of them (save != 0) or, while
 * not loading, read the current one
 * @difftest pick:0,1 @020ebbe4:32=pick:0,1 @020bf6a0:32=int:0:2 stub=0x02008c04,0x0200925c cases=10 */
void func_02042dcc(s32 save)
{
    if (save != 0) {
        func_02042e98();
        return;
    }
    if (S32_AT(STREAM, 0x1c) != 0)
        return;
    func_02042e28();
}

/* 0x02042e28: read the current level's data
 * @difftest @020bf6a0:32=int:0:2 stub=0x02008c04 cases=5 */
void func_02042e28(void)
{
    u16 off = 0;

    func_02051318(STREAM, LEVEL(SLOT()) << 1, 1);
    func_02051464(STREAM, &off, 2);
    func_02051318(STREAM, off, 0);
    func_02008c04(STREAM, 0);
}

/* 0x02042e98: write every level's data and the table of their positions
 * @difftest stub=0x02008c04,0x0200925c cases=3 */
void func_02042e98(void)
{
    u32 start = func_02051308(STREAM);
    s32 i;

    func_02051318(STREAM, 0x52, 1);
    for (i = 0; i < 0x29; i++) {
        if (S32_AT(data_020c3b60 + i * 0x60, 0x18) != -1) {
            data_020ebbec[i] = func_02051308(STREAM);
            func_0200925c(S32_AT(data_020c3b60 + i * 0x60, 0x18));
            func_02008c04(STREAM, 1);
        }
    }
    func_02051318(STREAM, start, 0);
    func_020513d0(STREAM, data_020ebbec, 0x52);
}
