/* KallistiOS ##version##

   uname.c
   Copyright (C) 2018 Lawrence Sebald

*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#include <arch/arch.h>
#include "../../banner.h"

#define UNAME_KERNEL "KallistiOS"

int uname(struct utsname *n) {
    if(!n) {
        errno = EFAULT;
        return -1;
    }

    memset(n, 0, sizeof(struct utsname));
    strcpy(n->sysname, UNAME_KERNEL);
    strcpy(n->release, kern_version);
    snprintf(n->version, 64, "%s %s", UNAME_KERNEL, kern_version);
    strcpy(n->machine, ARCH_NAME);

    return 0;
}
