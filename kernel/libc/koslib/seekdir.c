/* KallistiOS ##version##

   seekdir.c
   Copyright (C)2004 Megan Potter

*/

#include <kos/dbglog.h>
#include <sys/dirent.h>

void seekdir(DIR *dir, off_t offset) {
    (void)dir;
    (void)offset;

    dbglog(DBG_WARNING, "seekdir: call ignored\n");
}
