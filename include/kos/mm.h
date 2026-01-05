/* KallistiOS ##version##

   kos/mm.h
   Copyright (C) 2026 Paul Cercueil

   Memory management routines
*/

/** \file    kos/mm.h
    \brief   Memory management routines.
    \ingroup mm

    \author Paul Cercueil
*/

#ifndef __KOS_MM_H
#define __KOS_MM_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stddef.h>

/** \brief   Initialize the memory management system.
    \ingroup mm

    \retval 0               On success (no error conditions defined).
*/
int mm_init(void);

/** \brief   Request more core memory from the system.
    \ingroup mm

    \param  increment       The number of bytes requested.
    \return                 A pointer to the memory.
    \note                   This function will panic if no memory is available.
*/
void *mm_sbrk(size_t increment);

__END_DECLS
#endif /* __KOS_MM_H */
