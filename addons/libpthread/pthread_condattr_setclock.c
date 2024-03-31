/* KallistiOS ##version##

   pthread_condattr_setclock.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <time.h>

int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id) {
    if(!attr)
        return EINVAL;

    switch(clock_id) {
        case CLOCK_REALTIME:
        case CLOCK_MONOTONIC:
        case CLOCK_PROCESS_CPUTIME_ID:
            attr->clock_id = clock_id;
            break;

        default:
            return EINVAL;
    }

    return 0;
}
