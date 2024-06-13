/* KallistiOS ##version##

   pthread_setprio.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/thread.h>

int pthread_setprio(pthread_t thread, int prio) {
    kthread_t *thd = (kthread_t *)thread;

    if(!thd)
        return EINVAL;

    if(thd_set_prio(thd, (prio_t)prio))
        return EINVAL;

    return 0;
}

int pthread_setschedprio(pthread_t thread, int prio)
    __attribute__((alias ("pthread_setprio")));
