/*
 * The touch-screen pages (part 2): the page where the stylus drags the
 * camera's object (the hero's companion) along its path, and the page shown
 * when a spell is learned.
 * ARM9 main, 0x02034d50 - 0x02035afc (5 functions).
 *
 * Dragging: each stroke (in pixels << 12, data_020e602c previous and
 * data_020e6034 current touch) moves the object one step towards the next
 * point of its path (the object's method 0x38 returns its path state: +0x84
 * path, +0x85 next point, +0x89 last point) when the stroke points that
 * way; within 0x5000 (squared) of the point the drag ends and control
 * returns to the hero. During a fight (data_020df10c) a downward stroke
 * pushes the object instead, until it lands. data_020e6024 allows the drag
 * sound (handle data_020be680) once per stroke; data_020e6028 is the page's
 * path display.
 * The spell page (data_020e6054: the spell, 0xe-0x13) shows the spell's
 * background and its icon sprite data_020e6068.
 *
 * Testing: the set-ups run with the background loader func_0200aad0
 * stubbed (it reads from the card), the drag page's also with the path
 * display's loaders func_02055ad8 and func_020550ac stubbed; the drag runs
 * with a faked pad, hero, camera and object (whose method 0x38 is a bare
 * `bx lr`, so the object is its own path state).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Cnt;       /* BGxCNT value passed by value in a register */

extern Cnt data_020aacac;
extern s32 data_020be680;
extern const s32 data_020be684, data_020be688, data_020be68c, data_020be690, data_020be694;
extern const char data_020be698[];   /* "UI_SpellsMove_BKG.bin" */
extern const s32 data_020be6b0, data_020be6b4, data_020be6b8, data_020be6bc, data_020be6c0,
    data_020be6c4, data_020be6c8, data_020be6cc, data_020be6d0, data_020be6d4, data_020be6d8,
    data_020be6dc, data_020be6e0, data_020be6e4, data_020be6e8, data_020be6ec, data_020be6f0,
    data_020be6f4, data_020be6f8, data_020be6fc;
extern const char data_020be700[], data_020be71c[], data_020be734[], data_020be750[],
    data_020be76c[], data_020be788[];
extern s8 data_020df10c;
extern s8 data_020e3a80;
extern u32 data_020e3a84;
extern s8 data_020e6024;
extern u8 *data_020e6028;
extern s32 data_020e602c[2];
extern s32 data_020e6034[2];
extern s32 data_020e6054;
extern s32 data_020e6058;
extern u8 data_020e6068[];

void *func_02011f44(void);
s32 func_02081d34(void *pad, u32 key);
s32 func_02081cd0(void *pad, u32 key);
u32 func_02081c84(void *pad);
void func_0202f6dc(s32 *out, const u8 *pad);
void func_0203a028(void);
void func_0204378c(u32 screen);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
s32 func_02020440(u32 v);
u32 func_020200fc(u32 h);
void func_020095d8(s32 layer, u32 color, s32 x0, s32 y0, s32 x1, s32 y1);
void func_020150a0(s32 *out, const s32 *a, const s32 *b);
s32 func_0202df68(const s32 *v);
void func_0202ded8(s32 *v, const s32 *s);
void func_0202dea4(s32 *v);
void func_02029790(s32 *out, const s32 *a, const s32 *b);
void func_020297d4(s32 *out, const s32 *v, const s32 *s);
void func_0202a148(s32 *out, u8 *obj);
void func_020088e4(s32 *out, s32 i, s32 k);
void func_020088a8(s32 i, s32 k);
void func_0200afb0(u32 mode, u32 flags);
void func_0200af2c(u32 bits);
void func_0200b168(s32 layer, u32 cnt);
void func_0200af18(s32 layer);
void func_0200b098(s32 layer);
void func_0200b0ac(s32 layer);
u16 *func_0200aad0(s32 layer, u32 file, u32 tile, u32 pal, u32 x, u32 y);
void func_02074c14(void *sprite, u32 a, u32 tile, u32 tile2, u32 b, s32 c);
void *func_0207fe74(u32 size);
u8 *func_0205602c(void *p);
void func_02055bdc(void *p);
void func_02055ad8(u8 *p, u32 a, u32 b);
void func_02055094(u8 *p, s32 a, s32 b, s32 c);
void func_02055890(u8 *p, s32 v);
void func_0205555c(u8 *p);
void func_020550ac(u8 *p, u32 v);
void func_020551b4(u8 *p);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define CAM_TARGET() PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4)

