#ifndef SPYRO_TYPES_H
#define SPYRO_TYPES_H

/* Basic integer types, following NitroSDK naming. */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;

typedef volatile u8  vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;

typedef s32 fx32;   /* 20.12 fixed point */
typedef s16 fx16;   /* 4.12 fixed point  */

typedef int BOOL;
#define TRUE  1
#define FALSE 0

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Address of an original-game symbol that has not been given a real name yet,
 * e.g. DATA(020c4c00) -> data_020c4c00 as listed in config/arm9/symbols.txt. */
#define DATA(addr) data_##addr
#define FUNC(addr) func_##addr

#endif
