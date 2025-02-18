/** \file    sys/_types.h
    \brief   Internal typedefs.
    \ingroup system_types

    This file contains internal typedefs required by libc. You probably
    shouldn't use any of these in your code. Most of these are copied from
    newlib's %sys/_types.h.
*/

#ifndef _SYS__TYPES_H
#define _SYS__TYPES_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <sys/lock.h>
#include <newlib.h>

/** \addtogroup system_types
    @{
*/

/* This part copied from newlib's sys/_types.h. */
#ifndef __off_t_defined
/** \brief  File offset type. */
typedef long _off_t;
typedef _off_t __off_t;
#endif

#ifndef __dev_t_defined
/** \brief  Device ID type. */
typedef short __dev_t;
#endif

#ifndef __uid_t_defined
/** \brief  User ID type. */
typedef unsigned short __uid_t;
#endif
#ifndef __gid_t_defined
/** \brief  Group ID type. */
typedef unsigned short __gid_t;
#endif

#ifndef __off64_t_defined
/** \brief  64-bit file offset type. */
__extension__ typedef long long _off64_t;
#endif

#ifndef __fpos_t_defined
/** \brief  File position type. */
typedef long _fpos_t;
#endif

#ifdef __LARGE64_FILES
#ifndef __fpos64_t_defined
/** \brief  64-bit file position type. */
typedef _off64_t _fpos64_t;
#endif
#endif

#if defined(__INT_MAX__) && __INT_MAX__ == 2147483647
/** \brief  Signed size type. */
typedef int _ssize_t;
#else
typedef long _ssize_t;
#endif

/** \cond */
#define __need_wint_t
#include <stddef.h>
/** \endcond */

#ifndef __mbstate_t_defined
/** \brief  Conversion state information.
    \headerfile sys/_types.h
*/
typedef struct {
    int __count;
    union {
        wint_t __wch;
        unsigned char __wchb[4];
    } __value;            /* Value so far.  */
} _mbstate_t;
#endif

#ifndef __flock_t_defined
/** \brief  File lock type. */
typedef __newlib_recursive_lock_t _flock_t;
#endif

#ifndef __iconv_t_defined
/** \brief  Iconv descriptor type. */
typedef void *_iconv_t;
#endif

#ifndef __blkcnt_t_defined
typedef long __blkcnt_t;
#endif

#ifndef __blksize_t_defined
typedef long __blksize_t;
#endif

#ifndef __daddr_t_defined
typedef long __daddr_t;
#endif

#ifndef __fsblkcnt_t_defined
typedef unsigned long long __fsblkcnt_t;
#endif

#ifndef __fsfilcnt_t_defined
typedef unsigned long __fsfilcnt_t;
#endif

#ifndef __id_t_defined
typedef unsigned long __id_t;
#endif

#ifndef __ino_t_defined
typedef unsigned long __ino_t;
#endif

#ifndef __pid_t_defined
typedef int __pid_t;
#endif

#ifndef __key_t_defined
typedef long __key_t;
#endif

#ifndef __mode_t_defined
typedef unsigned long __mode_t;
#endif

typedef unsigned short __nlink_t;
typedef long        __suseconds_t;  /* microseconds (signed) */
typedef unsigned long   __useconds_t;   /* microseconds (unsigned) */

#if __NEWLIB__ >= 3
#define _TIME_T_ long long
#else
#define _TIME_T_ long
#endif
typedef _TIME_T_    __time_t;

#ifndef __clockid_t_defined
#define _CLOCKID_T_     unsigned long
#endif

typedef _CLOCKID_T_ __clockid_t;

#define _TIMER_T_   unsigned long
typedef _TIMER_T_   __timer_t;

#ifndef __clock_t_defined
#define _CLOCK_T_   unsigned long   /* clock() */
#endif

typedef _CLOCK_T_   __clock_t;

/* The architecture should define the macro BYTE_ORDER in <arch/types.h> to
   equal one of these macros for code that looks for these BSD-style macros. */
/** \brief  Little Endian test macro */
#define LITTLE_ENDIAN   1234

/** \brief  Big Endian test macro */
#define BIG_ENDIAN      4321

/** \brief  PDP Endian test macro */
#define PDP_ENDIAN      3412

/* Sigh... for some reason, Newlib only bothers defining these on Cygwin...
   We're only actually concerned with AT_SYMLINK_NOFOLLOW currently. These
   should all be defined in <fcntl.h>, by the way. */
#ifndef AT_EACCESS
/** \brief  Check access using effective user and group ID */
#define AT_EACCESS              1
#endif

#ifndef AT_SYMLINK_NOFOLLOW
/** \brief  Do not follow symlinks */
#define AT_SYMLINK_NOFOLLOW     2
#endif

#ifndef AT_SYMLINK_FOLLOW
/** \brief  Follow symbolic links */
#define AT_SYMLINK_FOLLOW       4
#endif

#ifndef AT_REMOVEDIR
/** \brief  Remove directory instead of file */
#define AT_REMOVEDIR            8
#endif

#ifndef IOV_MAX
/** \brief  Maximum length of an iovec, in elements. */
#define IOV_MAX                 1024
#endif

/* This is for old KOS source compatibility. */
#include <arch/types.h>

/* Include stuff to make pthreads work as well. */
#include <sys/_pthread.h>

#if __GNUC_MINOR__ > 95 || __GNUC__ >= 3
typedef __builtin_va_list   __va_list;
#else
typedef char *          __va_list;
#endif

/** @} */

__END_DECLS

#endif  /* _SYS__TYPES_H */

/* Grab our C11 time stuff if we got here from <time.h>. */
#ifdef _TIME_H_
#include <kos/time.h>
#endif

#ifdef _STDLIB_H_
#include <kos/stdlib.h>
#endif
