/*
 * Pickup scatterers (vtable data_020c0928): a list of pickups to throw out
 * ({u16 kind, u8, u8} each) and the list of the ones thrown. Each pickup
 * is spawned at the scatterer and given a random speed: outwards at a
 * random angle within 90 degrees of the direction given, and upwards.
 * ARM9 main, 0x020548c4 - 0x02054fec (6 functions).
 *
 * Layout: +4 the position {x, z}, +0xc the pickups {data, size, capacity},
 * +0x18 the spawned pickups' actors {data, size, capacity}.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c0928[];           /* vtable */
extern const char data_020c0948[];   /* source file name */

int rand(void);
u8 *func_02045ed0(u32 id, s32 f5, s32 f6, s32 *pos, s8 shrink);
void *func_020062b0(void);
void func_02017abc(void *list, void *item);
s64 func_020a4c50(s64 a, s64 b);
s32 func_0208eff8(s32 v);
void func_02002bd8(fx32 *out, const fx32 *vec2, const fx32 *len);
void func_0207fe28(const char *file, s32 line);
void func_0207fe24(void);
void func_020062f0(void *vec, void **value, u32 tag);
void func_02006330(void *v, u32 n);
s32 func_02050dd0(u8 *list, void *e);
void func_02006acc(void *v);
void func_0207fe44(void *p);

void func_02054db8(u8 *self, void *actor);

#define ACTOR(p) ((p) != NULL ? (p) + 0x90 : NULL)
/* (as in the original: the macro evaluates its argument twice, so the
 * test and the value use two different random numbers) */
#define FX_ROUND(x) ((x) > 0 ? (s32)(0.5f + (float)SHL(x, 12)) : (s32)((float)SHL(x, 12) - 0.5f))
#define RAND_DEG() ((s32)(((u32)rand() & 0x7fff) % 0xb5) - 0x5a)
#define RAND_UP() ((s32)(((u32)rand() & 0x7fff) % 0xb) + 0x14)

/* 0x020548c4: throw out all the pickups at `pos` + (dir) (whole units)
 * @difftest $S=zero:0x24 $S s32 s32 ptr:8:4 cases=3
 * @difftest $L=zero:0x10 @$L:32=pick:0x0d,0x10,0x1a,0x63,0x30001,0x20012 @$L+4:32=pick:2,0x15,0x40,0x70 $S=zero:0x24 @$S+0xc:32=$L @$S+0x10:32=int:1:3 $P=ptr:8:4 @$P:32=int:-0x10:0x10 @$P+4:32=int:-0x10:0x10 $S s32 s32 $P stub=0x02045ed0:0 cases=10
 * @difftest $L=zero:0x10 @$L:32=pick:0x0d,0x10,0x1a,0x63,0x30001,0x20012 @$L+4:32=pick:2,0x15,0x40,0x70 $D=zero:0x10 $S=zero:0x24 @$S+0xc:32=$L @$S+0x10:32=int:1:3 @$S+0x18:32=$D @$S+0x20:32=4 $P=ptr:8:4 @$P:32=int:-0x10:0x10 @$P+4:32=int:-0x10:0x10 $S s32 s32 $P stub=0x02045ed0:0x02300000,0x02050dd0,0x02017abc cases=20 */
void func_020548c4(u8 *self, s32 a, s32 b, const s32 *pos)
{
    s32 out[3];
    u32 i;

    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    for (i = 0; i < U32_AT(self, 0x10); i++) {
        u32 e = U32_AT(PTR_AT(self, 0xc), i * 4);
        u16 kind = (u16)e;
        s8 grow = 1;
        u8 *p;
        s32 ang, s, c, v[2], len, up;
        u32 idx;

        if ((kind >= 0xd && kind <= 0x16) || (kind >= 0x1a && kind <= 0x39) || kind == 0x63)
            grow = 0;
        p = func_02045ed0(kind, a, b, (s32 *)(self + 4), grow);
        if (p == NULL)
            continue;
        U32_AT(p, 0xc8) &= ~4u;
        U32_AT(p, 0x108) = kind;
        if (kind >= 0xd && kind <= 0x16) {
            U16_AT(p, 0x118) = (u8)(e >> 16);
            U16_AT(p, 0x11a) = (u8)(e >> 24);
        }
        func_02054db8(self, ACTOR(p));
        U32_AT(p, 0xc8) |= 4;
        ang = FX_ROUND(RAND_DEG());
        ang = (s32)func_020a4c50((s64)ang * 0x3244, 0xb4000);
        idx = FxRadToIdx(ang) >> 4;
        s = FX_SinCosTable_[idx * 2];
        c = FX_SinCosTable_[idx * 2 + 1];
        v[0] = s + FxMul(pos[0], 0x1800);
        v[1] = c + FxMul(pos[1], 0x1800);
        len = func_0208eff8(FxMul(v[0], v[0]) + FxMul(v[1], v[1]));
        if (len <= 1) {
            v[0] = 0;
            v[1] = 0;
        } else {
            s32 t[2];

            func_02002bd8(t, v, &len);
            v[0] = t[0];
            v[1] = t[1];
        }
        out[0] = FxMul(v[0], 0x1000);
        out[1] = FxMul(v[1], 0x1000);
        up = FX_ROUND(RAND_UP());
        out[2] = FxMul(up, 0x1000);
        if ((kind >= 0x1a && kind <= 0x39) || kind == 0x63) {
            out[0] = 0;
            out[1] = 0;
        }
        S32_AT(p, 0xdc) = out[0];
        S32_AT(p, 0xe0) = out[1];
        S32_AT(p, 0xe4) = out[2];
        p[0x101] = 1;
        func_02017abc((u8 *)func_020062b0() + 4, ACTOR(p));
    }
}

