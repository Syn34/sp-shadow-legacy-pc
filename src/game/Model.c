/*
 * The model class used by the game's objects (0xdc bytes, usually at
 * +0xf4 of an object): an NNS G3D render object with a joint animation and
 * a material animation (animation table data_020c3b04, 0x24 bytes per
 * animation: +0 type (9 = material), +4 file, +0x10 loops, +0x1c length,
 * +0x20 model), per-material alpha, and a map of the bones that other
 * things are attached to.
 * ARM9 main, 0x0204e42c - 0x0204fb70 (36 functions).
 *
 * Layout:
 *   +0x00 joint animation (-1: none)    +0x04 material animation
 *   +0x08 its animation object           +0x0c the material one's
 *   +0x10 the joint result buffer        +0x14 flags (bit 0 finished,
 *         2 animated, 3 set up, 4 has a material animation, 5 has a
 *         model, 6 has attached bones, 7 frame frozen)
 *   +0x18 / +0x1c materials drawn opaque / at half alpha (bit masks)
 *   +0x20 the render object (0x54)       +0x74 the model file (0x14;
 *         the file itself at +0x80)
 *   +0x88 / +0xa0 animation files (0x18 each: vtable data_020c02d8, file,
 *         loaded, size, data, animation object)
 *   +0xb8 a list of things to free       +0xbc frame (fx32, 30 per tick)
 *   +0xc0 the last frame                 +0xc4 the bones: a std::map of
 *         name -> {s8 done, u32 node, Mtx44}
 *   +0xd0 a std::vector of {parent, child} bone pairs
 *
 * Testing: with fake models; the NNS G3D and file routines run for real
 * where fake data is enough, and are stubbed where they need a real file.
 *
 * New bones get zeros where the original leaves stack garbage (see
 * ModelBones.c), so func_0204f258's recorded game calls are not replayed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 *data_020c3b04;            /* animations, 0x24 bytes each */
extern u8 *data_020c3b0c;            /* actor kinds, 0x48 bytes each */
extern u8 *data_020c3b40;            /* 0x40 bytes each */
extern u32 *data_020c3b54;           /* model files */
extern u8 data_020c4b8c[];
extern u8 data_020c02d8[];           /* animation file vtable */
extern const char data_020c02e0[], data_020c02f0[];   /* source file names */

typedef struct {
    u32 w[4];
} Allocator;

typedef struct {
    s8 done;
    u32 node;
    s32 mtx[16];
} Bone;

void MI_CpuFill8(void *dest, u8 data, u32 size);
void MI_Copy64B(const void *src, void *dest);
void func_02080488(void *file, u32 id);
u32 func_02080388(void *file);
void func_02080458(void *file);
void *func_0207fe5c(u32 size);
void func_0207fe2c(void *p);
void func_0207fe44(void *p);
void func_02080110(Allocator *out);
void *func_02089868(void *file, u32 i);
void func_0207fe28(const char *file, s32 line);
void func_0207fe24(void);
void *func_02089200(Allocator *a, void *res, void *mdl);
void func_020860b0(void *obj, void *res, void *mdl, u32 a);
void func_020891f4(Allocator *a, void *obj);
void func_02050f64(void *self);
void func_02050fd8(void *self);
void func_0208ea6c(void *mtx44);
void func_0208e31c(const void *mtx43, void *mtx44);
void func_02017538(s32 *out, const s32 *a, const s32 *b);
void func_02040230(u8 **it);
void *func_020062d0(void);
u32 func_020822c0(void *timer);
void *func_020062e0(void);
void func_02089228(void *mdl, u32 mat, u32 alpha);
void func_02089270(void *mdl, u32 mat, u32 id);
void func_02059a84(void *file);
void func_020885a0(void *obj);
void func_02088d08(void);
void func_02059af4(void *file);
void func_020040f4(u8 *dst, const u8 *src);
void *func_02082cb0(u8 *cam);
void func_02082ce8(void *cam);
s32 func_020890a8(void *obj, void *mtx43, u32 a, u32 node);
void func_0208e444(const void *mtx43, void *mtx44);
void func_02085d9c(void *obj, void *anm);
void func_02085e78(void *obj, void *anm);
void func_020599ec(void *link);
void func_02059b10(void *file);
void func_0204e5b0(u8 *l);
void func_0204fcf8(u8 *map);
void func_0204fd80(u8 *vec, u32 n, u32 tag);
void func_0204fc60(u8 **out, u8 *map, const void *key);
u8 *func_0204fb70(u8 *map, const void *key);
void func_0204fd94(u8 *vec, u32 n, const void *value, u32 tag);
u32 func_02088f60(u8 *out, void *sbc, u32 node);
void *func_020891dc(Allocator *a, void *mdl);
void func_02059bbc(void *file, u32 id, u32 a);
void func_02086048(void *obj, void *mdl);
void func_0204fd30(u8 *vec);
void func_0204fcd4(u8 *map);
void func_02059db4(void *file);
void func_02059dd8(void *file);

void func_0204e574(u8 *l, void *mdl);
u8 *func_0204e870(u8 *m, u32 node);
void func_0204ebe8(u8 *m);
void func_0204ed14(u8 *m);
void func_0204ed80(u8 *m, u32 id, s32 frame);
u8 *func_0204eed4(u8 *m, u32 id, u32 type);
void func_0204f258(u8 *m);
void func_0204f384(u8 *m);
void func_0204f648(u8 *m, u32 kind);
void func_0204f790(u8 *m, u32 i, u32 a);
void func_0204f7d4(u8 *m, u32 file, u32 anim);
void func_0204f8d4(u8 *m, u32 kind, u32 a, u32 bones);
u8 *func_0204e654(u8 *l);
u8 *func_0204e680(u8 *l);

