/* KallistiOS ##version##

   pthread_mutexattr_setrobust.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robust) {
    if(!attr)
        return EINVAL;

    /* We don't currently support robust mutexes. */
    if(robust != PTHREAD_MUTEX_STALLED)
        return ENOSYS;

    attr->robust = robust;
    return 0;
}
