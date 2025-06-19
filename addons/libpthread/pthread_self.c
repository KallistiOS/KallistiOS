/* KallistiOS ##version##

   pthread_self.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <kos/thread.h>

pthread_t pthread_self(void) {
    return (pthread_t)thd_get_current();
}
