/* KallistiOS ##version##

   telldir.c
   Copyright (C)2004 Megan Potter

*/

#include <kos/dbglog.h>
#include <dirent.h>
#include <errno.h>

off_t telldir(DIR *dir) {
    (void)dir;

    dbglog(DBG_WARNING, "telldir: call ignored\n");
    errno = ENOSYS;
    return -1;
}
