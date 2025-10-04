/* KallistiOS ##version##

   pthread_cond_wait.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/cond.h>
#include <kos/errno.h>

int pthread_cond_wait(pthread_cond_t *__RESTRICT cond,
                      pthread_mutex_t *__RESTRICT mutex) {
    errno_save_scoped();

    return errno_if_nonzero(cond_wait(&cond->cond, &mutex->mutex));
}
