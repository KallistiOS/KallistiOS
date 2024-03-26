/* KallistiOS ##version##

   pthread_mutex_unlock.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/mutex.h>

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    int old, rv = 0;

    old = errno;
    if(mutex_unlock(&mutex->mutex))
        rv = errno;

    errno = old;
    return rv;
}
