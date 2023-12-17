/* KallistiOS ##version##

   kos/string.h
   Copyright (C)2004 Megan Potter

*/

/** \file    kos/string.h
    \brief   Variants on standard block memory copy/set functions.
    \ingroup system_types

    This file contains variants on the standard block memory copy/set functions.
    These variants copy/set memory in the specified block sizes, which may be
    helpful for interacting with memory-mapped hardware.

    \author Megan Potter
*/

#ifndef __KOS_STRING_H
#define __KOS_STRING_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <string.h>

/** \addtogroup system_types
    @{
*/

/** \brief  Copy a block of memory, 4 bytes at a time.

    This function is identical to memcpy(), except it copies 4 bytes at a time.

    \param  dest            The destination of the copy.
    \param  src             The source to copy.
    \param  count           The number of bytes to copy. This should be
                            divisible by 4 (and will be rounded down if not).
    \return                 The original value of dest.
*/
void * memcpy4(void * dest, const void *src, size_t count);

/** \brief  Set a block of memory, 4 bytes at a time.

    This function is identical to memset(), except it sets 4 bytes at a time.
    This implies that all 32-bits of c are used, not just the first 8 as in
    memset().

    \param  s               The destination of the set.
    \param  c               The value to set to.
    \param  count           The number of bytes to set. This should be
                            divisible by 4 (and will be rounded down if not).
    \return                 The original value of dest.
*/
void * memset4(void * s, unsigned long c, size_t count);

/** \brief  Copy a block of memory, 2 bytes at a time.

    This function is identical to memcpy(), except it copies 2 bytes at a time.

    \param  dest            The destination of the copy.
    \param  src             The source to copy.
    \param  count           The number of bytes to copy. This should be
                            divisible by 2 (and will be rounded down if not).
    \return                 The original value of dest.
*/
void * memcpy2(void * dest, const void *src, size_t count);

/** \brief  Set a block of memory, 2 bytes at a time.

    This function is identical to memset(), except it sets 2 bytes at a time.
    This implies that all 16-bits of c are used, not just the first 8 as in
    memset().

    \param  s               The destination of the set.
    \param  c               The value to set to.
    \param  count           The number of bytes to set. This should be
                            divisible by 2 (and will be rounded down if not).
    \return                 The original value of dest.
*/
void * memset2(void * s, unsigned short c, size_t count);

/** @} */

__END_DECLS

#endif  /* __KOS_STRING_H */


