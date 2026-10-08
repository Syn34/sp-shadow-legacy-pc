/*
 * Script command handlers, part 3.
 * ARM9 main, 0x0200d8e0 - 0x0200e4d4 (22 functions).
 *
 * Command records: {.., args from +4}; object references are (s8 group @4,
 * u8 index @5) resolved with func_0200917c. Handlers return 1 when done;
 * the multi-frame ones (dialogue) return (busy == 0).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb88[];           /* RTTI: CAppearance */
extern u8 data_020bcb94[];           /* RTTI: CActor */
extern u8 data_020bcef0[];           /* RTTI */
extern const s32 data_020bcd1c;
extern u8 data_020d9f9c[];
extern u8 data_020da9a8;
extern s8 data_020da9cc;
extern s8 data_020da9d8;
extern s8 data_020da9dc;
extern u8 data_020da9e0;
extern u8 data_020da9e4;
extern u8 data_020da9e8;
extern u8 data_020da9f0;
extern u8 *data_020dedcc;
extern u8 data_020def18[];
extern u8 data_020def74[];
extern u8 data_020df074[];

void *func_02009394(void);
u32 func_02008930(s32 i);
u32 func_020089c0(s32 i);
s32 func_020089dc(s32 i);
void *func_0200917c(s32 group, s32 index);
void func_020091c0(s32 group);
void func_02011664(u32 text, u32 v);
s32 func_02011bc8(void);
s32 func_02011eac(void);
void func_02013be8(void *obj, s32 v);
void func_02017050(void *obj);
void func_02019640(u32 v);
void func_02019660(u32 v);
void func_02019b08(u32 a, u32 b);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02020440(u32 v);
void func_0203c00c(s32 a, s32 b);
s32 func_02046244(void *obj);
u32 func_020436d0(void);
u32 func_020436e0(void);
void func_020437c4(u32 screen);
s32 func_0205a3ec(void *obj, u32 id);
s32 func_0205a514(void *obj, u32 id);
void func_0205a560(void *obj, s32 v);
void func_0205a834(void *obj, s32 v);
void func_0205a9fc(void *obj, u32 id, s32 a, s32 b, s32 c, s32 d);
void func_0205f4b0(void *snd, u32 id);
void func_02070fe8(void *m, u32 i, u32 k, u32 n, u32 next);
s32 func_0208f30c(s32 x, s32 y);
void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */

#define LOOKUP(cmd) ((u8 *)func_0200917c((s8)(cmd)[4], (cmd)[5]))

static inline void *VCallR1(void *obj, u32 off)
{
    void *(*f)(void *, void *) = (void *(*)(void *, void *))(*(void ***)obj)[off / 4];
    return f(obj, (void *)f);
}

static inline void Sound(u32 id) { func_0205f4b0(func_02009394(), id); }

/* 0x0200d8e0: set an object's +0x38 bit 4
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200d8e0(u8 *cmd)
{
    u8 *a = LOOKUP(cmd);
    if (a != NULL)
        U32_AT(a, 0x38) |= 0x10;
    return 1;
}

/* 0x0200d91c: pay gems: if enough, take them and play cmd[6], else show
 * how many are missing and play cmd[7] when the text closes
 * @difftest $C=ptr:8 @$C+4:16=int:0:20 @020da9d8:8=pick:0,1 @020df0a3:8=int:0:20 $C cases=100 */
s32 func_0200d91c(u8 *cmd)
{
    if (data_020da9d8 == 0) {
        u32 have = data_020def74[0x12f];
        s32 need = S16_AT(cmd, 4);
        if ((s32)have >= need) {
            data_020da9d8 = 0;
            func_0205a560(data_020def74, (s8)-S16_AT(cmd, 4));
            Sound(cmd[6]);
        } else {
            func_02011664(need - have + 0x21b, 1);
            data_020da9d8 = 1;
        }
    } else if (data_020da9d8 != 0 && func_02011bc8() == 0) {
        data_020da9d8 = 0;
        Sound(cmd[7]);
    }
    return (s8)(data_020da9d8 == 0);
}

/* 0x0200d9f4
 * @difftest ptr:8 cases=20 */
s32 func_0200d9f4(u8 *cmd)
{
    func_0205a560(data_020def74, 1);
    func_02020440(0xd8);
    return 1;
}

