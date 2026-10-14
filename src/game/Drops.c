/*
 * Random drops: what a defeated enemy or an opened chest leaves behind.
 * ARM9 main, 0x0203ef98 - 0x0203f518 (5 functions).
 *
 * A drop kind (0-6) picks one of data_020bee88 random drop tables (from
 * data_020bf044, data_020beeb4, data_020bef40 or data_020bef90); a table
 * gives how many of each of the data_020bee84 pickups (data_020bef18) to
 * spawn. Some kinds add a random gem (kind 0xd-0x16, with a random size
 * and charge) or a fixed item (0xa, 0xb). Pickups are spawned with
 * func_02054e64; kinds 0-4 stand for the pickups 0x3a-0x3e.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const u32 data_020bee84;               /* number of pickup kinds */
extern const u32 data_020bee88;               /* number of tables per drop kind */
extern const u32 *const data_020beeb4[], *const data_020bef40[], *const data_020bef90[],
    *const data_020bf044[];                   /* drop tables */
extern const u32 data_020bef18[];             /* the pickup kinds */

int rand(void);
u64 func_020a500c(u32 a, u32 b);   /* unsigned divide: quotient in r0, remainder in r1 */
void func_02054e64(void *o, u32 kind, u32 a, u32 b);

void func_0203f470(void *o, void *pos, u32 kind, u32 a, u16 b);

/* a random number 0..99 */
static u32 Percent(void)
{
    return (u16)(((u32)rand() & 0x7fff) % 100);
}

/* 0x0203ef98: a random gem charge (1-3)
 * @difftest cases=60 */
u32 func_0203ef98(void)
{
    u32 r = Percent();

    if (r < 0x1e)
        return 1;
    return r < 0x32 ? 2 : 3;
}

/* 0x0203f004: a random gem size (1-3)
 * @difftest cases=60 */
u32 func_0203f004(void)
{
    u32 r = Percent();

    if (r < 0x3c)
        return 1;
    return r < 0x55 ? 2 : 3;
}

/* 0x0203f070: a random gem kind (0xd-0x16)
 * @difftest cases=200 */
u32 func_0203f070(void)
{
    u32 r = Percent();

    if (r < 0xf)
        return 0xd;
    if (r < 0x1e)
        return 0xe;
    if (r < 0x28)
        return 0xf;
    if (r < 0x32)
        return 0x10;
    if (r < 0x3c)
        return 0x11;
    if (r < 0x46)
        return 0x12;
    if (r < 0x50)
        return 0x13;
    if (r < 0x57)
        return 0x14;
    return r < 0x5f ? 0x15 : 0x16;
}

/* a random table of `tables` */
static const u32 *Table(const u32 *const *tables)
{
    u32 n = data_020bee88;

    return tables[(u32)(func_020a500c((u32)rand() & 0x7fff, n) >> 32)];
}

/* 0x0203f168: spawn a drop of kind `kind`
 * @difftest ptr:0x40 ptr:8 int:0:6 stub=0x02054e64 cases=100 */
void func_0203f168(void *o, void *pos, u32 kind)
{
    const u32 *t = NULL;
    s32 item11 = 0, item10 = 0, gem = 0;
    u32 i, j;

    if (kind == 0) {
        t = Table(data_020bf044);
    } else if (kind == 1) {
        t = Table(data_020beeb4);
    } else if (kind == 2) {
        t = Table(data_020bef40);
        item11 = 1;
    } else if (kind == 3) {
        t = Table(data_020bef40);
        item10 = 1;
    } else if (kind == 4) {
        t = Table(data_020bef40);
        if ((s32)(((u32)rand() & 0x7fff) % 100) < 0x19)
            gem = 1;
    } else if (kind == 5) {
        t = Table(data_020bef40);
        gem = 1;
    } else if (kind == 6) {
        t = Table(data_020bef90);
        gem = 1;
    }
    for (i = 0; i < data_020bee84; i++)
        for (j = 0; j < t[i]; j++)
            func_0203f470(o, pos, (u16)data_020bef18[i], 0, 0);
    if (gem != 0) {
        u32 k = func_0203f070();
        u32 size = func_0203f004();
        func_0203f470(o, pos, k, size, func_0203ef98());
    }
    if (item11 != 0)
        func_0203f470(o, pos, 0xb, 0, 0);
    if (item10 != 0)
        func_0203f470(o, pos, 0xa, 0, 0);
}

/* 0x0203f470: spawn one pickup (gems with their size and charge)
 * @difftest ptr:0x40 ptr:8 int:0:0x70 int:0:0x200 int:0:0x200 stub=0x02054e64 cases=200 */
void func_0203f470(void *o, void *pos, u32 kind, u32 a, u16 b)
{
    switch (kind) {
    case 0: kind = 0x3a; break;
    case 1: kind = 0x3b; break;
    case 2: kind = 0x3c; break;
    case 3: kind = 0x3d; break;
    case 4: kind = 0x3e; break;
    }
    if (kind >= 0x66)
        return;
    if (kind >= 0xd && kind <= 0x16)
        func_02054e64(o, kind, a & 0xff, b & 0xff);
    else
        func_02054e64(o, kind, 0xff, 0xff);
}
