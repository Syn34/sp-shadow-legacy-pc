/*
 * The touch-screen menu (spells, part 2): its per-frame update.
 * ARM9 main, 0x0202f910 - 0x02030b38 (1 function).
 *
 * The player can tap a spell icon or a side tab, or draw a rune with the
 * stylus. The stroke is collected as {x, y} u16 pairs in the vector at
 * data_020e4150 (0xffff pairs end a stroke) and recognised on release by
 * func_02053a94 (result in data_020be3bc, confidence in data_020e3a98).
 * After a rune, the result lines appear one by one (data_020e3a84 counts
 * frames) and the chosen spell (data_020e6054) is then started.
 *
 * Other state: data_020e3a80 drawing, data_020e3a88 casting allowed,
 * data_020e3a8c / data_020e3a9c a shake, data_020e3a90 showing the
 * result, data_020e3aa4 the last stylus position, data_020e5150 stroke
 * bookkeeping (+0x0c a stroke ended, +0x10 points), data_020e515c "end of
 * stroke written", data_020e6058 the next screen state.
 *
 * Testing: the pad is faked through its pointer data_020f6f30 (touch state
 * at +0x6c, position at +0x72/+0x74); the rune cases make the recognizer
 * func_02053a94 return each rune (stub with a value).
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

extern s32 data_020be2d0;            /* the menu's sound handle */
extern s32 data_020be3bc;            /* recognised rune */
extern u8 data_020e3a80;
extern u32 data_020e3a84;
extern u8 data_020e3a88;
extern u8 data_020e3a8c;
extern u8 data_020e3a90;
extern s32 data_020e3a98;
extern s32 data_020e3a9c[2];
extern s32 data_020e3aa4[2];
extern u8 data_020e3b24[];
extern u8 data_020e3c28[];
extern u8 data_020e3ca0[];
extern PtrVec data_020e4150;
extern u8 data_020e5150[];
extern u8 data_020e515c;
extern s32 data_020e6054;
extern s32 data_020e6058;
extern s8 data_020df10c;

void *func_02011f44(void);
u8 *func_02011f34(void);
s32 func_02011bc8(void);
s32 func_02081d34(void *pad, u32 key);
s32 func_02081cd0(void *pad, u32 key);
u32 func_02081c84(void *pad);
void func_020748e0(void *sprite, const s32 *pos);
void func_0201b770(u32 a, u32 b, u32 c);
void func_02019b08(u32 state, u32 screen);
void func_02016750(u8 *self, s32 mode);
s32 func_0204a80c(u32 counter);
s32 func_0204a868(u32 counter);
s32 func_0204ab08(u32 spell);
void func_020645e4(void *cam, u32 v);
const char *func_02028a20(u32 id);
void func_02076b44(void *obj, const char *text, u32 fbits);
void func_0204378c(u32 screen);
void func_020437c4(u32 screen);
void func_02004078(s32 layer);
void func_020095d8(s32 layer, u32 color, s32 x0, s32 y0, s32 x1, s32 y1);
s32 func_02053a94(PtrVec *strokes, s32 *score);
s32 func_02013cf8(void *obj);
void func_02064698(void);
void func_0201756c(void *self);
s32 func_02020440(u32 v);
void func_0203a028(void);
void func_0202f330(const u32 *pos);
void func_0202f3dc(void);
void func_0202f6dc(s32 *out, const u8 *pad);

#define HERO() ((u8 *)PTR_AT(data_020deebc, 8))
#define ICON(i) (data_020e3b24 + (i) * 0x34)
#define F_40960 0x47200000u           /* 40960.0f */
#define F_12288 0x46400000u           /* 12288.0f */

static void Touch(s32 *p)
{
    func_0202f6dc(p, func_02011f44());
}

/* give the camera back to the hero and let it move again */
static void ReleaseHero(void)
{
    void *h = HERO();

    PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = h;
    U32_AT(HERO(), 0x38) &= ~0x200u;
}

