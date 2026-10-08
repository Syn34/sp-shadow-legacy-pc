/*
 * Actor.cpp, part 3: actor pool set-up and the std::vector code it instantiates.
 * ARM9 main, 0x02005f20 - 0x02007290 (55 functions).
 *
 * Two vector kinds are instantiated here: std::vector<CActor *> (the actor
 * lists, PtrVec) and std::vector<CActor> (the pool, 0x114-byte elements with
 * a vtable). Growing uses a split buffer {items, count, cap, &owner->cap, front}
 * that is filled and then swapped with the vector, as in Metrowerks' MSL.
 * Allocation failure and overflow print an error and abort.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define ACTOR_SIZE 0x114
#define VEC_MAX_PTR 0x3fffffffu
#define VEC_MAX_ACTOR 0xed7303u          /* 0xffffffff / 0x114 */

typedef struct SplitBuf {
    u8 *items;
    u32 count;
    u32 cap;
    u32 *capref;
    u32 front;
} SplitBuf;

extern u8 data_020bcaec[];   /* "vector length error" */
extern u8 data_020bcb00[];   /* "Memory allocation failure" */
extern u8 data_020bcb1c[];   /* "Memory allocation failure" */
extern u8 data_020bcb38[];   /* "Error: %s\n" */
extern u8 data_020bcbb8[];   /* CActor vtable */
extern u8 data_020c037c[];   /* CAppearance vtable */
extern u8 data_020c0cb0[];   /* CEntity vtable */
extern u32 data_020f6f08;
extern void *data_020f62ac, *data_020ebd2c, *data_020f53ac, *data_020f6f38;
extern PtrVec data_020d9eb8[], data_020d9ed0[], data_020d9ee8[], data_020d9f00[];

void *func_02007d78(void *actor);                  /* CActor::~CActor */
void *func_02007d9c(void *actor);                  /* CActor::CActor */
void func_02018158(void);
void *func_0207fe74(u32 size);                     /* operator new */
void func_0207fe44(void *p);                       /* operator delete */
void func_0209dcb8(void);                          /* abort */
void *func_0209e7f0(void *dst, const void *src, u32 n);   /* memmove */
void *func_0209e83c(void *dst, const void *src, u32 n);   /* memcpy */
s32 func_0209e90c(const void *fmt, ...);            /* printf */
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */

/* this file's own functions, called before their definition */
void func_02006330(PtrVec *v, u32 n);
void func_02006410(PtrVec *v, u32 n, u32 tag);
void func_02006470(PtrVec *v, u32 n, const void *value, u32 tag);
void func_0200650c(PtrVec *v, u32 n, const void *value, u32 tag);
void *func_02006570(void *dst, const void *src);
void *func_02006800(void *dst, const void *src);
void *func_02006828(u32 size, void *p);
void func_02006830(PtrVec *v, u32 n, void *const *value);
u32 func_020069a0(PtrVec *v, u32 n, u32 tag);
void func_02006a78(PtrVec *buf, u32 n, u32 tag);
void *func_02006acc(PtrVec *v);
void *func_02006b18(PtrVec *buf);
void func_02006b64(PtrVec *v, u32 n, const void *value);
void func_02006c98(SplitBuf *sb, u8 *first, u8 *last, u32 tag);
void func_02006d20(SplitBuf *sb, u32 n, const void *value, u32 tag);
u32 func_02006d8c(PtrVec *v, u32 n, u32 tag);
void *func_02006e68(SplitBuf *sb);
void func_02006ec8(void);
void *func_02006ef4(SplitBuf *sb);
void *func_02006f14(SplitBuf *sb);
void func_02006f64(SplitBuf *sb, u32 n, u32 tag);
void *func_02006fc0(SplitBuf *sb);
void func_0200700c(SplitBuf *sb, u32 n, u32 tag);
void func_02007060(SplitBuf *sb, u32 n, u32 tag);
void *func_0200712c(void *v);
void *func_020071e8(void *v);
void *func_02007214(void *v);

