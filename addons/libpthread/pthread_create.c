/* KallistiOS ##version##

   pthread_create.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/thread.h>

int pthread_create(pthread_t *__RESTRICT thread,
                   const pthread_attr_t *__RESTRICT attr,
                   void *(*start_routine)(void *), void *__RESTRICT arg) {
    kthread_t *thd;
    const kthread_attr_t *rattr = NULL;

    if(!thread || !start_routine)
        return EFAULT;

    if(attr)
        rattr = &attr->attr;

    if(!(thd = thd_create_ex(rattr, start_routine, arg)))
        return EAGAIN;

    *thread = (pthread_t)thd;
    return 0;
}
