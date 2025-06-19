/* KallistiOS ##version##

   pthread_attr_setschedparam.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <kos/thread.h>

int pthread_attr_setschedparam(pthread_attr_t *__RESTRICT attr,
                               const struct sched_param *__RESTRICT param) {
    if(!attr)
        return EINVAL;

    if(!param)
        return EFAULT;

    if(param->sched_priority < 0 || param->sched_priority > PRIO_MAX)
        return EINVAL;

    attr->attr.prio = (prio_t)param->sched_priority;
    return 0;
}