static inline void VecPush(PtrVec *v, void **value)
{
    if ((u32)v->count < (u32)v->cap) {
        v->count++;
        v->items[v->count - 1] = *value;
    } else {
        func_020062f0(v, value, 0);
    }
}

/* call the destructor (vtable slot 0); r1 holds the function, as in the original */
static inline void CallDtor(void *obj)
{
    void (*dtor)(void *, void *) = (void (*)(void *, void *))(*(void ***)obj)[0];
    dtor(obj, (void *)dtor);
}

/* swap a split buffer's storage with the vector's */
static inline void SwapStorage(PtrVec *v, u32 *items, u32 *count, u32 *cap)
{
    u32 t;
    t = v->cap;
    v->cap = *cap;
    *cap = t;
    t = (u32)v->items;
    v->items = (void **)*items;
    *items = t;
    t = v->count;
    v->count = *count;
    *count = t;
}

/* 0x02005f20: create the actor pools and lists of both screens */
void func_02005f20(void)
{
    u32 scr;
    func_0207fe28(data_020bca78, 0xdc);
    for (scr = 0; scr < 2; scr++) {
        u32 n = scr == 0 ? 0x6a : 0x10;
        PtrVec *pool, *v;
        u8 *p;
        void *cur;

        data_020df0fc = scr;
        pool = func_020061a0();
        if (n > (u32)pool->count) {
            u32 proto[ACTOR_SIZE / 4];
            func_02007d9c(proto);
            func_02006470(pool, n - pool->count, proto, 0);
            func_02007d78(proto);
        } else if (n < (u32)pool->count) {
            func_02006410(pool, pool->count - n, 0);
        }
        v = &data_020d9f18[data_020df0fc];
        if (n > (u32)v->cap)
            func_02006330(v, n);
        v = &data_020d9f30[data_020df0fc];
        if (n > (u32)v->cap)
            func_02006330(v, n);
        v = func_02006200();
        if (n > (u32)v->cap)
            func_02006330(v, n);
        v = func_020061e0();
        if (n > (u32)v->cap)
            func_02006330(v, n);
        v = func_02006220();
        if (n > (u32)v->cap)
            func_02006330(v, n);
        v = func_020061c0();
        if ((u32)v->cap < 0xf)
            func_02006330(v, 0xf);

        p = (u8 *)func_020061a0()->items;
        v = func_020061a0();
        if (p != (u8 *)v->items + v->count * ACTOR_SIZE) {
            do {
                cur = p;
                VecPush(&data_020d9f18[data_020df0fc], &cur);
                p += ACTOR_SIZE;
                v = func_020061a0();
            } while (p != (u8 *)v->items + v->count * ACTOR_SIZE);
        }
        func_02018158();
    }
    func_0207fe24();
    data_020df0fc = 0;
}

/* 0x020061a0: actor pool of the current screen */
PtrVec *func_020061a0(void) { return &data_020d9f00[data_020df0fc]; }

/* 0x020061c0 */
PtrVec *func_020061c0(void) { return &data_020d9ee8[data_020df0fc]; }

/* 0x020061e0 */
PtrVec *func_020061e0(void) { return &data_020d9eb8[data_020df0fc]; }

/* 0x02006200: active actors of the current screen */
PtrVec *func_02006200(void) { return &data_020d9ea0[data_020df0fc]; }

/* 0x02006220: actors to draw on the current screen */
PtrVec *func_02006220(void) { return &data_020d9ed0[data_020df0fc]; }

/* 0x02006240: vector<CActor *>::erase(pos)
 * @difftest $I=ptr:0x40:4 $V=ptr:12:4 @$V:32=$I @$V+4:32=pick:1,4,9,16 $V $I u8 */
void **func_02006240(PtrVec *v, void **pos, u32 tag)
{
    s32 bytes = (u8 *)(v->items + v->count) - (u8 *)pos;
    func_0209e7f0(pos, pos + 1, (bytes / 4 - 1) * 4);
    v->count--;
    return pos;
}

