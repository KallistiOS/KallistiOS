/* KallistiOS ##version##

   pthread_rwlock_wrlock.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/rwsem.h>

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
    int old, rv = 0;

    if(!rwlock)
        return EFAULT;

    old = errno;

    if(rwsem_write_lock(&rwlock->rwsem)) {
        rv = errno;
        errno = old;
    }

    return rv;
}
