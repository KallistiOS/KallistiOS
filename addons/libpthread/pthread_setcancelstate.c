/* KallistiOS ##version##

   pthread_setcancelstate.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_setcancelstate(int state, int *oldstate) {
    (void)state;

    *oldstate = PTHREAD_CANCEL_DISABLE;
    return ENOSYS;
}
