/* KallistiOS ##version##

   pthread_cond_signal.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/cond.h>

int pthread_cond_signal(pthread_cond_t *cond) {
    int old, rv = 0;

    old = errno;
    if(cond_signal(&cond->cond))
        rv = errno;

    errno = old;
    return rv;
}
