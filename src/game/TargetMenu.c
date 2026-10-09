/*
 * The spell target page (touch screen): the actors the current spell can
 * be cast on get a marker at their projected screen position; touching
 * one casts the spell on it (the camera follows it) and returns to the
 * spells page.
 * ARM9 main, 0x02037fc0 - 0x02038b54 (8 functions).
 *
 * The spell is the hero holder's +0x18 (0x80, 0x200, 0x400 or 0x800).
 * Markers are up to fifteen 0x78-byte text entries from data_020e7198
 * (count data_020e7184; the text actor's +0xa0 is the target actor);
 * data_020e7188 counts the page's frames (it closes after 1.5 seconds
 * without a touch). The game's actor list is func_020062c0() + 4
 * ({?, items, count}, func_02038b1c / func_02038b28).
 *
 * Testing: the set-up runs with the numbered-file background loader
 * func_0200a8bc and the marker loader func_020107a8 stubbed (they read
 * from the card); the update with a faked pad, hero and markers (whose
 * target's method 0x38 is a bare `bx lr`) and sound effects stubbed.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

extern Cnt data_020aacac;
extern const u8 data_020b676c[];     /* marker text settings */
extern u8 data_020bcb88[], data_020be948[];   /* type infos for the cast */
extern const s32 data_020be92c, data_020be930, data_020be934;
extern s8 data_020df10c;
extern u32 data_020e7184;
extern u32 data_020e7188;
extern u8 data_020e7198[];

void *func_02011f44(void);
u8 *func_02011f34(void);
s32 func_02011bc8(void);
s32 func_02081d34(void *pad, u32 key);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_0203a028(void);
void *func_020062c0(void);
void *func_020062e0(void);
void func_0204378c(u32 screen);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
s32 func_02020440(u32 v);
void func_02020144(void);
void func_0202028c(u32 v);
s32 func_0204a80c(u32 counter);
s32 func_0204ab08(u32 spell);
void func_02013d64(void *obj, s32 v);
s32 func_02013cac(void *obj);
void func_02029f3c(void);
void func_02076b44(void *obj, const char *text, u32 fbits);
void func_020108f4(void *text, const u8 *s);
void func_020107a8(void *b);
void func_02005b98(s32 *out, const void *src);
void func_020829ec(s32 *out, const void *cam, const s32 *pos);
void func_02082ce8(void *cam);
void *func_020a6efc(void *p, void *from, void *to, s32 v);   /* __dynamic_cast */
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200a8bc(s32 layer, s32 handle, u32 tile, u32 pal, u32 x, u32 y);
void func_020a6d58(void *arr, u32 n, u32 size, void *(*dtor)(void *));   /* __destroy_arr */
void *func_02072130(void *p);

void func_02037fc0(u8 *o, s32 range);
void func_02038100(s32 *pos, u8 *target);
void *func_02038b1c(void *list, u32 i);
u32 func_02038b28(void *list);

#define FN(f) ((void *(*)(void *))(void (*)(void))(f))
#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define CAM_TARGET() PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4)
#define MARKER(i) (data_020e7198 + (i) * 0x78)
#define TEXT(b) PTR_AT(b, 0x10)
#define ACTORS() ((u8 *)func_020062c0() + 4)

/* the marker's target (the original reads a zeroed stack slot without a text) */
static u8 *Target(u8 *e)
{
    return TEXT(e) != NULL ? PTR_AT(TEXT(e), 0xa0) : NULL;
}

static s32 In(const s32 *p, const u8 *b)
{
    s32 in = 0;

    if (p[0] > S32_AT(b, 4) && p[0] < S32_AT(b, 0xc) && p[1] < S32_AT(b, 8) && p[1] > S32_AT(b, 0))
        in = 1;
    return in;
}

/* the actor's control state (method 0x38, called with its address in r1
 * like the original) */
static u8 *State(u8 *a)
{
    u8 *(*m)(void *, void *) = VCALL(a, 0x38, u8 *(*)(void *, void *));

    return m(a, (void *)m);
}

/* the same, called with the hero holder in r1 and the address in r2 */
static u8 *State2(u8 *a)
{
    u8 *(*m)(void *, void *, void *) = VCALL(a, 0x38, u8 *(*)(void *, void *, void *));

    return m(a, data_020deebc, (void *)m);
}

/* hand the camera back to the hero */
static void ReleaseHero(void)
{
    void *h = HERO();

    CAM_TARGET() = h;
    U32_AT(HERO(), 0x38) &= ~0x200u;
}

/* 0x02037fc0: actors of kind 4 within `range` (squared) of `o` wake up and
 * follow the hero
 * @difftest ptr:0x40 int:0:0x10000000 cases=20 */
