/* KallistiOS ##version##

   pthread_attr_setstack.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <stdint.h>

int pthread_attr_setstack(pthread_attr_t *restrict attr,
                          void *restrict stackaddr, size_t stacksize) {
    uintptr_t addr = (uintptr_t)stackaddr;

    if(!attr)
        return EINVAL;

    if(!stackaddr)
        return EACCES;

    if(stacksize < PTHREAD_STACK_MIN ||
       (addr & (PTHREAD_STACK_MIN_ALIGNMENT - 1)) ||
       (stacksize & (PTHREAD_STACK_MIN_ALIGNMENT - 1)))
        return EINVAL;

    attr->attr.stack_ptr = stackaddr;
    attr->attr.stack_size = (uint32_t)stacksize;

    return 0;
}
