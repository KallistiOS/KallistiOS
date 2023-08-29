/* KallistiOS ##version##

   pthread_attr_setdetachstate.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    if(!attr)
        return EINVAL;

    if(detachstate != PTHREAD_CREATE_DETACHED &&
       detachstate != PTHREAD_CREATE_JOINABLE)
        return EINVAL;

    attr->attr.create_detached = (detachstate == PTHREAD_CREATE_DETACHED);
    return 0;
}
