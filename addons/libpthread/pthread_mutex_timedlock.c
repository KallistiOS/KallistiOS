/* KallistiOS ##version##

   pthread_mutex_timedlock.c
   Copyright (C) 2023 Lawrence Sebald
   Copyright (C) 2024 Eric Fradella

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <sys/time.h>
#include <kos/errno.h>
#include <kos/mutex.h>

int pthread_mutex_timedlock(pthread_mutex_t *__RESTRICT mutex,
                            const struct timespec *__RESTRICT abstime) {
    int tmo;
    struct timespec ctv;

    if(!mutex || !abstime)
        return EFAULT;

    if(mutex->mutex.type > MUTEX_TYPE_RECURSIVE)
        return EINVAL;

    if(abstime->tv_nsec < 0 || abstime->tv_nsec > 1000000000L)
        return EINVAL;

    /* First, try to lock the lock before doing the hard work of figuring out
       the timing... POSIX says that if the lock can be acquired immediately
       then this function should never return a timeout, regardless of what
       abstime says. */
    errno_save_scoped();

    if(!mutex_trylock(&mutex->mutex))
        return 0;

    /* Figure out the timeout we need to provide in milliseconds. */
    clock_gettime(CLOCK_REALTIME, &ctv);

    tmo = (abstime->tv_sec - ctv.tv_sec) * 1000;
    tmo += (abstime->tv_nsec - ctv.tv_nsec) / (1000 * 1000);

    if(tmo <= 0)
        return ETIMEDOUT;

    return errno_if_nonzero(mutex_lock_timed(&mutex->mutex, tmo));
}
