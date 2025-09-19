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

/** \brief   Get the current uptime of the system (in milliseconds).
    \ingroup timers

    This function retrieves the number of milliseconds since KOS was started. It
    is equivalent to calling timer_ms_gettime() and combining the number of
    seconds and milliseconds into one 64-bit value.

    \return                 The number of milliseconds since KOS started.
*/
static inline uint64_t timer_ms_gettime64(void) {
    timespec_t time = timer_gettime();

    return (uint64_t)time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

/** \brief   Get the current uptime of the system (in microseconds).
    \ingroup timers

    This function retrieves the number of microseconds since KOS was started.

    \return                 The uptime in microseconds.
*/
static inline uint64_t timer_us_gettime64(void) {
    timespec_t time = timer_gettime();

    return (uint64_t)time.tv_sec * 1000000 + time.tv_nsec / 1000;
}

/** \brief   Get the current uptime of the system (in nanoseconds).
    \ingroup timers

    This function retrieves the number of nanoseconds since KOS was started.

    \return                 The uptime in nanoseconds.
*/
static inline uint64_t timer_ns_gettime64(void) {
    timespec_t time = timer_gettime();

    return (uint64_t)time.tv_sec * 1000000000 + time.tv_nsec;
}

/** \brief   Get the current uptime of the system (in secs and millisecs).
    \ingroup timers

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
    timespec_t time = timer_gettime();

    if(secs)  *secs = time.tv_sec;
    if(msecs) *msecs = time.tv_nsec / 1000000;
}

/** \brief   Get the current uptime of the system (in secs and microsecs).
    \ingroup timers

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
    timespec_t time = timer_gettime();

    if(secs)  *secs = time.tv_sec;
    if(usecs) *usecs = time.tv_nsec / 1000;
}

/** \brief   Get the current uptime of the system (in secs and nanosecs).
    \ingroup timers

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
    timespec_t time = timer_gettime();

    if(secs)  *secs = time.tv_sec;
    if(nsecs) *nsecs = time.tv_nsec;
}

/** \brief  Spin-loop delay function with microsecond granularity
    \ingroup timers

    This function is meant as a very accurate delay function, even if threading
    and interrupts are disabled. It is a delay and not a sleep, which means that
    the CPU will be busy-looping during that time frame. For any time frame
    bigger than a few hundred microseconds, it is recommended to sleep instead.

    Note that the parameter is 16-bit, which means that the maximum acceptable
    value is 65535 microseconds.

    \param  us              The number of microseconds to wait for.
    \sa timer_spin_delay_ns, thd_sleep
*/
static inline void timer_spin_delay_us(unsigned short us) {
    uint64_t timeout = timer_us_gettime64() + us;

    /* Note that we don't actually care about the counter overflowing.
       Nobody will run their Dreamcast straight for 584942 years. */
    while(timer_us_gettime64() < timeout);
}


/** \brief  Spin-loop delay function with nanosecond granularity
    \ingroup timers

    This function is meant as a very accurate delay function, even if threading
    and interrupts are disabled. It is a delay and not a sleep, which means that
    the CPU will be busy-looping during that time frame.

    Note that the parameter is 16-bit, which means that the maximum acceptable
    value is 65535 nanoseconds.

    \param  ns              The number of nanoseconds to wait for.
    \sa timer_spin_delay_us, thd_sleep
*/
static inline void timer_spin_delay_ns(unsigned short ns) {
    uint64_t timeout = timer_ns_gettime64() + ns;

    /* Note that we don't actually care about the counter overflowing.
       Nobody will run their Dreamcast straight for 585 years. */
    while(timer_ns_gettime64() < timeout);
}

/** \brief  Spin-loop delay function with millisecond granularity
    \ingroup timers

    This function should never be used, and is only used for compatibility with
    older code. It makes no sense to busy-wait for that long.

    \param  ms              The number of microseconds to wait for.
    \sa thd_sleep
*/

__depr("Do not use timer_spin_sleep(), use thd_sleep() instead")
static inline void timer_spin_sleep(unsigned int ms) {
    uint64_t timeout = timer_ms_gettime64() + ms;

    while(timer_ms_gettime64() < timeout);
}

__END_DECLS

#endif /* __KOS_TIMER_H */
