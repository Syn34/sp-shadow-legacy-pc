/*
 * Map state accessors, IManager vector helpers, the hardware divider and
 * small fixed-point helpers, and the per-screen game state machine
 * (state, next state, previous state; fade out, switch, fade in).
 * ARM9 main, 0x02019620 - 0x02019f58 (38 functions).
 *
 * data_020df1a4: per screen {current, next, previous} state (12 bytes);
 * data_020df184[screen]: transition phase (1 run, 2 fading in, 3 fading
 * out, 4 running, 5 switch); data_020df180[screen]: state change pending.
 * State handlers: data_020b690c + state * 12 = {enter, update, leave}.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define REG16(a) (*(vu16 *)(a))
#define REG32(a) (*(vu32 *)(a))

extern const s32 data_020b567c[];    /* sine, 0x40 entries per quarter turn */
extern void (*data_020b690c[])(void);   /* state handlers, 3 per state */
extern s8 data_020df108;
extern s8 data_020df120;
extern u32 data_020df128;
extern s8 data_020df17c;
extern s8 data_020df180[2];
extern u32 data_020df184[2];
extern u32 data_020df1a4[];
extern u8 data_020df2b8[];
extern void *data_020f6f34;

void *func_02006acc(struct PtrVec *v);
void func_02006330(struct PtrVec *v, u32 n);
void *func_020062e0(void);
void *func_02009394(void);
void func_02009cd4(void);
void func_0200bb0c(void);
void func_020128ec(s32 all);
void func_0201817c(u32 screen);
void func_02018950(s32 paused);
void func_0201a2e0(void);
s32 func_0201b678(void);
void func_0201b7f0(u32 screen);
void func_0201b808(u32 screen);
void func_0202064c(void);
void func_02028f04(void);
void func_0205f82c(void *snd);
void func_020811b0(void *obj);
void func_02081300(void *obj);
void func_02081f94(void *obj);

/* this file's own functions, called before their definition */
u32 *func_02019bb4(u32 screen);
u32 *func_02019bcc(u32 screen);
u32 *func_02019be0(u32 screen);
void func_02019b08(u32 state, u32 screen);
void func_0201980c(u8 *mgr);
void func_02019838(u8 *mgr);
s32 func_02019888(s32 num, s32 den);
s32 func_020198e0(s32 num, s32 den);
s32 func_02019954(s32 a);
s32 func_02019968(s32 a);
void func_020199e0(void);
void func_02019a30(void);
void func_02019a80(void);
void func_02019de0(void);

static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

/* 0x02019620
 * @difftest */
s32 func_02019620(void) { return data_020df120; }

/* 0x02019630
 * @difftest */
u32 func_02019630(void) { return (u8)data_020df108; }

/* 0x02019640
 * @difftest u32 */
void func_02019640(s32 v) { data_020df108 = v; }

/* 0x02019650
 * @difftest */
u32 func_02019650(void) { return data_020df128; }

/* 0x02019660
 * @difftest u32 */
void func_02019660(s32 v) { data_020df128 = v; }

/* 0x02019670
 * @difftest */
void *func_02019670(void) { return data_020f6f34; }

/* release a manager's list storage (swap with an empty vector) */
static void FreeList(u8 *mgr)
{
    struct PtrVec tmp;
    void *items;
    s32 count;
    tmp.items = NULL;
    tmp.count = 0;
    tmp.cap = 0;
    tmp.cap = S32_AT(mgr, 0xc);
    S32_AT(mgr, 0xc) = 0;
    items = PTR_AT(mgr, 4);
    PTR_AT(mgr, 4) = tmp.items;
    tmp.items = items;
    count = S32_AT(mgr, 8);
    S32_AT(mgr, 8) = tmp.count;
    tmp.count = count;
    func_02006acc(&tmp);
}

/* 0x02019680: clear a manager's list and free it
 * @difftest zero:0x10 */
void func_02019680(u8 *mgr)
{
    func_0201980c(mgr);
    FreeList(mgr);
}

/* reserve room for n items */
static inline void Reserve(u8 *mgr, u32 n)
{
    if (U32_AT(mgr, 0xc) >= n)
        return;
    if (n <= U32_AT(mgr, 0xc))
        return;
    func_02006330((struct PtrVec *)(mgr + 4), n);
}

/* 0x020196e0
 * @difftest zero:0x10 int:0:8 */
