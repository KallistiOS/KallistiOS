/* KallistiOS ##version##

   pthread_attr_getname_np.c
   Copyright (C) 2023 Lawrence Sebald
*/

#include "pthread-internal.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>

extern int pthread_attr_getname_np(const pthread_attr_t *__RESTRICT attr,
                                   char *__RESTRICT buf, size_t buflen) {
    if(!attr)
        return EINVAL;

    if(!buf)
        return EFAULT;

    memset(buf, 0, buflen);

    if(attr->attr.label) {
        if(buflen < strlen(attr->attr.label)) {
            strcpy(buf, attr->attr.label);
        }
        else {
            memcpy(buf, attr->attr.label, buflen - 1);
            buf[buflen - 1] = 0;
        }
    }

    return 0;
}
