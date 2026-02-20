/* KallistiOS ##version##

   pthread_rwlock_unlock.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/errno.h>
#include <kos/rwsem.h>

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
    if(!rwlock)
        return EFAULT;

    errno_save_scoped();

    return errno_if_nonzero(rwsem_unlock(&rwlock->rwsem));
}