/* 0x0200da24: put actor A on object B (offset by cmd[8], cmd[9])
 * @difftest $C=ptr:12 @$C+4:8=int:0:2 @$C+5:8=int:0:4 @$C+6:8=int:0:2 @$C+7:8=int:0:4 $C cases=100 */
s32 func_0200da24(u8 *cmd)
{
    u8 *a = LOOKUP(cmd), *b, *bb;
    s32 dx, dy, p[2], q[2], r[2];

    if (a != NULL)
        a = func_020a6efc(a, data_020bcb88, data_020bcb94, -1);
    b = func_0200917c((s8)cmd[6], cmd[7]);
    dx = SHL((s8)cmd[8], 16);
    dy = SHL((s8)cmd[9], 16);
    if (a == NULL || b == NULL)
        return 1;
    bb = b;
    if (b != NULL)
        bb = func_020a6efc(b, data_020bcb88, data_020bcef0, -1);
    if (bb == NULL)
        return 1;
    p[0] = S32_AT(b, 0x28);
    p[1] = S32_AT(b, 0x2c);
    S32_AT(bb, 0x64) = p[0];
    S32_AT(bb, 0x68) = p[1];
    func_02003b08(q, p, 4);
    p[0] = q[0];
    p[1] = q[1];
    S32_AT(a, 0x68) = func_02046244(bb);
    r[0] = p[0];
    r[1] = p[1];
    S32_AT(a, 0x24) = S32_AT(a, 0x68) + (s32)func_0201a0a0(r, NULL);
    S32_AT(a, 0x6c) = S32_AT(a, 0x24);
    PTR_AT(a, 0xa4) = b;
    func_02003c74(a, p[0] + dx, p[1] + dy);
    U32_AT(a, 0x38) |= 0x20000;
    return 1;
}

static inline void SetLightMode(void)
{
    func_0201b770(2, 0x3f, 1);
    func_02019b08(0x1e, 0);
    if (BITS(data_020deebc[8], 5, 1) == 1)
        func_02019b08(0xc, 1);
    else
        func_02019b08(0x14, 1);
}

/* 0x0200db7c
 * @difftest ptr:8 cases=10 */
s32 func_0200db7c(u8 *cmd)
{
    SetLightMode();
    func_02019660(func_020436e0());
    func_02019640(func_020436d0());
    return 1;
}

/* 0x0200dbf8
 * @difftest ptr:8 cases=20 */
s32 func_0200dbf8(u8 *cmd)
{
    func_02017050(data_020deebc);
    SetLightMode();
    func_02019660(cmd[4]);
    func_02019640(cmd[5]);
    data_020d9f9c[0x184] = cmd[6];
    return 1;
}

/* 0x0200dc88: make an object Spyro's target
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200dc88(u8 *cmd)
{
    u8 *a = LOOKUP(cmd);
    PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = a;
    return 1;
}

/* 0x0200dcc0
 * @difftest $C=ptr:8 @$C+4:16=int:0:64 $C cases=20 */
s32 func_0200dcc0(u8 *cmd)
{
    func_0203c00c(-1, S16_AT(cmd, 4));
    return 1;
}

/* 0x0200dce4
 * @difftest $C=ptr:8 @$C+4:16=int:-10:10 $C cases=20 */
s32 func_0200dce4(u8 *cmd)
{
    func_0205a834(data_020def74, S16_AT(cmd, 4));
    return 1;
}

/* 0x0200dd0c: play cmd[6] if a counter reaches S16 @4, else cmd[7]
 * @difftest ptr:8 cases=60 */
s32 func_0200dd0c(u8 *cmd)
{
    if (U16_AT(data_020df074, 0x1c) >= S16_AT(cmd, 4))
        Sound(cmd[6]);
    else
        Sound(cmd[7]);
    return 1;
}

/* 0x0200dd54: play cmd[6] if an item is owned, else cmd[7]
 * @difftest $C=ptr:8 @$C+4:16=int:0:16 $C cases=100 */
s32 func_0200dd54(u8 *cmd)
{
    s32 r = 0;
    if (func_0205a514(data_020def74, U16_AT(cmd, 4) & 0xff) != -1)
        r = 1;
    else if (func_0205a3ec(data_020def74, U16_AT(cmd, 4) & 0xff) != -1)
        r = 1;
    else if (U16_AT(cmd, 4) == 10)
        r = (s8)(data_020def74[0x12b] != 0 ? 1 : r);
    if (r)
        Sound(cmd[6]);
    else
        Sound(cmd[7]);
    return 1;
}