static void Touch(s32 *p)
{
    func_0202f6dc(p, func_02011f44());
}

/* give the camera back to the hero and let it move again */
static void ReleaseHero(void)
{
    void *h = HERO();

    CAM_TARGET() = h;
    U32_AT(HERO(), 0x38) &= ~0x200u;
}

/* back to the spells page */
static void Leave(void)
{
    func_0201b770(1, 0x3f, 0);
    func_02019b08(0xe, 1);
}

/* the object's path state (method 0x38, its address in r1 like the original) */
static u8 *PathState(u8 *o)
{
    u8 *(*m)(void *, void *) = VCALL(o, 0x38, u8 *(*)(void *, void *));

    return m(o, (void *)m);
}

static void Normalize(s32 *v)
{
    s32 n = func_0202df68(v);

    if (n > 1)
        func_0202ded8(v, &n);
    else
        func_0202dea4(v);
}

/* 0x02034d50: the drag page's update
 * @difftest $V=zero:0x40 $U=zero:0x40 @$U+0x38:32=0x020343a8 $O=zero:0x100 @$O+0:32=$U @$O+0x84:8=int:0:2 @$O+0x85:8=int:0:2 @$O+0x89:8=int:0:2 @$O+0x28:32=int:0:0x100000 @$O+0x2c:32=int:0:0x100000 @$O+0x6c:32=int:-0x8000:0x8000 $W=zero:0x10 $C=zero:0xb0 @$C+0xac:32=$W $H=zero:0x100 @$H+0:32=$V @$H+0x44:32=$C @$H+0x38:32=pick:0,0x200 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1,0x10 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e602c:32=int:0:256 @020e6030:32=int:0:192 @020e6034:32=int:0:256 @020e6038:32=int:0:192 @020e3a80:8=pick:0,1 @020e3a84:32=int:55:65 @020e6024:8=pick:0,1 @$W+4:32=$O @020df10c:8=pick:0,0,1 cases=100
 * @difftest $V=zero:0x40 $U=zero:0x40 @$U+0x38:32=0x020343a8 $O=zero:0x100 @$O+0:32=$U @$O+0x84:8=int:0:2 @$O+0x85:8=int:0:2 @$O+0x89:8=int:0:2 @$O+0x28:32=int:0:0x100000 @$O+0x2c:32=int:0:0x100000 @$O+0x6c:32=int:-0x8000:0x8000 $W=zero:0x10 $C=zero:0xb0 @$C+0xac:32=$W $H=zero:0x100 @$H+0:32=$V @$H+0x44:32=$C @$H+0x38:32=pick:0,0x200 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1,0x10 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e602c:32=int:0:256 @020e6030:32=int:0:192 @020e6034:32=int:0:256 @020e6038:32=int:0:192 @020e3a80:8=pick:0,1 @020e3a84:32=int:55:65 @020e6024:8=pick:0,1 @$W+4:32=$H cases=10
 * @difftest @020e3a80:8=pick:0,1 @020e3a84:32=int:55:65 cases=10 */
