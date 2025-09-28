/* KallistiOS ##version##

   pthread_cond_signal.c
   Copyright (C) 2023 Lawrence Sebald

*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/cond.h>
#include <kos/errno.h>

int pthread_cond_signal(pthread_cond_t *cond) {
    errno_save_scoped();

    return errno_if_nonzero(cond_signal(&cond->cond));
}
