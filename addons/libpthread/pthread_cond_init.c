/* KallistiOS ##version##

   pthread_cond_init.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/errno.h>
#include <kos/cond.h>

int pthread_cond_init(pthread_cond_t *__RESTRICT cond,
                      const pthread_condattr_t *__RESTRICT attr) {
    if(!cond)
        return EFAULT;

    errno_save_scoped();

    if(cond_init(&cond->cond))
        return errno;

    /* Copy attributes over into the condition variable. */
    if(attr)
        cond->clock_id = attr->clock_id;
    else
        cond->clock_id = CLOCK_REALTIME;

    return 0;
}