/* call hero method `off` with the method's address in r1, like the original */
static void HeroMethod(u32 off)
{
    u8 *h = HERO();
    void (*m)(void *, void *) = VCALL(h, off, void (*)(void *, void *));

    m(h, (void *)m);
}

static void HeroMethod3(void)
{
    u8 *h = HERO();
    void (*m)(void *, u32, void *) = VCALL(h, 0xc, void (*)(void *, u32, void *));

    m(h, 0, (void *)m);
}

/* the message line on the top screen */
static void Message(u32 id, u32 fbits)
{
    const char *s = func_02028a20(id);
    u8 *hud = func_02011f34();

    if (func_02011bc8() != 0)
        return;
    func_02076b44(hud + 0x768, s, fbits);
    func_02076b44(hud + 0x7c4, NULL, 0);
}

static void StartRune(u32 spell, u32 msg, s32 extra)
{
    func_020437c4(0);
    if (extra)
        func_02004078(0);
    HeroMethod3();
    HeroMethod(0x38);
    func_02064698();
    data_020e6054 = spell;
    data_020e6058 = 0xd;
    if (func_0204a868(spell) == 0) {
        data_020e6054 = 0;
        return;
    }
    Message(msg, F_40960);
}

/* a rune that costs mana (func_0204ab08): cast it or say there is not enough */
static void CastRune(u32 spell, u32 msg)
{
    s32 mana = func_02013cf8(data_020deebc);

    if (mana >= func_0204ab08(spell)) {
        StartRune(spell, msg, 0);
    } else {
        data_020e6054 = 0;
        Message(0xf6, F_12288);
    }
}

static void PickSpell(u32 i, u32 spell)
{
    data_020e6054 = spell;
    data_020e6058 = 0xe;
    func_0201b770(1, 0x3f, 1);
    func_02019b08(0x10, 1);
    U32_AT(ICON(i), 0x1c) &= 0xff000000u;
    U32_AT(ICON(i), 0x1c) |= 1;
}

static s32 IconTouched(u32 i)
{
    return (U32_AT(ICON(i), 0x1c) & 4) != 0 && (s8)ICON(i)[0x14] != 0;
}

static void StartCamera(u32 a, u32 b, u32 c)
{
    s32 n = func_0204a80c(0xc);

    if (n == 1)
        func_020645e4(PTR_AT(func_02016f24(data_020deebc), 0xac), a);
    else if (n == 2)
        func_020645e4(PTR_AT(func_02016f24(data_020deebc), 0xac), b);
    else if (n == 3)
        func_020645e4(PTR_AT(func_02016f24(data_020deebc), 0xac), c);
}

static void Leave(u32 mode)
{
    func_0201b770(1, 0x3f, 1);
    func_02019b08(0xd, 1);
    func_02016750((u8 *)data_020deebc, mode);
}

/* the rune's result is shown: reveal the result lines one per frame, then
 * act on the chosen spell */
