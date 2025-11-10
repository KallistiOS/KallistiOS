/* KallistiOS ##version##

   mtx_timedlock.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>
#include <errno.h>

int mtx_timedlock(mtx_t *restrict mtx, const struct timespec *restrict ts) {
    int ms = 0;

    if(mtx->type > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        return -1;
    }

    /* Calculate the number of milliseconds to sleep for. No, you don't get
       anywhere near nanosecond precision here. */
    ms = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;

    /* The standard wording implies that we must wait at least the time period
       specified, so if we have an uneven number of milliseconds, round up. */
    if(ts->tv_nsec % 1000000)
        ++ms;

    if(mutex_lock_timed(mtx, ms)) {
        if(errno == ETIMEDOUT)
            return thrd_timedout;

        return thrd_error;
    }

    return thrd_success;
}
