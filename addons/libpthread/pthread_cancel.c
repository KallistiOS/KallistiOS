/* KallistiOS ##version##

   pthread_cancel.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_cancel(pthread_t thread) {
    (void)thread;

    return ENOSYS;
}
