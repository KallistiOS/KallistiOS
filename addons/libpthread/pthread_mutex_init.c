/* KallistiOS ##version##

   pthread_mutex_init.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/mutex.h>

int pthread_mutex_init(pthread_mutex_t *__RESTRICT mutex,
                       const pthread_mutexattr_t *__RESTRICT attr) {
    unsigned int type = MUTEX_TYPE_NORMAL;
    int old, rv = 0;

    if(attr) {
        switch(attr->mtype) {
            case PTHREAD_MUTEX_NORMAL:
                type = MUTEX_TYPE_NORMAL;
                break;

            case PTHREAD_MUTEX_ERRORCHECK:
                type = MUTEX_TYPE_ERRORCHECK;
                break;

            case PTHREAD_MUTEX_RECURSIVE:
                type = MUTEX_TYPE_RECURSIVE;
                break;

            default:
                return EINVAL;
        }
    }

    old = errno;
    if(mutex_init(&mutex->mutex, type))
        rv = errno;

    errno = old;
    return rv;
}
