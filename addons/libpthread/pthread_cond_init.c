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

    (void)attr;

    if(!cond)
        return EFAULT;

    old = errno;
    if(cond_init(&cond->cond))
        rv = errno;

    errno = old;
    return rv;
}