void func_02034d50(void)
{
    s32 pos[2], d[2], v[2], t1[2], t2[2], p[2], q[2];
    s32 w[3], e[3], s[3], r[3];
    s32 dist, dot;
    u8 *o;
    u32 k;

    if (PTR_AT(data_020deebc, 8) == NULL)
        return;
    o = CAM_TARGET();
    if (CAM_TARGET() == PTR_AT(data_020deebc, 8)) {
        func_0204378c(0);
        U32_AT(HERO(), 0x38) &= ~0x200u;
        Leave();
    } else if (func_02081d34(func_02011f44(), 0xc) != 0) {
        /* a stroke starts */
        data_020e3a84 = 0;
        Touch(t1);
        data_020e602c[0] = t1[0];
        data_020e602c[1] = t1[1];
        data_020e6034[0] = t1[0];
        data_020e6034[1] = t1[1];
        data_020e6024 = 1;
    } else if (func_02081cd0(func_02011f44(), 0xc) != 0
               && func_02081c84(func_02011f44()) == 0) {
        pos[0] = S32_AT(o, 0x28);
        pos[1] = S32_AT(o, 0x2c);
        if (data_020e3a80 == 0)
            data_020e3a80 = 1;
        data_020e3a84 = 0;
        Touch(t1);
        Touch(t2);
        func_020095d8(0, 0x46, data_020e6034[0], data_020e6034[1], t1[0], t2[1]);
        Touch(t1);
        data_020e6034[0] = t1[0];
        data_020e6034[1] = t1[1];
        func_020150a0(d, data_020e6034, data_020e602c);
        func_02003b08(v, d, 0xc);
        d[0] = v[0];
        d[1] = v[1];
        w[0] = v[0];
        w[1] = data_020be684;
        w[2] = v[1];
        Normalize(w);
        if (data_020df10c != 0) {
            /* a downward stroke pushes the object */
            if (d[1] < 0) {
                s32 a[3];
                a[0] = data_020be688;
                a[1] = FxMul(d[1], -0x28000);
                a[2] = data_020be694;
                func_02029790(r, (s32 *)(o + 0x5c), a);
                S32_AT(o, 0x5c) = r[0];
                S32_AT(o, 0x60) = r[1];
                S32_AT(o, 0x64) = r[2];
            }
            func_0202a148(r, o);
            if (r[1] >= 0) {
                ReleaseHero();
                func_0204378c(0);
                Leave();
                func_02020440(0xde);
            }
        } else {
            /* step towards the next point of the path */
            k = PathState(o)[0x85];
            func_020088e4(p, PathState(o)[0x84], k);
            func_020038d4(q, p, 4);
            e[0] = q[0] - pos[0];
            e[1] = data_020be68c;
            e[2] = q[1] - pos[1];
            dist = FxMul(e[2], e[2]) + FxMul(e[0], e[0]) + FxMul(e[1], e[1]);
            Normalize(e);
            func_020297d4(s, e, &data_020be690);
            dot = FxMul(s[2], w[2]) + FxMul(s[0], w[0]) + FxMul(s[1], w[1]);
            if (dist < 0x5000) {
                /* reached the point */
                func_020088a8(PathState(o)[0x84], k);
                ReleaseHero();
                func_0204378c(0);
                Leave();
                func_02020440(0xde);
                if (k != PathState(o)[0x89])
                    PathState(o)[0x85] = k + 1;
                else
                    PathState(o)[0x85] = 0;
                S32_AT(o, 0x28) = pos[0];
                S32_AT(o, 0x2c) = pos[1];
            }
            if (dot > 0) {
                pos[0] += s[0];
                pos[1] += s[2];
                S32_AT(o, 0x28) = pos[0];
                S32_AT(o, 0x2c) = pos[1];
                if (data_020e6024 != 0
                    && (func_020200fc(data_020be680) == 0 || data_020be680 == -1)) {
                    data_020be680 = func_02020440(0x91);
                    data_020e6024 = 0;
                }
            }
        }
        data_020e602c[0] = data_020e6034[0];
        data_020e602c[1] = data_020e6034[1];
    } else if (data_020e3a80 != 0 && ++data_020e3a84 > 0x3c) {
        /* no stroke for a second */
        u8 *t = CAM_TARGET();
        s32 *tp = (s32 *)(t + 0x28);
        S32_AT(t, 0x28) = tp[0];
        S32_AT(t, 0x2c) = tp[1];
        func_0204378c(0);
        ReleaseHero();
        Leave();
    }
    func_0203a028();
}

/* 0x02035438: set up the drag page
 * @difftest @020e6028:32=0 $O=zero:0x100 $W=zero:0x10 @$W+4:32=$O $C=zero:0xb0 @$C+0xac:32=$W $H=zero:0x100 @$H+0x44:32=$C @020deec4:32=$H stub=0x0200aad0,0x02055ad8,0x020550ac cases=5
 * @difftest @020e6028:32=0 stub=0x0200aad0,0x0207fe74:0 cases=5 */
