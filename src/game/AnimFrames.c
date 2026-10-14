/*
 * Frame sets and frame animations. A frame set (0x10 bytes, vtable
 * data_020bfcd8) holds the frames (func_02059f80) of one animation id:
 * +4 the id, +8 the number of frames, +0xc a vector of them. Sets are
 * shared through the cache data_020f53b4 (0xc bytes per id: the vector
 * and a reference count). A frame animation (0x20 bytes, vtable
 * data_020bfc94) plays a set: +4 the set, +0x14 time per frame, +0x18
 * time, +0x1c the frame.
 * ARM9 main, 0x0204a1f4 - 0x0204a794 (14 functions).
 *
 * The frames of id `id` are the run of entries of data_020c3b70 (0x18
 * bytes each, data_020d5b8c + 0x5a4 of them) whose +0x14 is `id`.
 *
 * Testing: with fake sets; the cache is the game's.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bfc94[], data_020bfcd8[];   /* vtables */
extern const char data_020bfce0[];   /* source file name for the heap */
extern u8 *data_020c3b70;            /* frames, 0x18 bytes each */
extern u8 data_020d5b8c[];
extern u8 *data_020f53b4;            /* the frame set cache, 0xc bytes per id */

u64 func_020a4e00(s32 num, s32 den);  /* signed divide: quotient in r0, remainder in r1 */
void func_02006acc(void *v);
void func_02006330(void *v, u32 n);
void func_020062f0(void *vec, void **value, u32 tag);
void *func_02059f80(void *p, u32 i);
void func_0205914c(void *self);
void func_020591c0(void *self);
void *func_0207fe74(u32 size);
void func_0207fe44(void *p);
void func_0207fe28(const char *file, s32 line);
void func_0207fe24(void);

void func_0204a250(u8 *self, s32 id);
void func_0204a340(u8 *self);
void func_0204a450(u8 *self, s32 id);
void func_0204a5fc(u8 *self, s32 id);
u8 *func_0204a738(u8 *self);
u8 *func_0204a764(u8 *self);

#define CACHE(id) (data_020f53b4 + (id) * 0xc)

/* 0x0204a1f4: advance the animation
 * @difftest $S=zero:0x20 @$S+0xc:16=int:1:6 @$S+0x14:32=int:0:4 @$S+0x18:32=int:0:6 @$S+0x1c:16=int:0:6 $S int:0:3 cases=40 */
void func_0204a1f4(u8 *self, s32 dt)
{
    if (S32_AT(self, 0x18) < S32_AT(self, 0x14)) {
        S32_AT(self, 0x18) += dt;
        return;
    }
    S32_AT(self, 0x18) = 0;
    U16_AT(self, 0x1c)++;
    U16_AT(self, 0x1c) = (u32)(func_020a4e00(U16_AT(self, 0x1c), U16_AT(self, 0xc)) >> 32);
}

/* 0x0204a250: play frame set `id`
 * @difftest zero:0x20 int:-1:1 cases=3 */
void func_0204a250(u8 *self, s32 id)
{
    func_0204a5fc(self + 4, id);
}

/* 0x0204a260: the animation's deleting destructor
 * @difftest zero:0x20 stub=0x0207fe44 cases=3 */
u8 *func_0204a260(u8 *self)
{
    PTR_AT(self, 0) = data_020bfc94;
    func_0204a738(self + 4);
    func_0207fe44(self);
    return self;
}

/* 0x0204a290: the animation's destructor
 * @difftest zero:0x20 cases=3 */
u8 *func_0204a290(u8 *self)
{
    PTR_AT(self, 0) = data_020bfc94;
    func_0204a738(self + 4);
    return self;
}

/* 0x0204a2b8: the animation's constructor (frame set `id`)
 * @difftest zero:0x20 int:-1:1 cases=3 */
u8 *func_0204a2b8(u8 *self, s32 id)
{
    PTR_AT(self, 0) = data_020bfc94;
    func_0204a764(self + 4);
    S32_AT(self, 0x14) = 0;
    S32_AT(self, 0x18) = 0;
    U16_AT(self, 0x1c) = 0;
    func_0204a250(self, id);
    return self;
}

/* 0x0204a308: the animation's constructor
 * @difftest zero:0x20 cases=3 */
u8 *func_0204a308(u8 *self)
{
    PTR_AT(self, 0) = data_020bfc94;
    func_0204a764(self + 4);
    S32_AT(self, 0x14) = 0;
    S32_AT(self, 0x18) = 0;
    U16_AT(self, 0x1c) = 0;
    return self;
}

/* 0x0204a340: let go of the frames (deleted with the last user)
 * @difftest $S=zero:0x10 @$S+4:32=pick:-1,0 $S cases=3
 * @difftest $T=zero:0x10 @$T+4:32=0x020343a8 $F=zero:0x10 @$F+0:32=$T $D=zero:0x10 @$D+0:32=$F @$D+8:32=$F $V=zero:0xc @$V+0:32=$D @$V+4:32=3 @$V+8:32=4 $S=zero:0x10 @$S+4:32=1 @$S+0xc:32=$V $C=zero:0x30 @$C+0x14:32=1 @020f53b4:32=$C $S stub=0x02006acc,0x0207fe44 cases=3 */
