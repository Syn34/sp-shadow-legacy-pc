/*
 * Guarded accessors for the game's root object.
 * ARM9 main, 0x0202a5d0 - 0x0202aa14 (23 functions).
 *
 * Almost every function here follows the same pattern: if the root object is
 * valid (g_GameRootValid), read or write a field of one of the objects it
 * points to, otherwise do nothing / return 0. The objects are C++ classes; the
 * field meanings are not known yet, so they are referred to by offset.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 *
 * Testing note: the root object is not valid in any of the recorded memory
 * snapshots, so the second @difftest line of each function builds a fake root
 * (g_GameRootValid = 1, g_GameRoot -> buffers) to exercise the "valid" path.
 * The virtual calls are pointed at the original Math_Mul (0x02070a18), which
 * gives a deterministic, argument-dependent result.
 */
#include "types.h"

#define FIELD(type, base, offset) (*(type *)((u8 *)(base) + (offset)))

typedef struct GameObject GameObject;
typedef s32 (*GameObjectMethod)(GameObject *self, s32 arg);

struct GameObject {
    GameObjectMethod *vtable;
    /* ... fields at 0x304-0x320 are accessed below ... */
};

typedef struct GameRoot {
    u8 *unk0;              /* object with a field at +0x8e0 */
    u32 unk4;
    GameObject *unk8;      /* main game object */
} GameRoot;

extern s8 g_GameRootValid;     /* 0x020e36a4 */
extern GameRoot *g_GameRoot;   /* 0x020e36c8 */
extern s32 data_020e36b0;      /* mode-like value compared with 0, 1, 2 */
extern s8 data_020df10c;
extern s8 data_020e36a0;

s8 func_0202a7f0(void);
extern s32 func_020772c4(GameObject *obj);
extern s32 func_020772d8(GameObject *obj);

/* @difftest u32 
 * @difftest u32 $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
void func_0202a5d0(s32 value)
{
    if (g_GameRootValid)
        FIELD(s32, g_GameRoot->unk0, 0x8e0) = value;
}

/* @difftest 
 * @difftest $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
s32 func_0202a5f8(void)
{
    if (!g_GameRootValid)
        return 0;
    return FIELD(s32, g_GameRoot->unk0, 0x8e0);
}

/* calls virtual method slot 0x88/4 on the main game object
 * @difftest int:0:4 cases=200 
 * @difftest int:0:0x100000 $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O $V=zero:0x100 @$O+0:32=$V @$V+0x88:32=0x02070a18 cases=300 */
s32 func_0202a624(s32 arg)
{
    GameObject *obj;
    if (!g_GameRootValid)
        return 0;
    obj = g_GameRoot->unk8;
    return obj->vtable[0x88 / 4](obj, arg);
}

/* @difftest 
 * @difftest $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
s32 func_0202a674(void)
{
    if (!g_GameRootValid)
        return 0;
    return FIELD(s32, g_GameRoot->unk8, 0x30c);
}

/* calls virtual method slot 0x84/4 on the main game object
 * @difftest int:0:4 cases=200 
 * @difftest int:0:0x100000 $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O $V=zero:0x100 @$O+0:32=$V @$V+0x84:32=0x02070a18 cases=300 */
s32 func_0202a6a0(s32 arg)
{
    GameObject *obj;
    if (!g_GameRootValid)
        return 0;
    obj = g_GameRoot->unk8;
    return obj->vtable[0x84 / 4](obj, arg);
}

/* @difftest ptr:0x400:4 u32 */
void func_0202a6f0(GameObject *obj, s32 value)
{
    FIELD(s32, obj, 0x304) = value;
}

/* @difftest 
 * @difftest $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
s32 func_0202a6f8(void)
{
    if (!g_GameRootValid)
        return 0;
    return FIELD(s32, g_GameRoot->unk8, 0x304);
}

/* @difftest 
 * @difftest @020df10c:8=pick:0,1,0x80 @020e36b0:32=int:0:4 cases=300  */
s8 func_0202a724(void)
{
    return func_0202a7f0() != 0 && data_020e36b0 == 2;
}

/* @difftest 
 * @difftest @020df10c:8=pick:0,1,0x80 @020e36b0:32=int:0:4 cases=300 */
s8 func_0202a768(void)
{
    return func_0202a7f0() != 0 && data_020e36b0 == 1;
}

/* @difftest 
 * @difftest @020df10c:8=pick:0,1,0x80 @020e36b0:32=int:0:4 cases=300 */
s8 func_0202a7ac(void)
{
    return func_0202a7f0() != 0 && data_020e36b0 == 0;
}

/* @difftest
 * @difftest @020df10c:8=u8 cases=300 */
s8 func_0202a7f0(void) { return data_020df10c; }

/* @difftest 
 * @difftest @020e36a0:8=u8 cases=300 */
s8 func_0202a800(void) { return data_020e36a0; }

/* @difftest */
void func_0202a810(void) { data_020e36a0 = 1; }

/* @difftest cases=200 
 * @difftest $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O cases=200 */
s32 func_0202a824(void)
{
    if (!g_GameRootValid)
        return 1;
    return func_020772c4(g_GameRoot->unk8);
}

/* @difftest cases=200 
 * @difftest $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O cases=200 */
s32 func_0202a86c(void)
{
    if (!g_GameRootValid)
        return 1;
    return func_020772d8(g_GameRoot->unk8);
}

/* @difftest 
 * @difftest $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
s32 func_0202a8b4(void)
{
    if (!g_GameRootValid)
        return 0;
    return FIELD(s32, g_GameRoot->unk8, 0x320);
}

/* @difftest 
 * @difftest $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
s32 func_0202a8e0(void)
{
    if (!g_GameRootValid)
        return 0;
    return FIELD(s32, g_GameRoot->unk8, 0x31c);
}

/* @difftest u32 
 * @difftest u32 $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
void func_0202a90c(s32 value)
{
    if (g_GameRootValid)
        FIELD(s32, g_GameRoot->unk8, 0x320) = value;
}

/* @difftest u32 
 * @difftest u32 $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
void func_0202a934(s32 value)
{
    if (g_GameRootValid)
        FIELD(s32, g_GameRoot->unk8, 0x31c) = value;
}

/* @difftest 
 * @difftest $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
fx32 func_0202a95c(void)
{
    if (!g_GameRootValid)
        return 0;
    return FIELD(fx32, g_GameRoot->unk8, 0x314);
}

/* @difftest 
 * @difftest $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
fx32 func_0202a988(void)
{
    if (!g_GameRootValid)
        return 0;
    return FIELD(fx32, g_GameRoot->unk8, 0x310);
}

/* stores an integer as fx32 (value << 12)
 * @difftest s32 
 * @difftest s32 $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
void func_0202a9b4(s32 value)
{
    if (g_GameRootValid)
        FIELD(fx32, g_GameRoot->unk8, 0x314) = (fx32)((u32)value << 12);
}

/* stores an integer as fx32 (value << 12)
 * @difftest s32 
 * @difftest s32 $R=zero:16 $O=ptr:0x400:4 $Z=ptr:0x900:4 @020e36a4:8=1 @020e36c8:32=$R @$R+0:32=$Z @$R+8:32=$O */
void func_0202a9e4(s32 value)
{
    if (g_GameRootValid)
        FIELD(fx32, g_GameRoot->unk8, 0x310) = (fx32)((u32)value << 12);
}
