/*
 * BossBattle.cpp, part 2: setting up the three boss battles (creating the
 * battle's actors and filling the root and collision vectors), the arena
 * object's constructor and the constructors/destructors of two actor
 * bases that the file instantiates.
 * ARM9 main, 0x0202b968 - 0x0202d058 (15 functions).
 *
 * Every battle creates: the arena/level object (0x960/0x964 bytes, built by
 * func_0202c17c), the two fighters, the HUD counter (func_02079b60) and
 * one or three helpers, then pushes them into g_GameRoot and the two
 * collision lists data_020e36e0 / data_020e36f8 (see BossBattle1.c), runs
 * the common set-up func_0202b5f4 and places the hero. Objects are given
 * their final vtable in steps, as C++ constructors do.
 *
 * Testing: the three set-ups run with the common set-up func_0202b5f4
 * and the music start func_020202f0 stubbed (both load files from the
 * card), so they stop when placing the hero (whose 3D node the common
 * set-up would have created) and memory is compared at that point
 * (faultmem); the deleting destructors with
 * operator delete stubbed (their argument is a scratch buffer).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define FN(f) ((void *(*)(void *))(void (*)(void))(f))

extern const char data_020bdf50[];   /* "BossBattle.cpp" */
extern u8 data_020bdf84[];           /* vtables */
extern u8 data_020be030[];
extern u8 data_020c1e4c[], data_020c1ec8[], data_020c203c[], data_020c221c[],
    data_020c22d8[], data_020c23f0[], data_020c25e4[], data_020c2688[],
    data_020c285c[], data_020c2904[], data_020c29c4[], data_020c2b08[];
extern s32 data_020e36b0;
extern PtrVec data_020e36e0, data_020e36f8;
extern PtrVec g_GameRoot;

void *func_02018208(void);
void func_02082fb8(void *obj, u32 a, u32 b);
void func_02083044(void *obj, u32 a, u32 b, u32 c);
void *func_0207fe74(u32 size);                     /* operator new */
void func_0207fe44(void *p);                       /* operator delete */
void func_02006330(PtrVec *v, u32 n);              /* reserve */
void *func_02079b60(void *p);
u32 func_02078290(void *obj, u32 id);
void func_020772b8(void *obj, u32 a, u32 b);
void func_0202b5f4(void);
void func_020202f0(u32 v);
void func_020a6dcc(void *arr, u32 n, u32 size, void *(*ctor)(void *), void *(*dtor)(void *));
void func_020022e4(void *self);
void func_02071a78(void *p);
void func_02071968(void *p);
void func_0205dcb0(void *p);
void func_0205dc08(void *p);
void func_02061d60(void *p);
void func_02061c68(void *p);

void *func_0202c17c(u8 *p);
u32 *func_0202c2ac(u32 *p);
void *func_0202ce78(u8 *p);
void *func_0202cea4(u8 *p);
void *func_0202cf68(u8 *p);
void *func_0202cf94(u8 *p);

#define VPTR(p) PTR_AT(p, 0)

static inline void VecPush(PtrVec *v, void **value)
{
    if ((u32)v->count < (u32)v->cap) {
        v->count++;
        v->items[v->count - 1] = *value;
    } else {
        func_020062f0(v, value, 0);
    }
}

static inline void VecReserve8(PtrVec *v)
{
    if ((u32)v->cap < 8)
        func_02006330(v, 8);
}

/* a helper actor that refers to the two fighters */
static u8 *NewHelper(u32 size, void *vt, u8 *a, u8 *b)
{
    u8 *p = func_0207fe74(size);

    if (p != NULL) {
        func_0202cf94(p);
        VPTR(p) = data_020c1e4c;
        VPTR(p) = vt;
        U32_AT(p, 0x2e8) = 0;
        U32_AT(p, 0x2ec) = 0;
        U32_AT(p, 0x2f0) = 0;
        PTR_AT(p, 0x2dc) = a;
        PTR_AT(p, 0x2e0) = b;
    }
    return p;
}

/* 0x0202b968: set up boss battle 1
 * @difftest stub=0x0202b5f4,0x020202f0 faultmem=1 cases=3 */
