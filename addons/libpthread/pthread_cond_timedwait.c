/* KallistiOS ##version##

   pthread_cond_timedwait.c
   Copyright (C) 2023 Lawrence Sebald
   Copyright (C) 2024 Eric Fradella
   Copyright (C) 2024 Falco Girgis

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <sys/time.h>
#include <kos/cond.h>
#include <kos/errno.h>

int pthread_cond_timedwait(pthread_cond_t *__RESTRICT cond,
                           pthread_mutex_t *__RESTRICT mutex,
                           const struct timespec *__RESTRICT abstime) {
    int tmo;
    struct timespec ctv;

    if(!mutex || !abstime)
        return EFAULT;

    if(abstime->tv_nsec < 0 || abstime->tv_nsec > 1000000000L)
        return EINVAL;

    errno_save_scoped();

    /* Figure out the timeout we need to provide in milliseconds. */
    clock_gettime(cond->clock_id, &ctv);

    tmo = (abstime->tv_sec - ctv.tv_sec) * 1000;
    tmo += (abstime->tv_nsec - ctv.tv_nsec) / (1000 * 1000);

    if(tmo <= 0)
        return ETIMEDOUT;

    return errno_if_nonzero(cond_wait_timed(&cond->cond, &mutex->mutex, tmo));
}
