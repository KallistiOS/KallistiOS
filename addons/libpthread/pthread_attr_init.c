/* KallistiOS ##version##

   pthread_attr_init.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include <kos/thread.h>
#include <arch/arch.h>

int pthread_attr_init(pthread_attr_t *attr) {
    if(!attr)
        return EFAULT;

    /* Set up our default thread attributes. */
    memset(attr, 0, sizeof(*attr));
    attr->attr.stack_size = THD_STACK_SIZE;
    attr->attr.prio = PRIO_DEFAULT;

    return 0;
}
