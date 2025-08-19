/* KallistiOS ##version##

   pthread_setname_np.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <kos/thread.h>
#include <arch/irq.h>

int pthread_setname_np(pthread_t thread, const char *name) {
    kthread_t *thd = (kthread_t *)thread;
    int old;

    if(!thd)
        return EINVAL;

    if(!name)
        return EFAULT;

    if(strlen(name) >= KTHREAD_LABEL_SIZE)
        return EINVAL;

    old = irq_disable();
    strcpy(thd->label, name);
    irq_restore(old);
    return 0;
}