/* 0x02054db8: remember a thrown pickup (and take it off the list of
 * actors to process)
 * @difftest $D=ptr:0x10:4 $S=zero:0x24 @$S+0x18:32=$D @$S+0x1c:32=int:0:2 @$S+0x20:32=4 $S u32 stub=0x02050dd0 cases=8 */
void func_02054db8(u8 *self, void *actor)
{
    func_0207fe28(data_020c0948, 0x37);
    if (U32_AT(self, 0x1c) < U32_AT(self, 0x20)) {
        U32_AT(self, 0x1c)++;
        PTR_AT(PTR_AT(self, 0x18), (U32_AT(self, 0x1c) - 1) * 4) = actor;
    } else {
        func_020062f0(self + 0x18, &actor, 0);
    }
    func_0207fe24();
    func_02050dd0((u8 *)func_020062b0() + 4, actor);
}

/* 0x02054e64: add a pickup to throw
 * @difftest $D=ptr:0x10:4 $S=zero:0x24 @$S+0xc:32=$D @$S+0x10:32=int:0:2 @$S+0x14:32=4 $S u16 u8 u8 cases=8 */
void func_02054e64(u8 *self, u16 kind, u8 b2, u8 b3)
{
    u32 e = kind | (b2 << 16) | ((u32)b3 << 24);

    func_0207fe28(data_020c0948, 0x30);
    if (U32_AT(self, 0x10) < U32_AT(self, 0x14)) {
        U32_AT(self, 0x10)++;
        U32_AT(PTR_AT(self, 0xc), (U32_AT(self, 0x10) - 1) * 4) = e;
    } else {
        func_020062f0(self + 0xc, (void **)&e, 0);
    }
    func_0207fe24();
}

/* 0x02054f04: deleting destructor
 * @difftest zero:0x24 stub=0x02006acc,0x0207fe44 cases=3 */
u8 *func_02054f04(u8 *self)
{
    PTR_AT(self, 0) = data_020c0928;
    func_02006acc(self + 0x18);
    func_02006acc(self + 0xc);
    func_0207fe44(self);
    return self;
}

/* 0x02054f3c: destructor
 * @difftest zero:0x24 stub=0x02006acc cases=3 */
u8 *func_02054f3c(u8 *self)
{
    PTR_AT(self, 0) = data_020c0928;
    func_02006acc(self + 0x18);
    func_02006acc(self + 0xc);
    return self;
}

/* 0x02054f6c: constructor: at `pos`
 * @difftest ptr:0x24:4 ptr:8:4 stub=0x02006330 cases=3 */
u8 *func_02054f6c(u8 *self, const s32 *pos)
{
    PTR_AT(self, 0) = data_020c0928;
    S32_AT(self, 4) = pos[0];
    S32_AT(self, 8) = pos[1];
    U32_AT(self, 0xc) = 0;
    U32_AT(self, 0x10) = 0;
    U32_AT(self, 0x14) = 0;
    U32_AT(self, 0x18) = 0;
    U32_AT(self, 0x1c) = 0;
    U32_AT(self, 0x20) = 0;
    func_0207fe28(data_020c0948, 0x1c);
    if (U32_AT(self, 0x14) < 0xc)
        func_02006330(self + 0xc, 0xc);
    func_0207fe24();
    return self;
}
