/* KallistiOS 2.0.0

   arch/gba/include/types.h
   (c)2000-2001 Dan Potter

*/

#ifndef __ARCH_TYPES_H
#define __ARCH_TYPES_H

#include <stddef.h>

/* Generic types */
typedef unsigned long long uint64;
typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef long long int64;
typedef long int32;
typedef short int16;
typedef char int8;

/* Volatile types */
typedef volatile uint64 vuint64;
typedef volatile uint32 vuint32;
typedef volatile uint16 vuint16;
typedef volatile uint8 vuint8;
typedef volatile int64 vint64;
typedef volatile int32 vint32;
typedef volatile int16 vint16;
typedef volatile int8 vint8;

/* These are aliases for the common GBA types */
typedef uint64 u64;
typedef uint32 u32;
typedef uint16 u16;
typedef uint8 u8;
typedef int64 s64;
typedef int32 s32;
typedef int16 s16;
typedef int8 s8;
typedef vuint64 vu64;
typedef vuint32 vu32;
typedef vuint16 vu16;
typedef vuint8 vu8;
typedef vint64 vs64;
typedef vint32 vs32;
typedef vint16 vs16;
typedef vint8 vs8;


/* Pointer arithmetic types */
typedef uint32 ptr_t;

/* another format for type names */
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;
typedef unsigned short  ushort;
typedef unsigned int    uint;

#if 0
/* File-specific types */
typedef size_t ssize_t;
typedef size_t off_t;
#endif

/* This type may be used for any generic handle type that is allowed
   to be negative (for errors) and has no specific bit count
   restraints. */
typedef int handle_t;

/* Thread and priority types */
typedef handle_t tid_t;
typedef handle_t prio_t;

#ifndef AT_SYMLINK_NOFOLLOW
/** \brief  Do not follow symlinks */
#define AT_SYMLINK_NOFOLLOW     2
#endif


#endif  /* __ARCH_TYPES_H */

