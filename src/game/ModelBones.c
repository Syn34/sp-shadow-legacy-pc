/*
 * The model's containers: the std::map of attached bones (name -> bone,
 * compared with strcmp) and the std::vector of {parent, child} bone pairs.
 * ARM9 main, 0x0204fb70 - 0x02050430 (18 functions).
 *
 * The map is {size, header node}; the header's left is the root and its
 * right the leftmost node. Nodes are 0x7c bytes {left, right, parent | red,
 * name, bone (0x6c bytes: s8 done, u32 node, Mtx44, 0x24 more)}. The
 * vector is {data, size, capacity} of 8-byte elements; growing goes
 * through a split buffer {data, size, capacity, &vector capacity, offset}.
 *
 * A new bone gets zeros where the original copies whatever was on its
 * stack, so func_0204fb70's recorded game calls are not replayed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern u8 data_020bcb38[];           /* the exceptions' type */
extern const char data_020c0300[], data_020c031c[], data_020c0338[];   /* their messages */

typedef struct {
    const char *name;
    s8 done;
    u32 node;
    s32 mtx[16];
    s32 more[9];
} Pair;

typedef struct {
    u8 *data;
    u32 size;
    u32 cap;
    u32 *capref;
    u32 off;
} SplitBuf;

void *func_0207fe74(u32 size);
void func_0207fe44(void *p);
void func_0209e90c(void *e, const char *msg);
void func_0209dcb8(void);
void func_02006ec8(void);
void func_020409fc(u8 *x, u8 *root);
s8 func_02080b84(void *cmp, const char *a, const char *b);

u8 *func_0204fe50(u8 *map, const char **key, u8 *node, u8 *result);
u8 *func_0204feac(u8 *map, const char **key, u8 **pos, u8 *left, u8 *leftmost);
void func_0204ff40(u8 *map, u8 *node);
u8 *func_0204ff88(u8 *map, u8 *parent, u32 left, u32 leftmost, const Pair *pair);
void func_0204fd80(u8 *vec, u32 n, u32 tag);
void func_020500f0(u8 *vec, u32 n, const u32 *value);
u32 func_0205027c(u8 *vec, u32 n);
SplitBuf *func_02050354(SplitBuf *b);
SplitBuf *func_02050374(SplitBuf *b);
void func_020503c4(SplitBuf *b, u32 n, u32 tag);
void func_02050418(SplitBuf *b, u32 n, u32 tag);

#define LEFT(n) PTR_AT(n, 0)
#define RIGHT(n) PTR_AT(n, 4)
#define NAME(n) ((const char *)PTR_AT(n, 0xc))
#define LESS(map, a, b) func_02080b84((map) + 8, a, b)

/* 0x0204fb70: map[*key]: the {name, bone} pair, inserted if missing
 * @difftest $A=zero:4 @$A:8=0x4d $M=zero:0x10 @$M+0xc:32=$M+4 @$M+8:32=$M+4 $K=ptr:4:4 @$K:32=$A $M $K stub=0x0207fe74:0 cases=3
 * @difftest $A=zero:4 @$A:8=0x4d $B=zero:4 @$B:8=pick:0x41,0x4d,0x5a $M=zero:0x10 $T=zero:0x7c @$M+0:32=1 @$M+4:32=$T @$M+8:32=$T @$M+0xc:32=$M+4 @$T+8:32=$M+4 @$T+0xc:32=$A $K=ptr:4:4 @$K:32=$B $M $K stub=0x0207fe74:0 noreplay=1 cases=10 */
u8 *func_0204fb70(u8 *map, const char **key)
{
    u8 left, leftmost;
    u8 *pos;
    u8 *n = func_0204feac(map, key, &pos, &left, &leftmost);

    if (pos == NULL || LESS(map, NAME(pos), *key)) {
        /* (the original leaves the new bone's fields as whatever is on the
         * stack; the callers set them all but the last 0x24 bytes, which
         * nothing reads) */
        Pair p;
        u32 i;

        p.name = *key;
        p.done = 0;
        p.node = 0;
        for (i = 0; i < 16; i++)
            p.mtx[i] = 0;
        for (i = 0; i < 9; i++)
            p.more[i] = 0;
        return func_0204ff88(map, n, left, leftmost, &p) + 0xc;
    }
    return pos + 0xc;
}

