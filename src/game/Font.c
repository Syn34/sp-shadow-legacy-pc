/*
 * Text drawing with the bitmap fonts into BG tiles.
 * ARM9 main, 0x02012b14 - 0x02013b78 (22 functions).
 *
 * Each screen has a 0x3c-byte text state at data_020debf8 + screen * 0x3c:
 *   +0x00 font, +0x04 second font record, +0x08 ink colour, +0x0c paper
 *   colour (-1: draw over the tiles already there), +0x10 palette bits of
 *   the map entries, +0x14 line height, +0x18/+0x1c BG map, +0x20 BG
 *   characters, +0x24 bytes per tile, +0x28 8-bit tiles, +0x34 text drawn,
 *   +0x38 colour mode (0 font colours, 1 flat).
 * Fonts are 0x28-byte records at data_020dec70 + screen * 0xa0 + font * 0x28:
 *   +0 u8 glyph height, +2/+4 u16 first/last character, +6 u8 line height,
 *   +8 u16 bitmap offsets, +0xc u8 widths, +0x10 4-bit bitmaps (column by
 *   column).
 * Text may contain '/x' control codes (see Dialog2.c) and '@' escapes.
 * Drawing functions take and return the next free tile number.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

typedef struct { u16 v; } Rgb16;     /* BGxCNT value, passed by value in r0 */

extern u32 data_020b564c[];          /* {font file, line height} per font */
extern const char data_020bd068[];
extern u8 data_020debf8[];           /* text state, 0x3c bytes per screen */
extern u8 data_020dec70[];           /* fonts, 0xa0 bytes per screen */

void func_0201817c(u32 screen);
void func_0207fe24(void);
void func_0207fe28(const char *file, s32 line);

/* this file's own functions, called before their definition */
u32 func_02012b14(const u8 *font, u32 c);
u16 *func_02012b54(u16 *map, s32 x, s32 y);
void func_02012b78(const u32 *buf, u32 rows, u16 *map, u8 *chars, u32 tile);
void func_02012c40(u32 *buf, u32 rows, const u16 *map, const u8 *chars);
void func_02012cf8(u8 *tile);
void func_02012dd8(u32 *buf, u32 rows, u32 colour);
void func_02012e58(u32 font, u32 sub);
u32 func_02012f28(u32 tile, s32 x, s32 y, const u8 *str);
u32 func_0201341c(u32 tile, s32 x, s32 y, const u8 *str, u32 align);
const u8 *func_020137b8(const u8 *str, s32 maxw);

#define T(off) U32_AT(data_020debf8 + data_020df0fc * 0x3c, off)
#define FONT() ((u8 *)PTR_AT(data_020debf8 + data_020df0fc * 0x3c, 0))

/* 0x02012b14: width of a character
 * @difftest $F=ptr:0x14:4 $W=ptr:0x40 @$F+2:16=pick:0x20,0x41 @$F+4:16=pick:0x30,0x5a,0x7f @$F+0xc:32=$W $F int:0:0x90 */
u32 func_02012b14(const u8 *font, u32 c)
{
    s32 g;
    if (c == 0)
        return 0;
    g = c - U16_AT(font, 2);
    if (g >= 0 && g <= U16_AT(font, 4) - U16_AT(font, 2))
        return ((u8 *)PTR_AT(font, 0xc))[g];
    return ((u8 *)PTR_AT(font, 0xc))[1];
}

/* 0x02012b54: map entry under a pixel position
 * @difftest u32 int:-40:300 int:-40:200 */
u16 *func_02012b54(u16 *map, s32 x, s32 y)
{
    return map + (x / 8 + (y / 8) * 32);
}

/* 0x02012b78: store `rows` tiles of the buffer as tiles `tile`.. and put them
 * in a map column
 * @difftest ptr:0x200:4 int:0:4 ptr:0x100:2 ptr:0x800:4 int:0:8 */
void func_02012b78(const u32 *buf, u32 rows, u16 *map, u8 *chars, u32 tile)
{
    for (; rows != 0; rows--, map += 0x20, tile++) {
        u32 size = T(0x24), n;
        u32 *dst = (u32 *)(chars + tile * size);
        for (n = size >> 2; n != 0; n--)
            *dst++ = *buf++;
        if (T(0x28) == 1)
            *map = tile;
        else if (T(0xc) == (u32)-1)
            *map = (*map & 0xf000) | tile;
        else
            *map = tile | T(0x10);
    }
}

/* 0x02012c40: copy the tiles of a map column into the buffer
 * @difftest $M=ptr:0x100:2 @$M+0:16=pick:0,0x401,7 @$M+0x40:16=pick:0x402,3 @$M+0x80:16=pick:5,0x400 ptr:0x200:4 int:0:3 $M ptr:0x800:4 */
