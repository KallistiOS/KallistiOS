/* KallistiOS ##version##

   pthread_attr_getstacksize.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_attr_getstacksize(const pthread_attr_t *__RESTRICT attr,
                              size_t *__RESTRICT stacksize) {
    if(!attr)
        return EINVAL;

    if(!stacksize)
        return EFAULT;

    *stacksize = (size_t)attr->attr.stack_size;
    return 0;
}