#define ANIM(id) (data_020c3b04 + (id) * 0x24)
/* the model set of the model file */
#define MDL(m) ((u8 *)PTR_AT(m, 0x80) \
                + U32_AT((u8 *)PTR_AT(m, 0x80) + U16_AT(PTR_AT(m, 0x80), 0xe), 0xc))
/* the name of node i */
#define NODE_NAMES(mdl) ((mdl) + 0x40 + U16_AT((mdl) + 0x40, 6) \
                         + U16_AT((mdl) + 0x40 + U16_AT((mdl) + 0x40, 6), 2))

/* 0x0204e42c: switch an animation file to file `id`; its animation object
 * @difftest $L=zero:0x18 @$L+4:32=pick:5,6 @$L+0x10:32=$L @$L+0xc:32=0x10 @$L+0x14:32=u32 $L pick:5,6 ptr:4 stub=0x02080488,0x02080388:0x10,0x02080418,0x02080458,0x02089868:0,0x020860b0 cases=8 */
u8 *func_0204e42c(u8 *l, u32 id, void *mdl)
{
    u8 file[0x48];

    if (id != U32_AT(l, 4)) {
        U32_AT(l, 4) = id;
        MI_CpuFill8(PTR_AT(l, 0x10), 0, U32_AT(l, 0xc));
        func_02080488(file, id);
        func_02080418(PTR_AT(l, 0x10), func_02080388(file), 1, file);
        func_02080458(file);
        func_0204e574(l, mdl);
    }
    return PTR_AT(l, 0x14);
}

/* 0x0204e4ac: load animation file `id` for model `mdl`
 * @difftest zero:0x18 u32 ptr:4 stub=0x02080488,0x02080388:0x40,0x0207fe5c:0x1234,0x02080418,0x02080458,0x02089868:0x5678,0x02089200:0x9abc,0x020860b0 cases=3 */
void func_0204e4ac(u8 *l, u32 id, void *mdl)
{
    u8 file[0x48];
    Allocator a, t;
    void *res;

    U32_AT(l, 4) = id;
    func_02080488(file, id);
    U32_AT(l, 0xc) = func_02080388(file);
    PTR_AT(l, 0x10) = func_0207fe5c(U32_AT(l, 0xc));
    func_02080418(PTR_AT(l, 0x10), U32_AT(l, 0xc), 1, file);
    func_02080458(file);
    func_02080110(&t);
    a = t;
    res = func_02089868(PTR_AT(l, 0x10), 0);
    func_0207fe28(data_020c02e0, 0x76);
    PTR_AT(l, 0x14) = func_02089200(&a, res, mdl);
    func_020860b0(PTR_AT(l, 0x14), res, mdl, 0);
    func_0207fe24();
    l[8] = 1;
}

/* 0x0204e574: bind the animation to model `mdl`
 * @difftest ptr:0x18:4 ptr:4 stub=0x02089868:0x5678,0x020860b0 cases=3 */
void func_0204e574(u8 *l, void *mdl)
{
    func_020860b0(PTR_AT(l, 0x14), func_02089868(PTR_AT(l, 0x10), 0), mdl, 0);
}

/* 0x0204e5b0: free the animation file
 * @difftest $L=zero:0x18 @$L+8:8=pick:0,1 @$L+0x10:32=pick:0,0x1234 $L stub=0x020891f4,0x0207fe2c cases=8 */
void func_0204e5b0(u8 *l)
{
    Allocator a, t;

    if (S8_AT(l, 8) != 1)
        return;
    func_02080110(&t);
    a = t;
    func_020891f4(&a, PTR_AT(l, 0x14));
    if (PTR_AT(l, 0x10) != NULL) {
        func_0207fe2c(PTR_AT(l, 0x10));
        PTR_AT(l, 0x10) = NULL;
    }
    l[8] = 0;
}

/* 0x0204e620: the animation file's deleting destructor
 * @difftest zero:0x18 stub=0x02050f64,0x0207fe44 cases=3 */
u8 *func_0204e620(u8 *l)
{
    PTR_AT(l, 0) = data_020c02d8;
    func_0204e5b0(l);
    func_02050f64(l);
    func_0207fe44(l);
    return l;
}

/* 0x0204e654: the animation file's destructor
 * @difftest zero:0x18 stub=0x02050f64 cases=3 */
u8 *func_0204e654(u8 *l)
{
    PTR_AT(l, 0) = data_020c02d8;
    func_0204e5b0(l);
    func_02050f64(l);
    return l;
}

/* 0x0204e680: the animation file's constructor
 * @difftest ptr:0x18:4 stub=0x02050fd8 cases=3 */
u8 *func_0204e680(u8 *l)
{
    func_02050fd8(l);
    PTR_AT(l, 0) = data_020c02d8;
    l[8] = 0;
    U32_AT(l, 0xc) = 0;
    U32_AT(l, 0x10) = 0;
    U32_AT(l, 0x14) = 0;
    return l;
}

