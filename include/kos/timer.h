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

#include <stdint.h>
#include <kos/tuple.h>

/** \brief   Get the current uptime of the system (in seconds and milliseconds).
    \ingroup timers

    This function retrieves the number of seconds and milliseconds since KOS was
    started.

    \return                 A tuple type, where the first value is the number of
                            seconds, and the second value is the number of
                            milliseconds.
*/
tu32_t timer_get_time_ms(void);

/** \brief   Get the current uptime of the system (in seconds and microseconds).
    \ingroup timers

    This function retrieves the number of seconds and microseconds since KOS was
    started.

    \return                 A tuple type, where the first value is the number of
                            seconds, and the second value is the number of
                            microseconds.
*/
tu32_t timer_get_time_us(void);

/** \brief   Get the current uptime of the system (in seconds and nanoseconds).
    \ingroup timers

    This function retrieves the number of seconds and nanoseconds since KOS was
    started.

    \return                 A tuple type, where the first value is the number of
                            seconds, and the second value is the number of
                            nanoseconds.
*/
tu32_t timer_get_time_ns(void);

/** \brief   Get the current uptime of the system (in secs and millisecs).
    \ingroup tmu_uptime

    This function retrieves the number of seconds and milliseconds since KOS was
    started.

    \param  secs            A pointer to store the number of seconds since boot
                            into.
    \param  msecs           A pointer to store the number of milliseconds past
                            a second since boot.
    \note                   To get the total number of milliseconds since boot,
                            calculate (*secs * 1000) + *msecs, or use the
                            timer_ms_gettime64() function.
*/
static inline void timer_ms_gettime(uint32_t *secs, uint32_t *msecs) {
    tu32_t time = timer_get_time_ms();

    if(secs)  *secs = time.u0;
    if(msecs) *msecs = time.u1;
}

/** \brief   Get the current uptime of the system (in secs and microsecs).
    \ingroup tmu_uptime

    This function retrieves the number of seconds and microseconds since KOS was
    started.

    \note                   To get the total number of microseconds since boot,
                            calculate (*secs * 1000000) + *usecs, or use the
                            timer_us_gettime64() function.

    \param  secs            A pointer to store the number of seconds since boot
                            into.
    \param  usecs           A pointer to store the number of microseconds past
                            a second since boot.
*/
static inline void timer_us_gettime(uint32_t *secs, uint32_t *usecs) {
    tu32_t time = timer_get_time_us();

    if(secs)  *secs = time.u0;
    if(usecs) *usecs = time.u1;
}

/** \brief   Get the current uptime of the system (in secs and nanosecs).
    \ingroup tmu_uptime

    This function retrieves the number of seconds and nanoseconds since KOS was
    started.

    \note                   To get the total number of nanoseconds since boot,
                            calculate (*secs * 1000000000) + *nsecs, or use the
                            timer_ns_gettime64() function.

    \param  secs            A pointer to store the number of seconds since boot
                            into.
    \param  nsecs           A pointer to store the number of nanoseconds past
                            a second since boot.
*/
static inline void timer_ns_gettime(uint32_t *secs, uint32_t *nsecs) {
    tu32_t time = timer_get_time_ns();

    if(secs)  *secs = time.u0;
    if(nsecs) *nsecs = time.u1;
}

/** \brief   Get the current uptime of the system (in milliseconds).
    \ingroup tmu_uptime

    This function retrieves the number of milliseconds since KOS was started. It
    is equivalent to calling timer_ms_gettime() and combining the number of
    seconds and milliseconds into one 64-bit value.

    \return                 The number of milliseconds since KOS started.
*/
static inline uint64_t timer_ms_gettime64(void) {
    tu32_t time = timer_get_time_ms();

    return (uint64_t)time.u0 * 1000ull + (uint64_t)time.u1;
}

/** \brief   Get the current uptime of the system (in microseconds).
    \ingroup tmu_uptime

    This function retrieves the number of microseconds since KOS was started.

    \return                 The number of microseconds since KOS started.
*/
static inline uint64_t timer_us_gettime64(void) {
    tu32_t time = timer_get_time_us();

    return (uint64_t)time.u0 * 1000000ull + (uint64_t)time.u1;
}

/** \brief   Get the current uptime of the system (in nanoseconds).
    \ingroup tmu_uptime

    This function retrieves the number of nanoseconds since KOS was started.

    \return                 The number of nanoseconds since KOS started.
*/
static inline uint64_t timer_ns_gettime64(void) {
    tu32_t time = timer_get_time_ns();

    return (uint64_t)time.u0 * 1000000000ull + (uint64_t)time.u1;
}

__END_DECLS

#endif /* __KOS_TIMER_H */
