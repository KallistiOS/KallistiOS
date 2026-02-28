/* KallistiOS ##version##

   pthread_getsetconcurrency.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>

static int pth_concurrency = 0;

int pthread_getconcurrency(void) {
    return pth_concurrency;
}

int pthread_setconcurrency(int new_level) {

    if(new_level < 0)
        return EINVAL;

    pth_concurrency = new_level;

    return 0;
}
