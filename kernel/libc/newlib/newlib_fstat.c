/* KallistiOS ##version##

   newlib_fstat.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2016 Lawrence Sebald

*/

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <kos/fs.h>

int _fstat_r(struct _reent *reent, int fd, struct stat *pstat) {
    int err = errno, rv;
    size_t sz;

    (void)reent;

    /* Try to use the native stat function first... */
    if(!(rv = fs_fstat(fd, pstat)) || errno != ENOSYS)
        return rv;

    /* Set up our fallback behavior, which is to do what we've always done. */
    memset(pstat, 0, sizeof(struct stat));
    pstat->st_mode = S_IFCHR;

    /* Can we get some information out of total/fcntl? */
    if((sz = fs_total(fd)) == (size_t)-1) {
        errno = err;
        return 0;
    }

    if((rv = fs_fcntl(fd, F_GETFL)) == -1) {
        errno = err;
        return 0;
    }

    /* fs_fcntl and fs_total succeeded here, so let's look at what we have and
       try to do something useful with it. */
    pstat->st_size = sz;
    pstat->st_dev = (dev_t)0xBADC0DE;

    if(rv & O_DIR)
        pstat->st_mode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR |
            S_IXGRP | S_IXOTH;
    else
        pstat->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;

    if(rv & O_WRONLY)
        pstat->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;

    return 0;
}
