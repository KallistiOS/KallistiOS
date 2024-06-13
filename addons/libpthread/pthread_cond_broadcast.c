/* KallistiOS ##version##

   pthread_cond_broadcast.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/cond.h>

int pthread_cond_broadcast(pthread_cond_t *cond) {
    int old, rv = 0;

    old = errno;
    if(cond_broadcast(&cond->cond))
        rv = errno;

    errno = old;
    return rv;
}
