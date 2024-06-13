/* KallistiOS ##version##

   pthread_cond_wait.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/cond.h>

int pthread_cond_wait(pthread_cond_t *__RESTRICT cond,
                      pthread_mutex_t *__RESTRICT mutex) {
    int old, rv = 0;

    old = errno;
    if(cond_wait(&cond->cond, &mutex->mutex))
        rv = errno;

    errno = old;
    return rv;
}
