/* KallistiOS ##version##

   kos/cdefs.h
   Copyright (C) 2002, 2004 Megan Potter
   Copyright (C) 2020, 2023 Lawrence Sebald

   Based loosely around some stuff in BSD's sys/cdefs.h
*/

/** \file   kos/cdefs.h
    \brief  Potentially useful definitions for C Programs.

    This file contains definitions of various __attribute__ directives in
    shorter forms for use in programs (to aid in optimization, mainly).

    \author Megan Potter
    \author Lawrence Sebald
*/

#ifndef __KOS_CDEFS_H
#define __KOS_CDEFS_H

#include <sys/cdefs.h>

/* Check GCC version */
#if __GNUC__ <= 3
#   warning Your GCC is too old. This will probably not work right.
#endif

/* Special function/variable attributes */

#ifndef __noreturn
/** \brief  Identify a function that will never return. */
#define __noreturn  __attribute__((__noreturn__))
#endif

#ifndef __pure
/** \brief  Identify a function that has no side effects other than its return,
            and only uses its arguments for any work. */
#define __pure      __attribute__((__const__))
#endif

#ifndef __unused
/** \brief  Identify a function or variable that may be unused. */
#define __unused    __attribute__((__unused__))
#endif

#ifndef __dead2
/** \brief  Alias for \ref __noreturn. For BSD compatibility. */
#define __dead2     __noreturn  /* BSD compat */
#endif

#ifndef __pure2
/** \brief  Alias for \ref __pure. For BSD compatibility. */
#define __pure2     __pure      /* ditto */
#endif

#ifndef __deprecated
/** \brief  Mark something as deprecated.
    This should be used to warn users that a function/type/etc will be removed
    in a future version of KOS. */
#define __deprecated    __attribute__((deprecated))
#endif

#ifndef __depr
/** \brief  Mark something as deprecated, with an informative message.
    This should be used to warn users that a function/type/etc will be removed
    in a future version of KOS and to suggest an alternative that they can use
    instead.
    \param  m       A string literal that is included with the warning message
                    at compile time. */
#define __depr(m) __attribute__((deprecated(m)))
#endif

/* Printf/Scanf-like declaration */
#ifndef __printflike
/** \brief  Identify a function as accepting formatting like printf().

    Using this macro allows GCC to typecheck calls to printf-like functions,
    which can aid in finding mistakes.

    \param  fmtarg          The argument number (1-based) of the format string.
    \param  firstvararg     The argument number of the first vararg (the ...).
*/
#define __printflike(fmtarg, firstvararg) \
    __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#endif

#ifndef __scanflike
/** \brief  Identify a function as accepting formatting like scanf().

    Using this macro allows GCC to typecheck calls to scanf-like functions,
    which can aid in finding mistakes.

    \param  fmtarg          The argument number (1-based) of the format string.
    \param  firstvararg     The argument number of the first vararg (the ...).
*/
#define __scanflike(fmtarg, firstvararg) \
    __attribute__((__format__ (__scanf__, fmtarg, firstvararg)))
#endif

#if __GNUC__ >= 7
/** \brief  Identify a case statement that is expected to fall through to the
            statement underneath it. */
#define __fallthrough __attribute__((__fallthrough__))
#else
#define __fallthrough /* Fall through */
#endif

#ifndef __always_inline
/** \brief  Ask the compiler to always inline a given function. */
#define __always_inline inline __attribute__((__always_inline__))
#endif

/* GCC macros for special cases */
/* #if __GNUC__ ==  */

#endif  /* __KOS_CDEFS_H */
