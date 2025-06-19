/* KallistiOS ##version##

   pthread_attr_getstack.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_attr_getstack(const pthread_attr_t *__RESTRICT attr,
                          void **__RESTRICT stackaddr,
                          size_t *__RESTRICT stacksize) {
    if(!attr)
        return EINVAL;

    if(stackaddr)
        *stackaddr = attr->attr.stack_ptr;

    if(stacksize)
        *stacksize = (size_t)attr->attr.stack_size;

    return 0;
}
