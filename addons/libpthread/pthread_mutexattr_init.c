/* KallistiOS ##version##

   pthread_mutexattr_init.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
    if(!attr)
        return EFAULT;

    attr->mtype = PTHREAD_MUTEX_NORMAL;
    attr->robust = PTHREAD_MUTEX_STALLED;
    return 0;
}
