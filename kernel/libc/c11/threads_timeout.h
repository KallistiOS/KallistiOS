/* KallistiOS ##version##

   threads_timeout.h
   Copyright (C) 2026 The KallistiOS Contributors

*/

#ifndef __KOS_LIBC_C11_THREADS_TIMEOUT_H
#define __KOS_LIBC_C11_THREADS_TIMEOUT_H

#include <limits.h>
#include <stdint.h>
#include <threads.h>

/*
 * Convert the absolute TIME_UTC deadline required by the C threads API to the
 * relative millisecond timeout used by KOS synchronization primitives.
 *
 * Returns 1 for a future deadline, 0 for an expired deadline, and -1 for an
 * invalid deadline or unavailable clock.
 */
static inline int
c11_timeout_ms(const struct timespec *deadline, unsigned int *timeout) {
    struct timespec now;
    uintmax_t seconds;
    uintmax_t milliseconds;
    long nanoseconds;

    if(!deadline || !timeout ||
       deadline->tv_nsec < 0 || deadline->tv_nsec >= 1000000000L ||
       timespec_get(&now, TIME_UTC) != TIME_UTC)
        return -1;

    if(deadline->tv_sec < now.tv_sec ||
       (deadline->tv_sec == now.tv_sec &&
        deadline->tv_nsec <= now.tv_nsec))
        return 0;

    /*
     * The comparison above proves this modular subtraction is the real,
     * non-negative difference. Casting before subtraction also avoids signed
     * overflow for a deadline and current time at opposite time_t extremes.
     */
    seconds = (uintmax_t)deadline->tv_sec - (uintmax_t)now.tv_sec;
    nanoseconds = deadline->tv_nsec - now.tv_nsec;
    if(nanoseconds < 0) {
        --seconds;
        nanoseconds += 1000000000L;
    }

    /*
     * cond_wait_timed() still accepts a signed int even though genwait uses
     * unsigned milliseconds. Keep one ceiling that is representable through
     * every C11 wrapper and underlying KOS primitive.
     */
    if(seconds > INT_MAX / 1000U) {
        *timeout = INT_MAX;
        return 1;
    }

    milliseconds = seconds * 1000U;
    milliseconds += ((uintmax_t)nanoseconds + 999999U) / 1000000U;
    if(milliseconds > INT_MAX)
        milliseconds = INT_MAX;

    /* Zero means "wait forever" to KOS, never pass it for a future deadline. */
    *timeout = milliseconds ? (unsigned int)milliseconds : 1U;
    return 1;
}

#endif /* __KOS_LIBC_C11_THREADS_TIMEOUT_H */