/* set up bone value `b` (node `n`) from the joint results if not yet done */
static void SetUpBone(u8 *m, u8 *b, u32 n)
{
    u32 off;

    func_0208ea6c(b + 8);
    off = n * 0x58;
    if (!(U32_AT(PTR_AT(m, 0x10), off) & 2))
        func_0208e31c((u8 *)PTR_AT(m, 0x10) + off + 0x28, b + 8);
    if (!(U32_AT(PTR_AT(m, 0x10), off) & 4)) {
        u8 *r = (u8 *)PTR_AT(m, 0x10) + off;
        S32_AT(b, 0x38) = S32_AT(r, 0x4c);
        S32_AT(b, 0x3c) = S32_AT(r, 0x50);
        S32_AT(b, 0x40) = S32_AT(r, 0x54);
    }
    b[0] = 1;
}

/* 0x0204e6b8: work out the attached bones' matrices (parent before child)
 * @difftest $M=zero:0xdc $R=ptr:0x200:4 @$M+0x10:32=0 $A=ptr:0x48:4 $B=ptr:0x48:4 @$A+4:32=int:0:3 @$B+4:32=int:0:3 @$A:8=pick:0,1 @$B:8=pick:0,1 $P=zero:0x10 @$P:32=$A @$P+4:32=$B @$P+8:32=$B @$P+0xc:32=$A @$M+0xd0:32=$P @$M+0xd4:32=pick:0,1,2 $M cases=20
 * @difftest $M=zero:0xdc $R=ptr:0x200:4 @$M+0x10:32=$R $A=ptr:0x48:4 $B=ptr:0x48:4 @$A+4:32=int:0:3 @$B+4:32=int:0:3 @$A:8=pick:0,1 @$B:8=pick:0,1 $P=zero:0x10 @$P:32=$A @$P+4:32=$B @$P+8:32=$B @$P+0xc:32=$A @$M+0xd0:32=$P @$M+0xd4:32=pick:0,1,2 $M cases=20 */
void func_0204e6b8(u8 *m)
{
    u32 i;
    s32 t[16];

    if (PTR_AT(m, 0x10) == NULL)
        return;
    for (i = 0; i < U32_AT(m, 0xd4); i++) {
        u8 **pair = (u8 **)((u8 *)PTR_AT(m, 0xd0) + i * 8);
        u8 *a = pair[0], *b = pair[1];
        u32 na = U32_AT(a, 4), nb = U32_AT(b, 4);

        if (S8_AT(a, 0) == 0)
            SetUpBone(m, a, na);
        if (S8_AT(b, 0) == 0)
            SetUpBone(m, b, nb);
        func_02017538(t, (s32 *)(a + 8), (s32 *)(b + 8));
        MI_Copy64B(t, b + 8);
    }
}

/* 0x0204e820: mark all bones not done
 * @difftest $M=zero:0xdc @$M+0xcc:32=$M+0xc8 $M cases=3
 * @difftest $M=zero:0xdc $N=ptr:0x7c:4 $O=ptr:0x7c:4 @$N+0:32=0 @$N+4:32=$O @$N+8:32=$M+0xc8 @$O+0:32=0 @$O+4:32=0 @$O+8:32=$N+1 @$M+0xc8:32=$N @$M+0xcc:32=$N $M cases=3 */
void func_0204e820(u8 *m)
{
    u8 *it = PTR_AT(m, 0xcc);

    while (it != m + 0xc8) {
        it[0x10] = 0;
        func_02040230(&it);
    }
}

/* 0x0204e870: the bone of node `node` (NULL if none)
 * @difftest $M=zero:0xdc @$M+0xcc:32=$M+0xc8 $M u32 cases=3
 * @difftest $M=zero:0xdc $N=ptr:0x7c:4 $O=ptr:0x7c:4 @$N+0:32=0 @$N+4:32=$O @$N+8:32=$M+0xc8 @$N+0x14:32=1 @$O+0:32=0 @$O+4:32=0 @$O+8:32=$N+1 @$O+0x14:32=2 @$M+0xc8:32=$N @$M+0xcc:32=$N $M int:0:4 cases=12 */
u8 *func_0204e870(u8 *m, u32 node)
{
    u8 *it = PTR_AT(m, 0xcc);

    while (it != m + 0xc8) {
        if (node == U32_AT(it, 0x14))
            return it + 0x10;
        func_02040230(&it);
    }
    return NULL;
}

/* 0x0204e8d0: draw material `i` at half alpha or not
 * @difftest ptr:0xdc:4 int:0:32 pick:0,1 cases=20 */
void func_0204e8d0(u8 *m, u32 i, u32 on)
{
    if (on)
        U32_AT(m, 0x1c) |= 1u << i;
    else
        U32_AT(m, 0x1c) &= ~(1u << i);
}

/* 0x0204e90c: draw material `i` opaque or not
 * @difftest ptr:0xdc:4 int:0:32 pick:0,1 cases=20 */
void func_0204e90c(u8 *m, u32 i, u32 on)
{
    if (on)
        U32_AT(m, 0x18) |= 1u << i;
    else
        U32_AT(m, 0x18) &= ~(1u << i);
}

