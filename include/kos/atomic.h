/* KallistiOS ##version##

   include/kos/atomic.h
   Copyright (C) 2024 Paul Cercueil
*/

/** \file    kos/atomic.h
    \brief   Atomic compare-and-swap support.
    \ingroup kthreads

    This file contains KOS' atomic API.
 */

#ifndef __KOS_ATOMIC_H
#define __KOS_ATOMIC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

/** \brief  Atomically compare and swap values

    This function will atomically read the value pointed to by the "addr"
    pointer, and if it is equal to the "old" value, it will replace the
    pointed value with the "new" value.

    \param  addr            A pointer to the atomic value
    \param  old             The expected value
    \param  new             The new value

    \retval                 The value read from the "addr" pointer
*/
uint32_t compare_and_swap(uint32_t *addr, uint32_t old, uint32_t new);

__END_DECLS

#endif /* __KOS_ATOMIC_H */