static void ShowResult(void)
{
    u32 n = data_020e3a84;
    s32 saved;

    if (n < 10) {
        float f = (float)(n << 12);
        s32 x, thr;
        if (n != 0)
            f = 0.5f + f;
        else
            f = f - 0.5f;
        x = (s32)f;
        thr = (s32)(((s64)x * 0x19a + 0x800) >> 12);
        if (data_020e3a98 >= thr) {
            u8 *t = PTR_AT(data_020e3ca0 + n * 0x78, 0x10);
            if (t != NULL)
                U32_AT(t, 4) |= 8;
        }
    } else if (data_020e3a98 >= 0xf5c) {
        u8 *t = PTR_AT(data_020e3c28, 0x10);
        if (t != NULL)
            U32_AT(t, 4) |= 8;
    }
    data_020e3a84++;
    if (data_020e3a84 <= 0xf)
        return;
    saved = data_020df0fc;
    switch (data_020e6054) {
    case 0xe:
        Leave(0x80);
        break;
    case 0xf:
        if (func_0204a868(0xf) == 0) {
            data_020e3a90 = 0;
            ReleaseHero();
            func_0202f3dc();
        } else {
            const char *s;
            u8 *hud;
            data_020df0fc = 0;
            s = func_02028a20(0xff);
            hud = func_02011f34();
            if (func_02011bc8() == 0) {
                func_02076b44(hud + 0x768, s, F_40960);
                func_02076b44(hud + 0x7c4, NULL, 0);
            }
            data_020df0fc = saved;
            func_0201b770(1, 0x3f, 1);
            func_02019b08(0x12, 1);
        }
        break;
    case 0x10:
        Leave(0x200);
        StartCamera(0x1e, 0x23, 0x24);
        break;
    case 0x11:
        Leave(0x400);
        StartCamera(0x1f, 0x25, 0x26);
        break;
    case 0x12:
        Leave(0x800);
        break;
    default:
        func_0204378c(0);
        data_020e3a90 = 0;
        ReleaseHero();
        func_0202f3dc();
        break;
    }
}

/* the stylus went down: side tabs, spell icons */
static void Tap(void)
{
    s32 p[2], q[2], r[2], t[5][2];
    s8 shown;
    u32 i;

    Touch(p);
    if ((u32)p[0] > 0xde) {
        Touch(q);
        func_0202f330((u32 *)q);
    }
    Touch(r);
    shown = (s8)ICON(0)[0x14];
    data_020e3aa4[0] = r[0];
    data_020e3aa4[1] = r[1];
    if (shown) {
        Touch(t[0]);
        func_020748e0(ICON(0), t[0]);
    }
    for (i = 1; i < 5; i++) {
        if ((s8)ICON(i)[0x14]) {
            Touch(t[i]);
            func_020748e0(ICON(i), t[i]);
        }
    }
    if (IconTouched(0))
        PickSpell(0, 0x10);
    else if (IconTouched(1))
        PickSpell(1, 0xe);
    else if (IconTouched(2))
        PickSpell(2, 0xf);
    else if (IconTouched(3))
        PickSpell(3, 0x11);
    else if (IconTouched(4))
        PickSpell(4, 0x12);
}

static void PushPoint(s32 x, s32 y)
{
    PtrVec *v = &data_020e4150;

    ((u16 *)v->items)[v->count * 2] = x;
    ((u16 *)v->items)[v->count * 2 + 1] = y;
    v->count++;
}

/* the stylus is held: draw */
static void Draw(void)
{
    s32 p[2], q[2], r[2], t1[2], t2[2], t3[2];

    Touch(p);
    if ((u32)p[0] >= 0xde)
        return;
    Touch(q);
    if (q[1] <= 0x28)
        return;
    if ((s8)data_020e3a80 == 0) {
        s8 can = (s8)data_020e3a88;
        data_020e3a80 = 1;
        if (can != 0) {
            if (data_020df10c == 0) {
                func_020437c4(0);
                HeroMethod3();
            }
            U32_AT(HERO(), 0x38) |= 0x200;
            {
                void *h = HERO();
                PTR_AT(PTR_AT(func_02016f24(data_020deebc), 0xac), 4) = h;
            }
        }
        if (data_020be2d0 == -1)
            data_020be2d0 = func_02020440(0x53);
    }
    data_020e3a84 = 0;
    Touch(r);
    data_020e5150[0xc] = 0;
    if (!((u32)data_020e4150.count >= (u32)data_020e4150.cap)) {
        U32_AT(data_020e5150, 0x10)++;
        PushPoint(r[0], r[1]);
    }
    Touch(t1);
    Touch(t2);
    func_020095d8(0, 0x46, data_020e3aa4[0], data_020e3aa4[1], t1[0], t2[1]);
    Touch(t3);
    data_020e3aa4[0] = t3[0];
    data_020e3aa4[1] = t3[1];
}