/* 0x0204e948: draw the model at alpha `alpha`
 * @difftest $M=zero:0xdc @$M+0x14:32=pick:0,0x80,0x40 $X=zero:0x10 $H=zero:8 @$X+8:32=$H @$H+4:16=int:0:4 @$X:32=int:0:0x5000 @$M+0xc:32=0 @$M+4:32=int:1:3 $A=zero:4 @$M+8:32=0 @$M+0xbc:32=u32 $M int:0:32 stub=0x020822c0:1 cases=10
 * @difftest $M=zero:0xdc @$M+0x14:32=pick:0,0x80,0x40 $X=zero:0x10 $H=zero:8 @$X+8:32=$H @$H+4:16=int:0:4 @$X:32=int:0:0x5000 @$M+0xc:32=0 @$M+4:32=int:1:3 $A=zero:4 @$M+8:32=$A @$M+0xbc:32=u32 $M int:0:32 stub=0x020822c0:1 cases=10
 * @difftest $M=zero:0xdc @$M+0x14:32=pick:0,0x80,0x40 $X=zero:0x10 $H=zero:8 @$X+8:32=$H @$H+4:16=int:0:4 @$X:32=int:0:0x5000 @$M+0xc:32=$X @$M+4:32=int:1:3 $A=zero:4 @$M+8:32=0 @$M+0xbc:32=u32 $M int:0:32 stub=0x020822c0:1 cases=10
 * @difftest $M=zero:0xdc @$M+0x14:32=pick:0,0x80,0x40 $X=zero:0x10 $H=zero:8 @$X+8:32=$H @$H+4:16=int:0:4 @$X:32=int:0:0x5000 @$M+0xc:32=$X @$M+4:32=int:1:3 $A=zero:4 @$M+8:32=$A @$M+0xbc:32=u32 $M int:0:32 stub=0x020822c0:1 cases=10
 * @difftest $M=zero:0xdc @$M+0x14:32=pick:0x20,0x60,0xa0 $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$F+0x58:8=int:0:3 @$M+0x80:32=$F @$M+0x18:32=u32 @$M+0x1c:32=u32 $M int:0:32 stub=0x02089228,0x02089270,0x02059a84,0x020885a0,0x02088d08,0x0204ebe8 cases=40 */
void func_0204e948(u8 *m, u32 alpha)
{
    u8 *a = PTR_AT(m, 0xc);
    u32 i;

    if (a != NULL) {
        s32 n;

        if (BITS(U32_AT(m, 0x14), 7, 1) == 0)
            S32_AT(PTR_AT(m, 0xc), 0) += func_020822c0(func_020062d0()) * 0x1e;
        a = PTR_AT(m, 0xc);
        n = SHL(U16_AT(PTR_AT(a, 8), 4), 12);
        if (S32_AT(a, 0) >= n) {
            if (S32_AT(ANIM(U32_AT(m, 4)), 0x10) != 0) {
                S32_AT(a, 0) = 0;
            } else {
                S32_AT(a, 0) = n - 1;
                if (PTR_AT(m, 8) == NULL)
                    U32_AT(m, 0x14) = (U32_AT(m, 0x14) & ~1u) | 1;
            }
        }
    }
    if (PTR_AT(m, 8) != NULL)
        S32_AT(PTR_AT(m, 8), 0) = S32_AT(m, 0xbc);
    if (BITS(U32_AT(m, 0x14), 5, 1) == 0)
        return;
    for (i = 0; i < MDL(m)[0x18]; i++) {
        if ((U32_AT(m, 0x18) >> i) & 1)
            func_02089228(MDL(m), i, 1);
        else if ((U32_AT(m, 0x1c) >> i) & 1)
            func_02089228(MDL(m), i, alpha >> 1);
        else
            func_02089228(MDL(m), i, alpha);
    }
    for (i = 0; i < MDL(m)[0x18]; i++) {
        u8 *g = func_020062e0();

        U32_AT(g, 0x4ac)++;
        if (U32_AT(g, 0x4ac) > 0x3f)
            U32_AT(g, 0x4ac) = 1;
        func_02089270(MDL(m), i, U32_AT(func_020062e0(), 0x4ac));
    }
    func_02059a84(m + 0x74);
    func_020885a0(m + 0x20);
    func_02088d08();
    if (BITS(U32_AT(m, 0x14), 6, 1))
        func_0204ebe8(m);
}

/* 0x0204ebb4
 * @difftest $M=zero:0xdc @$M+0x14:32=pick:0,0x20 $M stub=0x02059af4 cases=4 */
void func_0204ebb4(u8 *m)
{
    if (BITS(U32_AT(m, 0x14), 5, 1) == 0)
        return;
    func_02059af4(m + 0x74);
}

typedef struct {
    u32 w[25];
} CamMtx;

/* 0x0204ebe8: put the attached bones' matrices in camera space
 * @difftest $M=zero:0xdc @$M+0xcc:32=$M+0xc8 $M cases=3
 * @difftest $M=zero:0xdc $N=ptr:0x7c:4 @$N+0:32=0 @$N+4:32=0 @$N+8:32=$M+0xc8 @$N+0x14:32=int:0:3 @$M+0xc8:32=$N @$M+0xcc:32=$N $M stub=0x020890a8:1 cases=5
 * @difftest $M=zero:0xdc $N=ptr:0x7c:4 @$N+0:32=0 @$N+4:32=0 @$N+8:32=$M+0xc8 @$M+0xc8:32=$N @$M+0xcc:32=$N $M stub=0x020890a8:0 cases=3 */
void func_0204ebe8(u8 *m)
{
    u8 cam[0x2c0];
    CamMtx c;
    s32 r[12], t[16], u[16], o[16];
    const u32 *src;
    u32 i;
    u8 *it;

    func_020040f4(cam, func_020062e0());
    src = func_02082cb0(cam);
    for (i = 0; i < 25; i++)
        c.w[i] = src[i];
    func_02082ce8(cam);
    it = PTR_AT(m, 0xcc);
    while (it != m + 0xc8) {
        if (func_020890a8(m + 0x20, r, 0, U32_AT(it, 0x14)) == 1) {
            func_0208e444(r, t);
            func_02017538(o, (s32 *)&c, t);
            MI_Copy64B(o, u);
            MI_Copy64B(u, it + 0x18);
        }
        func_02040230(&it);
    }
}