void func_02037fc0(u8 *o, s32 range)
{
    u32 i = 0;

    if (func_02038b28(ACTORS()) == 0)
        return;
    do {
        u8 *a = func_02038b1c(ACTORS(), i);
        if (S32_AT(a, 0xc) == 4) {
            s32 dx = S32_AT(a, 0x28) - S32_AT(o, 0x28);
            s32 dz = S32_AT(a, 0x2c) - S32_AT(o, 0x2c);
            s32 dy = data_020be934;
            if (FxMul(dz, dz) + FxMul(dx, dx) + FxMul(dy, dy) <= range) {
                void *h;
                if ((s8)BITS(U32_AT(a, 0x38), 21, 1) != 0)
                    U32_AT(a, 0x38) |= 0x400000;
                else
                    U32_AT(a, 0x38) |= 0x400;
                h = HERO();
                PTR_AT(State2(a), 8) = h;
            }
        }
        i++;
    } while (i < func_02038b28(ACTORS()));
}

/* 0x02038100: add a marker at `pos` for `target`
 * @difftest ptr:8:4 ptr:0x100:4 @020e7184:32=pick:0,3,14,15 stub=0x020107a8 cases=20 */
void func_02038100(s32 *pos, u8 *target)
{
    u32 n = data_020e7184;
    s32 w, h, right, bottom;
    u8 *e;

    if (n + 1 >= 0xf)
        return;
    w = data_020be930;
    h = data_020be92c;
    e = MARKER(n);
    right = pos[0] + w;
    bottom = pos[1] + h;
    func_020036e8((s32 *)e, &pos[1], &pos[0], &bottom, &right);
    func_020108f4(e + 0x14, data_020b676c);
    func_020107a8(e);
    if (TEXT(MARKER(data_020e7184)) != NULL)
        func_02003cbc(TEXT(MARKER(data_020e7184)), 0);
    if (TEXT(MARKER(data_020e7184)) != NULL)
        U32_AT(TEXT(MARKER(data_020e7184)), 4) |= 8;
    if (TEXT(MARKER(data_020e7184)) != NULL)
        PTR_AT(TEXT(MARKER(data_020e7184)), 0xa0) = target;
    data_020e7184++;
}

/* 0x02038238: leave the page
 * @difftest cases=5 */
void func_02038238(void)
{
    u8 *x = func_02011f34();
    s32 i;

    if (func_02011bc8() == 0) {
        func_02076b44(x + 0x7c4, NULL, 0);
        func_02076b44(x + 0x768, NULL, 0);
    }
    func_02020144();
    for (i = 0; i < 0xf; i++) {
        u8 *e = MARKER(i);
        if (TEXT(e) != NULL) {
            func_020044a0(TEXT(e));
            TEXT(e) = NULL;
        }
    }
}

/* cast spell 0x200 / 0x400 on the target: wake the actors around it */
static void Wake(u8 *e)
{
    if (func_0204a80c(0xc) == 2)
        func_02037fc0(Target(e), 0x9c4000);
    else if (func_0204a80c(0xc) == 3)
        func_02037fc0(Target(e), 0x2710000);
}

/* 0x020382c0: the target page's update
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 $U=zero:0x40 @$U+0x38:32=0x020343a8 $T=zero:0x100 @$T+0:32=$U @$T+8:32=pick:0,0x10d @$T+0xc:32=pick:4,0xc @$T+0x38:32=pick:0,0x200000 $X=zero:0xc0 @$X+0xa0:32=$T $W=zero:0x10 $C=zero:0xb0 @$C+0xac:32=$W $V=zero:0x40 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=pick:0,0x200 @$H+0x44:32=$C @020deec4:32=$H @020deed4:32=pick:0x80,0x200,0x400,0x800 @020e7184:32=pick:0,1 @020e7188:32=int:80:100 @020e7198:32=int:0:96 @020e719c:32=int:0:128 @020e71a0:32=int:96:192 @020e71a4:32=int:128:256 @020e71a8:32=$X stub=0x02020440 cases=150 */
void func_020382c0(void)
{
    u8 *holder = (u8 *)data_020deebc;
    s32 p[2];
    u32 i;

    if ((s8)BITS(U32_AT(HERO(), 0x38), 9, 1) == 0) {
        void *h;
        func_0204378c(0);
        h = HERO();
        CAM_TARGET() = h;
        func_0201b770(1, 0x3f, 0);
        func_02019b08(0xe, 1);
    }
    if (func_02081d34(func_02011f44(), 0xc) != 0) {
        for (i = 0; i < data_020e7184; i++) {
            u8 *e = MARKER(i);
            u32 scr;
            void *h;
            func_0202f6dc(p, func_02011f44());
            if (!In(p, e))
                continue;
            scr = data_020df0fc;
            data_020df0fc = 0;
            func_02020440(0x51);
            if (S32_AT(Target(e), 8) == 0x10d) {
                if (S32_AT(holder, 0x18) == 0x400)
                    U32_AT(Target(e), 0x38) |= 0x400;
            } else {
                s32 k;
                if (S32_AT(holder, 0x18) != 0x800) {
                    if ((s8)BITS(U32_AT(Target(e), 0x38), 21, 1) != 0)
                        U32_AT(Target(e), 0x38) |= 0x400000;
                    else
                        U32_AT(Target(e), 0x38) |= 0x400;
                }
                k = S32_AT(holder, 0x18);
                if (k == 0x200 || k == 0x400)
                    Wake(e);
            }
            h = PTR_AT(holder, 8);
            PTR_AT(State(Target(e)), 8) = h;
            h = Target(e);
            CAM_TARGET() = h;
            U32_AT(HERO(), 0x38) &= ~0x200u;
            data_020df0fc = scr;
            func_0201b770(1, 0x3f, 0);
            if (S32_AT(holder, 0x18) == 0x80) {
                func_02013d64(holder, -func_0204ab08(0xe));
                if (data_020df10c != 0)
                    func_0204378c(0);
                if (S32_AT(Target(e), 0xc) == 4) {
                    ReleaseHero();
                    func_0201b770(1, 0x3f, 0);
                    func_02019b08(0xe, 1);
                } else {
                    func_02019b08(0xb, 1);
                }
            } else {
                func_0204378c(0);
                func_02019b08(0xe, 1);
            }
        }
    } else if (data_020e7188 > 0x5a) {
        func_0204378c(0);
        ReleaseHero();
        func_0201b770(1, 0x3f, 0);
        func_02019b08(0xe, 1);
    }
    data_020e7188++;
    func_0203a028();
}

