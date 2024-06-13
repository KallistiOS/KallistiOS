/* KallistiOS ##version##

   pthread_mutexattr_gettype.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_mutexattr_gettype(const pthread_mutexattr_t *__RESTRICT attr,
                              int *__RESTRICT type) {
    if(!attr)
        return EINVAL;

    if(!type)
        return EFAULT;

    *type = attr->mtype;
    return 0;
}
