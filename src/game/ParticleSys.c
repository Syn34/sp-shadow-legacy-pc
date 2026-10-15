/*
 * Particle emitters (see ParticleBase.c): constructors, destructors and
 * placement. The object base is func_0205c068 / func_0205bf90 (vtable
 * data_020c0c64), with a second base at +0xac (func_0205b76c /
 * func_0205b728, vtable data_020c0c8c; this-adjusting entry points
 * subtract 0xac), 60 particles at +0xb4 (constructed with func_0205bd70,
 * destroyed with func_0205bd40) and a texture reference at +0x3bd0.
 * ARM9 main, 0x02058900 - 0x020590c4 (12 functions).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020c0c64[], data_020c0c8c[];   /* vtables */
extern const fx32 data_020b6cdc;
extern const fx32 data_020c0a6c, data_020c0a70, data_020c0a74, data_020c0a78, data_020c0c1c,
    data_020c0c20, data_020c0c24, data_020c0c28, data_020c0c2c, data_020c0c30, data_020c0c34,
    data_020c0c38, data_020c0c3c, data_020c0c40, data_020c0c44, data_020c0c48, data_020c0c4c,
    data_020c0c50, data_020c0c54, data_020c0c58;   /* default settings */

void func_02057464(u8 *self, u32 kind);
void func_02057074(u8 *self);
void func_02005b98(s32 *out, const void *src);
void func_0201751c(s32 *out, const void *obj);
void func_02071b84(void *ref);
void func_02071d78(void *ref);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));
void func_020a6dcc(void *arr, u32 n, u32 size, void *(*ctor)(void *), void *(*dtor)(void *));
void *func_0205bd40(void *p);
void *func_0205bd70(void *p);
void func_0205b728(void *self);
void func_0205b76c(void *self);
void func_0205bf90(void *self);
void func_0205c068(void *self);
void func_0207fe44(void *p);

void func_02058954(u8 *self, u32 kind, void *follow);
u8 *func_02058a5c(u8 *self);
u8 *func_02058ac8(u8 *self);

#define F(self, off) S32_AT(self, 0x3000 + (off))

/* 0x02058900: use preset `kind` at `pos`, turned by `rot`
 * @difftest zero:0x3d00 int:0:14 ptr:0xc:4 ptr:0xc:4 stub=0x02057464 cases=5 */
void func_02058900(u8 *self, u32 kind, const s32 *pos, const s32 *rot)
{
    S32_AT(self, 4) = pos[0];
    S32_AT(self, 8) = pos[1];
    S32_AT(self, 0xc) = pos[2];
    self[0x8c] = 1;
    S32_AT(self, 0x10) = rot[0];
    S32_AT(self, 0x14) = rot[1];
    S32_AT(self, 0x18) = rot[2];
    self[0x8c] = 1;
    func_02057464(self, kind);
}

/* 0x02058954: use preset `kind`, following `follow` (if any)
 * @difftest zero:0x3d00 int:0:14 pick:0 stub=0x02057464 cases=3
 * @difftest zero:0x3d00 int:0:14 ptr:0x100:4 stub=0x02057464 cases=3 */
void func_02058954(u8 *self, u32 kind, void *follow)
{
    PTR_AT(self, 0x3bdc) = follow;
    if (PTR_AT(self, 0x3bdc) != NULL) {
        s32 p[3], r[3];

        func_02005b98(p, follow);
        S32_AT(self, 4) = p[0];
        S32_AT(self, 8) = p[1];
        S32_AT(self, 0xc) = p[2];
        self[0x8c] = 1;
        func_0201751c(r, PTR_AT(self, 0x3bdc));
        S32_AT(self, 0x10) = r[0];
        S32_AT(self, 0x14) = r[1];
        S32_AT(self, 0x18) = r[2];
        self[0x8c] = 1;
    }
    func_02057464(self, kind);
}

/* 0x020589e8: start spawning again
 * @difftest ptr:0x3d00:4 cases=3 */
void func_020589e8(u8 *self)
{
    self[0x3bc5] = 0;
}

/* what the destructors do */
static void Destroy(u8 *self)
{
    PTR_AT(self, 0) = data_020c0c64;
    PTR_AT(self, 0xac) = data_020c0c8c;
    func_02071b84(self + 0x3bd0);
    func_020a6d58(self + 0xb4, 0x3c, 0xfc, func_0205bd40);
    func_0205b728(self + 0xac);
    func_0205bf90(self);
}

/* 0x020589f8: destructor
 * @difftest zero:0x3d00 stub=0x02071b84,0x020a6d58,0x0205b728,0x0205bf90 cases=3 */
u8 *func_020589f8(u8 *self)
{
    Destroy(self);
    return self;
}

/* 0x02058a5c: deleting destructor
 * @difftest zero:0x3d00 stub=0x02071b84,0x020a6d58,0x0205b728,0x0205bf90,0x0207fe44 cases=3 */
u8 *func_02058a5c(u8 *self)
{
    Destroy(self);
    func_0207fe44(self);
    return self;
}

/* 0x02058ac8: destructor (another copy)
 * @difftest zero:0x3d00 stub=0x02071b84,0x020a6d58,0x0205b728,0x0205bf90 cases=3 */
u8 *func_02058ac8(u8 *self)
{
    Destroy(self);
    return self;
}

