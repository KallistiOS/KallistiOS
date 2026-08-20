/* KallistiOS ##version##

   threads_timeout.h
   Copyright (C) 2026 Cypress

*/

#ifndef __LOCAL_C11_THREADS_TIMEOUT_H
#define __LOCAL_C11_THREADS_TIMEOUT_H

#include <stdint.h>
#include <threads.h>

/* Convert the absolute TIME_UTC deadline required by the C threads API to the
   relative millisecond timeout used by KOS synchronization primitives.

   Returns:
      1  the deadline is in the future, with *timeout set to the wait
      0  the deadline has already expired
     -1  a null argument, or the clock could not be read
*/
static inline int
c11_timeout_ms(const struct timespec *deadline, unsigned int *timeout) {
    struct timespec now;
    uint32_t seconds;
    uint32_t milliseconds;
    long nanoseconds;

    /* Make sure we have valid inputs. */
    if(!deadline || !timeout || timespec_get(&now, TIME_UTC) != TIME_UTC)
        return -1;

    /* The deadline has passed, so there is no time left to wait. */
    if(deadline->tv_sec < now.tv_sec ||
      (deadline->tv_sec == now.tv_sec &&
       deadline->tv_nsec <= now.tv_nsec))
        return 0;

    /* Calculate the time difference between the deadline and current time. */
    seconds = (uint32_t)deadline->tv_sec - (uint32_t)now.tv_sec;
    nanoseconds = deadline->tv_nsec - now.tv_nsec;
    if(nanoseconds < 0) {
        --seconds;
        nanoseconds += 1000000000;
    }

    /* Calculate the number of milliseconds to sleep for (Rounding up to at
       least 1 ms). */
    milliseconds = seconds * 1000;
    milliseconds += ((uint32_t)nanoseconds + 999999) / 1000000;

    *timeout = milliseconds;

    return 1;
}

#endif /* __LOCAL_C11_THREADS_TIMEOUT_H */