void func_020196e0(u8 *mgr, u32 n) { Reserve(mgr, n); }

/* 0x02019724
 * @difftest zero:0x10 */
void func_02019724(u8 *mgr)
{
    func_02019838(mgr);
    FreeList(mgr);
}

/* 0x02019784
 * @difftest zero:0x10 int:0:8 */
void func_02019784(u8 *mgr, u32 n) { Reserve(mgr, n); }

/* 0x020197c8
 * @difftest zero:0x10 int:0:8 */
void func_020197c8(u8 *mgr, u32 n) { Reserve(mgr, n); }

/* 0x0201980c: empty a manager's list
 * @difftest ptr:0x10:4 */
void func_0201980c(u8 *mgr) { S32_AT(mgr, 8) -= S32_AT(mgr, 8); }

/* 0x02019838
 * @difftest ptr:0x10:4 */
void func_02019838(u8 *mgr) { S32_AT(mgr, 8) -= S32_AT(mgr, 8); }

/* 0x02019864: out = a + b (2D)
 * @difftest ptr:8:4 ptr:8:4 ptr:8:4 */
void func_02019864(const s32 *a, const s32 *b, s32 *out)
{
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
}

/* 0x02019888: num / den with the hardware divider
 * @difftest @04000280:16=pick:0 @040002a0:32=u32 s32 s32 */
s32 func_02019888(s32 num, s32 den)
{
    REG16(0x04000280) = 0;
    REG32(0x04000290) = num;
    REG32(0x04000298) = den;
    REG32(0x0400029c) = 0;
    while (REG16(0x04000280) & 0x8000)
        ;
    return REG32(0x040002a0);
}

/* 0x020198e0
 * @difftest @04000280:16=pick:0 @040002a0:32=u32 s32 s32 */
s32 func_020198e0(s32 num, s32 den) { return func_02019888(num, den); }

/* 0x020198ec: fixed-point divide
 * @difftest @04000280:16=pick:0 @040002a0:32=u32 s32 s32 */
s32 func_020198ec(s32 num, s32 den) { return SHL(func_020198e0(SHL(num, 6), den >> 6), 4); }

/* 0x02019910: fixed-point multiply of two values with 6 extra bits
 * @difftest s32 s32 */
s32 func_02019910(s32 a, s32 b) { return (a / 64) * (b / 64) / 16; }

/* 0x0201993c
 * @difftest s32 s32 */
s32 func_0201993c(s32 a, s32 b) { return (s16)((a * b) / 256); }

/* 0x02019954: cosine of an 8-bit angle
 * @difftest u32 */
s32 func_02019954(s32 a) { return func_02019968((a + 0x40) & 0xff); }

/* 0x02019968: sine of an 8-bit angle
 * @difftest int:0:0x100 */
s32 func_02019968(s32 a)
{
    u32 q = (u32)(a >> 6) & 0xff;
    if (q & 1)
        a = (0x80 - a) & 0xff;
    if (q & 2)
        return -data_020b567c[a - 0x80];
    return data_020b567c[a];
}

/* 0x020199a0
 * @difftest u32 */
s32 func_020199a0(s32 a) { return (s16)(func_02019954(a) >> 8); }

/* 0x020199c0
 * @difftest int:0:0x100 */
s32 func_020199c0(s32 a) { return (s16)(func_02019968(a) >> 8); }

#define CUR(scr) (*func_02019bcc(scr))

/* 0x020199e0: leave the current state
 * @difftest @020df0fc:32=int:0:1 */
void func_020199e0(void)
{
    void (*fn)(void) = data_020b690c[CUR(data_020df0fc) * 3 + 2];
    if (fn != NULL)
        fn();
}

/* 0x02019a30: update the current state */
void func_02019a30(void)
{
    void (*fn)(void) = data_020b690c[CUR(data_020df0fc) * 3 + 1];
    if (fn != NULL)
        fn();
}

/* 0x02019a80: enter the current state */
void func_02019a80(void)
{
    void (*fn)(void) = data_020b690c[CUR(data_020df0fc) * 3];
    if (fn != NULL)
        fn();
}

/* 0x02019ad0: is `state` the current state
 * @difftest int:0:0x30 @020df0fc:32=int:0:1 */
s32 func_02019ad0(u32 state) { return (s8)(state == CUR(data_020df0fc)); }

/* 0x02019b08: request a change to `state` on a screen
 * @difftest int:0:0x30 int:0:1 */
