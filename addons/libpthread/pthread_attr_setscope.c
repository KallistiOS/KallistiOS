/* KallistiOS ##version##

   pthread_attr_setscope.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_attr_setscope(pthread_attr_t *attr, int contentionscope) {
    if(!attr)
        return EINVAL;

    if(contentionscope != PTHREAD_SCOPE_SYSTEM)
        return ENOTSUP;

    return 0;
}
