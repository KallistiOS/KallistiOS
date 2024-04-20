/* KallistiOS ##version##

   pthread_condattr_getclock.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <time.h>

int pthread_condattr_getclock(const pthread_condattr_t *__RESTRICT attr,
                              clockid_t *__RESTRICT clock_id) {
    if(!attr)
        return EINVAL;

    *clock_id = attr->clock_id;
    return 0;
}
