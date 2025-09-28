/* KallistiOS ##version##

   pthread_cond_broadcast.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/errno.h>
#include <kos/cond.h>

int pthread_cond_broadcast(pthread_cond_t *cond) {
    errno_save_scoped();

    return errno_if_nonzero(cond_broadcast(&cond->cond));
}