/* the screen position of 3D point `src` (an actor's position source) */
static void Project(s32 *pos, const void *src)
{
    u32 cam[0x2c0 / 4];
    s32 v[3], s[2];

    func_020040f4((u8 *)cam, func_020062e0());
    func_02005b98(v, src);
    func_020829ec(s, cam, v);
    pos[1] = s[1];
    pos[0] = s[0];
    func_02082ce8(cam);
}

/* 0x020387d0: set up the target page: a marker on every actor the
 * current spell can be cast on
 * @difftest stub=0x0200a8bc,0x020107a8 cases=5 */
void func_020387d0(void)
{
    u8 *holder = (u8 *)data_020deebc;
    s32 pos[2];
    u32 i;

    pos[0] = 0;
    pos[1] = 0;
    data_020e7188 = 0;
    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020aacac.v);
    func_0200a8bc(0, 0x178, 0, 0, 0, 0);
    data_020e7184 = 0;
    if (data_020df10c != 0) {
        void *h;
        func_02029f3c();
        h = HERO();
        CAM_TARGET() = h;
    } else if (func_02013cac(holder) != 0) {
        s32 spell = S32_AT(holder, 0x18);
        for (i = 0; i < func_02038b28(ACTORS()); i++) {
            u8 *a = func_02038b1c(ACTORS(), i);
            if (spell == 0x200 || spell == 0x400 || spell == 0x800) {
                if (S32_AT(a, 0xc) != 4)
                    continue;
            } else if (spell == 0x80) {
                s32 t = S32_AT(a, 0xc);
                if (t != 0xc && t != 0x11)
                    continue;
                if ((s8)BITS(S32_AT(State(a), 0x24), 7, 1) == 0)
                    continue;
                if ((s8)BITS(U32_AT(a, 0x38), 23, 1) == 1)
                    continue;
            }
            if (S32_AT(a, 0xc) == 0xc) {
                u8 *o = a;
                if (a != NULL)
                    o = func_020a6efc(a, data_020bcb88, data_020be948, -1);
                Project(pos, o);
            } else if (PTR_AT(a, 0x44) != NULL) {
                Project(pos, PTR_AT(a, 0x44));
            }
            pos[1] = 0xc0 - pos[1];
            pos[1] -= 0xf;
            pos[0] -= 0x15;
            func_02038100(pos, a);
        }
    }
    if (data_020e7184 == 0) {
        func_0204378c(0);
        ReleaseHero();
        func_0201b770(1, 0x3f, 1);
        func_02019b08(0xe, 1);
    } else {
        func_02020440(0x50);
        func_0202028c(0x16);
    }
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
}

/* 0x02038b1c: item i of a list
 * @difftest $A=ptr:0x40:4 $L=zero:0x10 @$L+4:32=$A $L int:0:15 */
void *func_02038b1c(void *list, u32 i)
{
    return ((void **)PTR_AT(list, 4))[i];
}

/* 0x02038b28: a list's count
 * @difftest ptr:0x10:4 */
u32 func_02038b28(void *list)
{
    return U32_AT(list, 8);
}

/* 0x02038b30: destroy the markers
 * @difftest cases=3 */
void func_02038b30(void)
{
    func_020a6d58(data_020e7198, 0xf, 0x78, FN(func_02072130));
}
