/* KallistiOS ##version##

   pthread_rwlock_init.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/errno.h>
#include <kos/rwsem.h>

int pthread_rwlock_init(pthread_rwlock_t *__RESTRICT rwlock,
                        const pthread_rwlockattr_t *__RESTRICT attr) {
    (void)attr;

    if(!rwlock)
        return EFAULT;

    errno_save_scoped();

    return errno_if_nonzero(rwsem_init(&rwlock->rwsem));
}
