/* KallistiOS ##version##

   scandir.c
   Copyright (C)2004 Megan Potter

*/

#include <kos/dbglog.h>
#include <errno.h>

// This isn't properly prototyped... sosume :)
int scandir() {
    dbglog(DBG_WARNING, "scandir: call ignored\n");
    errno = ENOSYS;
    return -1;
}