void func_02012c40(u32 *buf, u32 rows, const u16 *map, const u8 *chars)
{
    for (; rows != 0; rows--, map += 0x20) {
        u32 size = T(0x24), n;
        const u32 *src = (const u32 *)(chars + (*map & 0x3ff) * size);
        for (n = size >> 2; n != 0; n--)
            *buf++ = *src++;
        if (*map & 0x400)
            func_02012cf8((u8 *)buf - T(0x24));
    }
}

/* 0x02012cf8: mirror a tile horizontally
 * @difftest ptr:0x40:4 */
void func_02012cf8(u8 *tile)
{
    s32 row, i;
    if (U32_AT(data_020debf8 + data_020df0fc * 0x3c, 0x28) == 1) {
        for (row = 8; row > 0; row--) {
            u8 *q = tile + 7;
            for (i = 4; i > 0; i--) {
                u8 a = *tile, b = *q;
                *tile++ = b;
                *q-- = a;
            }
            tile += 4;
        }
    } else {
        for (row = 8; row > 0; row--) {
            u8 *q = tile + 3;
            for (i = 2; i > 0; i--) {
                u8 b = *q, a = *tile;
                *tile++ = (b >> 4) | (b << 4);
                *q-- = (a >> 4) | (a << 4);
            }
            tile += 2;
        }
    }
}

/* 0x02012dd8: fill the buffer with `rows` tiles of one colour
 * @difftest ptr:0x200:4 int:0:4 u32 */
void func_02012dd8(u32 *buf, u32 rows, u32 colour)
{
    u32 n;
    if (U32_AT(data_020debf8 + data_020df0fc * 0x3c, 0x28) == 1) {
        colour = colour + (colour << 24) + (colour << 16) + (colour << 8);
        n = rows << 4;
    } else {
        colour &= 0xf;
        colour += colour << 4;
        colour += colour << 8;
        colour += colour << 16;
        n = rows << 3;
    }
    for (; n != 0; n--)
        *buf++ = colour;
}

/* 0x02012e58: load font `font` of the current screen */
void func_02012e58(u32 font, u32 sub)
{
    u8 *f = data_020dec70 + data_020df0fc * 0xa0 + font * 0x28;
    u8 *r = f + sub * 0x14;
    u8 *h;

    func_0207fe28(data_020bd068, 0x599);
    h = func_02012a64(data_020b564c[font * 2]);
    func_0207fe24();
    U16_AT(r, 2) = U16_AT(h, 0);
    U16_AT(r, 4) = U16_AT(h, 2);
    r[0] = h[4];
    r[6] = data_020b564c[font * 2 + 1];
    PTR_AT(r, 8) = h + ((h[6] << 8) + h[7]);
    PTR_AT(r, 0xc) = h + ((h[0xa] << 8) + h[0xb]);
    PTR_AT(r, 0x10) = h + ((h[0xe] << 8) + h[0xf]);
}

/* 0x02012f28: draw one line of text at (x, y)
 * @difftest $S=bytes:0x10:1 @$S+2:8=pick:47,65 @$S+3:8=pick:101,110,103,65,0 @$S+5:8=pick:47,66 @$S+6:8=pick:101,121,47 int:0:10 int:0:40 int:0:40 $S cases=80 */
