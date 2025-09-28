/* KallistiOS ##version##

   include/kos/errno.h
   Copyright (C) 2025 Paul Cercueil

*/

/** \file    kos/errno.h
    \brief   Errno helper functions.
    \ingroup errno

    This file contains functions and macros related to the 'errno' variable.

    \author Paul Cercueil
*/

#ifndef __KOS_ERRNO_H
#define __KOS_ERRNO_H

#include <errno.h>

/** \cond */
static inline void __errno_scoped_cleanup(int *e) {
    if(e)
        errno = *e;
}

#define ___errno_save_scoped(l) \
    int __scoped_errno_##l __attribute__((cleanup(__errno_scoped_cleanup))) = (errno)

#define __errno_save_scoped(l) ___errno_save_scoped(l)
/** \endcond */

/** \brief  Save the current 'errno' value until the block exit

    This macro will keep a copy of the current value of the errno variable, and
    will restore it once the execution exits the functional block in which the
    macro was called.
*/
#define errno_save_scoped() __errno_save_scoped(__LINE__)

/** \brief  Return errno if the value is non-zero, otherwise return zero

    This simple macro can be used to interface the functions that return 0 on
    success and -1 on error, with the actual error code stored in the errno
    variable.

    \param  x               The integer value to process
    \return                 0 on success, a positive errno code on error
*/
#define errno_if_nonzero(x) ((x) ? errno : 0)

#endif /* __KOS_ERRNO_H */
