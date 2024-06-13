/* KallistiOS ##version##

   pthread_rwlock_timedrdlock.c
   Copyright (C) 2023 Lawrence Sebald
   Copyright (C) 2024 Eric Fradella

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <kos/rwsem.h>

int pthread_rwlock_timedrdlock(pthread_rwlock_t *__RESTRICT rwlock,
                               const struct timespec *__RESTRICT abstime) {
    int old, rv = 0;
    int tmo;
    struct timespec ctv;

    if(!rwlock || !abstime)
        return EFAULT;

    if(abstime->tv_nsec < 0 || abstime->tv_nsec > 1000000000L)
        return EINVAL;

    /* First, try to lock the lock before doing the hard work of figuring out
       the timing... POSIX says that if the lock can be acquired immediately
       then this function should never return a timeout, regardless of what
       abstime says. */
    old = errno;

    if(!rwsem_read_trylock(&rwlock->rwsem))
        return 0;

    /* Figure out the timeout we need to provide in milliseconds. */
    clock_gettime(CLOCK_REALTIME, &ctv);

    tmo = (abstime->tv_sec - ctv.tv_sec) * 1000;
    tmo += (abstime->tv_nsec - ctv.tv_nsec) / (1000 * 1000);

    if(tmo <= 0)
        return ETIMEDOUT;

    if(rwsem_read_lock_timed(&rwlock->rwsem, tmo))
        rv = errno;

    errno = old;
    return rv;
}
