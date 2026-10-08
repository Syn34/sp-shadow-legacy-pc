/*
 * FileSystem.cpp: files loaded by number, with reference counts.
 * ARM9 main, 0x020126d0 - 0x02012b14 (6 functions).
 *
 * data_020dad64[id] is the loaded file (0xc83 entries); data_020ddf70[id]
 * holds its reference count (bits 0-6) and a "kept" flag (bit 7) that
 * protects it from func_020128ec. Numbers >= 0xc83 are not files: callers
 * pass pointers to data they already have through the same functions.
 *
 * Status: functionally equivalent (tools/difftest.py), not byte-matching.
 */
#include "game.h"

#define NUM_FILES 0xc83

typedef struct {
    void *arc;
    u32 id;
} FSFileID;

extern const char data_020bd054[];   /* "/" */
extern const char data_020bd058[];   /* "FileSystem.cpp" */
extern void *data_020dad64[NUM_FILES];
extern u8 data_020ddf70[NUM_FILES];
extern u32 data_020debf4;            /* loading */

void func_02097870(void *dir, const char *path);   /* FS_FindDir */
void func_020978b4(void *dir, void *entry);        /* FS_ReadDir */
u32 func_02097998(void *file, void *dst, u32 len); /* FS_ReadFile */
void func_02097ab8(void *file);                    /* FS_CloseFile */
s32 func_02097b54(void *file, FSFileID id);        /* FS_OpenFileFast */
void func_02097e90(void *file);                    /* FS_InitFile */
void func_02097eb8(u32 dma);                       /* FS_Init */

#define COUNT(r) ((r) & 0x7f)
#define SET_COUNT(r, n) ((r) = ((r) & ~0x7f) | ((n) & 0x7f))

/* 0x020126d0: load file `id` into `buf` (or a new buffer when buf is NULL);
 * 0 failed, 1 loaded, 2 already loaded elsewhere
 * @difftest pick:-1,0xc83,0xd00 u32 u32 cases=30 */
s32 func_020126d0(s32 id, void *buf, u32 size)
{
    u32 file[0x44 / 4], dir[0x44 / 4], entry[0x90 / 4];
    s32 ret = 0;
    u32 len;

    if (id == -1)
        return 0;
    if (id >= NUM_FILES)
        return 0;
    data_020debf4 = 1;
    if (data_020dad64[id] != NULL && data_020dad64[id] != buf) {
        ret = 2;
    } else {
        func_02097e90(dir);
        func_02097e90(file);
        func_02097870(dir, data_020bd054);
        func_020978b4(dir, entry);
        entry[1] = id;
        if (func_02097b54(file, *(FSFileID *)entry)) {
            len = file[0x24 / 4] - file[0x20 / 4];
            if (buf == NULL)
                data_020dad64[id] = func_0207ff70(len, data_020bd058, 0x1f3);
            else if (size >= len)
                data_020dad64[id] = buf;
            else
                data_020dad64[id] = NULL;
            if (data_020dad64[id] != NULL) {
                if (func_02097998(file, data_020dad64[id], len) >= len) {
                    ret = 1;
                } else {
                    func_0207ff14(data_020dad64[id], data_020bd058, 0x205);
                    data_020dad64[id] = NULL;
                }
            }
            func_02097ab8(file);
        }
    }
    data_020debf4 = 0;
    return ret;
}

/* 0x02012854: take a reference to file `id` after loading it with `status`
 * @difftest int:0:0x40 pick:0,1,2 */
void *func_02012854(s32 id, s32 status)
{
    u8 *r;
    if (status == 0)
        return NULL;
    r = &data_020ddf70[id];
    if (status == 1) {
        SET_COUNT(*r, 1);
        *r |= 0x80;
    } else {
        if (!(*r & 0x80)) {
            *r |= 0x80;
            *r &= ~0x7f;
        }
        SET_COUNT(*r, COUNT(*r) + 1);
    }
    return data_020dad64[id];
}

/* 0x020128ec: drop unreferenced files (all files not kept when `all`)
 * @difftest pick:0,1 cases=20 */
void func_020128ec(s32 all)
{
    s32 i;
    u8 *r = data_020ddf70;
    for (i = 0; i < NUM_FILES; i++, r++) {
        if (data_020dad64[i] == NULL)
            continue;
        if (*r & 0x80)
            continue;
        if (COUNT(*r) != 0 && all == 0) {
            SET_COUNT(*r, COUNT(*r) + 0xff);
            continue;
        }
        func_0207ff14(data_020dad64[i], data_020bd058, 0x18b);
        data_020dad64[i] = NULL;
        *r &= ~0x7f;
    }
}

/* 0x020129ac: release a reference to file `id`
 * @difftest pick:-1,0xc83,0,1,2,3,4,5,6,7,8,9,10 */
void func_020129ac(s32 id)
{
    u8 *r;
    if (id == -1)
        return;
    if (id >= NUM_FILES)
        return;
    if (data_020dad64[id] == NULL)
        return;
    r = &data_020ddf70[id];
    if (COUNT(*r) != 0)
        SET_COUNT(*r, COUNT(*r) + 0xff);
    if (COUNT(*r) != 0)
        return;
    func_0207ff14(data_020dad64[id], data_020bd058, 0x118);
    data_020dad64[id] = NULL;
}

/* 0x02012a64: the data of file `id`, loading it when needed (ids >= 0xc83
 * are pointers and returned as they are)
 * @difftest pick:0xc83,0x02100000 */
void *func_02012a64(s32 id)
{
    if (id < NUM_FILES)
        return func_02012854(id, func_020126d0(id, NULL, 0));
    return (void *)id;
}

/* 0x02012aa4: initialise the file system and the file table
 * @difftest cases=5 */
void func_02012aa4(void)
{
    s32 i;
    u8 *r = data_020ddf70;
    func_02097eb8(4);
    for (i = 0; i < NUM_FILES; i++, r++) {
        data_020dad64[i] = NULL;
        *r &= ~0x7f;
        *r &= ~0x80;
    }
    data_020debf4 = 0;
}
