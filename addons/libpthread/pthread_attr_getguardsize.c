/* KallistiOS ##version##

   pthread_attr_getguardsize.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_attr_getguardsize(const pthread_attr_t *__RESTRICT attr,
                              size_t *__RESTRICT guardsize) {
    if(!attr)
        return EINVAL;

    if(!guardsize)
        return EFAULT;

    *guardsize = 0;
    return 0;
}
