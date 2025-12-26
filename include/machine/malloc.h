/* KallistiOS ##version##

   machine/malloc.h
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2015 Lawrence Sebald
   Copyright (C) 2025 Donald Haase

*/

/** \file    malloc.h
    \brief   KOS-specific Allocator functionality
    \ingroup system_allocator

    This implements custom KOS extensions to allocator functionality.

    \author Megan Potter
    \author Lawrence Sebald
    \author Donald Haase
*/

#ifndef __MACHINE_MALLOC_H
#define __MACHINE_MALLOC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup system_allocator  Allocator Extensions
    \brief                      KOS custom allocator extensions
    \ingroup                    system

    @{
*/

/* mallopt default defines */
#define DEFAULT_MXFAST 64

#define DEFAULT_TRIM_THRESHOLD (256*1024)

#define DEFAULT_TOP_PAD 0

#define DEFAULT_MMAP_THRESHOLD (256*1024)

#define DEFAULT_MMAP_MAX 65536

/** \brief  Determine if it is safe to call malloc() in an IRQ context.

    This function checks the value of the internal spinlock that is used for
    malloc() to ensure that a call to it will not freeze the running process.
    This is only really useful in an IRQ context to ensure that a call to
    malloc() (or some other memory allocation function) won't cause a deadlock.

    \retval     1           If it is safe to call malloc() in the current IRQ.
    \retval     0           Otherwise.
*/
int malloc_irq_safe(void);

/** \brief Only available with KM_DBG
*/
int mem_check_block(void *p);

/** \brief Only available with KM_DBG
 */
int mem_check_all(void);

/** @} */

__END_DECLS

#endif  /* __MACHINE_MALLOC_H */
