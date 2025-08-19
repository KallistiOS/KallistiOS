/* KallistiOS ##version##

   pthread_join.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <kos/thread.h>

int pthread_join(pthread_t thread, void **value_ptr) {
    int status;

    if(thread == (pthread_t)thd_get_current())
        return EDEADLK;

    status = thd_join((kthread_t *)thread, value_ptr);

    if(status == -3)
        return EINVAL;
    else if(status)
        return ESRCH;

    return 0;
}
