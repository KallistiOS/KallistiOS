/* KallistiOS ##version##

   telldir.c
   Copyright (C)2004 Megan Potter

*/

#include <kos/dbglog.h>
#include <errno.h>

// This isn't properly prototyped... sosume :)
off_t telldir() {
    dbglog(DBG_WARNING, "telldir: call ignored\n");
    errno = ENOSYS;
    return -1;
}
