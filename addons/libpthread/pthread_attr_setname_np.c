/* KallistiOS ##version##

   pthread_attr_setname_np.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <kos/thread.h>

int pthread_attr_setname_np(pthread_attr_t *__RESTRICT attr,
                            const char *__RESTRICT name) {
    int old, rv;

    if(!attr)
        return EINVAL;

    if(!name)
        return EFAULT;

    if(strlen(name) >= KTHREAD_LABEL_SIZE)
        return EINVAL;

    old = errno;

    if(!(attr->attr.label = strdup(name))) {
        rv = errno;
        errno = old;
        return rv;
    }

    return 0;
}