/* 0x0200de04
 * @difftest $C=ptr:8 @$C+4:16=int:0:32 $C cases=40 */
s32 func_0200de04(u8 *cmd)
{
    func_0205a9fc(data_020def74, U16_AT(cmd, 4), -1, -1, -1, 0);
    return 1;
}

/* 0x0200de40
 * @difftest $C=ptr:8 @$C+4:16=int:0:32 $C cases=40 */
s32 func_0200de40(u8 *cmd)
{
    func_0205a9fc(data_020def74, U16_AT(cmd, 4), 1, -1, -1, 0);
    return 1;
}

/* 0x0200de7c: send the camera along one of four paths, chosen by the
 * direction from the player to its target
 * @difftest ptr:12 cases=100
 * @difftest $O=ptr:0x60:4 $M=ptr:0xb0:4 $K=ptr:8:4 $T=ptr:0x30:4 @$O+0x44:32=$M @$M+0xac:32=$K @$K+4:32=$T @020dedcc:32=$O $C=ptr:12 @$C+4:8=pick:0,1 @$C+5:8=pick:0,1 @$C+6:8=pick:0,1 @$C+7:8=pick:0,1 @$C+8:8=pick:0,1 @$C+9:8=pick:0,1 @$C+10:8=pick:0,1 @$C+11:8=pick:0,1 $C cases=200 */
s32 func_0200de7c(u8 *cmd)
{
    u8 *o = data_020dedcc, *t;
    s32 vec[3], a, pi4;
    u32 i, k;

    if (o == NULL)
        return 1;
    t = PTR_AT(PTR_AT(PTR_AT(o, 0x44), 0xac), 4);
    vec[0] = S32_AT(t, 0x28) - S32_AT(o, 0x28);
    vec[1] = data_020bcd1c;
    vec[2] = S32_AT(t, 0x2c) - S32_AT(o, 0x2c);
    (void)vec;
    a = func_0208f30c(S32_AT(t, 0x28) - S32_AT(o, 0x28), S32_AT(t, 0x2c) - S32_AT(o, 0x2c));
    pi4 = g_FxPi_4;
    if (a <= pi4 && a > -pi4) {
        i = cmd[4];
        if (i != 0)
            k = cmd[5];
        else if (a < g_FxZero) {
            i = cmd[6];
            k = cmd[7];
        } else {
            i = cmd[8];
            k = cmd[9];
        }
    } else if (a > pi4 && a < g_Fx3Pi_4) {
        i = cmd[8];
        if (i != 0)
            k = cmd[9];
        else if (a < g_FxPi_2) {
            i = cmd[4];
            k = cmd[5];
        } else {
            i = cmd[0xa];
            k = cmd[0xb];
        }
    } else if (a <= -pi4 && a > -g_Fx3Pi_4) {
        i = cmd[6];
        if (i != 0)
            k = cmd[7];
        else if (a > -g_FxPi_2) {
            i = cmd[4];
            k = cmd[5];
        } else {
            i = cmd[0xa];
            k = cmd[0xb];
        }
    } else {
        i = cmd[0xa];
        if (i != 0)
            k = cmd[0xb];
        else if (a < g_FxZero) {
            i = cmd[6];
            k = cmd[7];
        } else {
            i = cmd[8];
            k = cmd[9];
        }
    }
    if (i != 0) {
        u32 n = func_02008930(i);
        func_02070fe8(PTR_AT(PTR_AT(data_020dedcc, 0x44), 0xac), i, k, n, (k + 1) & 0xff);
    }
    return 1;
}

/* 0x0200e04c
 * @difftest $C=ptr:8 @$C+4:16=int:0:8 $C cases=20 */
s32 func_0200e04c(u8 *cmd)
{
    func_02013be8(data_020deebc, S16_AT(cmd, 4));
    return 1;
}

/* 0x0200e074: notify(cmd[4]) every active actor of kind 4
 * @difftest $C=ptr:8 @$C+4:8=pick:0,1 $C cases=20 */
s32 func_0200e074(u8 *cmd)
{
    void **it = func_02006200()->items;
    PtrVec *v = func_02006200();
    if (it != v->items + v->count) {
        do {
            u8 *a = *it;
            if (S32_AT(a, 0xc) == 4)
                VCALL(a, 0xc, void (*)(void *, s32))(a, (s8)cmd[4]);
            it++;
            v = func_02006200();
        } while (it != v->items + v->count);
    }
    return 1;
}

