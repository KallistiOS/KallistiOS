/* KallistiOS ##version##

   include/kos/timer.h
   Copyright (C) 2025 Paul Cercueil
*/

/** \file    kos/timer.h
    \brief   Timer functionality.
    \ingroup timers

    This file contains functions for reading the internal timer provided
    by the architecture.

    \sa arch/timer.h

    \author Paul Cercueil
*/
#ifndef __KOS_TIMER_H
#define __KOS_TIMER_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/timer.h>
#include <stdint.h>
#include <time.h>

typedef struct timespec timespec_t;

/** \brief   Get the current uptime of the system.
    \ingroup timers

    This function retrieves the number of seconds and nanoseconds since KOS was
    started.

    \return                 The current uptime of the system as a timespec struct.
*/
static inline timespec_t timer_gettime(void) {
    return arch_timer_gettime();
}

__END_DECLS

#endif /* __KOS_TIMER_H */
