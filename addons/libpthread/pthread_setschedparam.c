/* KallistiOS ##version##

   pthread_setschedparam.c
   Copyright (C) 2025 Eric Fradella
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <kos/thread.h>

int pthread_setschedparam(pthread_t thread, int policy,
                          const struct sched_param *param) {
    kthread_t *thd = (kthread_t *)thread;

    if(!thd)
        return EINVAL;

    if(policy != SCHED_RR)
        return EINVAL;

    if(!param)
        return EFAULT;

    if(thd_set_prio(thd, (prio_t)param->sched_priority))
        return EINVAL;

    return 0;
}
