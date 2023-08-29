/* KallistiOS ##version##

   pthread_attr_setguardsize.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize) {
    if(!attr)
        return EINVAL;

    if(guardsize)
        return ENOSYS;

    return 0;
}
