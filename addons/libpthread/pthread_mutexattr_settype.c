/* KallistiOS ##version##

   pthread_mutexattr_settype.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    if(!attr)
        return EINVAL;

    switch(type) {
        case PTHREAD_MUTEX_NORMAL:
        case PTHREAD_MUTEX_ERRORCHECK:
        case PTHREAD_MUTEX_RECURSIVE:
            attr->mtype = type;
            return 0;

        default:
            return EINVAL;
    }
}
