/*
 * Two small tasks of the task list (func_020062a0): a screen flasher that
 * blinks the master brightness for a time (vtable data_020bfc24) and a
 * countdown (vtable data_020bfc68).
 * ARM9 main, 0x020499c0 - 0x02049ccc (14 functions).
 *
 * Task: +4 flags (bit 1 finished, bit 2 paused), +8 / +0xc -1, +0x10 time
 * left, +0x14 blink period, +0x18 time to the next blink.
 *
 * Testing: with fake tasks; the task base class stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bfc24[], data_020bfc68[];   /* vtables */
extern const char data_020bfc34[];   /* source file name for the heap */

#define REG_MASTER_BRIGHT 0x0400006c

void func_0208f800(u32 reg, s32 level);
s32 func_0208f7c0(u32 reg);
void *func_020062a0(void);
void func_0205924c(void *task);
void func_020592a8(void *task);
void func_0207fe44(void *p);
void func_0207fe28(const char *file, s32 line);
void func_0207fe24(void);
void func_020062f0(void *vec, void **value, u32 tag);

void func_020499cc(void);
void func_02049b78(u8 *list, void *item);

/* 0x020499c0: flash for `time`, blinking every `period`
 * @difftest zero:0x1c s32 s32 cases=3 */
void func_020499c0(u8 *self, s32 time, s32 period)
{
    S32_AT(self, 0x10) = time;
    S32_AT(self, 0x14) = period;
}

/* 0x020499cc: normal brightness
 * @difftest cases=3 */
void func_020499cc(void)
{
    func_0208f800(REG_MASTER_BRIGHT, 0);
}

/* 0x020499e4: put the task on the task list
 * @difftest zero:0x1c stub=0x02049b78 cases=3 */
void func_020499e4(u8 *self)
{
    S32_AT(self, 0xc) = -1;
    S32_AT(self, 8) = -1;
    func_02049b78((u8 *)func_020062a0() + 4, self);
}

/* 0x02049a10: the flasher's update
 * @difftest $S=zero:0x1c @$S+0x10:32=int:-2:0x40 @$S+0x14:32=int:0:8 @$S+0x18:32=int:-2:8 $S int:0:4 stub=0x0208f800,0x0208f7c0:0 cases=40
 * @difftest $S=zero:0x1c @$S+0x10:32=int:-2:0x40 @$S+0x14:32=int:0:8 @$S+0x18:32=int:-2:8 $S int:0:4 stub=0x0208f800,0x0208f7c0:0x10 cases=40 */
void func_02049a10(u8 *self, s32 dt)
{
    s32 v = func_0208f7c0(REG_MASTER_BRIGHT);

    if (S32_AT(self, 0x10) > 0) {
        if (S32_AT(self, 0x18) < 0) {
            if (v == 0)
                func_0208f800(REG_MASTER_BRIGHT, 0x10);
            else
                func_0208f800(REG_MASTER_BRIGHT, 0);
            S32_AT(self, 0x18) = S32_AT(self, 0x14);
        }
        S32_AT(self, 0x10) -= dt;
        S32_AT(self, 0x18) -= dt;
        if (S32_AT(self, 0x10) <= 0)
            U32_AT(self, 4) |= 2;
        return;
    }
    if (v == 0)
        return;
    func_0208f800(REG_MASTER_BRIGHT, 0);
}

/* 0x02049adc: the flasher's deleting destructor
 * @difftest zero:0x1c stub=0x0208f800,0x0205924c,0x0207fe44 cases=3 */
u8 *func_02049adc(u8 *self)
{
    PTR_AT(self, 0) = data_020bfc24;
    func_020499cc();
    func_0205924c(self);
    func_0207fe44(self);
    return self;
}

/* 0x02049b10: the flasher's destructor
 * @difftest zero:0x1c stub=0x0208f800,0x0205924c cases=3 */
u8 *func_02049b10(u8 *self)
{
    PTR_AT(self, 0) = data_020bfc24;
    func_020499cc();
    func_0205924c(self);
    return self;
}

/* 0x02049b3c: the flasher's constructor
 * @difftest zero:0x1c stub=0x020592a8,0x02049b78 cases=3 */
u8 *func_02049b3c(u8 *self)
{
    func_020592a8(self);
    PTR_AT(self, 0) = data_020bfc24;
    S32_AT(self, 0x10) = 0;
    S32_AT(self, 0x14) = 0;
    S32_AT(self, 0x18) = 0;
    func_020499e4(self);
    return self;
}

/* 0x02049b78: vector<task *>::push_back
 * @difftest $L=ptr:0x20:4 $M=zero:0x10 @$M+4:32=$L @$M+8:32=int:0:3 @$M+0xc:32=int:3:8 $M u32 */
void func_02049b78(u8 *list, void *item)
{
    func_0207fe28(data_020bfc34, 0x58);
    if (U32_AT(list, 8) < U32_AT(list, 0xc)) {
        U32_AT(list, 8) += 1;
        ((void **)PTR_AT(list, 4))[U32_AT(list, 8) - 1] = item;
    } else {
        func_020062f0(list + 4, &item, 0);
    }
    func_0207fe24();
}

/* 0x02049c14: the countdown's update
 * @difftest $S=zero:0x14 @$S+4:32=pick:0,4 @$S+0x10:32=int:-2:4 $S cases=10 */
void func_02049c14(u8 *self)
{
    if ((s8)BITS(U32_AT(self, 4), 2, 1) != 0)
        return;
    if (S32_AT(self, 0x10) >= 0)
        S32_AT(self, 0x10)--;
}

/* 0x02049c48: the countdown's deleting destructor
 * @difftest zero:0x14 stub=0x0205924c,0x0207fe44 cases=3 */
u8 *func_02049c48(u8 *self)
{
    PTR_AT(self, 0) = data_020bfc68;
    func_0205924c(self);
    func_0207fe44(self);
    return self;
}

/* 0x02049c74: the countdown's destructor
 * @difftest zero:0x14 stub=0x0205924c cases=3 */
u8 *func_02049c74(u8 *self)
{
    PTR_AT(self, 0) = data_020bfc68;
    func_0205924c(self);
    return self;
}

/* 0x02049c98: set the countdown
 * @difftest zero:0x14 s32 cases=3 */
void func_02049c98(u8 *self, s32 v)
{
    S32_AT(self, 0x10) = v;
}

/* 0x02049ca0: the countdown's constructor
 * @difftest zero:0x14 stub=0x020592a8 cases=3 */
u8 *func_02049ca0(u8 *self)
{
    func_020592a8(self);
    PTR_AT(self, 0) = data_020bfc68;
    S32_AT(self, 0x10) = 0;
    return self;
}
