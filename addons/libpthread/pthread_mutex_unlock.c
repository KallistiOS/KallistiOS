/* KallistiOS ##version##

   pthread_mutex_unlock.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/errno.h>
#include <kos/mutex.h>

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    if(mutex->type == PTHREAD_MUTEX_ERRORCHECK &&
       (mutex->mutex.count == 0 || mutex->mutex.holder != thd_get_current())) {
        return EFAULT;
    }

    errno_save_scoped();

    return errno_if_nonzero(mutex_unlock(&mutex->mutex));
}
