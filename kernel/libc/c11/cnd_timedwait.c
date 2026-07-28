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
    int deadline_status = c11_timeout_ms(ts, &timeout);

    if(deadline_status < 0)
        return thrd_error;
    if(!deadline_status) {
        /* Even when the deadline has already passed, C11 requires this
           operation to release and reacquire the mutex. A one-millisecond
           timed wait preserves cond_wait_timed()'s atomic release-and-wait
           behavior without turning timeout == 0 into an indefinite wait. */
        if(cond_wait_timed(cond, mtx, 1) && errno != ETIMEDOUT)
            return thrd_error;

        return thrd_timedout;
    }

    if(cond_wait_timed(cond, mtx, timeout)) {
        if(errno == ETIMEDOUT)
            return thrd_timedout;

        return thrd_error;
    }

    return thrd_success;
}