/* 0x0204fc60: find `*key` (end: the header)
 * @difftest $A=zero:4 @$A:8=0x4d $M=zero:0x10 @$M+0xc:32=$M+4 $K=ptr:4:4 @$K:32=$A ptr:4:4 $M $K cases=3
 * @difftest $A=zero:4 @$A:8=0x4d $L=zero:4 @$L:8=0x46 $R=zero:4 @$R:8=0x54 $B=zero:4 @$B:8=pick:0x41,0x46,0x4d,0x50,0x54,0x5a $M=zero:0x10 $T=zero:0x7c $X=zero:0x7c $Y=zero:0x7c @$M+0:32=3 @$M+4:32=$T @$M+8:32=$X @$M+0xc:32=$M+4 @$T+0:32=$X @$T+4:32=$Y @$T+8:32=$M+4 @$T+0xc:32=$A @$X+8:32=$T+1 @$X+0xc:32=$L @$Y+8:32=$T+1 @$Y+0xc:32=$R $K=ptr:4:4 @$K:32=$B ptr:4:4 $M $K cases=30 */
void func_0204fc60(u8 **out, u8 *map, const char **key)
{
    u8 *n = func_0204fe50(map, key, PTR_AT(map, 4), map + 4);

    if (n == map + 4 || LESS(map, *key, NAME(n)))
        *out = map + 4;
    else
        *out = n;
}

/* 0x0204fcd4: the map's destructor
 * @difftest $M=zero:0x10 $M cases=3
 * @difftest $M=zero:0x10 $T=zero:0x7c $X=zero:0x7c @$M+0:32=2 @$M+4:32=$T @$M+8:32=$X @$T+0:32=$X $M stub=0x0207fe44 cases=3 */
u8 *func_0204fcd4(u8 *map)
{
    if (PTR_AT(map, 4) != NULL)
        func_0204ff40(map, PTR_AT(map, 4));
    return map;
}

/* 0x0204fcf8: clear the map
 * @difftest $M=zero:0x10 $M cases=3
 * @difftest $M=zero:0x10 $T=zero:0x7c $X=zero:0x7c @$M+0:32=2 @$M+4:32=$T @$M+8:32=$X @$T+0:32=$X $M stub=0x0207fe44 cases=3 */
void func_0204fcf8(u8 *map)
{
    if (PTR_AT(map, 4) == NULL)
        return;
    func_0204ff40(map, PTR_AT(map, 4));
    U32_AT(map, 0) = 0;
    PTR_AT(map, 4) = NULL;
    PTR_AT(map, 8) = map + 4;
}

/* 0x0204fd30: the vector's destructor
 * @difftest $V=zero:0xc @$V:32=pick:0,4 @$V+4:32=int:0:4 $V stub=0x0207fe44 cases=6 */
u8 *func_0204fd30(u8 *vec)
{
    func_0204fd80(vec, U32_AT(vec, 4), 0);
    if (PTR_AT(vec, 0) != NULL)
        func_0207fe44(PTR_AT(vec, 0));
    return vec;
}

/* 0x0204fd80: drop the last `n` elements
 * @difftest ptr:0xc:4 int:0:4 cases=5 */
void func_0204fd80(u8 *vec, u32 n, u32 tag)
{
    U32_AT(vec, 4) -= n;
}

/* 0x0204fd94: append `n` copies of `*value`
 * @difftest $D=ptr:0x40:4 $V=zero:0xc @$V:32=$D @$V+4:32=int:0:3 @$V+8:32=8 $V int:0:5 ptr:8:4 stub=0x0205027c:0,0x020500f0 cases=20 */
void func_0204fd94(u8 *vec, u32 n, const u32 *value, u32 tag)
{
    u32 cap = U32_AT(vec, 8);
    u32 *p;
    u32 k;

    if (n > cap || U32_AT(vec, 4) > cap - n) {
        func_0205027c(vec, U32_AT(vec, 4) + n - cap);
        func_020500f0(vec, n, value);
        return;
    }
    p = (u32 *)((u8 *)PTR_AT(vec, 0) + U32_AT(vec, 4) * 8);
    for (k = n; k != 0; k--) {
        p[0] = value[0];
        p[1] = value[1];
        p += 2;
    }
    U32_AT(vec, 4) += n;
}

/* 0x0204fe50: the first node under `node` whose name is not below *key
 * @difftest ptr:0x10 ptr:4:4 pick:0 pick:0x1234 cases=3
 * @difftest $A=zero:4 @$A:8=0x4d $L=zero:4 @$L:8=0x46 $R=zero:4 @$R:8=0x54 $B=zero:4 @$B:8=pick:0x41,0x46,0x4d,0x50,0x54,0x5a $M=zero:0x10 $T=zero:0x7c $X=zero:0x7c $Y=zero:0x7c @$T+0:32=$X @$T+4:32=$Y @$T+0xc:32=$A @$X+0xc:32=$L @$Y+0xc:32=$R $K=ptr:4:4 @$K:32=$B $M $K $T $M+4 cases=30 */