/* 0x020062a0 */
void *func_020062a0(void) { return data_020f62ac; }

/* 0x020062b0 */
void *func_020062b0(void) { return data_020ebd2c; }

/* 0x020062c0 */
void *func_020062c0(void) { return data_020f53ac; }

/* 0x020062d0 */
void *func_020062d0(void) { return data_020f6f38; }

/* 0x020062e0 */
void *func_020062e0(void) { return (void *)data_020f6f08; }

/* 0x020062f0: vector<CActor *>::push_back when full
 * @difftest zero:12 ptr:4:4 u8 */
void func_020062f0(void *vec, void **value, u32 tag)
{
    func_020069a0(vec, 1, 0);
    func_02006830(vec, 1, value);
}

/* 0x02006330: vector<CActor *>::reserve(n)
 * @difftest zero:12 int:0:40 */
void func_02006330(PtrVec *v, u32 n)
{
    PtrVec tmp;
    s32 bytes, k;

    tmp.items = NULL;
    tmp.count = 0;
    tmp.cap = 0;
    func_02006a78(&tmp, n, 0);
    bytes = (u8 *)(v->items + v->count) - (u8 *)v->items;
    k = bytes / 4;
    func_0209e83c(tmp.items + tmp.count, v->items, k * 4);
    tmp.count += k;
    v->count = 0;
    SwapStorage(v, (u32 *)&tmp.items, (u32 *)&tmp.count, (u32 *)&tmp.cap);
    func_02006b18(&tmp);
}

/* 0x02006410: vector<CActor>: destroy the last n elements
 * @difftest pick:0x020d9f00,0x020d9f0c int:0:4 u8 cases=20 */
void func_02006410(PtrVec *v, u32 n, u32 tag)
{
    u8 *end = (u8 *)v->items + v->count * ACTOR_SIZE;
    v->count -= n;
    if (n == 0)
        return;
    do {
        end -= ACTOR_SIZE;
        CallDtor(end);
    } while (--n != 0);
}

/* 0x02006470: vector<CActor>: append n copies of *value */
void func_02006470(PtrVec *v, u32 n, const void *value, u32 tag)
{
    u32 cap = v->cap;
    if (n > cap || (u32)v->count > cap - n) {
        func_02006d8c(v, v->count + n - cap, 0);
        func_02006b64(v, n, value);
    } else {
        func_0200650c(v, n, value, 0);
    }
}

/* 0x0200650c: vector<CActor>: construct n copies at the end (capacity suffices)
 * @difftest $I=ptr:0x900:4 $V=ptr:12:4 @$V:32=$I @$V+4:32=int:0:4 $V int:0:4 ptr:0x114:4 u8 */
void func_0200650c(PtrVec *v, u32 n, const void *value, u32 tag)
{
    u8 *p = (u8 *)v->items + v->count * ACTOR_SIZE;
    if (n == 0)
        return;
    do {
        void *q = func_02006828(ACTOR_SIZE, p);
        if (q != NULL)
            func_02006570(q, value);
        v->count++;
        p += ACTOR_SIZE;
    } while (--n != 0);
}

/* 0x02006570: CActor copy constructor
 * @difftest ptr:0x114:4 ptr:0x114:4 */
void *func_02006570(void *dst, const void *src)
{
    u32 i;
    func_02006800(dst, src);
    PTR_AT(dst, 0) = data_020c037c;
    for (i = 0x10; i < 0x40; i += 4)
        U32_AT(dst, i) = U32_AT(src, i);
    S16_AT(dst, 0x40) = S16_AT(src, 0x40);
    S8_AT(dst, 0x42) = S8_AT(src, 0x42);
    U32_AT(dst, 0x44) = U32_AT(src, 0x44);
    U32_AT(dst, 0x48) = U32_AT(src, 0x48);
    PTR_AT(dst, 0) = data_020bcbb8;
    for (i = 0x4c; i < ACTOR_SIZE; i += 4)
        U32_AT(dst, i) = U32_AT(src, i);
    return dst;
}