u32 func_02012f28(u32 tile, s32 x, s32 y, const u8 *str)
{
    u32 buf[0x104 / 4];
    s32 stop = 0, ok, col, w, rows;
    u32 ink, px = 0, sh;
    u16 *map;
    u8 *chars, *font, *row8;
    const u8 *bits;

    map = func_02012b54(PTR_AT(data_020debf8 + data_020df0fc * 0x3c, 0x18), x, y);
    font = FONT();
    chars = PTR_AT(data_020debf8 + data_020df0fc * 0x3c, 0x20);
    ink = T(8);
    y &= 7;
    col = x & 7;
    rows = (s32)(y + font[0] + 7) >> 3;
    if (T(0xc) == (u32)-1)
        func_02012c40(buf, rows, map, chars);
    else
        func_02012dd8(buf, rows, T(0xc));
    ok = 1;
    row8 = (u8 *)buf + y * 8;

    while (*str != 0) {
        u32 c;
        s32 g;
        while ((c = *str) == '/' && ok) {
            switch (str[1]) {
            case 'g': case '(': case '+': case '!': case '?': case 'p': case 'y': case 'z':
                break;
            case 'e':
                ink = ink == 0 ? ink + 1 : 0;
                break;
            case 'n':
                stop = 1;
                break;
            default:
                ok = 0;
                break;
            }
            if (ok) {
                str++;
                if (*str != 0)
                    str++;
            }
        }
        if (c == 0 || stop)
            break;

        g = c - U16_AT(font, 2);
        if (g < 0 || g > U16_AT(font, 4) - U16_AT(font, 2))
            g = 1;
        bits = (u8 *)PTR_AT(font, 0x10) + ((u16 *)PTR_AT(font, 8))[g];
        sh = 0;
        for (w = ((u8 *)PTR_AT(font, 0xc))[g]; w != 0; w--) {
            s32 n;
            if (T(0x28) == 1) {
                u8 *p = row8 + col;
                for (n = font[0]; n != 0; n--, p += 8, sh += 4) {
                    u32 v;
                    if (sh == 8) {
                        bits++;
                        sh = 0;
                    }
                    v = (*bits >> sh) & 0xf;
                    if (v != 0) {
                        if (T(0x38) == 0)
                            px = v;
                        else if (T(0x38) == 1)
                            px = 0;
                        *p = px + ink;
                    }
                }
            } else {
                s32 o = col + y * 8;
                u8 *p = (u8 *)buf + o / 2;
                u32 odd = col & 1;
                for (n = font[0]; n != 0; n--, p += 4, sh += 4) {
                    u32 v;
                    if (sh == 8) {
                        bits++;
                        sh = 0;
                    }
                    v = (*bits >> sh) & 0xf;
                    if (v != 0) {
                        if (T(0x38) == 0)
                            px = v;
                        else if (T(0x38) == 1)
                            px = 0;
                        if (odd)
                            *p = (*p & 0xf) | ((px + ink) << 4);
                        else
                            *p = (*p & 0xf0) | (px + ink);
                    }
                }
            }
            col++;
            if (col == 8) {
                func_02012b78(buf, rows, map, chars, tile);
                tile += rows;
                map++;
                col = 0;
                if (T(0xc) == (u32)-1)
                    func_02012c40(buf, rows, map, chars);
                else
                    func_02012dd8(buf, rows, T(0xc));
            }
        }
        str++;
    }
    if (col > 0) {
        func_02012b78(buf, rows, map, chars, tile);
        tile += rows;
    }
    T(0x34) = 1;
    return tile;
}

/* 0x0201341c: draw one line of text aligned on (x, y)
 * (1/2 centre/right in x, 4/8 middle/bottom in y) */
u32 func_0201341c(u32 tile, s32 x, s32 y, const u8 *str, u32 align)
{
    if (align == 1)
        x -= (func_020138a0((const char *)str) + 1) / 2;
    else if (align == 2)
        x -= func_020138a0((const char *)str);
    if (align == 4)
        y -= (func_02013868((const char *)str) + 1) / 2;
    else if (align == 8)
        y -= func_02013868((const char *)str);
    return func_02012f28(tile, x, y, str);
}

/* 0x020134b8: draw the next line of *pstr that fits in maxw, at most *left
 * characters */
u32 func_020134b8(u32 tile, s32 x, s32 y, s32 maxw, const char **text, u32 align, s32 *left)
{
    const u8 **pstr = (const u8 **)text;
    u8 line[0x64], *d = line;
    const u8 *end = func_020137b8(*pstr, maxw), *p;
    if (end == NULL)
        return tile;
    p = *pstr;
    while (p != end && *left != 0) {
        u8 c = *p;
        if (c == '@') {
            p++;
            *d++ = c;
        }
        *d++ = *p++;
        *left -= 1;
    }
    *d = 0;
    *pstr = p;
    tile = func_0201341c(tile, x, y, line, align);
    if (**pstr == '\n')
        *pstr += 1;
    while (**pstr == ' ')
        *pstr += 1;
    return tile;
}

/* 0x020135a8: draw a text, breaking lines at maxw */
u32 func_020135a8(u32 tile, s32 x, s32 y, s32 maxw, const u8 *str, u32 align)
{
    s32 left = 0x7fffffff;
    if (*str == 0)
        return tile;
    do {
        tile = func_020134b8(tile, x, y, maxw, (const char **)&str, align, &left);
        y += T(0x14);
    } while (*str != 0);
    return tile;
}

/* 0x02013644: draw the next line of a text if it fits in maxh */
u32 func_02013644(u32 tile, s32 x, s32 y, s32 maxw, s32 maxh, const u8 **pstr, u32 align,
                  s32 *left)
{
    if (**pstr == 0)
        return tile;
    if (maxh < (s32)T(0x14))
        return tile;
    return func_020134b8(tile, x, y, maxw, (const char **)pstr, align, left);
}

/* 0x020136c0: draw as many lines of a text as fit in maxh
 * @difftest $S=bytes:0x18:1 $P=ptr:4:4 @$P+0:32=$S int:0:10 int:0:40 int:0:40 int:40:200 int:0:40 $P pick:0,1,2,4,8 cases=60 */