void func_02035438(void)
{
    u8 *p;

    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020aacac.v);
    func_0200af18(0);
    func_0200aad0(0, (u32)data_020be698, 0, 0, 0, 0);
    data_020e3a80 = 0;
    data_020e6024 = 0;
    data_020be680 = -1;
    if (data_020e6028 != NULL) {
        func_02055bdc(data_020e6028);
        data_020e6028 = NULL;
    }
    p = func_0207fe74(0xf0);
    if (p != NULL)
        p = func_0205602c(p);
    data_020e6028 = p;
    if (p != NULL) {
        func_02055ad8(p, 0x124, 0);
        func_02055094(data_020e6028, 0, 0xa000, 0);
        func_02055890(data_020e6028, S32_AT(CAM_TARGET(), 0x44));
        func_0205555c(data_020e6028);
        func_020550ac(data_020e6028, 0x283);
        data_020e6028[0xcb] = 1;
        func_020551b4(data_020e6028);
    }
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
}

/* 0x020355bc: hide the spell page's icon
 * @difftest cases=3 */
void func_020355bc(void)
{
    data_020e6068[0x14] = 0;
}

/* 0x020355d0: the spell page's update: it closes on a touch or after a
 * while (a second for spells 0xe and 0xf, a quarter otherwise)
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1 @020e6058:32=pick:0xe,0xf,0x10,0x14 @020e3a80:8=pick:0,1 @020e3a84:32=int:10:65 cases=60 */
void func_020355d0(void)
{
    s32 st = data_020e6058;
    u32 wait = (st == 0xe || st == 0xf) ? 0x3c : 0xf;

    if (func_02081d34(func_02011f44(), 0xc) != 0) {
        data_020e3a80 = 1;
        return;
    }
    if (++data_020e3a84 > wait || data_020e3a80 != 0) {
        func_0201b770(1, 0x3f, 1);
        func_02019b08(data_020e6058, 1);
    }
}

/* the spell's background and icon */
static void Spell(const char *file, s32 a, s32 b, s32 c, s32 d, u32 tile)
{
    u8 *s = data_020e6068;
    void (*show)(void *, u32, void *);

    func_0200aad0(0, (u32)file, 0, 0, 0, 0);
    S32_AT(s, 4) = a;
    S32_AT(s, 8) = b;
    S32_AT(s, 0xc) = c;
    S32_AT(s, 0x10) = d;
    show = VCALL(s, 0, void (*)(void *, u32, void *));
    show(s, 1, (void *)show);
    s[0x14] = 1;
    func_02074c14(s, 0, tile, tile, 0x20, -1);
}

/* 0x02035680: set up the spell page
 * @difftest @020e6054:32=pick:0xd,0xe,0xf,0x10,0x11,0x12,0x13 stub=0x0200aad0 cases=40 */
void func_02035680(void)
{
    s32 st;

    func_0200afb0(0, 0);
    func_0200af2c(0x1000);
    func_0200b168(0, data_020aacac.v);
    func_0200af18(0);
    st = data_020e6054;
    if (st == 0x10)
        Spell(data_020be700, data_020be6ec, data_020be6e8, data_020be6e4, data_020be6e0, 0x301);
    else if (st == 0xe)
        Spell(data_020be71c, data_020be6dc, data_020be6d0, data_020be6d4, data_020be6fc, 0x306);
    else if (st == 0xf)
        Spell(data_020be734, data_020be6f8, data_020be6f4, data_020be6f0, data_020be6c0, 0x30b);
    else if (st == 0x11)
        Spell(data_020be750, data_020be6bc, data_020be6d8, data_020be6cc, data_020be6c4, 0x310);
    else if (st == 0x12)
        Spell(data_020be76c, data_020be6b0, data_020be6b8, data_020be6c8, data_020be6b4, 0x316);
    else if (st == 0x13)
        func_0200aad0(0, (u32)data_020be788, 0, 0, 0, 0);
    data_020e3a80 = 0;
    data_020e3a84 = 0;
    func_0200b0ac(0);
    func_0200b098(1);
    func_0200b098(2);
    func_0200b098(3);
}