/* 0x0204ed14: back to frame 0
 * @difftest ptr:0xdc:4 cases=3 */
void func_0204ed14(u8 *m)
{
    S32_AT(m, 0xbc) = 0;
}

/* 0x0204ed20: stop the joint animation
 * @difftest $M=ptr:0xdc:4 @$M+0x14:32=pick:0,4,5 $M stub=0x02085d9c cases=6 */
void func_0204ed20(u8 *m)
{
    if (BITS(U32_AT(m, 0x14), 2, 1) == 0)
        return;
    func_0204ed14(m);
    func_02085d9c(m + 0x20, PTR_AT(m, 8));
    U32_AT(m, 0x14) &= ~4u;
    U32_AT(m, 0x14) = (U32_AT(m, 0x14) & ~1u) | 1;
    S32_AT(m, 0) = -1;
    S32_AT(m, 0xc0) = -1;
}

/* 0x0204ed80: play animation `id` from frame `frame`
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$M+0x80:32=$F @$M+0:32=pick:-1,1 @$M+4:32=pick:-1,1 $X=zero:0x10 $H=zero:8 @$X+8:32=$H @$H+4:16=int:1:4 @$M+8:32=0 @$M+0xc:32=0 @$M+0x9c:32=$X @$M+0xb4:32=$X $M int:0:3 int:0:0x3000 stub=0x02085d9c,0x02085e78,0x02080488,0x02080388:0,0x02080418,0x02080458,0x02089868:0,0x020860b0 cases=10
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$M+0x80:32=$F @$M+0:32=pick:-1,1 @$M+4:32=pick:-1,1 $X=zero:0x10 $H=zero:8 @$X+8:32=$H @$H+4:16=int:1:4 @$M+8:32=0 @$M+0xc:32=$X @$M+0x9c:32=$X @$M+0xb4:32=$X $M int:0:3 int:0:0x3000 stub=0x02085d9c,0x02085e78,0x02080488,0x02080388:0,0x02080418,0x02080458,0x02089868:0,0x020860b0 cases=10
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$M+0x80:32=$F @$M+0:32=pick:-1,1 @$M+4:32=pick:-1,1 $X=zero:0x10 $H=zero:8 @$X+8:32=$H @$H+4:16=int:1:4 @$M+8:32=$X @$M+0xc:32=0 @$M+0x9c:32=$X @$M+0xb4:32=$X $M int:0:3 int:0:0x3000 stub=0x02085d9c,0x02085e78,0x02080488,0x02080388:0,0x02080418,0x02080458,0x02089868:0,0x020860b0 cases=10
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$M+0x80:32=$F @$M+0:32=pick:-1,1 @$M+4:32=pick:-1,1 $X=zero:0x10 $H=zero:8 @$X+8:32=$H @$H+4:16=int:1:4 @$M+8:32=$X @$M+0xc:32=$X @$M+0x9c:32=$X @$M+0xb4:32=$X $M int:0:3 int:0:0x3000 stub=0x02085d9c,0x02085e78,0x02080488,0x02080388:0,0x02080418,0x02080458,0x02089868:0,0x020860b0 cases=10 */
void func_0204ed80(u8 *m, u32 id, s32 frame)
{
    U32_AT(m, 0x14) |= 4;
    if (id == 0)
        return;
    if (S32_AT(ANIM(id), 0) == 9) {
        if (U32_AT(m, 4) != id) {
            if (PTR_AT(m, 0xc) != NULL)
                func_02085d9c(m + 0x20, PTR_AT(m, 0xc));
            U32_AT(m, 4) = id;
            PTR_AT(m, 0xc) = func_0204eed4(m, id, 9);
            S32_AT(PTR_AT(m, 0xc), 0) = frame;
            func_02085e78(m + 0x20, PTR_AT(m, 0xc));
        }
    } else {
        S32_AT(m, 0xbc) = frame;
        if (id != U32_AT(m, 0)) {
            if (PTR_AT(m, 8) != NULL)
                func_02085d9c(m + 0x20, PTR_AT(m, 8));
            U32_AT(m, 0) = id;
            PTR_AT(m, 8) = func_0204eed4(m, id, 0);
            func_02085e78(m + 0x20, PTR_AT(m, 8));
        }
    }
    if (PTR_AT(m, 8) != NULL)
        S32_AT(PTR_AT(m, 8), 0) = S32_AT(m, 0xbc);
    if (PTR_AT(m, 0x10) == NULL)
        return;
    U32_AT(m, 0x20) |= 2;
    U32_AT(m, 0x20) |= 1;
    func_020885a0(m + 0x20);
    U32_AT(m, 0x20) &= ~2u;
    U32_AT(m, 0x20) &= ~1u;
    func_0204e820(m);
    func_0204e6b8(m);
}

