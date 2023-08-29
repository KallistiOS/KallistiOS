/* KallistiOS ##version##

   pthread_attr_setstacksize.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
    if(!attr)
        return EINVAL;

    if(stacksize < PTHREAD_STACK_MIN ||
       (stacksize & (PTHREAD_STACK_MIN_ALIGNMENT - 1)))
        return EINVAL;

    attr->attr.stack_size = (uint32_t)stacksize;
    return 0;
}
