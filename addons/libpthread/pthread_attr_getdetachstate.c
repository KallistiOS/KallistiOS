/* KallistiOS ##version##

   pthread_attr_getdetachstate.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
    if(!attr)
        return EINVAL;

    if(!detachstate)
        return EFAULT;

    *detachstate = attr->attr.create_detached;
    return 0;
}
