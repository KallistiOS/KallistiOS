/* KallistiOS ##version##

   fs_null.c
   Copyright (C) 2024 Donald Haase
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <arch/types.h>
#include <kos/mutex.h>
#include <kos/fs_null.h>
#include <sys/queue.h>

/* File handles */
typedef struct null_fh_str {
    int mode;                            /* mode the file was opened with */

    TAILQ_ENTRY(null_fh_str) listent;    /* list entry */
} null_fh_t;

/* Linked list of open files (controlled by "mutex") */
TAILQ_HEAD(null_fh_list, null_fh_str) null_fh;

/* Thread mutex for null_fh access */
static mutex_t fh_mutex;

/* openfile function */
static null_fh_t *null_open_file(vfs_handler_t *vfs, const char *fn, int mode) {
    (void) vfs;
    (void) fn;

    null_fh_t    * fd;       /* file descriptor */

    /* Malloc a new fh struct */
    fd = malloc(sizeof(null_fh_t));
    if(!fd) {
        errno = ENOMEM;
        return NULL;
    }

    /* Fill in the filehandle struct */
    fd->mode = mode;

    return fd;
}

/* open function */
static void * null_open(vfs_handler_t *vfs, const char *path, int mode) {
    null_fh_t *fh = null_open_file(vfs, path, mode);
    if(!fh) {
        return NULL;
    }

    /* link the fh onto the top of the list */
    mutex_lock(&fh_mutex);
    TAILQ_INSERT_TAIL(&null_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    return (void *)fh;
}

/* Verify that a given hnd is actually in the list */
static int null_verify_hnd(void *hnd) {
    null_fh_t    *cur;
    int     rv = 0;

    mutex_lock(&fh_mutex);
    TAILQ_FOREACH(cur, &null_fh, listent) {
        if((void *)cur == hnd) {
            rv = 1;
            break;
        }
    }
    mutex_unlock(&fh_mutex);

    return rv;
}

/* close a file */
static int null_close(void *hnd) {
    null_fh_t *fh;

    /* Check the handle */
    if(!null_verify_hnd(hnd)) {
        errno = EBADF;
        return -1;
    }

    fh = (null_fh_t *)hnd;

    /* Look for the one to get rid of */
    mutex_lock(&fh_mutex);
    TAILQ_REMOVE(&null_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    free(fh);
    return 0;
}

/* read function */
static ssize_t null_read(void *hnd, void *buffer, size_t cnt) {
    (void)buffer;
    (void)cnt;
    
    null_fh_t *fh;

    /* Check the handle */
    if(!null_verify_hnd(hnd)) {
        errno = EBADF;
        return -1;
    }

    fh = (null_fh_t *)hnd;

    /* make sure we're opened for reading */
    if((fh->mode & O_MODE_MASK) != O_RDONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return 0;

    return 0;
}

/* write function */
static ssize_t null_write(void *hnd, const void *buffer, size_t cnt) {
    (void) buffer;

    null_fh_t    *fh;

    /* Check the handle */
    if(!null_verify_hnd(hnd)) {
        errno = EBADF;
        return -1;
    }

    fh = (null_fh_t *)hnd;

    /* Make sure we're opened for writing */
    if((fh->mode & O_MODE_MASK) != O_WRONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return -1;

    return cnt;
}

/* Seek elsewhere in a file */
static off_t null_seek(void *hnd, off_t offset, int whence) {
    (void) offset;
    (void) whence;

    /* Check the handle */
    if(!null_verify_hnd(hnd))
        return -1;

    return 0;
}

/* tell the current position in the file */
static off_t null_tell(void *hnd) {
    /* Check the handle */
    if(!null_verify_hnd(hnd))
        return -1;

    return 0;
}

/* return the filesize */
static size_t null_total(void *fd) {
    /* Check the handle */
    if(!null_verify_hnd(fd))
        return -1;

    /* The size of /dev/null always returns 0 */
    return 0;
}

static int null_stat(vfs_handler_t *vfs, const char *fn, struct stat *rv,
                    int flag) {
    (void)vfs;
    (void)fn;
    (void)flag;

    memset(rv, 0, sizeof(struct stat));
    rv->st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    rv->st_nlink = 1;

    return 0;
}

static int null_fstat(void *fd, struct stat *st) {

    /* Check the handle */
    if(!null_verify_hnd(fd)) {
        errno = EBADF;
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_mode =  S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    st->st_nlink = 1;

    return 0;
}

/* handler interface */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/dev/null",          /* name */
        0,                      /* tbfi */
        0x00010000,             /* Version 1.0 */
        NMMGR_FLAGS_INDEV,      /* flags */
        NMMGR_TYPE_VFS,         /* VFS handler */
        NMMGR_LIST_INIT
    },
    0, NULL,            /* In-kernel, privdata */

    null_open,
    null_close,
    null_read,
    null_write,
    null_seek,
    null_tell,
    null_total,
    NULL,
    NULL,               /* ioctl */
    NULL,               /* rename/move */
    NULL,               /* unlink */
    NULL,
    NULL,               /* complete */
    null_stat,          /* stat */
    NULL,               /* mkdir */
    NULL,               /* rmdir */
    NULL,               /* fcntl */
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    NULL,
    null_fstat
};

int fs_null_init(void) {
    int rv = 0;
    TAILQ_INIT(&null_fh);
    mutex_init(&fh_mutex, MUTEX_TYPE_NORMAL);

    nmmgr_handler_add(&vh.nmmgr);

    return rv;
}

int fs_null_shutdown(void) {
    null_fh_t * c, * n;

    mutex_lock(&fh_mutex);

    /* First, clean up any open files */
    TAILQ_FOREACH_SAFE(c, &null_fh, listent, n) {
        free(c);
    }

    mutex_unlock(&fh_mutex);
    mutex_destroy(&fh_mutex);

    nmmgr_handler_remove(&vh.nmmgr);

    return 0;
}

