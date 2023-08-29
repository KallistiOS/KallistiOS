/* KallistiOS ##version##

   pthread_setcanceltype.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

int pthread_setcanceltype(int type, int *oldtype) {
    (void)type;

    *oldtype = PTHREAD_CANCEL_DEFERRED;
    return ENOSYS;
}
