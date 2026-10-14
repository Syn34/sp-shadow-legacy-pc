/*
 * The chests' drop map (a std::map<u32, void *> instantiation): a
 * red-black tree keyed by the chest actor.
 * ARM9 main, 0x02040088 - 0x02040d1c (16 functions).
 *
 * The map is {size, header node}; the header's left is the root and its
 * right the leftmost node (begin). Nodes are 0x14 bytes {left, right,
 * parent | red, key, value}: the low bit of the parent pointer is the
 * colour (1 red). Rotations and fix-ups update a caller's copy of the
 * root; the header is updated through the parent links.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern const u32 data_020bf0f0;      /* the default value */
extern u8 data_020bcb38[];           /* the length_error exception's type */
extern const char data_020bf0f4[], data_020bf110[];   /* exception messages */
extern s8 data_020ebb4c;
extern u8 data_020ebb6c[];

void *func_0207fe74(u32 size);
void func_0207fe44(void *p);
void func_0209e90c(void *e, const char *msg);
void func_0209dcb8(void);
void func_02040230(u8 **it);

void func_020401d4(u8 **out, u8 *map, const u32 *key);
u8 *func_020402d4(u8 *map, const u32 *key, u8 *node, u8 *result);
u8 *func_02040304(u8 *map, const u32 *key, u8 **pos, u8 *left, u8 *leftmost);
void func_02040378(u8 *map, u8 *node);
u8 *func_020403c0(u8 *map, u8 *parent, u32 left, u32 leftmost, const u32 *pair);
void func_020404d0(u8 *map, u8 *node);
void func_02040530(u8 *z, u8 *root);
void func_020409fc(u8 *x, u8 *root);
void func_02040c00(u8 *x, u8 **root);
void func_02040c7c(u8 *x, u8 **root);

#define LEFT(n) PTR_AT(n, 0)
#define RIGHT(n) PTR_AT(n, 4)
#define PARENT(n) ((u8 *)(U32_AT(n, 8) & ~1u))
#define RED(n) ((U32_AT(n, 8) & 1) != 0)
#define SET_RED(n) (U32_AT(n, 8) |= 1)
#define SET_BLACK(n) (U32_AT(n, 8) &= ~1u)
#define KEY(n) U32_AT(n, 0xc)

/* 0x02040088: set up the locked-chest message once
 * @difftest @020ebb4c:8=pick:0,1 cases=5 */
void func_02040088(void)
{
    if (data_020ebb4c != 0)
        return;
    U32_AT(data_020ebb6c, 0) = 0x34;
    U16_AT(data_020ebb6c, 4) = 0x3da;
    U32_AT(data_020ebb6c, 0x18) = 0;
    data_020ebb4c = 1;
}

/* 0x020400e4: map[key]: the {key, value} pair, inserted if missing
 * @difftest $M=zero:0x10 @$M+0xc:32=$M+4 @$M+8:32=$M+4 $K=ptr:4:4 @$K+0:32=int:0:8 $M $K cases=20
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 @$M+0:32=2 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 $K=ptr:4:4 @$K+0:32=int:0:25 $M $K cases=60
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 @$M+0:32=3 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 @$R+8:32=$T+1 @$R+0xc:32=15 $K=ptr:4:4 @$K+0:32=int:0:25 $M $K cases=60
 * @difftest $M=zero:0x10 $T=zero:0x14 $R=zero:0x14 @$M+0:32=2 @$M+4:32=$T @$M+8:32=$T @$M+0xc:32=$M+4 @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$R+8:32=$T+1 @$R+0xc:32=15 $K=ptr:4:4 @$K+0:32=int:0:25 $M $K cases=60
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $K=ptr:4:4 @$K+0:32=int:0:25 $M $K cases=60 */
u32 *func_020400e4(u8 *map, const u32 *key)
{
    u8 left, leftmost;
    u8 *pos;
    u8 *n = func_02040304(map, key, &pos, &left, &leftmost);

    if (pos == NULL || KEY(pos) < *key) {
        u32 pair[2];
        pair[0] = *key;
        pair[1] = data_020bf0f0;
        return (u32 *)(func_020403c0(map, n, left, leftmost, pair) + 0xc);
    }
    return (u32 *)(pos + 0xc);
}