void func_0202b968(void)
{
    u8 *lvl, *fa, *fb, *hud, *h1, *h2, *h3, *hero;

    func_02082fb8(func_02018208(), 0x100, 0x10);
    func_02083044(func_02018208(), 10, 0x20, 0x200);
    data_020e36b0 = 2;

    lvl = func_0207fe74(0x960);
    if (lvl != NULL) {
        func_0202c17c(lvl);
        VPTR(lvl) = data_020c2b08;
    }
    fa = func_0207fe74(0x61c);
    if (fa != NULL) {
        func_0202cf94(fa);
        VPTR(fa) = data_020c1e4c;
        VPTR(fa) = data_020c1ec8;
        U32_AT(fa, 0x2dc) = 0;
        U32_AT(fa, 0x2e0) = 0;
        U32_AT(fa, 0x2e4) = 0;
        VPTR(fa) = data_020c29c4;
        func_0202cf68(fa + 0x338);
    }
    fb = func_0207fe74(0x2dc);
    if (fb != NULL) {
        func_0202cf94(fb);
        VPTR(fb) = data_020c1e4c;
        VPTR(fb) = data_020c22d8;
    }
    hud = func_0207fe74(0x300);
    if (hud != NULL)
        hud = func_02079b60(hud);
    h1 = NewHelper(0x2f8, data_020c2904, fa, fb);
    h2 = NewHelper(0x2f8, data_020c2904, fa, fb);
    h3 = NewHelper(0x2f8, data_020c2904, fa, fb);

    func_0207fe28(data_020bdf50, 0x106);
    VecReserve8(&g_GameRoot);
    VecPush(&g_GameRoot, (void **)&lvl);
    VecPush(&g_GameRoot, (void **)&fb);
    VecPush(&g_GameRoot, (void **)&fa);
    VecPush(&g_GameRoot, (void **)&hud);
    VecPush(&g_GameRoot, (void **)&h1);
    VecPush(&g_GameRoot, (void **)&h2);
    VecPush(&g_GameRoot, (void **)&h3);
    func_0207fe24();
    func_0207fe28(data_020bdf50, 0x112);
    VecReserve8(&data_020e36e0);
    VecPush(&data_020e36e0, (void **)&fb);
    VecPush(&data_020e36e0, (void **)&h1);
    VecPush(&data_020e36e0, (void **)&h2);
    VecPush(&data_020e36e0, (void **)&h3);
    func_0207fe24();
    func_0207fe28(data_020bdf50, 0x11a);
    VecReserve8(&data_020e36f8);
    VecPush(&data_020e36f8, (void **)&lvl);
    VecPush(&data_020e36f8, (void **)&fa);
    VecPush(&data_020e36f8, (void **)&fb);
    func_0207fe24();

    func_0202b5f4();
    func_020202f0(0x25);
    {
        u32 a = func_02078290(lvl, 0x12c);
        u32 b = func_02078290(lvl, 0x12e);
        func_020772b8(fa, a, b);
    }
    hero = g_GameRoot.items[1];
    U32_AT(PTR_AT(hero, 0x44), 4) = 0;
    U32_AT(PTR_AT(hero, 0x44), 8) = 0x5000;
    U32_AT(PTR_AT(hero, 0x44), 0xc) = 0x14000;
    ((u8 *)PTR_AT(hero, 0x44))[0x8c] = 1;
    U32_AT(hero, 0x6c) = 0;
    U32_AT(hero, 0x70) = 0x5000;
    U32_AT(hero, 0x74) = 0x14000;
    hero[0xf4] = 1;
    PTR_AT(PTR_AT(PTR_AT(g_GameRoot.items[0], 0x44), 0xac), 4) = g_GameRoot.items[1];
}

/* 0x0202c17c: construct the arena object
 * @difftest zero:0x964 cases=10 */