/* 0x02006800: CEntity copy constructor
 * @difftest ptr:16:4 ptr:16:4 */
void *func_02006800(void *dst, const void *src)
{
    PTR_AT(dst, 0) = data_020c0cb0;
    U32_AT(dst, 4) = U32_AT(src, 4);
    U32_AT(dst, 8) = U32_AT(src, 8);
    U32_AT(dst, 0xc) = U32_AT(src, 0xc);
    return dst;
}

/* 0x02006828: placement operator new
 * @difftest u32 u32 */
void *func_02006828(u32 size, void *p) { return p; }

/* 0x02006830: vector<CActor *>: reallocate and append n copies of *value
 * @difftest zero:12 int:1:6 ptr:4:4 */
void func_02006830(PtrVec *v, u32 n, void *const *value)
{
    SplitBuf sb;
    u32 **dst, i;
    s32 bytes, k;

    sb.items = NULL;
    sb.count = 0;
    sb.cap = 0;
    sb.front = 0;
    sb.capref = (u32 *)&v->cap;
    func_0200700c(&sb, func_020069a0(v, v->count + n - v->cap, 0), 0);
    sb.front = v->count;
    dst = (u32 **)(sb.items + sb.front * 4 + sb.count * 4);
    for (i = n; i != 0; i--)
        *dst++ = *value;
    sb.count += n;
    bytes = (u8 *)(v->items + v->count) - (u8 *)v->items;
    k = bytes / 4;
    sb.front -= k;
    func_0209e83c(sb.items + sb.front * 4, v->items, k * 4);
    sb.count += k;
    v->count = 0;
    SwapStorage(v, (u32 *)&sb.items, &sb.count, &sb.cap);
    func_02006ef4(&sb);
}

/* MSL growth policy: new capacity for a vector that needs `n` more slots */
static inline u32 GrowCap(u32 cap, u32 n, u32 max, u32 lim1, u32 lim2)
{
    u32 g;
    if (n > max - cap)
        func_02006ec8();
    if (cap < lim1) {
        g = (cap + 1) * 3 / 5;
        return cap + (g < n ? n : g);
    }
    if (cap < lim2) {
        g = (cap + 1) >> 1;
        return cap + (g < n ? n : g);
    }
    return max;
}

/* 0x020069a0: vector<CActor *> growth policy
 * @difftest ptr:12:4 int:0:100 u8
 * @difftest $V=ptr:12:4 @$V+8:32=pick:0,7,100,0x15555554,0x15555555,0x2aaaaaa9,0x2aaaaaaa $V int:0:0x10000000 u8 */
u32 func_020069a0(PtrVec *v, u32 n, u32 tag)
{
    return GrowCap(v->cap, n, VEC_MAX_PTR, 0x15555555, 0x2aaaaaaa);
}

/* 0x02006a78: allocate storage for n pointers
 * @difftest zero:12 int:0:64 u8 */
void func_02006a78(PtrVec *buf, u32 n, u32 tag)
{
    void *p;
    if (n > VEC_MAX_PTR)
        func_02006ec8();
    p = func_0207fe74(n << 2);
    if (p == NULL) {
        func_0209e90c(data_020bcb38, data_020bcb1c);
        func_0209dcb8();
    }
    buf->items = p;
    buf->cap = n;
}

/* 0x02006acc: vector<CActor *> destructor */
void *func_02006acc(PtrVec *v)
{
    v->count -= v->count;
    if (v->items != NULL)
        func_0207fe44(v->items);
    return v;
}

/* 0x02006b18: vector<CActor *> storage destructor */
void *func_02006b18(PtrVec *buf)
{
    buf->count -= buf->count;
    if (buf->items != NULL)
        func_0207fe44(buf->items);
    return buf;
}

