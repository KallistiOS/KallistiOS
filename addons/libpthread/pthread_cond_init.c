/* KallistiOS ##version##

   pthread_cond_init.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/cond.h>

int pthread_cond_init(pthread_cond_t *__RESTRICT cond,
                      const pthread_condattr_t *__RESTRICT attr) {
    int old, rv = 0;

    if(!cond)
        return EFAULT;

    old = errno;
    if(cond_init(&cond->cond))
        rv = errno;

    /* Copy attributes over into the condition variable. */
    if(attr)
        cond->clock_id = attr->clock_id;
    else
        cond->clock_id = CLOCK_REALTIME;

    errno = old;
    return rv;
}
