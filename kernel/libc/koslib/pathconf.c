/* KallistiOS ##version##

   pathconf.c
   Copyright (C)2026 Yevhen Lohachov
*/

#include <errno.h>
#include <unistd.h>

#include <kos/fs.h>
#include <kos/limits.h>

long pathconf(const char *path, int name) {
    (void)path;
    switch (name)
    {
    case _PC_LINK_MAX:
        return SYMLOOP_MAX;
        break;
    case _PC_NAME_MAX:
        return NAME_MAX;
        break;
    case _PC_PATH_MAX:
        return PATH_MAX;
        break;
    case _PC_CHOWN_RESTRICTED:
        return 1;
        break;
    case _PC_NO_TRUNC:
        return 1;
        break;    
    default:
        errno = EINVAL;
        return -1;
    }
}