/* 0x0204eed4: load animation `id` (of type `type`) into its file; its object
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$M+0x80:32=$F $X=zero:0x10 $H=zero:8 @$X+8:32=$H @$H+4:16=int:0:4 @$M+0x9c:32=$X @$M+0xb4:32=$X $M int:0:3 pick:0,9 stub=0x02080488,0x02080388:0,0x02080418,0x02080458,0x02089868:0,0x020860b0 cases=10 */
u8 *func_0204eed4(u8 *m, u32 id, u32 type)
{
    u8 *a;

    if (type == 9)
        a = func_0204e42c(m + 0xa0, U32_AT(ANIM(id), 4), MDL(m));
    else
        a = func_0204e42c(m + 0x88, U32_AT(ANIM(id), 4), MDL(m));
    S32_AT(ANIM(id), 0x1c) = SHL(U16_AT(PTR_AT(a, 8), 4), 12);
    return a;
}

/* 0x0204ef94: free the list at +0xb8
 * @difftest $M=zero:0xdc $A=zero:8 $B=zero:8 @$A:32=0 @$M+0xb8:32=0 $M stub=0x0207fe44 cases=3
 * @difftest $M=zero:0xdc $A=zero:8 $B=zero:8 @$A:32=0 @$M+0xb8:32=$A $M stub=0x0207fe44 cases=3
 * @difftest $M=zero:0xdc $A=zero:8 $B=zero:8 @$A:32=$B @$M+0xb8:32=0 $M stub=0x0207fe44 cases=3
 * @difftest $M=zero:0xdc $A=zero:8 $B=zero:8 @$A:32=$B @$M+0xb8:32=$A $M stub=0x0207fe44 cases=3 */
void func_0204ef94(u8 *m)
{
    u8 *p;

    while ((p = PTR_AT(m, 0xb8)) != NULL) {
        PTR_AT(m, 0xb8) = PTR_AT(p, 0);
        func_020599ec(p);
        func_0207fe44(p);
    }
}

/* 0x0204efec: advance the joint animation by `dt` ticks
 * @difftest $M=zero:0xdc @$M+0x14:32=pick:0,4,0x84 @$M+8:32=pick:0,4 @$M+0:32=int:1:3 @$M+0xbc:32=int:0:0x20000 $M int:0:0x2000 cases=40 */
void func_0204efec(u8 *m, u32 dt)
{
    u32 f = U32_AT(m, 0x14);

    if (BITS(f, 2, 1) == 0)
        return;
    if (PTR_AT(m, 8) == NULL)
        return;
    if (BITS(f, 7, 1) == 0)
        S32_AT(m, 0xbc) += dt * 0x1e;
    if (S32_AT(m, 0xbc) >= S32_AT(ANIM(U32_AT(m, 0)), 0x1c)) {
        U32_AT(m, 0x14) = (U32_AT(m, 0x14) & ~1u) | 1;
        if (S32_AT(ANIM(U32_AT(m, 0)), 0x10) != 0)
            S32_AT(m, 0xbc) = 0;
        else
            S32_AT(m, 0xbc) = S32_AT(ANIM(U32_AT(m, 0)), 0x1c) - 1;
    }
    S32_AT(m, 0xc0) = S32_AT(m, 0xbc);
}

/* 0x0204f094: free everything
 * @difftest $M=zero:0xdc @$M+0x10:32=pick:0,4 @$M+0x58:32=pick:0,4 @$M+0x20:32=u32 @$M+0xd4:32=int:0:4 @$M+0x14:32=u32 $M stub=0x02059b10 cases=8 */
void func_0204f094(u8 *m)
{
    if (PTR_AT(m, 0x10) != NULL) {
        if (PTR_AT(m, 0x58) == NULL)
            U32_AT(m, 0x20) &= ~1u;
        U32_AT(m, 0x54) = 0;
        PTR_AT(m, 0x10) = NULL;
    }
    func_02059b10(m + 0x74);
    func_0204e5b0(m + 0x88);
    func_0204e5b0(m + 0xa0);
    func_0204ef94(m);
    S32_AT(m, 0) = -1;
    S32_AT(m, 4) = -1;
    U32_AT(m, 8) = 0;
    U32_AT(m, 0xc) = 0;
    S32_AT(m, 0xbc) = 0;
    S32_AT(m, 0xc0) = 0;
    U32_AT(m, 0xb8) = 0;
    U32_AT(m, 0x18) = 0;
    func_0204fcf8(m + 0xc4);
    func_0204fd80(m + 0xd0, U32_AT(m, 0xd4), 0);
    U32_AT(m, 0x14) = 0;
}

/* 0x0204f15c: the biggest joint animation file of model `model` (-1: none)
 * @difftest $M=zero:0xdc $M int:0:0x60 stub=0x02080488,0x02080388:0x10,0x02080458 cases=20 */
s32 func_0204f15c(u8 *m, u32 model)
{
    u8 file[0x48];
    s32 best = -1, size = -1;
    u32 n = U32_AT(data_020c4b8c, 0x444), i;

    for (i = 1; i < n; i++) {
        if (model == U32_AT(ANIM(i), 0x20)) {
            s32 s;

            if (U32_AT(ANIM(i), 0) == 9)
                U32_AT(m, 0x14) |= 0x10;
            func_02080488(file, U32_AT(ANIM(i), 4));
            s = func_02080388(file);
            func_02080458(file);
            if (s > size) {
                size = s;
                best = i;
            }
        }
    }
    return best;
}

/* 0x0204f220: the matrix of bone `name`
 * @difftest $M=zero:0xdc @$M+0xcc:32=$M+0xc8 $M u32 cases=3
 * @difftest $M=zero:0xdc $N=ptr:0x7c:4 @$N+0:32=0 @$N+4:32=0 @$N+8:32=$M+0xc8 @$N+0xc:32=0x02000000 @$M+0xc8:32=$N @$M+0xcc:32=$N @$M+0xc4:32=1 $M pick:0x02000000,0x02000010 cases=4 */
