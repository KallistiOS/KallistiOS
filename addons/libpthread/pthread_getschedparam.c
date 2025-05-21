/* KallistiOS ##version##

   pthread_getschedparam.c
   Copyright (C) 2025 Eric Fradella
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <kos/thread.h>

int pthread_getschedparam(pthread_t thread, int *__RESTRICT policy,
                          struct sched_param *__RESTRICT param) {
    kthread_t *thd = (kthread_t *)thread;

    if(!thd)
        return EINVAL;

    if(!policy)
        return EINVAL;

    if(!param)
        return EFAULT;

    *policy = SCHED_RR;

    param->sched_priority = (int)thd_get_prio(thd);

    return 0;
}