/* 0x02040178: erase `key`; whether it was there
 * @difftest $M=zero:0x10 @$M+0xc:32=$M+4 @$M+8:32=$M+4 $K=ptr:4:4 $M $K cases=5
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $K=ptr:4:4 @$K+0:32=pick:5,10,12,15,20,7 $M $K stub=0x0207fe44 cases=60
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 @$M+0:32=3 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 @$R+8:32=$T+1 @$R+0xc:32=15 $K=ptr:4:4 @$K+0:32=pick:5,10,15 $M $K stub=0x0207fe44 cases=30
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 @$M+0:32=2 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 $K=ptr:4:4 @$K+0:32=pick:5,10 $M $K stub=0x0207fe44 cases=20 */
s32 func_02040178(u8 *map, const u32 *key)
{
    u8 *it;

    func_020401d4(&it, map, key);
    if (it == map + 4)
        return 0;
    func_020404d0(map, it);
    return 1;
}

/* 0x020401d4: find `key` (end: the header)
 * @difftest ptr:4:4 $M=zero:0x10 @$M+0xc:32=$M+4 $M ptr:4:4 cases=5
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $O=ptr:4:4 $K=ptr:4:4 @$K+0:32=int:0:25 $O $M $K cases=60 */
void func_020401d4(u8 **out, u8 *map, const u32 *key)
{
    u8 *n = func_020402d4(map, key, PTR_AT(map, 4), map + 4);

    if (n == map + 4 || *key < KEY(n))
        *out = map + 4;
    else
        *out = n;
}

/* 0x02040230: ++iterator
 * @difftest $H=zero:0xc $N=zero:0x14 @$N+8:32=$H @$H+0:32=$N $I=ptr:4:4 @$I+0:32=$N $I cases=5
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $I=ptr:4:4 @$I+0:32=$L $I cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $I=ptr:4:4 @$I+0:32=$T $I cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $I=ptr:4:4 @$I+0:32=$RL $I cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $I=ptr:4:4 @$I+0:32=$RR $I cases=3 */
void func_02040230(u8 **it)
{
    u8 *n = *it, *p;

    if (RIGHT(n) != NULL) {
        n = RIGHT(n);
        while (LEFT(n) != NULL)
            n = LEFT(n);
        *it = n;
        return;
    }
    p = PARENT(n);
    while (n != LEFT(p)) {
        *it = p;
        n = *it;
        p = PARENT(n);
    }
    *it = p;
}

/* 0x0204029c: clear the map
 * @difftest $M=zero:0x10 $M cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $M stub=0x0207fe44 cases=3 */
void func_0204029c(u8 *map)
{
    if (PTR_AT(map, 4) == NULL)
        return;
    func_02040378(map, PTR_AT(map, 4));
    U32_AT(map, 0) = 0;
    PTR_AT(map, 4) = NULL;
    PTR_AT(map, 8) = map + 4;
}

/* 0x020402d4: the first node under `node` whose key is not below *key
 * @difftest ptr:0x10 ptr:4:4 pick:0 pick:0x1234 cases=5
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $K=ptr:4:4 @$K+0:32=int:0:25 $M $K $T $M+4 cases=60 */
u8 *func_020402d4(u8 *map, const u32 *key, u8 *node, u8 *result)
{
    if (node != NULL) {
        u32 k = *key;
        do {
            if (KEY(node) >= k) {
                result = node;
                node = LEFT(node);
            } else {
                node = RIGHT(node);
            }
        } while (node != NULL);
    }
    return result;
}

