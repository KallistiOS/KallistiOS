/* KallistiOS ##version##

   cnd_timedwait.c
   Copyright (C) 2014 Lawrence Sebald
*/

#include <threads.h>
#include <errno.h>

#include "threads_timeout.h"

int cnd_timedwait(cnd_t *restrict cond, mtx_t *restrict mtx,
                  const struct timespec *restrict ts) {
    unsigned int timeout;

    /* Convert deadline to relative milliseconds. */
    int deadline_status = c11_timeout_ms(ts, &timeout);

    /* Negative means a bad deadline or an unusable clock. */
    if(deadline_status < 0)
        return thrd_error;

    /* Deadline has already expired. C11 still requires the mutex to be
       released and reacquired, but there is nothing left to wait for. */
    if(!deadline_status) {
        mutex_unlock(mtx);
        mutex_lock(mtx);
        return thrd_timedout;
    }

    /* Wait for a signal or the deadline, whichever comes first. */
    if(cond_wait_timed(cond, mtx, timeout)) {
        if(errno == ETIMEDOUT)
            return thrd_timedout;

        return thrd_error;
    }

    return thrd_success;
}
