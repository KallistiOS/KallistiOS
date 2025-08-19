/* KallistiOS ##version##

   pthread_getprio.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/thread.h>

int pthread_getprio(pthread_t thread) {
    kthread_t *thd = (kthread_t *)thread;

    if(!thd)
        return EINVAL;

    return (int)thd->prio;
}