u8 *func_0204f220(u8 *m, const char *name)
{
    u8 *it;

    func_0204fc60(&it, m + 0xc4, &name);
    return it + 0x18;
}

/* 0x0204f258: attach the bones whose names start with "xx"
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$F+0x57:8=int:0:3 @$F+0x86:16=8 @$F+0x8a:16=4 @$F+0x8c:8=0x78 @$F+0x8d:8=pick:0x78,0x41 @$F+0x9c:8=0x78 @$F+0x9d:8=0x78 @$M+0x80:32=$F @$M+0x14:32=u32 $M stub=0x0204fb70:0x02000000 noreplay=1 cases=10 */
void func_0204f258(u8 *m)
{
    struct {
        const char *name;
        Bone b;
    } kv;
    u32 i;

    /* (the original leaves the bone's done flag unset: no one reads it
     * before func_0204e820 clears it) */
    kv.b.done = 0;
    func_0208ea6c(kv.b.mtx);
    func_0207fe28(data_020c02f0, 0x198);
    for (i = 0; i < MDL(m)[0x17]; i++) {
        const char *s = (const char *)NODE_NAMES(MDL(m));

        kv.name = s + i * 16;
        if (s[i * 16] == 'x' && kv.name[1] == 'x') {
            u8 *v;

            kv.b.node = i;
            v = func_0204fb70(m + 0xc4, &kv.name) + 4;
            v[0] = kv.b.done;
            U32_AT(v, 4) = kv.b.node;
            MI_Copy64B(kv.b.mtx, v + 8);
        }
    }
    func_0207fe24();
    U32_AT(m, 0x14) = (U32_AT(m, 0x14) & ~0x40u) | ((PTR_AT(m, 0xc4) != NULL) << 6);
}

/* 0x0204f384: attach all the bones, with their parent links
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$F+0x57:8=1 @$M+0x80:32=$F @$M+0xd4:32=pick:0,1 @$M+0xcc:32=$M+0xc8 @$M+0x10:32=pick:0,4 $M stub=0x0204fb70:0x02000000,0x0204fd94,0x02088f60:0,0x020891dc:0x1234 cases=6 */
void func_0204f384(u8 *m)
{
    u8 kids[0x80];
    Bone b;
    const char *name;
    u32 pairs[2];
    u32 n, cur, i, j;

    b.done = 0;   /* (unset in the original, see func_0204f258) */
    func_0208ea6c(b.mtx);
    func_0207fe28(data_020c02f0, 0x15a);
    n = MDL(m)[0x17] - 1;
    cur = U32_AT(m, 0xd4);
    if (n > cur)
        func_0204fd94(m + 0xd0, n - cur, pairs, 0);
    else if (n < cur)
        func_0204fd80(m + 0xd0, cur - n, 0);
    func_0207fe24();
    func_0207fe28(data_020c02f0, 0x15f);
    for (i = 0; i < MDL(m)[0x17]; i++) {
        u8 *v;

        name = (const char *)NODE_NAMES(MDL(m)) + i * 16;
        b.node = i;
        v = func_0204fb70(m + 0xc4, &name) + 4;
        v[0] = b.done;
        U32_AT(v, 4) = b.node;
        MI_Copy64B(b.mtx, v + 8);
    }
    j = 0;
    for (i = 0; i < MDL(m)[0x17]; i++) {
        u8 *parent = func_0204e870(m, i);
        u8 *mdl = MDL(m);
        u32 cnt = func_02088f60(kids, mdl + U32_AT(mdl, 4), i), k;

        for (k = 0; k < cnt; k++) {
            u8 *child = func_0204e870(m, kids[k]);

            PTR_AT(PTR_AT(m, 0xd0), j * 8) = parent;
            PTR_AT(PTR_AT(m, 0xd0), j * 8 + 4) = child;
            j++;
        }
    }
    func_0207fe24();
    if (PTR_AT(m, 0x10) == NULL) {
        Allocator a, t;

        func_0207fe28(data_020c02f0, 0x17d);
        func_02080110(&t);
        a = t;
        PTR_AT(m, 0x10) = func_020891dc(&a, MDL(m));
        PTR_AT(m, 0x54) = PTR_AT(m, 0x10);
        func_0207fe24();
    }
    U32_AT(m, 0x14) &= ~0x40u;
}

/* 0x0204f648: load the joint (and material) animation files of actor kind `kind`
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$M+0x80:32=$F @$M+0x14:32=pick:0,0x10 $M int:0:0x20 stub=0x0204f15c:0,0x0204e4ac cases=10
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$M+0x80:32=$F @$M+0x14:32=pick:0,0x10 $M int:0:0x20 stub=0x0204f15c:-1,0x0204e4ac cases=10
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$M+0x80:32=$F @$M+0x14:32=pick:0,0x10 $M int:0:0x20 stub=0x0204f15c:2,0x0204e4ac cases=10 */
void func_0204f648(u8 *m, u32 kind)
{
    u32 model = kind, a;
    s32 best;

    if (U32_AT(data_020c3b0c + kind * 0x48, 0x1c) != 0)
        model = U32_AT(data_020c3b40 + U32_AT(data_020c3b0c + kind * 0x48, 0x1c) * 0x40, 0x3c);
    best = func_0204f15c(m, model);
    if (best != -1) {
        func_0204e4ac(m + 0x88, U32_AT(ANIM(best), 4), MDL(m));
        if (BITS(U32_AT(m, 0x14), 4, 1) != 1)
            return;
        func_0204e4ac(m + 0xa0, U32_AT(ANIM(best), 4), MDL(m));
        return;
    }
    a = U32_AT(data_020c3b40 + U32_AT(data_020c3b0c + kind * 0x48, 0x1c) * 0x40, 0);
    if (a == 0)
        return;
    func_0204e4ac(m + 0x88, U32_AT(ANIM(a), 4), MDL(m));
}

