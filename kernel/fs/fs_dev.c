/* KallistiOS ##version##

   fs_dev.c
   Copyright (C) 2023 Luke Benstead
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <errno.h>
#include <time.h>
#include <arch/types.h>
#include <kos/mutex.h>
#include <kos/fs_dev.h>
#include <sys/queue.h>
#include <errno.h>
#include <sys/time.h>

/* This function is declared in <stdlib.h> but behind an if __BSD_VISIBLE
   Declaring as extern here to avoid implicit declaration */
extern void arc4random_buf(void *, size_t);

#if !defined(__NEWLIB__) || (__NEWLIB__ < 2 && __NEWLIB_MINOR__ < 4)
/* Ensure the function is defined on versions of Newlib where it doesn't exist,
   even though this isn't functional at all. This makes sure we don't get any
   linker errors when -ffunction-sections isn't used, for instance. */
void arc4random_buf(void *a, size_t b) {
    (void)a;
    (void)b;
}
#endif

/* File handles */
typedef struct dev_fh_str {
    int mode;                           /* mode the file was opened with */

    TAILQ_ENTRY(dev_fh_str) listent;    /* list entry */
} dev_fh_t;

/* Linked list of open files (controlled by "mutex") */
TAILQ_HEAD(dev_fh_list, dev_fh_str) dev_fh;

/* Thread mutex for dev_fh access */
static mutex_t fh_mutex;


/* openfile function */
static dev_fh_t *dev_open_file(vfs_handler_t * vfs, const char *fn, int mode) {
    (void) vfs;

    if(strcmp(fn, "/urandom") != 0 && strcmp(fn, "/random") != 0) {
        return NULL;
    }

    dev_fh_t    * fd;       /* file descriptor */
    int     realmode;

    /* Malloc a new fh struct */
    fd = malloc(sizeof(dev_fh_t));

    /* Fill in the filehandle struct */
    fd->mode = mode;

    realmode = mode & O_MODE_MASK;

    /* We only allow reading, not writing */
    if(realmode != O_RDONLY) {
        free(fd);
        return NULL;
    }

    return fd;
}

