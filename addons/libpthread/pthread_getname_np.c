/* KallistiOS ##version##

   pthread_getname_np.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <kos/thread.h>
#include <arch/irq.h>

int pthread_getname_np(pthread_t thread, char *buf, size_t buflen) {
    kthread_t *thd = (kthread_t *)thread;
    int old;

    if(!thd)
        return EINVAL;

    if(!buf)
        return EFAULT;

    if(buflen <= KTHREAD_LABEL_SIZE) {
        old = irq_disable();
        memcpy(buf, thd->label, buflen - 1);
        irq_restore(old);
        buf[buflen - 1] = 0;
    }
    else {
        old = irq_disable();
        memcpy(buf, thd->label, KTHREAD_LABEL_SIZE);
        irq_restore(old);
        memset(buf + KTHREAD_LABEL_SIZE, 0, buflen - KTHREAD_LABEL_SIZE);
    }

    return 0;
}
