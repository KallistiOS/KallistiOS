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

    return errno_if_nonzero(mutex_trylock(&mutex->mutex));
}
