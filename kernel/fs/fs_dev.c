/* KallistiOS ##version##

   fs_dev.c
   Copyright (C) 2024 Donald Haase
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <kos/dbglog.h>
#include <kos/fs_dev.h>
#include <sys/queue.h>

/* File handle structure; this is an entirely internal structure so it does
   not go in a header file. */
typedef struct dev_hnd {
    nmmgr_handler_t * handler;
    void *      hnd;        /* Handler-internal */
    int     refcnt;     /* Reference count */
} dev_hnd_t;

static dev_hnd_t dev_root_hnd = {0};
static dirent_t dev_readdir_dirent;

static dirent_t *dev_root_readdir(dev_hnd_t * handle) {
    nmmgr_handler_t *nmhnd;
    nmmgr_list_t    *nmhead;
    int         cnt;

    cnt = (int)handle->hnd;

    nmhead = nmmgr_get_list();

    LIST_FOREACH(nmhnd, nmhead, list_ent) {
        if(!(nmhnd->flags & NMMGR_FLAGS_INDEV))
            continue;

        if(!(cnt--))
            break;
    }

    if(nmhnd == NULL)
        return NULL;

    dev_readdir_dirent.size = -1;

    strcpy(dev_readdir_dirent.name, nmhnd->pathname + strlen("/dev/"));

    handle->hnd = (void *)((int)handle->hnd + 1);

    return &dev_readdir_dirent;
}


static dirent_t *dev_readdir(void *f) {
    dev_hnd_t * hnd = (dev_hnd_t *)f;

    if((!hnd) || (hnd != &dev_root_hnd)
        || (hnd->refcnt <= 0)) {
        errno = EBADF;
        return NULL;
    }

    return dev_root_readdir(hnd);
}

static int dev_rewinddir(void *f) {
    dev_hnd_t * hnd = (dev_hnd_t *)f;

    if((!hnd) || (hnd != &dev_root_hnd)
        || (hnd->refcnt <= 0)) {
        errno = EBADF;
        return -1;
    }

    /* Reset our position */
    hnd->hnd = 0;

    return 0;
}

static void *dev_open(vfs_handler_t *vfs, const char *fn, int mode) {
    (void)vfs;

    if(!strcmp(fn, "/") || !strcmp(fn, "")) {
        if((mode & O_DIR)) {
            dev_root_hnd.refcnt++;
            return &dev_root_hnd;
        }
        else {
            errno = EISDIR;
            return NULL;
        }
    }
    else {
        dbglog(DBG_DEBUG, "fs_dev: open isn't valid for %s\n", fn);
        errno = ENODEV;
        return NULL;
    }
}

/* Close a file and clean up the handle */
static int dev_close(void *f) {
    dev_hnd_t * hnd = (dev_hnd_t *)f;

    if((hnd != &dev_root_hnd)
        || (hnd->refcnt <= 0)) {
      errno = EBADF;
      return -1;
    }

    hnd->refcnt--;

    /* Reset our position */
    if(hnd->refcnt == 0)
        hnd->hnd = 0;

    return 0;
}

static int dev_stat(vfs_handler_t *vfs, const char *path, struct stat *st,
                    int flag) {
    (void)vfs;
    (void)path;
    (void)flag;

    memset(st, 0, sizeof(struct stat));
    st->st_dev = (dev_t)('d' | ('e' << 8) | ('v' << 16));
    st->st_mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | 
        S_IXGRP | S_IROTH | S_IXOTH;
    st->st_size = -1;
    st->st_nlink = 2 + 3; /* 2 + (number of subdirectories) */

    return 0;
}

/* handler interface */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/dev",         /* name */
        0,              /* tbfi */
        0x00010000,     /* Version 1.0 */
        0,              /* flags */
        NMMGR_TYPE_VFS, /* VFS handler */
        NMMGR_LIST_INIT
    },
    0, NULL,            /* In-kernel, privdata */

    dev_open,
    dev_close,
    NULL,               /* read */
    NULL,               /* write */
    NULL,               /* seek */
    NULL,               /* tell */
    NULL,               /* total */
    dev_readdir,
    NULL,               /* ioctl */
    NULL,               /* rename/move */
    NULL,               /* unlink */
    NULL,               /* mmap */
    NULL,               /* complete */
    dev_stat,
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
    dev_rewinddir,
    NULL                /* fstat */
};

void fs_dev_init(void) {
    dev_root_hnd.handler = &vh.nmmgr;
    dev_root_hnd.refcnt = 0;
    nmmgr_handler_add(&vh.nmmgr);
}

void fs_dev_shutdown(void) {
    memset(&dev_root_hnd, 0, sizeof(dev_root_hnd));
    nmmgr_handler_remove(&vh.nmmgr);
}