/* 0x02006b64: vector<CActor>: reallocate and append n copies of *value */
void func_02006b64(PtrVec *v, u32 n, const void *value)
{
    SplitBuf sb;
    sb.items = NULL;
    sb.count = 0;
    sb.cap = 0;
    sb.front = 0;
    sb.capref = (u32 *)&v->cap;
    func_02006f64(&sb, func_02006d8c(v, v->count + n - v->cap, 0), 0);
    sb.front = v->count;
    func_02006d20(&sb, n, value, 0);
    func_02006c98(&sb, (u8 *)v->items, (u8 *)v->items + v->count * ACTOR_SIZE, 0);
    SwapStorage(v, (u32 *)&sb.items, &sb.count, &sb.cap);
    func_02006e68(&sb);
}

/* 0x02006c98: copy-construct [first, last) in front of the split buffer's contents */
void func_02006c98(SplitBuf *sb, u8 *first, u8 *last, u32 tag)
{
    u8 *dst = sb->items + sb->front * ACTOR_SIZE;
    if (last <= first)
        return;
    do {
        void *q;
        dst -= ACTOR_SIZE;
        last -= ACTOR_SIZE;
        q = func_02006828(ACTOR_SIZE, dst);
        if (q != NULL)
            func_02006570(q, last);
        sb->front--;
        sb->count++;
    } while (last > first);
}

/* 0x02006d20: construct n copies of *value at the back of the split buffer */
void func_02006d20(SplitBuf *sb, u32 n, const void *value, u32 tag)
{
    u8 *p = sb->items + sb->front * ACTOR_SIZE + sb->count * ACTOR_SIZE;
    if (n == 0)
        return;
    do {
        void *q = func_02006828(ACTOR_SIZE, p);
        if (q != NULL)
            func_02006570(q, value);
        sb->count++;
        p += ACTOR_SIZE;
    } while (--n != 0);
}

/* 0x02006d8c: vector<CActor> growth policy
 * @difftest ptr:12:4 int:0:100 u8
 * @difftest $V=ptr:12:4 @$V+8:32=pick:0,7,100,0x4f2655,0x4f2656,0x9e4cab,0x9e4cac $V int:0:0x600000 u8 */
u32 func_02006d8c(PtrVec *v, u32 n, u32 tag)
{
    return GrowCap(v->cap, n, VEC_MAX_ACTOR, 0x4f2656, 0x9e4cac);
}

/* 0x02006e68: split buffer of CActor: destroy the elements and free */
void *func_02006e68(SplitBuf *sb)
{
    u8 *begin = sb->items + sb->front * ACTOR_SIZE;
    u8 *end = begin + sb->count * ACTOR_SIZE;
    while (end > begin) {
        end -= ACTOR_SIZE;
        CallDtor(end);
    }
    sb->count = 0;
    func_02006f14(sb);
    return sb;
}

/* 0x02006ec8: throw std::length_error("vector length error")
 * @difftest cases=2 */
void func_02006ec8(void)
{
    func_0209e90c(data_020bcb38, data_020bcaec);
    func_0209dcb8();
}

/* 0x02006ef4: split buffer of pointers: free */
void *func_02006ef4(SplitBuf *sb)
{
    sb->count = 0;
    func_02006fc0(sb);
    return sb;
}

/* 0x02006f14: split buffer of CActor: destroy remaining elements and free */
void *func_02006f14(SplitBuf *sb)
{
    func_02007060(sb, sb->count, 0);
    if (sb->items != NULL)
        func_0207fe44(sb->items);
    return sb;
}

/* 0x02006f64: allocate storage for n actors
 * @difftest zero:20 int:0:4 u8 */
void func_02006f64(SplitBuf *sb, u32 n, u32 tag)
{
    void *p;
    if (n > VEC_MAX_ACTOR)
        func_02006ec8();
    p = func_0207fe74(n * ACTOR_SIZE);
    if (p == NULL) {
        func_0209e90c(data_020bcb38, data_020bcb00);
        func_0209dcb8();
    }
    sb->items = p;
    sb->cap = n;
}

/* 0x02006fc0: storage destructor */
void *func_02006fc0(SplitBuf *sb)
{
    sb->count -= sb->count;
    if (sb->items != NULL)
        func_0207fe44(sb->items);
    return sb;
}

