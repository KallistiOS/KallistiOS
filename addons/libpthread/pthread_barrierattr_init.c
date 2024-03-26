/* KallistiOS ##version##

   pthread_barrierattr_init.c
   Copyright (C) 2024 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>

int pthread_barrierattr_init(pthread_barrierattr_t *attr) {
    if(!attr)
        return EFAULT;

    memset(attr, 0, sizeof(*attr));
    return 0;
}
