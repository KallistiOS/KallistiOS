/* KallistiOS ##version##

   pthread_mutex_trylock.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/errno.h>
#include <kos/mutex.h>

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    if(mutex->mutex.type > MUTEX_TYPE_RECURSIVE)
        return EINVAL;

    errno_save_scoped();

    if(mutex_trylock(&mutex->mutex))
        return errno;

    if(mutex->type == PTHREAD_MUTEX_ERRORCHECK && mutex->mutex.count > 1) {
        mutex_unlock(&mutex->mutex);
        return EDEADLK;
    }

    return 0;
}
