/* KallistiOS ##version##

   scandir.c
   Copyright (C)2004 Megan Potter

*/

#include <kos/dbglog.h>
#include <sys/dirent.h>
#include <errno.h>

int scandir(const char *dir, struct dirent ***namelist, 
    int(*filter)(const struct dirent *), 
    int(*compar)(const struct dirent ** , const struct dirent **) ) {

    (void)dir;
    (void)namelist;
    (void)filter;
    (void)compar;

    dbglog(DBG_WARNING, "scandir: call ignored\n");
    errno = ENOSYS;
    return -1;
}
