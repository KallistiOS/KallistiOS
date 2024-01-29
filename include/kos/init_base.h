/* KallistiOS ##version##

   include/kos/init_base.h
   Copyright (C) 2023 Falco Girgis

*/

/** \file    kos/init_base.h
    \brief   Shared initialization macros and utilities
    \ingroup init_flags

    This file contains common utilities which can be included within
    architecture-specific `init_flags.h` files, providing a base
    layer of infrastructure for managing initialization flags.

    \sa    kos/init.h
    \sa    arch/init_flags.h

    \author Falco Girgis
*/

#ifndef __KOS_INIT_BASE_H
#define __KOS_INIT_BASE_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \cond */

/* Declares a weak function pointer which can be optionally
   overridden and given a value later. */
#define KOS_INIT_FLAG_WEAK(func, dft_on) \
    void (*func##_weak)(void) __weak = (dft_on) ? func : NULL

/* Invokes the given function if its weak function pointer
   has been overridden to point to a valid function. */
#define KOS_INIT_FLAG_CALL(func) ({ \
    int ret = 0; \
    if(func##_weak) { \
        (*func##_weak)(); \
        ret = 1; \
    } \
    ret; \
})

/* Export the given function pointer by assigning it to the weak function
 * pointer if the given flags value contains all the flags set in the mask. */
#define KOS_INIT_FLAG_ALL(flags, mask, func) \
    extern void func(void); \
    void (*func##_weak)(void) = ((flags) & (mask)) == (mask) ? func : NULL

/* Export the given function by assigning it to the weak function pointer if the
   given flags value contains none of the flags set in the mask. */
#define KOS_INIT_FLAG_NONE(flags, mask, func) \
    extern void func(void); \
    void (*func##_weak)(void) = ((flags) & (mask)) ? NULL : func

/* Export the given function by assigning it to the weak function pointer if the
   given flags value contains the selected flag */
#define KOS_INIT_FLAG(flags, mask, func) \
    extern void func(void); \
    void (*func##_weak)(void) = ((flags) & (mask)) ? func : NULL

/** \endcond */

__END_DECLS

#endif /* !__ARCH_INIT_BASE_H */
