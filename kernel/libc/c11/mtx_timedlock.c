/* KallistiOS ##version##

   mtx_timedlock.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>
#include <errno.h>

#include "threads_timeout.h"

int mtx_timedlock(mtx_t *restrict mtx, const struct timespec *restrict ts) {
    unsigned int timeout;
    int deadline_status;

    if(mtx->type > MUTEX_TYPE_RECURSIVE) {
        errno = EINVAL;
        return thrd_error;
    }

    /* Uncontended locks succeed even on an expired deadline. */
    if(!mutex_trylock(mtx))
        return thrd_success;

    /* Anything but EBUSY is a real failure. */
    if(errno != EBUSY)
        return thrd_error;

    /* Convert deadline to relative milliseconds. */
    deadline_status = c11_timeout_ms(ts, &timeout);

    /* Negative means a bad deadline or an unusable clock. */
    if(deadline_status < 0)
        return thrd_error;

    /* Deadline has already expired. */
    if(!deadline_status)
        return thrd_timedout;

    /* Block until we own the mutex or the deadline arrives. */
    if(mutex_lock_timed(mtx, timeout)) {
        if(errno == ETIMEDOUT)
            return thrd_timedout;

        return thrd_error;
    }

    return thrd_success;
}
