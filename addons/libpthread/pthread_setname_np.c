/* KallistiOS ##version##

   pthread_setname_np.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <kos/irq.h>
#include <kos/thread.h>

int pthread_setname_np(pthread_t thread, const char *name) {
    kthread_t *thd = (kthread_t *)thread;

    if(!thd)
        return EINVAL;

    if(!name)
        return EFAULT;

    if(strlen(name) >= KTHREAD_LABEL_SIZE)
        return EINVAL;

    irq_disable_scoped();
    strcpy(thd->label, name);

    return 0;
}
