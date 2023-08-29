/* KallistiOS ##version##

   pthread_detach.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/thread.h>

int pthread_detach(pthread_t thread) {
    int status = thd_detach((kthread_t *)thread);

    if(status == -3)
        return EINVAL;
    else if(status)
        return ESRCH;

    return 0;
}