u8 *func_0204fe50(u8 *map, const char **key, u8 *node, u8 *result)
{
    while (node != NULL) {
        if (!LESS(map, NAME(node), *key)) {
            result = node;
            node = LEFT(node);
        } else {
            node = RIGHT(node);
        }
    }
    return result;
}

/* 0x0204feac: where to insert *key: the parent, the last node not above it
 * (*pos), whether it goes left and whether it becomes the leftmost
 * @difftest $M=zero:0x10 $M ptr:4:4 ptr:4:4 ptr:4:4 ptr:4:4 cases=3
 * @difftest $A=zero:4 @$A:8=0x4d $L=zero:4 @$L:8=0x46 $R=zero:4 @$R:8=0x54 $B=zero:4 @$B:8=pick:0x41,0x46,0x4d,0x50,0x54,0x5a $M=zero:0x10 $T=zero:0x7c $X=zero:0x7c $Y=zero:0x7c @$M+4:32=$T @$T+0:32=$X @$T+4:32=$Y @$T+0xc:32=$A @$X+0xc:32=$L @$Y+0xc:32=$R $K=ptr:4:4 @$K:32=$B $M $K ptr:4:4 ptr:4:4 ptr:4:4 cases=30 */
u8 *func_0204feac(u8 *map, const char **key, u8 **pos, u8 *left, u8 *leftmost)
{
    u8 *node = PTR_AT(map, 4), *parent = map + 4;

    *pos = NULL;
    *left = 1;
    *leftmost = 1;
    while (node != NULL) {
        parent = node;
        if (LESS(map, *key, NAME(node))) {
            node = LEFT(node);
            *left = 1;
        } else {
            *pos = node;
            node = RIGHT(node);
            *left = 0;
            *leftmost = 0;
        }
    }
    return parent;
}

/* 0x0204ff40: free a subtree
 * @difftest ptr:0x10 zero:0x7c stub=0x0207fe44 cases=3
 * @difftest $T=zero:0x7c $X=zero:0x7c $Y=zero:0x7c $Z=zero:0x7c @$T+0:32=$X @$T+4:32=$Y @$X+4:32=$Z ptr:0x10 $T stub=0x0207fe44 cases=3 */
void func_0204ff40(u8 *map, u8 *node)
{
    if (LEFT(node) != NULL)
        func_0204ff40(map, LEFT(node));
    if (RIGHT(node) != NULL)
        func_0204ff40(map, RIGHT(node));
    func_0207fe44(node);
}

/* 0x0204ff88: insert a node for `pair` under `parent`
 * @difftest $M=zero:0x10 @$M+0xc:32=$M+4 $P=ptr:0x70:4 $M $M+4 pick:0,1 pick:0,1 $P cases=10
 * @difftest $A=zero:4 @$A:8=0x4d $M=zero:0x10 $T=zero:0x7c $X=zero:0x7c @$M+0:32=2 @$M+4:32=$T @$M+8:32=$X @$M+0xc:32=$M+4 @$T+0:32=$X @$T+8:32=$M+4 @$T+0xc:32=$A @$X+8:32=$T+1 @$X+0xc:32=$A $P=ptr:0x70:4 $M $X pick:0,1 pick:0,1 $P cases=20 */
u8 *func_0204ff88(u8 *map, u8 *parent, u32 left, u32 leftmost, const Pair *pair)
{
    u8 *node, *hold;
    u32 i;

    if (S32_AT(map, 0) == -1) {
        func_0209e90c(data_020bcb38, data_020c0300);
        func_0209dcb8();
    }
    node = func_0207fe74(0x7c);
    if (node == NULL) {
        func_0209e90c(data_020bcb38, data_020c0338);
        func_0209dcb8();
    }
    /* (the original tests node + 0xc against NULL: always true) */
    {
        Pair *d = (Pair *)(node + 0xc);

        d->name = pair->name;
        d->done = pair->done;
        d->node = pair->node;
        for (i = 0; i < 16; i++)
            d->mtx[i] = pair->mtx[i];
        for (i = 0; i < 9; i++)
            d->more[i] = pair->more[i];
    }
    hold = NULL;
    RIGHT(node) = NULL;
    LEFT(node) = RIGHT(node);
    U32_AT(node, 8) = (u32)parent | (U32_AT(node, 8) & 1);
    if ((u8)left != 0)
        LEFT(parent) = node;
    else
        RIGHT(parent) = node;
    U32_AT(map, 0)++;
    func_020409fc(node, PTR_AT(map, 4));
    if ((u8)leftmost != 0)
        PTR_AT(map, 8) = node;
    if (hold != NULL)
        func_0207fe44(hold);
    return node;
}

