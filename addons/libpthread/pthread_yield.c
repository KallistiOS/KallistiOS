/* KallistiOS ##version##

   pthread_yield.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/thread.h>

int pthread_yield(void) {
    thd_pass();
    return 0;
}