void *func_0202c17c(u8 *p)
{
    u16 *h;

    func_0202cea4(p);
    VPTR(p) = data_020c203c;
    func_0202ce78(p + 0x2f4);
    U32_AT(p, 0x5e8) = 0;
    U32_AT(p, 0x5ec) = 0;
    U32_AT(p, 0x5f0) = 0;
    U32_AT(p, 0x5f4) = 0;
    U32_AT(p, 0x5f8) = 0;
    U32_AT(p, 0x5fc) = 0;
    func_0202cf68(p + 0x600);
    U32_AT(p, 0x8e8) = 0;
    U32_AT(p, 0x8ec) = 0;
    U32_AT(p, 0x8f0) = 0;
    func_020a6dcc(p + 0x908, 3, 0xc, FN(func_0202c2ac), FN(func_020022e4));
    for (h = (u16 *)(p + 0x92c); h != (u16 *)(p + 0x932); h++)
        *h = 0xffff;
    func_020a6dcc(p + 0x934, 3, 0xc, FN(func_0202c2ac), FN(func_020022e4));
    for (h = (u16 *)(p + 0x958); h != (u16 *)(p + 0x95e); h++)
        *h = 0xffff;
    return p;
}

/* 0x0202c2ac: clear a 3-word entry
 * @difftest ptr:12:4 cases=10 */
u32 *func_0202c2ac(u32 *p)
{
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    return p;
}

/* boss battles 2 and 3 differ only in these constants */
static void SetupBattle(u32 mode, void *lvl_vt, void *fa_vt, u32 line, u32 msg_a, u32 msg_b)
{
    u8 *lvl, *fa, *fb, *hud, *h1, *hero;

    func_02082fb8(func_02018208(), 0x100, 0x10);
    func_02083044(func_02018208(), 10, 0x20, 0x200);
    data_020e36b0 = mode;

    lvl = func_0207fe74(0x964);
    if (lvl != NULL) {
        func_0202c17c(lvl);
        VPTR(lvl) = lvl_vt;
    }
    fa = func_0207fe74(0x33c);
    if (fa != NULL) {
        func_0202cf94(fa);
        VPTR(fa) = data_020c1e4c;
        VPTR(fa) = data_020c1ec8;
        U32_AT(fa, 0x2dc) = 0;
        U32_AT(fa, 0x2e0) = 0;
        U32_AT(fa, 0x2e4) = 0;
        VPTR(fa) = fa_vt;
    }
    fb = func_0207fe74(0x2dc);
    if (fb != NULL) {
        func_0202cf94(fb);
        VPTR(fb) = data_020c1e4c;
        VPTR(fb) = data_020c22d8;
    }
    hud = func_0207fe74(0x300);
    if (hud != NULL)
        hud = func_02079b60(hud);
    h1 = func_0207fe74(0x2f0);
    if (h1 != NULL) {
        func_0202cf94(h1);
        VPTR(h1) = data_020c1e4c;
        VPTR(h1) = data_020c221c;
        U32_AT(h1, 0x2e0) = 0;
        U32_AT(h1, 0x2e4) = 0;
        U32_AT(h1, 0x2e8) = 0;
    }

    func_0207fe28(data_020bdf50, line);
    VecReserve8(&g_GameRoot);
    VecPush(&g_GameRoot, (void **)&lvl);
    VecPush(&g_GameRoot, (void **)&fb);
    VecPush(&g_GameRoot, (void **)&fa);
    VecPush(&g_GameRoot, (void **)&hud);
    VecPush(&g_GameRoot, (void **)&h1);
    func_0207fe24();
    func_0207fe28(data_020bdf50, line + 9);
    VecReserve8(&data_020e36e0);
    VecPush(&data_020e36e0, (void **)&fb);
    VecPush(&data_020e36e0, (void **)&h1);
    func_0207fe24();
    func_0207fe28(data_020bdf50, line + 0xf);
    VecReserve8(&data_020e36f8);
    VecPush(&data_020e36f8, (void **)&lvl);
    VecPush(&data_020e36f8, (void **)&fa);
    VecPush(&data_020e36f8, (void **)&h1);
    func_0207fe24();

    func_0202b5f4();
    func_020202f0(0xe);
    {
        u32 a = func_02078290(lvl, msg_a);
        u32 b = func_02078290(lvl, msg_b);
        func_020772b8(fa, a, b);
    }
    hero = g_GameRoot.items[1];
    U32_AT(PTR_AT(hero, 0x44), 4) = 0;
    U32_AT(PTR_AT(hero, 0x44), 8) = 0x1000;
    U32_AT(PTR_AT(hero, 0x44), 0xc) = 0x1e000;
    ((u8 *)PTR_AT(hero, 0x44))[0x8c] = 1;
    U32_AT(hero, 0x6c) = 0;
    U32_AT(hero, 0x70) = 0x1000;
    U32_AT(hero, 0x74) = 0x1e000;
    hero[0xf4] = 1;
}

