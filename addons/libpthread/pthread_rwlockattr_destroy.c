/* KallistiOS ##version##

   pthread_rwlockattr_destroy.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr) {
    if(!attr)
        return EINVAL;

    memset(attr, 0, sizeof(*attr));
    return 0;
}
