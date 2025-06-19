/* KallistiOS ##version##

   pthread_rwlock_init.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/rwsem.h>

int pthread_rwlock_init(pthread_rwlock_t *__RESTRICT rwlock,
                        const pthread_rwlockattr_t *__RESTRICT attr) {
    int old, rv = 0;

    (void)attr;

    if(!rwlock)
        return EFAULT;

    old = errno;

    if(rwsem_init(&rwlock->rwsem)) {
        rv = errno;
        errno = old;
    }

    return rv;
}
