/* KallistiOS ##version##

   pthread_mutex_lock.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/mutex.h>

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    int old, rv = 0;

    if(mutex->mutex.type > MUTEX_TYPE_RECURSIVE)
        return EINVAL;

    old = errno;
    if(mutex_lock(&mutex->mutex))
        rv = errno;

    errno = old;
    return rv;
}