/* what the constructors share up to the followed object */
static void Init(u8 *self)
{
    func_0205c068(self);
    func_0205b76c(self + 0xac);
    PTR_AT(self, 0) = data_020c0c64;
    PTR_AT(self, 0xac) = data_020c0c8c;
    func_020a6dcc(self + 0xb4, 0x3c, 0xfc, func_0205bd70, func_0205bd40);
    self[0x3bc4] = 0;
    self[0x3bc5] = 0;
    self[0x3bc6] = 0;
    F(self, 0xbc8) = 0;
    F(self, 0xbcc) = 0x200;
    func_02071d78(self + 0x3bd0);
}

/* the fields after the followed object (velocity, spread, scale and
 * sphere radius given) */
static void InitFields(u8 *self, const fx32 *v0, const fx32 *v1, const fx32 *v2, const fx32 *s0,
                       const fx32 *s1, const fx32 *s2, const fx32 *s3, const fx32 *r0,
                       const fx32 *r1, const fx32 *r2)
{
    F(self, 0xbe0) = 0;
    F(self, 0xbe4) = 0;
    F(self, 0xbe8) = 0;
    F(self, 0xbec) = 0;
    F(self, 0xbf0) = 0;
    F(self, 0xbf4) = 0;
    F(self, 0xbf8) = 0;
    F(self, 0xbfc) = 0;
    F(self, 0xc00) = 0;
    F(self, 0xc04) = 0x1000;
    F(self, 0xc08) = 0;
    F(self, 0xc0c) = 0;
    F(self, 0xc10) = -1;
    F(self, 0xc14) = 0xb33;
    F(self, 0xc18) = 0x2000;
    F(self, 0xc1c) = 0x32000;
    F(self, 0xc20) = *v0;
    F(self, 0xc24) = *v1;
    F(self, 0xc28) = *v2;
    F(self, 0xc2c) = data_020b6cdc;
    F(self, 0xc30) = *s0;
    F(self, 0xc34) = *s1;
    F(self, 0xc38) = *s2;
    F(self, 0xc3c) = *s3;
    F(self, 0xc40) = 0x1f;
    F(self, 0xc44) = 0;
    F(self, 0xc48) = 0;
    F(self, 0xc4c) = *r0;
    F(self, 0xc50) = *r1;
    F(self, 0xc54) = *r2;
    F(self, 0xc58) = 0;
    F(self, 0xc5c) = 0;
    self[0x3c60] = 0;
}

/* 0x02058b2c: constructor: preset `kind`, following `follow`
 * @difftest ptr:0x3d00:4 int:0:14 pick:0 stub=0x0205c068,0x0205b76c,0x020a6dcc,0x02071d78,0x02057464 cases=3 */
u8 *func_02058b2c(u8 *self, u32 kind, void *follow)
{
    Init(self);
    PTR_AT(self, 0x3bdc) = follow;
    InitFields(self, &data_020c0c40, &data_020c0c3c, &data_020c0c38, &data_020c0c34,
               &data_020c0c30, &data_020c0c2c, &data_020c0c28, &data_020c0c24, &data_020c0c20,
               &data_020c0c1c);
    F(self, 0xc64) = kind;
    func_02058954(self, kind, follow);
    return self;
}

/* 0x02058d0c: constructor
 * @difftest ptr:0x3d00:4 stub=0x0205c068,0x0205b76c,0x020a6dcc,0x02071d78 cases=3 */
u8 *func_02058d0c(u8 *self)
{
    Init(self);
    PTR_AT(self, 0x3bdc) = NULL;
    InitFields(self, &data_020c0a74, &data_020c0a70, &data_020c0a78, &data_020c0a6c,
               &data_020c0c58, &data_020c0c54, &data_020c0c50, &data_020c0c4c, &data_020c0c48,
               &data_020c0c44);
    return self;
}

/* 0x02058ed0: constructor (another copy)
 * @difftest ptr:0x3d00:4 stub=0x0205c068,0x0205b76c,0x020a6dcc,0x02071d78 cases=3 */
u8 *func_02058ed0(u8 *self)
{
    Init(self);
    PTR_AT(self, 0x3bdc) = NULL;
    InitFields(self, &data_020c0a74, &data_020c0a70, &data_020c0a78, &data_020c0a6c,
               &data_020c0c58, &data_020c0c54, &data_020c0c50, &data_020c0c4c, &data_020c0c48,
               &data_020c0c44);
    return self;
}

/* The second base's this-adjusting entry points. */

/* 0x02059094
 * @difftest $S=zero:0x3d00 $S+0xac stub=0x02071b84,0x020a6d58,0x0205b728,0x0205bf90,0x0207fe44 cases=3 */
u8 *func_02059094(u8 *a)
{
    return func_02058a5c(a - 0xac);
}

/* 0x020590a4
 * @difftest $S=zero:0x3d00 $S+0xac stub=0x02071b84,0x020a6d58,0x0205b728,0x0205bf90 cases=3 */
u8 *func_020590a4(u8 *a)
{
    return func_02058ac8(a - 0xac);
}

/* 0x020590b4
 * @difftest $S=zero:0x3d00 $S+0xac cases=3 */
void func_020590b4(u8 *a)
{
    func_02057074(a - 0xac);
}
