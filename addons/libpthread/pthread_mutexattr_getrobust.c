/* KallistiOS ##version##

   pthread_mutexattr_getrobust.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_mutexattr_getrobust(const pthread_mutexattr_t *__RESTRICT attr,
                                int *__RESTRICT robust) {
    if(!attr)
        return EINVAL;

    if(!robust)
        return EFAULT;

    *robust = attr->robust;
    return 0;
}