/* 0x020500f0: append `n` copies of `*value` into a bigger buffer
 * @difftest $D=ptr:0x40:4 $V=zero:0xc @$V:32=$D @$V+4:32=int:0:4 @$V+8:32=4 $V int:1:4 ptr:8:4 stub=0x0207fe44 cases=20 */
void func_020500f0(u8 *vec, u32 n, const u32 *value)
{
    SplitBuf b;
    u32 *p, *src, *dst, k, t;
    u8 *td;

    b.data = NULL;
    b.size = 0;
    b.cap = 0;
    b.off = 0;
    b.capref = (u32 *)(vec + 8);
    func_020503c4(&b, func_0205027c(vec, U32_AT(vec, 4) + n - U32_AT(vec, 8)), 0);
    b.off = U32_AT(vec, 4);
    p = (u32 *)(b.data + b.off * 8 + b.size * 8);
    for (k = n; k != 0; k--) {
        p[0] = value[0];
        p[1] = value[1];
        p += 2;
    }
    b.size += n;
    src = (u32 *)((u8 *)PTR_AT(vec, 0) + U32_AT(vec, 4) * 8);
    dst = (u32 *)(b.data + b.off * 8);
    while (src > (u32 *)PTR_AT(vec, 0)) {
        src -= 2;
        dst -= 2;
        if (dst != NULL) {
            dst[0] = src[0];
            dst[1] = src[1];
        }
        b.off--;
        b.size++;
    }
    t = *b.capref;
    *b.capref = b.cap;
    b.cap = t;
    td = PTR_AT(vec, 0);
    PTR_AT(vec, 0) = b.data;
    b.data = td;
    t = U32_AT(vec, 4);
    U32_AT(vec, 4) = b.size;
    b.size = t;
    func_02050354(&b);
}

/* 0x0205027c: the capacity to grow to for `n` more elements
 * @difftest $V=zero:0xc @$V+8:32=pick:0,1,5,100,0xaaaaaa9,0xaaaaaaa,0x15555553,0x15555554,0x1fffffff $V int:0:0x200 cases=40 stub=0x02006ec8 */
u32 func_0205027c(u8 *vec, u32 n)
{
    u32 cap = U32_AT(vec, 8);
    u32 g;

    if (n > 0x1fffffff - cap)
        func_02006ec8();
    if (cap < 0xaaaaaaa) {
        g = (cap + 1) * 3 / 5;
        return cap + (g < n ? n : g);
    }
    if (cap < 0x15555554) {
        g = (cap + 1) >> 1;
        return cap + (g < n ? n : g);
    }
    return 0x1fffffff;
}

/* 0x02050354: the split buffer's destructor (it owns no elements)
 * @difftest $B=zero:0x14 @$B:32=pick:0,4 @$B+4:32=int:0:4 $B stub=0x0207fe44 cases=6 */
SplitBuf *func_02050354(SplitBuf *b)
{
    b->size = 0;
    func_02050374(b);
    return b;
}

/* 0x02050374: free the split buffer
 * @difftest $B=zero:0x14 @$B:32=pick:0,4 @$B+4:32=int:0:4 $B stub=0x0207fe44 cases=6 */
SplitBuf *func_02050374(SplitBuf *b)
{
    func_02050418(b, b->size, 0);
    if (b->data != NULL)
        func_0207fe44(b->data);
    return b;
}

/* 0x020503c4: give the split buffer room for `n` elements
 * @difftest zero:0x14 int:0:8 stub=0x0207fe74:0x1234 cases=5
 * @difftest zero:0x14 pick:0x20000000 stub=0x02006ec8,0x0207fe74:0x1234 cases=2 */
void func_020503c4(SplitBuf *b, u32 n, u32 tag)
{
    u8 *p;

    if (n > 0x1fffffff)
        func_02006ec8();
    p = func_0207fe74(n * 8);
    if (p == NULL) {
        func_0209e90c(data_020bcb38, data_020c031c);
        func_0209dcb8();
    }
    b->data = p;
    b->cap = n;
}

/* 0x02050418: drop the last `n` elements of the split buffer
 * @difftest ptr:0x14:4 int:0:4 cases=5 */
void func_02050418(SplitBuf *b, u32 n, u32 tag)
{
    b->size -= n;
}

/* 0x0205042c
 * @difftest ptr:0x14:4 cases=3 */
void func_0205042c(void *p)
{
}