/* 0x0204f790: load model file `i` of data_020c3b54
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$M+0x80:32=$F $M int:0:8 u32 stub=0x02059bbc,0x02086048 cases=5 */
void func_0204f790(u8 *m, u32 i, u32 a)
{
    func_02059bbc(m + 0x74, data_020c3b54[i], a);
    func_02086048(m + 0x20, MDL(m));
}

/* 0x0204f7d4: load model file `file` and play animation `anim`
 * @difftest $M=zero:0xdc $F=zero:0x100 @$F+0xe:16=0x10 @$F+0x1c:32=0x40 @$M+0x80:32=$F @$M+0x14:32=u32 $M pick:0,3 int:0:0x40 stub=0x02059bbc,0x02086048,0x0204e4ac,0x0204ed80 cases=20 */
void func_0204f7d4(u8 *m, u32 file, u32 anim)
{
    if (file != 0) {
        func_02059bbc(m + 0x74, file, 0);
        func_02086048(m + 0x20, MDL(m));
        U32_AT(m, 0x14) |= 0x20;
        if (U32_AT(ANIM(anim), 0) == 9) {
            U32_AT(m, 0x14) |= 0x10;
            func_0204e4ac(m + 0xa0, U32_AT(ANIM(anim), 4), MDL(m));
        } else {
            func_0204e4ac(m + 0x88, U32_AT(ANIM(anim), 4), MDL(m));
        }
    }
    U32_AT(m, 0x14) |= 8;
    U32_AT(m, 0x18) = 0;
    U32_AT(m, 0x1c) = 0;
    func_0204ed80(m, anim, 0);
}

/* 0x0204f8d4: load the model of actor kind `kind`
 * @difftest $M=zero:0xdc @$M+0x14:32=u32 $M int:0:0x40 u32 pick:0,1 stub=0x0204f790,0x0204f648,0x0204f384,0x0204f258 cases=20 */
void func_0204f8d4(u8 *m, u32 kind, u32 a, u32 bones)
{
    if (U32_AT(data_020c3b0c + kind * 0x48, 4) != 0) {
        func_0204f790(m, U32_AT(data_020c3b0c + kind * 0x48, 4), a);
        U32_AT(m, 0x14) |= 0x20;
        func_0204f648(m, kind);
        if (bones)
            func_0204f384(m);
        else
            func_0204f258(m);
    }
    U32_AT(m, 0x14) |= 8;
    U32_AT(m, 0x18) = 0;
    U32_AT(m, 0x1c) = 0;
}

/* 0x0204f964: the model's destructor
 * @difftest zero:0xdc stub=0x02059b10,0x02050f64,0x02059db4,0x0207fe44 cases=3 */
u8 *func_0204f964(u8 *m)
{
    func_0204f094(m);
    func_0204fd30(m + 0xd0);
    func_0204fcd4(m + 0xc4);
    func_0204e654(m + 0xa0);
    func_0204e654(m + 0x88);
    func_02059db4(m + 0x74);
    return m;
}

/* what the constructors share */
static void Init(u8 *m)
{
    S32_AT(m, 0) = -1;
    S32_AT(m, 4) = -1;
    U32_AT(m, 8) = 0;
    U32_AT(m, 0xc) = 0;
    U32_AT(m, 0x10) = 0;
    U32_AT(m, 0x18) = 0;
    U32_AT(m, 0x1c) = 0;
    func_02059dd8(m + 0x74);
    func_0204e680(m + 0x88);
    func_0204e680(m + 0xa0);
    U32_AT(m, 0xb8) = 0;
    U32_AT(m, 0xbc) = 0;
    U32_AT(m, 0xc0) = 0;
    U32_AT(m, 0xc4) = 0;
    U32_AT(m, 0xc8) = 0;
    PTR_AT(m, 0xcc) = m + 0xc8;
    U32_AT(m, 0xd0) = 0;
    U32_AT(m, 0xd4) = 0;
    U32_AT(m, 0xd8) = 0;
    U32_AT(m, 0x14) = 0;
}

/* 0x0204f9a4: constructor: model file `file`, playing animation `anim`
 * @difftest ptr:0xdc:4 pick:0 pick:0 stub=0x02059dd8,0x02050fd8,0x0204ed80 cases=3 */
u8 *func_0204f9a4(u8 *m, u32 file, u32 anim)
{
    Init(m);
    func_0204f7d4(m, file, anim);
    return m;
}

/* 0x0204fa44: constructor: the model of actor kind `kind`
 * @difftest ptr:0xdc:4 pick:0 stub=0x02059dd8,0x02050fd8 cases=3 */
u8 *func_0204fa44(u8 *m, u32 kind)
{
    Init(m);
    func_0204f8d4(m, kind, 0, 0);
    return m;
}

/* 0x0204fae8: constructor: no model
 * @difftest ptr:0xdc:4 stub=0x02059dd8,0x02050fd8 cases=3 */
u8 *func_0204fae8(u8 *m)
{
    Init(m);
    return m;
}