/* open function */
static void * dev_open(vfs_handler_t * vfs, const char *path, int mode) {
    dev_fh_t *fh = dev_open_file(vfs, path, mode);
    if(!fh) {
        return NULL;
    }

    /* link the fh onto the top of the list */
    mutex_lock(&fh_mutex);
    TAILQ_INSERT_TAIL(&dev_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    return (void *)fh;
}

/* Verify that a given hnd is actually in the list */
static int dev_verify_hnd(void * hnd) {
    dev_fh_t    *cur;
    int     rv;

    rv = 0;

    mutex_lock(&fh_mutex);
    TAILQ_FOREACH(cur, &dev_fh, listent) {
        if((void *)cur == hnd) {
            rv = 1;
            break;
        }
    }
    mutex_unlock(&fh_mutex);

    if(rv)
        return 1;
    else
        return 0;
}

/* close a file */
static int dev_close(void * hnd) {
    dev_fh_t *fh;
    int retval = 0;

    /* Check the handle */
    if(!dev_verify_hnd(hnd)) {
        errno = EBADF;
        return -1;
    }

    fh = (dev_fh_t *)hnd;

    /* Look for the one to get rid of */
    mutex_lock(&fh_mutex);
    TAILQ_REMOVE(&dev_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    free(fh);
    return retval;
}

/* read function */
static ssize_t dev_read(void * hnd, void *buffer, size_t cnt) {
    dev_fh_t *fh;
    uint8_t* buf = buffer;

    /* Check the handle */
    if(!dev_verify_hnd(hnd))
        return -1;

    fh = (dev_fh_t *)hnd;

    /* make sure we're opened for reading */
    if((fh->mode & O_MODE_MASK) != O_RDONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return 0;

    arc4random_buf(buf, cnt);

    return cnt;
}

/* write function */
static ssize_t dev_write(void * hnd, const void *buffer, size_t cnt) {
    (void) buffer;
    (void) cnt;

    dev_fh_t    *fh;

    /* Check the handle we were given */
    if(!dev_verify_hnd(hnd))
        return -1;

    fh = (dev_fh_t *)hnd;

    /* Make sure we're opened for writing */
    if((fh->mode & O_MODE_MASK) != O_WRONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return -1;

    dbglog(DBG_ERROR, "RANDOMFS: writing entropy is not supported\n");
    return -1;
}


/* Seek elsewhere in a file */
static off_t dev_seek(void * hnd, off_t offset, int whence) {
    (void) offset;
    (void) whence;

    /* Check the handle */
    if(!dev_verify_hnd(hnd))
        return -1;

    return 0;
}

/* tell the current position in the file */
static off_t dev_tell(void * hnd) {
    /* Check the handle */
    if(!dev_verify_hnd(hnd))
        return -1;

    return 0;
}

/* return the filesize */
static size_t dev_total(void * fd) {
    /* Check the handle */
    if(!dev_verify_hnd(fd))
        return -1;

    /* The size of /dev/urandom always returns 0 */
    return 0;
}


/* Delete a file */
static int dev_unlink(vfs_handler_t * vfs, const char *path) {
    (void) vfs;
    (void) path;

    dbglog(DBG_ERROR, "RANDOMFS: Attempted to delete system file\n");
    return -1;
}

static int dev_stat(vfs_handler_t *vfs, const char *fn, struct stat *rv,
                    int flag) {
    (void)vfs;
    (void)fn;
    (void)flag;

    memset(rv, 0, sizeof(struct stat));
    rv->st_mode = S_IRUSR;
    rv->st_nlink = 1;

    return 0;
}

static int dev_fcntl(void *fd, int cmd, va_list ap) {
    int rv = -1;

    (void)ap;

    /* Check the handle */
    if(!dev_verify_hnd(fd)) {
        errno = EBADF;
        return -1;
    }

    switch(cmd) {
        case F_GETFL:
            rv = O_RDONLY;
            break;

        case F_SETFL:
        case F_GETFD:
        case F_SETFD:
            rv = 0;
            break;

        default:
            errno = EINVAL;
    }

    return rv;
}

static int dev_fstat(void *fd, struct stat *st) {

    /* Check the handle */
    if(!dev_verify_hnd(fd)) {
        errno = EBADF;
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_mode = S_IFREG | S_IRUSR;
    st->st_nlink = 1;

    return 0;
}

/* handler interface */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/dev", /* name */
        0,              /* tbfi */
        0x00010000,     /* Version 1.0 */
        0,              /* flags */
        NMMGR_TYPE_VFS, /* VFS handler */
        NMMGR_LIST_INIT
    },
    0, NULL,            /* In-kernel, privdata */

    dev_open,
    dev_close,
    dev_read,
    dev_write,
    dev_seek,
    dev_tell,
    dev_total,
    NULL,
    NULL,               /* ioctl */
    NULL,               /* rename/move */
    dev_unlink,
    NULL,
    NULL,               /* complete */
    dev_stat,           /* stat */
    NULL,               /* mkdir */
    NULL,               /* rmdir */
    dev_fcntl,
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    NULL,
    dev_fstat
};

int fs_dev_init() {
    TAILQ_INIT(&dev_fh);
    mutex_init(&fh_mutex, MUTEX_TYPE_NORMAL);
    return nmmgr_handler_add(&vh.nmmgr);
}

int fs_dev_shutdown() {
    dev_fh_t * c, * n;

    c = TAILQ_FIRST(&dev_fh);

    while(c) {
        n = TAILQ_NEXT(c, listent);
        free(c);
        c = n;
    }

    mutex_destroy(&fh_mutex);

    return nmmgr_handler_remove(&vh.nmmgr);
}