/* the stylus is up: after a stroke, end it; after a pause, read the rune */
static void Release(void)
{
    if ((s8)data_020e3a80 == 0)
        return;
    data_020e3a84++;
    if ((s8)data_020e515c == 0) {
        data_020e515c = 1;
        if ((u32)data_020e4150.count < (u32)data_020e4150.cap)
            PushPoint(0xffff, 0xffff);
    }
    if (data_020e3a84 <= 10)
        return;
    data_020e3a84 = 0;
    data_020be3bc = func_02053a94(&data_020e4150, &data_020e3a98);
    data_020e3a90 = 1;
    if ((s8)data_020e3a88 == 0)
        return;
    if (data_020e3a98 <= 0xccd)
        return;
    if (!((U32_AT(HERO(), 0x38) >> 23) & 1))
        data_020be3bc = -1;
    switch (data_020be3bc) {
    case 0:
        CastRune(0x10, 0xfb);
        break;
    case 1:
        if (data_020df10c != 0)
            break;
        if (func_0204a868(0x13) == 0)
            break;
        func_0204378c(0);
        func_0201756c(data_020deebc);
        data_020e3a90 = 0;
        ReleaseHero();
        func_02020440(0xd8);
        func_0201b770(1, 0x3f, 1);
        func_02019b08(0xc, 1);
        break;
    case 2:
        StartRune(0xe, 0xfe, 1);
        break;
    case 3:
        CastRune(0x11, 0xfc);
        break;
    case 4:
        if (data_020df10c != 0)
            break;
        CastRune(0x12, 0xfd);
        break;
    case 5: {
        s32 mana;
        if (data_020df10c != 0)
            break;
        mana = func_02013cf8(data_020deebc);
        if (mana >= func_0204ab08(0xf)) {
            data_020e6054 = 0xf;
            if (func_0204a868(0xf) == 0)
                data_020e6054 = 0;
        } else {
            data_020e6054 = 0;
            Message(0xf6, F_12288);
        }
        break;
    }
    }
}

