/* KallistiOS ##version##

   nanosleep.c
   Copyright (C) 2014 Lawrence Sebald
   Copyright (C) 2024 Falco Girgis

*/

#include <time.h>
#include <errno.h>
#include <assert.h>

#include <kos/thread.h>

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp) {
    uint64_t ns;

    /* Make sure we aren't inside an interrupt first... */
    if(irq_inside_int()) {
        if(rmtp)
            *rmtp = *rqtp;

        errno = EINTR;  /* XXXX: Sorta. */
        return -1;
    }

    /* Calculate the number of milliseconds to sleep for. No, you don't get
       anywhere near nanosecond precision here. */
    ns = rqtp->tv_sec * 1000000000 + rqtp->tv_nsec;

    /* Make sure they gave us something valid. */
    if(!(ns > 0)) {
        if(rmtp)
            *rmtp = *rqtp;

        errno = EINVAL;
        return -1;
    }

    assert(ns <= UINT32_MAX);

    /* Sleep! */
    thd_sleep_ns(ns);

    /* thd_sleep will always sleep for at least the specified time, so clear out
       the remaining time, if it was given to us. */
    if(rmtp) {
        rmtp->tv_sec = 0;
        rmtp->tv_nsec = 0;
    }

    return 0;
}