u32 func_020136c0(u32 tile, s32 x, s32 y, s32 maxw, s32 maxh, const u8 **pstr, u32 align)
{
    s32 left = 0x7fffffff;
    while (**pstr != 0 && maxh >= (s32)T(0x14)) {
        tile = func_020134b8(tile, x, y, maxw, (const char **)pstr, align, &left);
        y += T(0x14);
        maxh -= T(0x14);
    }
    return tile;
}

/* 0x02013770: copy at most n bytes of a text (0xf0-0xff start a two-byte
 * character)
 * @difftest ptr:0x40 bytes:0x30:1 int:0:0x30 */
u8 *func_02013770(u8 *dst, const u8 *src, s32 n)
{
    u8 *ret = dst;
    while (*src != 0 && n != 0) {
        if (*src >= 0xf0) {
            *dst++ = *src++;
            n--;
        }
        n--;
        *dst++ = *src++;
    }
    *dst = 0;
    return ret;
}

/* 0x020137b8: where to break a text to fit in maxw (NULL if nowhere) */
const u8 *func_020137b8(const u8 *str, s32 maxw)
{
    const u8 *brk = NULL, *hyphen = NULL;
    s32 w = 0;
    u32 c;
    while ((c = *str) != 0 && w <= maxw) {
        if (c == '\n') {
            brk = str;
            break;
        }
        if (c == ' ')
            brk = str;
        w += func_02012b14(FONT(), c);
        if (w <= maxw && c == '-')
            hyphen = str + 1;
        str++;
    }
    if (c == 0 && w <= maxw)
        brk = str;
    if (brk == NULL)
        brk = hyphen;
    return brk;
}

/* 0x02013868: height of a text line
 * @difftest pick:0x02100000 */
s32 func_02013868(const char *str)
{
    if (*(const u8 *)str == 0)
        return 0;
    return FONT()[0];
}

/* 0x020138a0: width of a text */
s32 func_020138a0(const char *text)
{
    const u8 *str = (const u8 *)text;
    s32 w = 0;
    u32 c = *str;
    while (c != 0) {
        w += func_02012b14(FONT(), c);
        c = *++str;
    }
    return w;
}

/* 0x020138f8: line height
 * @difftest @020df0fc:32=int:0:1 */
s32 func_020138f8(void) { return T(0x14); }

/* 0x0201391c: choose the font and colours of the current screen
 * @difftest @020df0fc:32=int:0:1 @020dec20:32=pick:0,1 @020dec5c:32=pick:0,1 int:0:3 s32 s32 */
void func_0201391c(s32 font, s32 ink, s32 paper)
{
    u8 *f = data_020dec70 + data_020df0fc * 0xa0 + font * 0x28;
    PTR_AT(data_020debf8 + data_020df0fc * 0x3c, 0) = f;
    PTR_AT(data_020debf8 + data_020df0fc * 0x3c, 4) = f + 0x14;
    T(0x14) = FONT()[6];
    if (T(0x28) == 1) {
        T(8) = ink;
        T(0x10) = 0;
    } else {
        T(8) = ink & 0xf;
        T(0x10) = (ink >> 4) << 12;
    }
    T(0xc) = paper;
}

/* 0x020139f8: draw into this BG map and these characters
 * @difftest @020df0fc:32=int:0:1 u32 u32 pick:0,1 */
void func_020139f8(void *map, void *chars, u32 bpp8)
{
    T(0x18) = (u32)map;
    T(0x1c) = (u32)map;
    T(0x20) = (u32)chars;
    T(0x24) = (bpp8 + 1) << 5;
    T(0x28) = bpp8;
}

/* 0x02013a5c: draw into the BG described by a BGxCNT value
 * @difftest @020df0fc:32=int:0:1 u16 */
void func_02013a5c(Rgb16 cnt)
{
    u32 v = cnt.v, scr = data_020df0fc;
    u32 bpp8 = BITS(v, 7, 1);
    u32 vram = (scr << 21) + 0x06000000;
    u32 map = vram + (BITS(v, 8, 5) << 11);
    T(0x18) = map;
    T(0x1c) = map;
    T(0x20) = vram + (BITS(v, 2, 3) << 14);
    T(0x24) = (bpp8 + 1) << 5;
    T(0x28) = bpp8;
}

/* 0x02013afc: load the fonts of both screens and reset their text state */
void func_02013afc(void)
{
    s32 scr;
    u32 f;
    for (scr = 0; scr < 2; scr++) {
        func_0201817c(scr);
        for (f = 0; f < 4; f++)
            func_02012e58(f, 0);
        MI_CpuFill8(data_020debf8 + data_020df0fc * 0x3c, 0, 0x3c);
    }
}