/* 0x02040304: where to insert *key: the parent, the last node not above
 * it (*pos), whether it goes left and whether it becomes the leftmost
 * @difftest $M=zero:0x10 $M ptr:4:4 ptr:4:4 ptr:4:4 ptr:4:4 cases=5
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $K=ptr:4:4 @$K+0:32=int:0:25 $M $K ptr:4:4 ptr:4:4 ptr:4:4 cases=60 */
u8 *func_02040304(u8 *map, const u32 *key, u8 **pos, u8 *left, u8 *leftmost)
{
    u8 *node, *parent;

    *pos = NULL;
    node = PTR_AT(map, 4);
    *left = 1;
    *leftmost = 1;
    if (node == NULL)
        return map + 4;
    do {
        parent = node;
        if (*key < KEY(node)) {
            node = LEFT(node);
            *left = 1;
        } else {
            *pos = node;
            node = RIGHT(node);
            *left = 0;
            *leftmost = 0;
        }
    } while (node != NULL);
    return parent;
}

/* 0x02040378: free a subtree
 * @difftest ptr:0x10 zero:0x14 stub=0x0207fe44 cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $M $T stub=0x0207fe44 cases=3 */
void func_02040378(u8 *map, u8 *node)
{
    if (LEFT(node) != NULL)
        func_02040378(map, LEFT(node));
    if (RIGHT(node) != NULL)
        func_02040378(map, RIGHT(node));
    func_0207fe44(node);
}