void func_02019b08(u32 state, u32 screen)
{
    *func_02019bb4(screen) = state;
    data_020df180[screen] = 1;
}

/* 0x02019b40: go back to the previous state
 * @difftest int:0:1 */
void func_02019b40(u32 screen) { func_02019b08(*func_02019be0(screen), screen); }

/* 0x02019b60
 * @difftest int:0:1 */
u32 func_02019b60(u32 screen) { return *func_02019be0(screen); }

/* 0x02019b7c
 * @difftest int:0:1 */
u32 func_02019b7c(u32 screen) { return *func_02019bb4(screen); }

/* 0x02019b98
 * @difftest int:0:1 */
u32 func_02019b98(u32 screen) { return *func_02019bcc(screen); }

/* 0x02019bb4: next state
 * @difftest int:0:1 */
u32 *func_02019bb4(u32 screen) { return &data_020df1a4[screen * 3 + 1]; }

/* 0x02019bcc: current state
 * @difftest int:0:1 */
u32 *func_02019bcc(u32 screen) { return &data_020df1a4[screen * 3]; }

/* 0x02019be0: previous state
 * @difftest int:0:1 */
u32 *func_02019be0(u32 screen) { return &data_020df1a4[screen * 3 + 2]; }

/* 0x02019bf8: run one frame of both screens */
void func_02019bf8(void)
{
    s32 scr;

    func_02081300(func_020062e0());
    for (scr = 1; scr >= 0; scr--) {
        func_0201817c(scr);
        switch (data_020df184[scr]) {
        case 1:
            func_02019a80();
            func_0201b808(data_020df0fc);
            data_020df184[scr] = 2;
            break;
        case 2:
            if (func_0201b678() == 0)
                data_020df184[scr] = 4;
            break;
        case 4: {
            u32 s;
            s32 change;
            if (func_0201b678() != 0)
                break;
            s = data_020df0fc;
            change = 1;
            if (data_020df180[s] == 0 && *func_02019bcc(s) == *func_02019bb4(s))
                change = 0;
            if ((s8)change) {
                func_0201b7f0(data_020df0fc);
                data_020df184[scr] = 3;
            } else {
                if (scr == 0)
                    data_020df17c = 0;
                func_02019a30();
            }
            break;
        }
        case 3:
            if (func_0201b678() == 0)
                data_020df184[scr] = 5;
            break;
        case 5:
            func_020199e0();
            func_02019de0();
            data_020df184[scr] = 1;
            func_02028f04();
            break;
        }
    }
    if (data_020df184[0] == 5 || data_020df184[1] == 5)
        func_020128ec(1);
    VCallR1(func_020062c0(), 0xc);
    func_02081f94(func_02019670());
    for (scr = 1; scr >= 0; scr--) {
        func_0201817c(scr);
        func_02018950(data_020df0fc);
        func_02009cd4();
        func_0200bb0c();
        func_0201a2e0();
    }
    func_0205f82c(func_02009394());
    func_0202064c();
    func_020128ec(0);
    func_020811b0(func_020062e0());
}

/* 0x02019de0: switch to the next state
 * @difftest @020df0fc:32=int:0:1 */
void func_02019de0(void)
{
    u32 *cur = func_02019bcc(data_020df0fc), *p;
    *func_02019be0(data_020df0fc) = *cur;
    p = func_02019bb4(data_020df0fc);
    *func_02019bcc(data_020df0fc) = *p;
    data_020df180[data_020df0fc] = 0;
}

/* 0x02019e50: start both screens' state machines
 * @difftest */
void func_02019e50(void)
{
    s32 scr;
    for (scr = 0; scr < 2; scr++) {
        u32 *c;
        func_0201817c(scr);
        *func_02019bcc(data_020df0fc) = scr == 0 ? 1 : 2;
        c = func_02019bcc(data_020df0fc);
        *func_02019bb4(data_020df0fc) = *c;
        c = func_02019bcc(data_020df0fc);
        *func_02019be0(data_020df0fc) = *c;
        data_020df184[scr] = 1;
    }
    U32_AT(data_020df2b8, 0x298) = 0x100000;
    U32_AT(data_020df2b8, 0x29c) = 0xc0000;
    U32_AT(data_020df2b8, 0x270) = 0x1000;
    U32_AT(data_020df2b8, 0x274) = 0x3e8000;
    data_020df2b8[0x284] = 1;
}