void func_0204a340(u8 *self)
{
    s32 id = S32_AT(self, 4);
    s32 ok;
    u8 *v;
    u32 i;

    ok = id > 0 ? S32_AT(CACHE(id), 8) == 1 : 0;
    if (ok == 0)
        return;
    v = PTR_AT(self, 0xc);
    for (i = 0; i < U32_AT(v, 4); i++) {
        u8 *e = ((u8 **)PTR_AT(v, 0))[i];

        if (e != NULL) {
            if (e != NULL) {
                void (*m)(void *, void *) = VCALL(e, 4, void (*)(void *, void *));
                m(e, (void *)m);
            }
            ((u8 **)PTR_AT(PTR_AT(self, 0xc), 0))[i] = NULL;
        }
        v = PTR_AT(self, 0xc);
    }
    U32_AT(v, 4) -= U32_AT(v, 4);
    v = PTR_AT(self, 0xc);
    if (v != NULL) {
        func_02006acc(v);
        func_0207fe44(v);
    }
    PTR_AT(self, 0xc) = NULL;
}

/* 0x0204a450: make the frames of id `id`
 * @difftest $S=zero:0x10 $S int:0:4 stub=0x02059f80 cases=5 */
void func_0204a450(u8 *self, s32 id)
{
    u32 first = 0, n = U32_AT(data_020d5b8c, 0x5a4), i;
    u8 *v;

    for (i = 0; i < n; i++) {
        if (id == S32_AT(data_020c3b70 + i * 0x18, 0x14)) {
            if (first == 0)
                first = i;
            U16_AT(self, 8)++;
        }
    }
    func_0207fe28(data_020bfce0, 0x5a);
    v = func_0207fe74(0xc);
    if (v != NULL) {
        U32_AT(v, 0) = 0;
        U32_AT(v, 4) = 0;
        U32_AT(v, 8) = 0;
    }
    PTR_AT(self, 0xc) = v;
    if (U16_AT(self, 8) > U32_AT(PTR_AT(self, 0xc), 8))
        func_02006330(PTR_AT(self, 0xc), U16_AT(self, 8));
    func_0207fe24();
    v = PTR_AT(self, 0xc);
    U32_AT(v, 4) -= U32_AT(v, 4);
    func_0207fe28(data_020bfce0, 0x62);
    for (i = first; i < first + U16_AT(self, 8); i++) {
        void *f = func_0207fe74(0xc);

        if (f != NULL)
            f = func_02059f80(f, i);
        v = PTR_AT(self, 0xc);
        if (U32_AT(v, 4) < U32_AT(v, 8)) {
            U32_AT(v, 4)++;
            ((void **)PTR_AT(v, 0))[U32_AT(v, 4) - 1] = f;
        } else {
            func_020062f0(v, &f, 0);
        }
    }
    func_0207fe24();
}

/* 0x0204a5fc: use frame set `id` (made on first use)
 * @difftest $S=zero:0x10 $S pick:-1,0 cases=3
 * @difftest $V=zero:0xc @$V+4:32=3 $C=zero:0x30 @$C+0x14:32=pick:0,2 @$C+0xc:32=$V @020f53b4:32=$C $S=zero:0x10 @$S+0xc:32=$V $S 1 stub=0x0204a450 cases=4 */
void func_0204a5fc(u8 *self, s32 id)
{
    s32 ok;

    S32_AT(self, 4) = id;
    ok = S32_AT(self, 4) > 0 ? S32_AT(CACHE(S32_AT(self, 4)), 8) == 0 : 0;
    if (ok != 0) {
        u8 *v;

        if (PTR_AT(self, 0xc) == NULL)
            func_0204a450(self, id);
        v = PTR_AT(self, 0xc);
        if (S32_AT(self, 4) <= 0)
            return;
        PTR_AT(CACHE(S32_AT(self, 4)), 0) = v;
        S32_AT(CACHE(S32_AT(self, 4)), 8)++;
        return;
    }
    {
        u8 *v;

        if (S32_AT(self, 4) <= 0) {
            v = NULL;
        } else {
            S32_AT(CACHE(S32_AT(self, 4)), 8)++;
            v = PTR_AT(CACHE(S32_AT(self, 4)), 0);
        }
        PTR_AT(self, 0xc) = v;
        U16_AT(self, 8) = U32_AT(PTR_AT(self, 0xc), 4);
    }
}

/* 0x0204a704: the frame set's deleting destructor
 * @difftest $S=zero:0x10 @$S+4:32=-1 $S stub=0x0205914c,0x0207fe44 cases=3 */
u8 *func_0204a704(u8 *self)
{
    PTR_AT(self, 0) = data_020bfcd8;
    func_0204a340(self);
    func_0205914c(self);
    func_0207fe44(self);
    return self;
}

/* 0x0204a738: the frame set's destructor
 * @difftest $S=zero:0x10 @$S+4:32=-1 $S stub=0x0205914c cases=3 */
u8 *func_0204a738(u8 *self)
{
    PTR_AT(self, 0) = data_020bfcd8;
    func_0204a340(self);
    func_0205914c(self);
    return self;
}

/* 0x0204a764: the frame set's constructor
 * @difftest zero:0x10 stub=0x020591c0 cases=3 */
u8 *func_0204a764(u8 *self)
{
    func_020591c0(self);
    PTR_AT(self, 0) = data_020bfcd8;
    U16_AT(self, 8) = 0;
    PTR_AT(self, 0xc) = NULL;
    return self;
}
