/* KallistiOS ##version##

   pthread_attr_setname_np.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <string.h>
#include <kos/errno.h>
#include <kos/thread.h>

int pthread_attr_setname_np(pthread_attr_t *__RESTRICT attr,
                            const char *__RESTRICT name) {
    if(!attr)
        return EINVAL;

    if(!name)
        return EFAULT;

    if(strlen(name) >= KTHREAD_LABEL_SIZE)
        return EINVAL;

    errno_save_scoped();

    if(!(attr->attr.label = strdup(name)))
        return errno;

    return 0;
}
