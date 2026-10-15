/*
 * The task base class (0x10 bytes, vtable data_020c0cb0) of the game's
 * objects and actors: +4, +8 and +0xc cleared.
 * ARM9 main, 0x0205924c - 0x020592c8 (4 functions).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c0cb0[];           /* vtable */

void func_0207fe44(void *p);

/* 0x0205924c: destructor
 * @difftest ptr:0x10:4 cases=3 */
void func_0205924c(void *task)
{
    PTR_AT(task, 0) = data_020c0cb0;
    U32_AT(task, 4) = 0;
}

/* 0x02059264: deleting destructor
 * @difftest ptr:0x10:4 stub=0x0207fe44 cases=3 */
void *func_02059264(void *task)
{
    PTR_AT(task, 0) = data_020c0cb0;
    U32_AT(task, 4) = 0;
    func_0207fe44(task);
    return task;
}

/* 0x02059290: destructor (another copy)
 * @difftest ptr:0x10:4 cases=3 */
void func_02059290(void *task)
{
    PTR_AT(task, 0) = data_020c0cb0;
    U32_AT(task, 4) = 0;
}

/* 0x020592a8: constructor
 * @difftest ptr:0x10:4 cases=3 */
void func_020592a8(void *task)
{
    PTR_AT(task, 0) = data_020c0cb0;
    U32_AT(task, 8) = 0;
    U32_AT(task, 0xc) = 0;
    U32_AT(task, 4) = 0;
}