/* 0x0202c2c0: set up boss battle 2
 * @difftest stub=0x0202b5f4,0x020202f0 faultmem=1 cases=3 */
void func_0202c2c0(void)
{
    SetupBattle(1, data_020c285c, data_020c2688, 0xd1, 0x12b, 0x12d);
}

/* 0x0202c89c: set up boss battle 3
 * @difftest stub=0x0202b5f4,0x020202f0 faultmem=1 cases=3 */
void func_0202c89c(void)
{
    SetupBattle(0, data_020c25e4, data_020c23f0, 0xa1, 0x116, 0x117);
}

/* 0x0202ce78: constructor of an actor base (vtable data_020bdf84)
 * @difftest zero:0x300 cases=10 */
void *func_0202ce78(u8 *p)
{
    func_02071a78(p);
    VPTR(p) = data_020bdf84;
    func_0205dcb0(p + 0x234);
    return p;
}

/* 0x0202cea4: the same constructor, another instance
 * @difftest zero:0x300 cases=10 */
void *func_0202cea4(u8 *p)
{
    func_02071a78(p);
    VPTR(p) = data_020bdf84;
    func_0205dcb0(p + 0x234);
    return p;
}

/* 0x0202ced0: its destructor
 * @difftest zero:0x300 cases=10 */
void *func_0202ced0(u8 *p)
{
    VPTR(p) = data_020bdf84;
    func_0205dc08(p + 0x234);
    func_02071968(p);
    return p;
}

/* 0x0202cf00: its deleting destructor
 * @difftest zero:0x300 stub=0x0207fe44 cases=10 */
void *func_0202cf00(u8 *p)
{
    VPTR(p) = data_020bdf84;
    func_0205dc08(p + 0x234);
    func_02071968(p);
    func_0207fe44(p);
    return p;
}

/* 0x0202cf38: its destructor, another instance
 * @difftest zero:0x300 cases=10 */
void *func_0202cf38(u8 *p)
{
    VPTR(p) = data_020bdf84;
    func_0205dc08(p + 0x234);
    func_02071968(p);
    return p;
}

/* 0x0202cf68: constructor of the other actor base (vtable data_020be030)
 * @difftest zero:0x300 cases=10 */
void *func_0202cf68(u8 *p)
{
    func_02071a78(p);
    VPTR(p) = data_020be030;
    func_02061d60(p + 0x234);
    return p;
}

/* 0x0202cf94: the same constructor, another instance
 * @difftest zero:0x300 cases=10 */
void *func_0202cf94(u8 *p)
{
    func_02071a78(p);
    VPTR(p) = data_020be030;
    func_02061d60(p + 0x234);
    return p;
}

/* 0x0202cfc0: its destructor
 * @difftest zero:0x300 cases=10 */
void *func_0202cfc0(u8 *p)
{
    VPTR(p) = data_020be030;
    func_02061c68(p + 0x234);
    func_02071968(p);
    return p;
}

/* 0x0202cff0: its deleting destructor
 * @difftest zero:0x300 stub=0x0207fe44 cases=10 */
void *func_0202cff0(u8 *p)
{
    VPTR(p) = data_020be030;
    func_02061c68(p + 0x234);
    func_02071968(p);
    func_0207fe44(p);
    return p;
}

/* 0x0202d028: its destructor, another instance
 * @difftest zero:0x300 cases=10 */
void *func_0202d028(u8 *p)
{
    VPTR(p) = data_020be030;
    func_02061c68(p + 0x234);
    func_02071968(p);
    return p;
}