/* 0x0200e0ec: when the dialogue box is open, remember a sound and a group;
 * when it closes, play the sound and spawn the group
 * @difftest @020da9dc:8=pick:0,1 ptr:8 cases=40 */
s32 func_0200e0ec(u8 *cmd)
{
    if (data_020da9dc == 0) {
        if (func_02011bc8() != 0) {
            data_020da9a8 = cmd[5];
            data_020da9f0 = cmd[4];
            data_020da9dc = 1;
        }
    } else if (func_02011bc8() == 0) {
        Sound(data_020da9a8);
        func_020091c0((s8)data_020da9f0);
        data_020da9a8 = 0;
        data_020da9f0 = 0;
        data_020da9dc = 0;
    }
    return (s8)(data_020da9dc == 0);
}

/* 0x0200e1ac: show or hide all objects of a group
 * @difftest $C=ptr:8 @$C+4:8=int:0:3 @$C+5:8=pick:0,1 $C cases=40 */
s32 func_0200e1ac(u8 *cmd)
{
    u32 on = cmd[5] != 0, g = cmd[4];
    s32 gs, n, k;
    if (func_020089dc(g))
        return 1;
    gs = (s8)g;
    n = func_020089c0(gs);
    for (k = 0; k < n; k++) {
        u8 *a = func_0200917c(gs, (s16)k);
        if (a != NULL) {
            U32_AT(a, 0x38) = (U32_AT(a, 0x38) & ~4u) | ((on & 1) << 2);
            U32_AT(a, 4) = (U32_AT(a, 4) & ~8u) | ((on & 1) << 3);
        }
    }
    return 1;
}

/* 0x0200e258: turn a stored direction (0-11, 4 per face)
 * @difftest $C=ptr:8 @$C+4:16=int:0:0x200 @$C+6:8=int:0:8 $C */
s32 func_0200e258(u8 *cmd)
{
    u8 *p = data_020d9f9c + U16_AT(cmd, 4);
    s32 v = (s8)*p;
    switch (cmd[6]) {
    case 0:
        v = (s8)(v - 4);
        if (v < 0)
            v = (s8)(v + 12);
        break;
    case 4:
        v = (s8)(v + 4);
        if (v > 11)
            v = (s8)(v - 12);
        break;
    case 2:
        if (v == 3)
            v = 0;
        else if (v == 7)
            v = 4;
        else if (v == 11)
            v = 8;
        else
            v = (s8)(v + 1);
        break;
    case 6:
        if (v == 0)
            v = 3;
        else if (v == 4)
            v = 7;
        else if (v == 8)
            v = 11;
        else
            v = (s8)(v - 1);
        break;
    }
    *p = v;
    return 1;
}

/* 0x0200e33c: call an object's virtual slot 4
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200e33c(u8 *cmd)
{
    u8 *a = LOOKUP(cmd);
    if (a != NULL)
        VCallR1(a, 0x10);
    return 1;
}

/* 0x0200e378: call an object's virtual slot 4
 * @difftest $C=ptr:8 @$C+4:8=int:0:2 @$C+5:8=int:0:4 $C cases=60 */
s32 func_0200e378(u8 *cmd)
{
    u8 *a = LOOKUP(cmd);
    if (a != NULL)
        VCallR1(a, 0x10);
    return 1;
}

/* 0x0200e3b4: ask a yes/no question; when answered, play the sound for
 * the answer and spawn a group
 * @difftest @020da9cc:8=pick:0,1 ptr:12 cases=40 */
s32 func_0200e3b4(u8 *cmd)
{
    if (data_020da9cc == 0) {
        u32 scr;
        void *o;
        data_020da9e4 = cmd[7];
        scr = data_020df0fc;
        data_020da9e8 = cmd[8];
        data_020da9e0 = cmd[6];
        data_020da9cc = 1;
        func_020437c4(scr);
        o = PTR_AT(data_020def18, 8);
        VCALL(o, 0xc, void (*)(void *, s32))(o, 0);
        func_02011664(U16_AT(cmd, 4), 1);
    } else if (func_02011bc8() == 0) {
        if (func_02011eac() == 1)
            Sound(data_020da9e4);
        else
            Sound(data_020da9e8);
        func_020091c0((s8)data_020da9e0);
        data_020da9e4 = 0;
        data_020da9e8 = 0;
        data_020da9e0 = 0;
        data_020da9cc = 0;
    }
    return (s8)(data_020da9cc == 0);
}