/* 0x020403c0: insert a node for `pair` under `parent`
 * @difftest $M=zero:0x10 @$M+0xc:32=$M+4 $P=ptr:8:4 $M $M+4 pick:0,1 pick:0,1 $P cases=10
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 @$M+0:32=2 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 $P=ptr:8:4 $M $L pick:0,1 pick:0,1 $P cases=20 */
u8 *func_020403c0(u8 *map, u8 *parent, u32 left, u32 leftmost, const u32 *pair)
{
    u8 *node, *hold;

    if (S32_AT(map, 0) == -1) {
        func_0209e90c(data_020bcb38, data_020bf0f4);
        func_0209dcb8();
    }
    node = func_0207fe74(0x14);
    if (node == NULL) {
        func_0209e90c(data_020bcb38, data_020bf110);
        func_0209dcb8();
    }
    /* (the original tests node + 0xc against NULL: always true) */
    U32_AT(node, 0xc) = pair[0];
    U32_AT(node, 0x10) = pair[1];
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

/* 0x020404d0: erase the node at `node`
 * @difftest $M=zero:0x10 $M zero:0x14 stub=0x0207fe44,0x02040530 cases=3 */
void func_020404d0(u8 *map, u8 *node)
{
    if (node == PTR_AT(map, 8)) {
        u8 *it = node;
        func_02040230(&it);
        PTR_AT(map, 8) = it;
    }
    func_02040530(node, PTR_AT(map, 4));
    func_0207fe44(node);
    U32_AT(map, 0)--;
}

/* 0x02040530: unlink `z` and restore the red-black properties
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $T $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $L $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $R $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $RL $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $RR $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 @$M+0:32=3 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 @$R+8:32=$T+1 @$R+0xc:32=15 $L $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 @$M+0:32=3 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 @$R+8:32=$T+1 @$R+0xc:32=15 $T $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 @$M+0:32=2 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 $T $T cases=3 */
void func_02040530(u8 *z, u8 *root)
{
    u8 *y = z, *x, *xp, *p, *w;
    s32 xleft, fix;

    if (LEFT(z) != NULL && RIGHT(z) != NULL)
        func_02040230(&y);
    x = LEFT(y);
    if (x == NULL)
        x = RIGHT(y);
    xp = PARENT(y);
    if (x != NULL)
        U32_AT(x, 8) = (u32)xp | (U32_AT(x, 8) & 1);
    p = PARENT(y);
    if (y == LEFT(p)) {
        LEFT(p) = x;
        xleft = 1;
    } else {
        RIGHT(p) = x;
        xleft = 0;
    }
    fix = !RED(y);
    if (y != z) {
        /* y takes z's place */
        U32_AT(y, 8) = (U32_AT(z, 8) & ~1u) | (U32_AT(y, 8) & 1);
        p = PARENT(y);
        if (z == LEFT(p))
            LEFT(p) = y;
        else
            RIGHT(p) = y;
        LEFT(y) = LEFT(z);
        if (LEFT(y) == NULL)
            xp = y;
        else
            U32_AT(LEFT(y), 8) = (u32)y | (U32_AT(LEFT(y), 8) & 1);
        RIGHT(y) = RIGHT(z);
        if (RIGHT(y) == NULL)
            xp = y;
        else
            U32_AT(RIGHT(y), 8) = (u32)y | (U32_AT(RIGHT(y), 8) & 1);
        if (RED(z))
            SET_RED(y);
        else
            SET_BLACK(y);
        if (root == z)
            root = y;
    } else if (x == NULL && z == root) {
        fix = 0;
    }
    if (!fix)
        return;
    while (x != root && (x == NULL || !RED(x))) {
        if (xleft) {
            w = RIGHT(xp);
            if (RED(w)) {
                SET_BLACK(w);
                SET_RED(xp);
                func_02040c00(xp, &root);
                w = RIGHT(xp);
            }
            if ((LEFT(w) == NULL || !RED(LEFT(w))) && (RIGHT(w) == NULL || !RED(RIGHT(w)))) {
                SET_RED(w);
                x = xp;
                xp = PARENT(x);
                xleft = x == LEFT(xp);
            } else {
                if (RIGHT(w) == NULL || !RED(RIGHT(w))) {
                    SET_BLACK(LEFT(w));
                    SET_RED(w);
                    func_02040c7c(w, &root);
                    w = RIGHT(xp);
                }
                if (RED(xp))
                    SET_RED(w);
                else
                    SET_BLACK(w);
                SET_BLACK(xp);
                SET_BLACK(RIGHT(w));
                func_02040c00(xp, &root);
                x = root;
            }
        } else {
            w = LEFT(xp);
            if (RED(w)) {
                SET_BLACK(w);
                SET_RED(xp);
                func_02040c7c(xp, &root);
                w = LEFT(xp);
            }
            if ((LEFT(w) == NULL || !RED(LEFT(w))) && (RIGHT(w) == NULL || !RED(RIGHT(w)))) {
                SET_RED(w);
                x = xp;
                xp = PARENT(x);
                xleft = x == LEFT(xp);
            } else {
                if (LEFT(w) == NULL || !RED(LEFT(w))) {
                    SET_BLACK(RIGHT(w));
                    SET_RED(w);
                    func_02040c00(w, &root);
                    w = LEFT(xp);
                }
                if (RED(xp))
                    SET_RED(w);
                else
                    SET_BLACK(w);
                SET_BLACK(xp);
                SET_BLACK(LEFT(w));
                func_02040c7c(xp, &root);
                x = root;
            }
        }
    }
    if (x != NULL)
        SET_BLACK(x);
}

/* 0x020409fc: restore the red-black properties after inserting `x`
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $X=zero:0x14 @$M+4:32=$T @$M+0xc:32=$M+4 @$T+0:32=$L @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 @$L+0:32=$X @$X+8:32=$L $X $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $X=zero:0x14 @$M+4:32=$T @$M+0xc:32=$M+4 @$T+0:32=$L @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 @$L+4:32=$X @$X+8:32=$L $X $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $X=zero:0x14 @$M+4:32=$T @$M+0xc:32=$M+4 @$T+0:32=$L @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 $R=zero:0x14 @$T+4:32=$R @$R+8:32=$T+1 @$L+0:32=$X @$X+8:32=$L $X $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $X=zero:0x14 @$M+4:32=$T @$M+0xc:32=$M+4 @$T+0:32=$L @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 $R=zero:0x14 @$T+4:32=$R @$R+8:32=$T+1 @$R+0:32=$X @$X+8:32=$R $X $T cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $X=zero:0x14 @$M+4:32=$T @$M+0xc:32=$M+4 @$T+0:32=$L @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T+1 @$L+0xc:32=5 $R=zero:0x14 @$T+4:32=$R @$R+8:32=$T+1 @$R+4:32=$X @$X+8:32=$R $X $T cases=3 */
void func_020409fc(u8 *x, u8 *root)
{
    SET_RED(x);
    while (x != root && RED(PARENT(x))) {
        u8 *p = PARENT(x), *g = PARENT(p), *u;
        if (p == LEFT(g)) {
            u = RIGHT(g);
            if (u != NULL && RED(u)) {
                SET_BLACK(p);
                SET_BLACK(u);
                x = PARENT(PARENT(x));
                SET_RED(x);
                continue;
            }
            if (x == RIGHT(p)) {
                x = p;
                func_02040c00(x, &root);
            }
            SET_BLACK(PARENT(x));
            SET_RED(PARENT(PARENT(x)));
            func_02040c7c(PARENT(PARENT(x)), &root);
        } else {
            u = LEFT(g);
            if (u != NULL && RED(u)) {
                SET_BLACK(p);
                SET_BLACK(u);
                x = PARENT(PARENT(x));
                SET_RED(x);
                continue;
            }
            if (x == LEFT(p)) {
                x = p;
                func_02040c7c(x, &root);
            }
            SET_BLACK(PARENT(x));
            SET_RED(PARENT(PARENT(x)));
            func_02040c00(PARENT(PARENT(x)), &root);
        }
    }
    SET_BLACK(root);
}

/* 0x02040c00: rotate left at x
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $P=ptr:4:4 @$P+0:32=$T $T $P cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $P=ptr:4:4 @$P+0:32=$T $R $P cases=3 */
void func_02040c00(u8 *x, u8 **root)
{
    u8 *y = RIGHT(x), *p;

    if (*root == x)
        *root = y;
    RIGHT(x) = LEFT(y);
    if (LEFT(y) != NULL)
        U32_AT(LEFT(y), 8) = (u32)x | (U32_AT(LEFT(y), 8) & 1);
    U32_AT(y, 8) = (U32_AT(x, 8) & ~1u) | (U32_AT(y, 8) & 1);
    p = PARENT(x);
    if (x == LEFT(p))
        LEFT(p) = y;
    else
        RIGHT(p) = y;
    LEFT(y) = x;
    U32_AT(x, 8) = (u32)y | (U32_AT(x, 8) & 1);
}

/* 0x02040c7c: rotate right at x
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $P=ptr:4:4 @$P+0:32=$T $T $P cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $P=ptr:4:4 @$P+0:32=$T $R $P cases=3 */
void func_02040c7c(u8 *x, u8 **root)
{
    u8 *y = LEFT(x), *p;

    if (*root == x)
        *root = y;
    LEFT(x) = RIGHT(y);
    if (RIGHT(y) != NULL)
        U32_AT(RIGHT(y), 8) = (u32)x | (U32_AT(RIGHT(y), 8) & 1);
    U32_AT(y, 8) = (U32_AT(x, 8) & ~1u) | (U32_AT(y, 8) & 1);
    p = PARENT(x);
    if (x == LEFT(p))
        LEFT(p) = y;
    else
        RIGHT(p) = y;
    RIGHT(y) = x;
    U32_AT(x, 8) = (u32)y | (U32_AT(x, 8) & 1);
}

/* 0x02040cf8: the map's destructor
 * @difftest $M=zero:0x10 $M cases=3
 * @difftest $M=zero:0x10 $T=zero:0x14 $L=zero:0x14 $R=zero:0x14 $RL=zero:0x14 $RR=zero:0x14 @$M+0:32=5 @$M+4:32=$T @$M+8:32=$L @$M+0xc:32=$M+4 @$T+0:32=$L @$T+4:32=$R @$T+8:32=$M+4 @$T+0xc:32=10 @$L+8:32=$T @$L+0xc:32=5 @$R+0:32=$RL @$R+4:32=$RR @$R+8:32=$T @$R+0xc:32=15 @$RL+8:32=$R+1 @$RL+0xc:32=12 @$RR+8:32=$R+1 @$RR+0xc:32=20 $M stub=0x0207fe44 cases=3 */
u8 *func_02040cf8(u8 *map)
{
    if (PTR_AT(map, 4) != NULL)
        func_02040378(map, PTR_AT(map, 4));
    return map;
}
