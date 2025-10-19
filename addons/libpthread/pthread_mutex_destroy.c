/* KallistiOS ##version##

   pthread_mutex_destroy.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/errno.h>
#include <kos/mutex.h>

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    errno_save_scoped();

    return errno_if_nonzero(mutex_destroy(&mutex->mutex));
}
