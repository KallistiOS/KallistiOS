/* KallistiOS ##version##

   fs_random.c
   Copyright (C) 2023 Luke Benstead
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#include <arch/types.h>
#include <kos/mutex.h>
#include <kos/fs_random.h>
#include <sys/queue.h>
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
typedef struct rnd_fh_str {
    int mode;                           /* mode the file was opened with */

    TAILQ_ENTRY(rnd_fh_str) listent;    /* list entry */
} rnd_fh_t;

/* Linked list of open files (controlled by "mutex") */
TAILQ_HEAD(rnd_fh_list, rnd_fh_str) rnd_fh;

/* Thread mutex for rnd_fh access */
static mutex_t fh_mutex;

/* openfile function */
static rnd_fh_t *rnd_open_file(vfs_handler_t *vfs, const char *fn, int mode) {
    (void) vfs;
    (void) fn;

    rnd_fh_t    * fd;       /* file descriptor */

    /* We only allow reading, not writing */
    if((mode & O_MODE_MASK) != O_RDONLY) {
        errno = EPERM;
        return NULL;
    }

    /* Malloc a new fh struct */
    fd = malloc(sizeof(rnd_fh_t));
    if(!fd) {
        errno = ENOMEM;
        return NULL;
    }

    /* Fill in the filehandle struct */
    fd->mode = mode;

    return fd;
}

/* open function */
static void * rnd_open(vfs_handler_t *vfs, const char *path, int mode) {
    rnd_fh_t *fh = rnd_open_file(vfs, path, mode);
    if(!fh) {
        return NULL;
    }

    /* link the fh onto the top of the list */
    mutex_lock(&fh_mutex);
    TAILQ_INSERT_TAIL(&rnd_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    return (void *)fh;
}

/* Verify that a given hnd is actually in the list */
static int rnd_verify_hnd(void *hnd) {
    rnd_fh_t    *cur;
    int     rv = 0;

    mutex_lock(&fh_mutex);
    TAILQ_FOREACH(cur, &rnd_fh, listent) {
        if((void *)cur == hnd) {
            rv = 1;
            break;
        }
    }
    mutex_unlock(&fh_mutex);

    return rv;
}

/* close a file */
static int rnd_close(void *hnd) {
    rnd_fh_t *fh;
    int retval = 0;

    /* Check the handle */
    if(!rnd_verify_hnd(hnd)) {
        errno = EBADF;
        return -1;
    }

    fh = (rnd_fh_t *)hnd;

    /* Look for the one to get rid of */
    mutex_lock(&fh_mutex);
    TAILQ_REMOVE(&rnd_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    free(fh);
    return retval;
}

/* read function */
static ssize_t rnd_read(void *hnd, void *buffer, size_t cnt) {
    rnd_fh_t *fh;
    uint8_t* buf = buffer;

    /* Check the handle */
    if(!rnd_verify_hnd(hnd))
        return -1;

    fh = (rnd_fh_t *)hnd;

    /* make sure we're opened for reading */
    if((fh->mode & O_MODE_MASK) != O_RDONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return 0;

    arc4random_buf(buf, cnt);

    return cnt;
}

/* write function */
static ssize_t rnd_write(void *hnd, const void *buffer, size_t cnt) {
    (void) buffer;
    (void) cnt;

    rnd_fh_t    *fh;

    /* Check the handle we were given */
    if(!rnd_verify_hnd(hnd))
        return -1;

    fh = (rnd_fh_t *)hnd;

    /* Make sure we're opened for writing */
    if((fh->mode & O_MODE_MASK) != O_WRONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return -1;

    dbglog(DBG_ERROR, "fs_random: writing entropy is not supported\n");
    return -1;
}


/* Seek elsewhere in a file */
static off_t rnd_seek(void *hnd, off_t offset, int whence) {
    (void) offset;
    (void) whence;

    /* Check the handle */
    if(!rnd_verify_hnd(hnd))
        return -1;

    return 0;
}

/* tell the current position in the file */
static off_t rnd_tell(void *hnd) {
    /* Check the handle */
    if(!rnd_verify_hnd(hnd))
        return -1;

    return 0;
}

/* return the filesize */
static size_t rnd_total(void *fd) {
    /* Check the handle */
    if(!rnd_verify_hnd(fd))
        return -1;

    /* The size of /dev/urandom always returns 0 */
    return 0;
}


/* Delete a file */
static int rnd_unlink(vfs_handler_t *vfs, const char *path) {
    (void) vfs;
    (void) path;

    dbglog(DBG_ERROR, "fs_random: Attempted to delete system file\n");
    return -1;
}

static int rnd_stat(vfs_handler_t *vfs, const char *fn, struct stat *rv,
                    int flag) {
    (void)vfs;
    (void)fn;
    (void)flag;

    memset(rv, 0, sizeof(struct stat));
    rv->st_mode = S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH;
    rv->st_nlink = 1;

    return 0;
}

static int rnd_fcntl(void *fd, int cmd, va_list ap) {
    int rv = -1;

    (void)ap;

    /* Check the handle */
    if(!rnd_verify_hnd(fd)) {
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

static int rnd_fstat(void *fd, struct stat *st) {

    /* Check the handle */
    if(!rnd_verify_hnd(fd)) {
        errno = EBADF;
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_mode = S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH;
    st->st_nlink = 1;

    return 0;
}

/* handler interface */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/dev/random",          /* name */
        0,                      /* tbfi */
        0x00010000,             /* Version 1.0 */
        NMMGR_FLAGS_INDEV,      /* flags */
        NMMGR_TYPE_VFS,         /* VFS handler */
        NMMGR_LIST_INIT
    },
    0, NULL,            /* In-kernel, privdata */

    rnd_open,
    rnd_close,
    rnd_read,
    rnd_write,
    rnd_seek,
    rnd_tell,
    rnd_total,
    NULL,
    NULL,               /* ioctl */
    NULL,               /* rename/move */
    rnd_unlink,
    NULL,
    NULL,               /* complete */
    rnd_stat,
    NULL,               /* mkdir */
    NULL,               /* rmdir */
    rnd_fcntl,
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    NULL,
    rnd_fstat
};

/* alias handler interface */
static alias_handler_t ah_u = {
    {
        "/dev/urandom",         /* name */
        0,                      /* tbfi */
        0x00010000,             /* Version 1.0 */
        NMMGR_FLAGS_INDEV |
        NMMGR_FLAGS_ALIAS ,     /* flags */
        NMMGR_TYPE_VFS,         /* VFS handler */
        NMMGR_LIST_INIT
    },
    &vh.nmmgr                   /* Aliased nmmgr */
};

int fs_rnd_init(void) {
    int rv = 0;
    TAILQ_INIT(&rnd_fh);
    mutex_init(&fh_mutex, MUTEX_TYPE_NORMAL);

    nmmgr_handler_add(&vh.nmmgr);
    nmmgr_handler_add(&ah_u.nmmgr);

    return rv;
}

int fs_rnd_shutdown(void) {
    rnd_fh_t * c, * n;

    mutex_lock(&fh_mutex);

    /* First, clean up any open files */
    TAILQ_FOREACH_SAFE(c, &rnd_fh, listent, n) {
        free(c);
    }

    mutex_unlock(&fh_mutex);
    mutex_destroy(&fh_mutex);

    nmmgr_handler_remove(&vh.nmmgr);
    nmmgr_handler_remove(&ah_u.nmmgr);

    return 0;
}