/* 0x0202f910: per-frame update of the spells menu
 * @difftest $V=zero:0x40 @$V+0xc:32=0x02070a18 @$V+0x38:32=0x02070a18 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=pick:0,0x800000 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @$K+0x6c:32=1 @020e3a90:8=0 @020e3b38:8=pick:0,1 @020e3b40:32=pick:0,4 @020e3b6c:8=pick:0,1 @020e3b74:32=pick:0,4 @020e3ba0:8=pick:0,1 @020e3ba8:32=pick:0,4 @020e3bd4:8=pick:0,1 @020e3bdc:32=pick:0,4 @020e3c08:8=pick:0,1 @020e3c10:32=pick:0,4 cases=60
 * @difftest $V=zero:0x40 @$V+0xc:32=0x02070a18 @$V+0x38:32=0x02070a18 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=0x800000 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e3a90:8=0 @020e3a80:8=1 @020e3a84:32=int:11:14 @020e515c:8=1 @020e3a88:8=1 @020e3a98:32=pick:0x1000,0x800 @020df10c:8=pick:0,1 stub=0x02053a94:0 cases=20
 * @difftest $V=zero:0x40 @$V+0xc:32=0x02070a18 @$V+0x38:32=0x02070a18 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=0x800000 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e3a90:8=0 @020e3a80:8=1 @020e3a84:32=int:11:14 @020e515c:8=1 @020e3a88:8=1 @020e3a98:32=pick:0x1000,0x800 @020df10c:8=pick:0,1 stub=0x02053a94:1 cases=20
 * @difftest $V=zero:0x40 @$V+0xc:32=0x02070a18 @$V+0x38:32=0x02070a18 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=0x800000 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e3a90:8=0 @020e3a80:8=1 @020e3a84:32=int:11:14 @020e515c:8=1 @020e3a88:8=1 @020e3a98:32=pick:0x1000,0x800 @020df10c:8=pick:0,1 stub=0x02053a94:2 cases=20
 * @difftest $V=zero:0x40 @$V+0xc:32=0x02070a18 @$V+0x38:32=0x02070a18 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=0x800000 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e3a90:8=0 @020e3a80:8=1 @020e3a84:32=int:11:14 @020e515c:8=1 @020e3a88:8=1 @020e3a98:32=pick:0x1000,0x800 @020df10c:8=pick:0,1 stub=0x02053a94:3 cases=20
 * @difftest $V=zero:0x40 @$V+0xc:32=0x02070a18 @$V+0x38:32=0x02070a18 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=0x800000 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e3a90:8=0 @020e3a80:8=1 @020e3a84:32=int:11:14 @020e515c:8=1 @020e3a88:8=1 @020e3a98:32=pick:0x1000,0x800 @020df10c:8=pick:0,1 stub=0x02053a94:4 cases=20
 * @difftest $V=zero:0x40 @$V+0xc:32=0x02070a18 @$V+0x38:32=0x02070a18 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=0x800000 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e3a90:8=0 @020e3a80:8=1 @020e3a84:32=int:11:14 @020e515c:8=1 @020e3a88:8=1 @020e3a98:32=pick:0x1000,0x800 @020df10c:8=pick:0,1 stub=0x02053a94:5 cases=20
 * @difftest @020e3a90:8=1 @020e3a84:32=int:0:20 @020e3a98:32=int:0:0x1400 @020e6054:32=pick:0,0xe,0xf,0x10,0x11,0x12 cases=60
 * @difftest $V=zero:0x40 @$V+0xc:32=0x02070a18 @$V+0x38:32=0x02070a18 $H=zero:0x100 @$H+0:32=$V @$H+0x38:32=pick:0,0x800000,0x400 @020deec4:32=$H $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1,0x10 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e3a90:8=0 @020e3a80:8=pick:0,1 @020e3a84:32=int:0:14 @020e3a88:8=pick:0,1 @020e515c:8=pick:0,1 @020e3a8c:8=pick:0,1 cases=100
 * @difftest $K=zero:0x80 @020f6f30:32=$K @$K+0x6c:32=pick:0,1,0x10 @$K+0x72:16=int:0:256 @$K+0x74:16=int:0:192 @020e3a90:8=0 @020e3a80:8=pick:0,1 @020e3a84:32=int:0:14 @020e3a88:8=pick:0,1 cases=60 */
void func_0202f910(void)
{
    u8 *h = HERO();

    if (h == NULL)
        return;
    if ((s8)((U32_AT(h, 0x38) >> 10) & 1) != 0) {
        data_020e3a90 = 0;
        func_0202f3dc();
        return;
    }
    if ((s8)data_020e3a90 != 0) {
        ShowResult();
        func_0203a028();
        return;
    }
    if (func_02011bc8() != 0) {
        s32 p[2], q[2];
        if (data_020be2d0 != -1)
            func_0202f3dc();
        if (func_02081d34(func_02011f44(), 0xc) == 0)
            return;
        Touch(p);
        if ((u32)p[0] <= 0xde)
            return;
        Touch(q);
        func_0202f330((u32 *)q);
        return;
    }
    if (func_02081d34(func_02011f44(), 0xc) != 0)
        Tap();
    else if (func_02081cd0(func_02011f44(), 0xc) != 0 && func_02081c84(func_02011f44()) == 0
             && ((U32_AT(HERO(), 0x38) >> 23) & 1))
        Draw();
    else
        Release();

    if ((s8)data_020e3a8c != 0) {
        if (data_020e3a9c[0] > 0)
            data_020e3a9c[0]--;
        else if (data_020e3a9c[0] < 0)
            data_020e3a9c[0]++;
        if (data_020e3a9c[1] > 2)
            data_020e3a9c[1]--;
        else if (data_020e3a9c[1] < 0)
            data_020e3a9c[1] += 2;
        else if (data_020e3a9c[1] == 0)
            data_020e3a9c[1] += 2;
    }
    func_0203a028();
}
