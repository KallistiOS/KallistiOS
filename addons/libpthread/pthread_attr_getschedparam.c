/* KallistiOS ##version##

   pthread_attr_getschedparam.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <sched.h>
#include <errno.h>

int pthread_attr_getschedparam(const pthread_attr_t *__RESTRICT attr,
                               struct sched_param *__RESTRICT param) {
    if(!attr)
        return EINVAL;

    if(!param)
        return EFAULT;

    param->sched_priority = (int)attr->attr.prio;
    return 0;
}