/* 0x0200700c: allocate storage for n pointers (split buffer)
 * @difftest zero:20 int:0:64 u8 */
void func_0200700c(SplitBuf *sb, u32 n, u32 tag)
{
    void *p;
    if (n > VEC_MAX_PTR)
        func_02006ec8();
    p = func_0207fe74(n << 2);
    if (p == NULL) {
        func_0209e90c(data_020bcb38, data_020bcb1c);
        func_0209dcb8();
    }
    sb->items = p;
    sb->cap = n;
}

/* 0x02007060: split buffer of CActor: destroy the last n elements */
void func_02007060(SplitBuf *sb, u32 n, u32 tag)
{
    u8 *end = sb->items + sb->count * ACTOR_SIZE;
    sb->count -= n;
    if (n == 0)
        return;
    do {
        end -= ACTOR_SIZE;
        CallDtor(end);
    } while (--n != 0);
}

/* 0x020070c0-0x020071c4: static destructors of the per-screen vectors
 * @difftest cases=2 */
void func_020070c0(void) { func_020a6d58(data_020d9f30, 2, 0xc, func_020071e8); }
/* @difftest cases=2 */
void func_020070e4(void) { func_020a6d58(data_020d9f18, 2, 0xc, func_020071e8); }
/* @difftest cases=2 */
void func_02007108(void) { func_020a6d58(data_020d9f00, 2, 0xc, func_0200712c); }

/* 0x0200712c: vector<CActor> destructor
 * @difftest pick:0x020d9f00,0x020d9f0c cases=4 */
void *func_0200712c(void *v)
{
    func_02007214(v);
    return v;
}

/* 0x02007144: vector constructor
 * @difftest ptr:12:4 */
void *func_02007144(PtrVec *v)
{
    v->items = NULL;
    v->count = 0;
    v->cap = 0;
    return v;
}

/* @difftest cases=2 */
void func_02007158(void) { func_020a6d58(data_020d9ee8, 2, 0xc, func_020071e8); }
/* @difftest cases=2 */
void func_0200717c(void) { func_020a6d58(data_020d9ed0, 2, 0xc, func_020071e8); }
/* @difftest cases=2 */
void func_020071a0(void) { func_020a6d58(data_020d9eb8, 2, 0xc, func_020071e8); }
/* @difftest cases=2 */
void func_020071c4(void) { func_020a6d58(data_020d9ea0, 2, 0xc, func_020071e8); }

/* 0x020071e8: vector<CActor *> destructor
 * @difftest pick:0x020d9ea0,0x020d9eac,0x020d9f30 cases=6
 * @difftest zero:12 cases=5 */
void *func_020071e8(void *v)
{
    func_02006acc(v);
    return v;
}

/* 0x02007200: vector constructor
 * @difftest ptr:12:4 */
void *func_02007200(PtrVec *v)
{
    v->items = NULL;
    v->count = 0;
    v->cap = 0;
    return v;
}

/* 0x02007214: vector<CActor> storage destructor
 * @difftest pick:0x020d9f00,0x020d9f0c cases=4 */
void *func_02007214(void *vec)
{
    PtrVec *v = vec;
    func_02006410(v, v->count, 0);
    if (v->items != NULL)
        func_0207fe44(v->items);
    return v;
}

/* 0x02007264
 * @difftest ptr:0x114:4 */
void func_02007264(void *a) { U8_AT(a, 0x10e) = 3; }

/* 0x02007270 (virtual, empty)
 * @difftest u32 */
void func_02007270(void *a) { (void)a; }

/* 0x02007274 (virtual, empty)
 * @difftest u32 */
void func_02007274(void *a) { (void)a; }

/* 0x02007278
 * @difftest u32 */
s32 func_02007278(void *a) { return 0; }

/* 0x02007280
 * @difftest u32 */
s32 func_02007280(void *a) { return 0; }

/* 0x02007288
 * @difftest u32 */
s32 func_02007288(void *a) { return 0; }
