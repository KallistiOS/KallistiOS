/* KallistiOS ##version##

   pthread_attr_getscope.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_attr_getscope(const pthread_attr_t *__RESTRICT attr,
                          int *__RESTRICT contentionscope) {
    if(!attr)
        return EINVAL;

    if(!contentionscope)
        return EFAULT;

    *contentionscope = PTHREAD_SCOPE_SYSTEM;
    return 0;
}
