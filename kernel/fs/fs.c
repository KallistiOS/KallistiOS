/* KallistiOS ##version##

   fs.c
   Copyright (C) 2000, 2001, 2002, 2003 Megan Potter
   Copyright (C) 2012, 2013, 2014, 2015, 2016 Lawrence Sebald
   Copyright (C) 2026 Joseph Black

*/

/*

This module manages all of the file system code. Basically the VFS works
something like this:

- The kernel contains path primitives. There is a table of VFS path handlers
  installed by loaded servers. When the kernel needs to open a file, it will
  search this path handler table from the bottom until it finds a handler
  that is willing to take the request. The request is then handed off to
  the handler. (This function is now handled by the name manager.)
- The path handler receives the part of the path that is left after the
  part in the handler table. The path handler should return an internal
  handle for accessing the file. An internal handle of zero is always
  assumed to mean failure.
- The kernel open function takes this value and wraps it in a structure that
  describes which service handled the request, and its internal handle.
- Subsequent operations go through this abstraction layer to land in the
  right place.

*/

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

#include <kos/fs.h>
#include <kos/thread.h>
#include <kos/mutex.h>
#include <kos/nmmgr.h>
#include <kos/dbglog.h>

/* File handle structure; this is an entirely internal structure so it does
   not go in a header file. */
typedef struct fs_hnd {
    vfs_handler_t *handler;   /* Handler */
    void *hnd;   /* Handler-internal */
    atomic_int refcnt;  /* Descriptor and in-flight operation references */
    int idx;     /* Current index for readdir */
} fs_hnd_t;

/* The global file descriptor table */
fs_hnd_t *fd_table[FD_SETSIZE] = { NULL };
static mutex_t fd_mutex = MUTEX_INITIALIZER;
static bool fs_accepting_descriptors;

/* Internal file commands for root dir reading */
static fs_hnd_t *fs_root_opendir(void) {
    return calloc(1, sizeof(fs_hnd_t));
}

static dirent_t root_readdir_dirent;
static dirent_t *fs_root_readdir(fs_hnd_t *handle) {
    char pathname[NAME_MAX];
    const uintptr_t cnt = (uintptr_t)handle->hnd;

    if(nmmgr_handler_get_path(cnt, NMMGR_TYPE_VFS, 0,
                              NMMGR_FLAGS_INDEV, pathname,
                              sizeof(pathname)) < 0)
        return NULL;

    root_readdir_dirent.attr = O_DIR;
    root_readdir_dirent.size = -1;

    if(pathname[0] == '/')
        strcpy(root_readdir_dirent.name, pathname + 1);
    else
        strcpy(root_readdir_dirent.name, pathname);

    handle->hnd = (void *)((uintptr_t)handle->hnd + 1);

    return &root_readdir_dirent;
}

/* This version of open deals with raw handles only. This is below the level
   of file descriptors. It is used by the standard fs_open below. The
   returned handle will have no references attached to it. */
static fs_hnd_t *fs_hnd_open(const char *fn, int mode) {
    nmmgr_handler_t *nmhnd;
    vfs_handler_t   *cur;
    const char  *cname;
    void        *h;
    fs_hnd_t    *hnd;
    char        rfn[PATH_MAX];

    if(!fs_normalize_path(fn, rfn))
        return NULL;

    /* Are they trying to open the root? */
    if(!strcmp(rfn, "/")) {
        if((mode & O_DIR))
            return fs_root_opendir();
        else {
            errno = EISDIR;
            return NULL;
        }
    }

    /* Look for a handler */
    nmhnd = nmmgr_lookup_ref(rfn);

    if(nmhnd == NULL || nmhnd->type != NMMGR_TYPE_VFS) {
        if(nmhnd)
            nmmgr_handler_release(nmhnd);

        errno = ENOENT;
        return NULL;
    }

    cur = (vfs_handler_t *)nmhnd;

    /* Found one -- get the "canonical" path name */
    cname = rfn + strlen(nmhnd->pathname);

    /* Invoke the handler */
    if(cur->open == NULL) {
        nmmgr_handler_release(nmhnd);
        errno = ENOSYS;
        return NULL;
    }

    h = cur->open(cur, cname, mode);

    if(h == NULL) {
        nmmgr_handler_release(nmhnd);
        return NULL;
    }

    /* Wrap it up in a structure */
    hnd = malloc(sizeof(fs_hnd_t));

    if(hnd == NULL) {
        if(cur->close)
            cur->close(h);

        nmmgr_handler_release(nmhnd);
        errno = ENOMEM;
        return NULL;
    }

    hnd->handler = cur;
    hnd->hnd = h;
    hnd->refcnt = 0;
    hnd->idx = -2;

    return hnd;
}

/* Reference a file handle. This should be called when a persistent reference
   to a raw handle is created somewhere. */
static void fs_hnd_ref(fs_hnd_t *ref) {
    assert(ref);
    assert(ref->refcnt < (1 << 30));

    atomic_fetch_add(&ref->refcnt, 1);
}

/* Unreference a file handle. Should be called when a persistent reference
   to a raw handle is no longer applicable. This function may destroy the
   file handle, so under no circumstances should you presume that it will
   still exist later. */
static int fs_hnd_unref(fs_hnd_t *ref) {
    int retval = 0;
    assert(ref);
    assert(ref->refcnt > 0);

    if(atomic_fetch_sub(&ref->refcnt, 1) == 1) {
        if(ref->handler && ref->handler->close)
            retval = ref->handler->close(ref->hnd);

        const int saved_errno = errno;
        if(ref->handler)
            nmmgr_handler_release(&ref->handler->nmmgr);

        free(ref);
        errno = saved_errno;
    }

    return retval;
}

/* Assign a descriptor and acquire its reference only on success.
   The caller owns the wrapper and underlying handle on failure. */
static file_t fs_hnd_assign(fs_hnd_t *hnd) {
    int i;

    if(mutex_lock(&fd_mutex) < 0) {
        return FILEHND_INVALID;
    }

    if(!fs_accepting_descriptors) {
        mutex_unlock(&fd_mutex);
        errno = ENODEV;
        return FILEHND_INVALID;
    }

    for(i = 3; i < FD_SETSIZE; i++) {
        if(!fd_table[i]) {
            fs_hnd_ref(hnd);
            fd_table[i] = hnd;
            break;
        }
    }

    mutex_unlock(&fd_mutex);

    if(i >= FD_SETSIZE) {
        dbglog(DBG_ERROR, "fs_hnd_assign: Update FD_SETSIZE definition in \
              opts.h to support additional files being opened. Current \
              limit is %d\n", FD_SETSIZE);

        errno = EMFILE;
        return FILEHND_INVALID;
    }

    return (file_t)i;
}

static int fs_fdtbl_drain(bool stop_new_descriptors) {
    fs_hnd_t *detached[FD_SETSIZE];

    if(mutex_lock(&fd_mutex) < 0)
        return -1;

    if(stop_new_descriptors)
        fs_accepting_descriptors = false;

    for(size_t i = 0; i < FD_SETSIZE; i++) {
        detached[i] = fd_table[i];
        fd_table[i] = NULL;
    }

    mutex_unlock(&fd_mutex);

    for(size_t i = 0; i < FD_SETSIZE; i++) {
        if(detached[i])
            fs_hnd_unref(detached[i]);
    }

    return 0;
}

int fs_fdtbl_destroy(void) {
    return fs_fdtbl_drain(false);
}

/* Attempt to open a file, given a path name. Follows the process described
   in the above comments. */
file_t fs_open(const char *fn, int mode) {
    /* First try to open the file handle */
    fs_hnd_t *hnd = fs_hnd_open(fn, mode);

    if(!hnd)
        return FILEHND_INVALID;

    /* Ok, that succeeded -- now look for a file descriptor. */
    file_t fd = fs_hnd_assign(hnd);
    if(fd == FILEHND_INVALID) {
        const int saved_errno = errno;
        fs_hnd_ref(hnd);
        fs_hnd_unref(hnd);
        errno = saved_errno;
    }
    return fd;
}

/* See header for comments */
file_t fs_open_handle(vfs_handler_t *vfs, void *vhnd) {
    /* Wrap it up in a structure */
    fs_hnd_t *hnd = malloc(sizeof(fs_hnd_t));

    if(hnd == NULL) {
        errno = ENOMEM;
        return FILEHND_INVALID;
    }

    if(!vfs || nmmgr_handler_retain(&vfs->nmmgr) < 0) {
        free(hnd);
        errno = ENODEV;
        return FILEHND_INVALID;
    }

    hnd->handler = vfs;
    hnd->hnd = vhnd;
    hnd->refcnt = 0;
    hnd->idx = -2;

    /* Ok, that succeeded -- now look for a file descriptor. */
    file_t fd = fs_hnd_assign(hnd);
    if(fd == FILEHND_INVALID) {
        const int saved_errno = errno;
        nmmgr_handler_release(&vfs->nmmgr);
        free(hnd);
        errno = saved_errno;
    }
    return fd;
}

/* Returns a file handle for a given fd, or NULL if the parameters
   are not valid. */
static fs_hnd_t *fs_map_hnd_ref(file_t fd) {
    fs_hnd_t *hnd;

    if(fd < 0 || fd >= FD_SETSIZE || fd == FILEHND_INVALID) {
        errno = EBADF;
        return NULL;
    }

    if(mutex_lock(&fd_mutex) < 0)
        return NULL;

    hnd = fd_table[fd];

    if(hnd)
        fs_hnd_ref(hnd);

    mutex_unlock(&fd_mutex);

    if(!hnd) {
        errno = EBADF;
        return NULL;
    }

    return hnd;
}

static void fs_hnd_cleanup(fs_hnd_t **hnd) {
    const int saved_errno = errno;
    if(*hnd)
        fs_hnd_unref(*hnd);
    errno = saved_errno;
}

vfs_handler_t *fs_get_handler(file_t fd) {
    vfs_handler_t *handler;
    bool exists;

    if(fd < 0 || fd >= FD_SETSIZE || fd == FILEHND_INVALID) {
        errno = EBADF;
        return NULL;
    }

    if(mutex_lock(&fd_mutex) < 0)
        return NULL;
    exists = fd_table[fd] != NULL;
    handler = exists ? fd_table[fd]->handler : NULL;
    mutex_unlock(&fd_mutex);

    if(!exists)
        errno = EBADF;

    return handler;
}

void *fs_get_handle(file_t fd) {
    void *handle;
    bool exists;

    if(fd < 0 || fd >= FD_SETSIZE || fd == FILEHND_INVALID) {
        errno = EBADF;
        return NULL;
    }

    if(mutex_lock(&fd_mutex) < 0)
        return NULL;
    exists = fd_table[fd] != NULL;
    handle = exists ? fd_table[fd]->hnd : NULL;
    mutex_unlock(&fd_mutex);

    if(!exists)
        errno = EBADF;

    return handle;
}

file_t fs_dup(file_t oldfd) {
    fs_hnd_t *hnd;
    int newfd;

    if(oldfd < 0 || oldfd >= FD_SETSIZE || oldfd == FILEHND_INVALID) {
        errno = EBADF;
        return FILEHND_INVALID;
    }

    if(mutex_lock(&fd_mutex) < 0)
        return FILEHND_INVALID;

    hnd = fd_table[oldfd];

    if(!hnd) {
        mutex_unlock(&fd_mutex);
        errno = EBADF;
        return FILEHND_INVALID;
    }

    for(newfd = 3; newfd < FD_SETSIZE; ++newfd) {
        if(!fd_table[newfd]) {
            fs_hnd_ref(hnd);
            fd_table[newfd] = hnd;
            break;
        }
    }

    mutex_unlock(&fd_mutex);

    if(newfd >= FD_SETSIZE) {
        errno = EMFILE;
        return FILEHND_INVALID;
    }

    return newfd;
}

file_t fs_dup2(file_t oldfd, file_t newfd) {
    fs_hnd_t *prev;
    fs_hnd_t *hnd;

    /* Make sure the descriptors are valid */
    if(oldfd < 0 || oldfd >= FD_SETSIZE || oldfd == FILEHND_INVALID ||
       newfd < 0 || newfd >= FD_SETSIZE || newfd == FILEHND_INVALID) {
        errno = EBADF;
        return FILEHND_INVALID;
    }
    if(mutex_lock(&fd_mutex) < 0)
        return FILEHND_INVALID;

    hnd = fd_table[oldfd];

    if(!hnd) {
        mutex_unlock(&fd_mutex);
        errno = EBADF;
        return FILEHND_INVALID;
    }

    if(oldfd == newfd) {
        mutex_unlock(&fd_mutex);
        return newfd;
    }

    fs_hnd_ref(hnd);
    prev = fd_table[newfd];
    fd_table[newfd] = hnd;
    mutex_unlock(&fd_mutex);

    if(prev)
        fs_hnd_unref(prev);

    return newfd;
}

/* Close a file and clean up the handle */
int fs_close(file_t fd) {
    int retval;
    fs_hnd_t *h;

    if(fd < 0 || fd >= FD_SETSIZE || fd == FILEHND_INVALID) {
        errno = EBADF;
        return -1;
    }

    if(mutex_lock(&fd_mutex) < 0)
        return -1;

    h = fd_table[fd];

    if(!h) {
        mutex_unlock(&fd_mutex);
        errno = EBADF;
        return -1;
    }

    /* Detach first so a concurrent operation can no longer acquire a
       temporary reference to a handle whose final close is in progress. */
    fd_table[fd] = NULL;
    mutex_unlock(&fd_mutex);

    retval = fs_hnd_unref(h);
    return retval ? -1 : 0;
}

/* The rest of these pretty much map straight through */
ssize_t fs_read(file_t fd, void *buffer, size_t cnt) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(h->handler == NULL || h->handler->read == NULL) {
        errno = EINVAL;
        return -1;
    }

    return h->handler->read(h->hnd, buffer, cnt);
}

ssize_t fs_write(file_t fd, const void *buffer, size_t cnt) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(h->handler == NULL || h->handler->write == NULL) {
        errno = EINVAL;
        return -1;
    }

    return h->handler->write(h->hnd, buffer, cnt);
}

off_t fs_seek(file_t fd, off_t offset, int whence) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(h->handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Prefer the 32-bit version, but fall back if needed to the 64-bit one. */
    if(h->handler->seek)
        return h->handler->seek(h->hnd, offset, whence);
    else if(h->handler->seek64)
        return (off_t)h->handler->seek64(h->hnd, (_off64_t)offset, whence);

    errno = EINVAL;
    return -1;
}

_off64_t fs_seek64(file_t fd, _off64_t offset, int whence) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(h->handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Prefer the 64-bit version, but fall back if needed to the 32-bit one. */
    if(h->handler->seek64)
        return h->handler->seek64(h->hnd, offset, whence);
    else if(h->handler->seek)
        return (_off64_t)h->handler->seek(h->hnd, (off_t)offset, whence);

    errno = EINVAL;
    return -1;
}

off_t fs_tell(file_t fd) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(h->handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Prefer the 32-bit version, but fall back if needed to the 64-bit one. */
    if(h->handler->tell)
        return h->handler->tell(h->hnd);
    else if(h->handler->tell64)
        return (off_t)h->handler->tell64(h->hnd);

    errno = EINVAL;
    return -1;
}

_off64_t fs_tell64(file_t fd) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(h->handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Prefer the 64-bit version, but fall back if needed to the 32-bit one. */
    if(h->handler->tell64)
        return h->handler->tell64(h->hnd);
    else if(h->handler->tell)
        return (_off64_t)h->handler->tell(h->hnd);

    errno = EINVAL;
    return -1;
}

ssize_t fs_total(file_t fd) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(h->handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Prefer the 32-bit version, but fall back if needed to the 64-bit one. */
    if(h->handler->total)
        return (ssize_t)h->handler->total(h->hnd);
    else if(h->handler->total64)
        return (ssize_t)h->handler->total64(h->hnd);

    errno = EINVAL;
    return -1;
}

int64_t fs_total64(file_t fd) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(h->handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Prefer the 64-bit version, but fall back if needed to the 32-bit one. */
    if(h->handler->total64)
        return (int64_t)h->handler->total64(h->hnd);
    else if(h->handler->total)
        return (int64_t)h->handler->total(h->hnd);

    errno = EINVAL;
    return -1;
}

static const dirent_t dot_dirent = {
    .name = ".",
    .attr = O_DIR,
    .size = -1,
    .time = 0,
};

static const dirent_t dotdot_dirent = {
    .name = "..",
    .attr = O_DIR,
    .size = -1,
    .time = 0,
};

const dirent_t *fs_readdir(file_t fd) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);
    const dirent_t *dirent;

    if(!h) return NULL;

    if(h->handler == NULL)
        return fs_root_readdir(h);

    if(h->handler->readdir == NULL) {
        errno = ENOSYS;
        return NULL;
    }

    switch (h->idx) {
        case -2:
            h->idx++;
            /* Send . directory first */
            return &dot_dirent;
        case -1:
            h->idx++;
            /* Send .. directory second */
            return &dotdot_dirent;
        default:
            for(;; h->idx++) {
                dirent = h->handler->readdir(h->hnd);
                if(!dirent ||
                   (strcmp(dirent->name, ".") && strcmp(dirent->name, "..")))
                    return dirent;
            }
    }
}

int fs_vioctl(file_t fd, int cmd, va_list ap) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(!h->handler || !h->handler->ioctl) {
        errno = EINVAL;
        return -1;
    }

    return h->handler->ioctl(h->hnd, cmd, ap);
}

int fs_ioctl(file_t fd, int cmd, ...) {
    va_list ap;
    int rv;

    va_start(ap, cmd);
    rv = fs_vioctl(fd, cmd, ap);
    va_end(ap);
    return rv;
}

static void fs_vfs_cleanup(vfs_handler_t **handler) {
    const int saved_errno = errno;
    if(*handler)
        nmmgr_handler_release(&(*handler)->nmmgr);
    errno = saved_errno;
}

static vfs_handler_t *fs_verify_handler_ref(const char *fn) {
    nmmgr_handler_t *nh = nmmgr_lookup_ref(fn);

    if(nh == NULL)
        return NULL;

    if(nh->type != NMMGR_TYPE_VFS) {
        nmmgr_handler_release(nh);
        errno = ENOENT;
        return NULL;
    }

    return (vfs_handler_t *)nh;
}

int fs_rename(const char *fn1, const char *fn2) {
    vfs_handler_t *fh1 __attribute__((cleanup(fs_vfs_cleanup))) = NULL;
    vfs_handler_t *fh2 __attribute__((cleanup(fs_vfs_cleanup))) = NULL;
    char        rfn1[PATH_MAX], rfn2[PATH_MAX];

    if(!fs_normalize_path(fn1, rfn1) || !fs_normalize_path(fn2, rfn2))
        return -1;

    /* Look for handlers */
    fh1 = fs_verify_handler_ref(rfn1);

    if(fh1 == NULL) {
        errno = ENOENT;
        return -1;
    }

    fh2 = fs_verify_handler_ref(rfn2);

    if(fh2 == NULL) {
        errno = ENOENT;
        return -1;
    }

    if(fh1 != fh2) {
        errno = EXDEV;
        return -1;
    }

    if(fh1->rename)
        return fh1->rename(fh1, rfn1 + strlen(fh1->nmmgr.pathname),
                           rfn2 + strlen(fh1->nmmgr.pathname));
    else {
        errno = EINVAL;
        return -1;
    }
}

int fs_unlink(const char *fn) {
    vfs_handler_t *cur __attribute__((cleanup(fs_vfs_cleanup))) = NULL;
    char        rfn[PATH_MAX];

    if(!fs_normalize_path(fn, rfn))
        return -1;

    /* Look for a handler */
    cur = fs_verify_handler_ref(rfn);

    if(cur == NULL) return 1;

    if(cur->unlink)
        return cur->unlink(cur, rfn + strlen(cur->nmmgr.pathname));
    else {
        errno = EINVAL;
        return -1;
    }
}

int fs_chdir(const char *fn) {
    char        rfn[PATH_MAX];

    if(!fs_normalize_path(fn, rfn))
        return -1;

    thd_set_pwd(thd_get_current(), rfn);
    return 0;
}

const char *fs_getwd(void) {
    return thd_get_pwd(thd_get_current());
}

void *fs_mmap(file_t fd) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return NULL;

    if(h->handler == NULL || h->handler->mmap == NULL) {
        errno = EINVAL;
        return NULL;
    }

    return h->handler->mmap(h->hnd);
}

int fs_complete(file_t fd, ssize_t *rv) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(h->handler == NULL || h->handler->complete == NULL) {
        errno = EINVAL;
        return -1;
    }

    return h->handler->complete(h->hnd, rv);
}

int fs_mkdir(const char *fn) {
    vfs_handler_t *cur __attribute__((cleanup(fs_vfs_cleanup))) = NULL;
    char        rfn[PATH_MAX];

    if(!fs_normalize_path(fn, rfn))
        return -1;

    /* Look for a handler */
    cur = fs_verify_handler_ref(rfn);

    if(cur == NULL) return -1;

    if(cur->mkdir)
        return cur->mkdir(cur, rfn + strlen(cur->nmmgr.pathname));
    else {
        errno = EINVAL;
        return -1;
    }
}

int fs_rmdir(const char *fn) {
    vfs_handler_t *cur __attribute__((cleanup(fs_vfs_cleanup))) = NULL;
    char        rfn[PATH_MAX];

    if(!fs_normalize_path(fn, rfn))
        return -1;

    /* Look for a handler */
    cur = fs_verify_handler_ref(rfn);

    if(cur == NULL) return -1;

    if(cur->rmdir)
        return cur->rmdir(cur, rfn + strlen(cur->nmmgr.pathname));
    else {
        errno = EINVAL;
        return -1;
    }
}

static int fs_vfcntl(file_t fd, int cmd, va_list ap) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(!h->handler || !h->handler->fcntl) {
        errno = ENOSYS;
        return -1;
    }

    return h->handler->fcntl(h->hnd, cmd, ap);
}

int fs_fcntl(file_t fd, int cmd, ...) {
    va_list ap;
    int rv;

    va_start(ap, cmd);
    rv = fs_vfcntl(fd, cmd, ap);
    va_end(ap);
    return rv;
}

int fs_link(const char *path1, const char *path2) {
    vfs_handler_t *fh1 __attribute__((cleanup(fs_vfs_cleanup))) = NULL;
    vfs_handler_t *fh2 __attribute__((cleanup(fs_vfs_cleanup))) = NULL;
    char rfn1[PATH_MAX], rfn2[PATH_MAX];

    if(!fs_normalize_path(path1, rfn1) || !fs_normalize_path(path2, rfn2))
        return -1;

    /* Look for handlers */
    fh1 = fs_verify_handler_ref(rfn1);

    if(!fh1) {
        errno = ENOENT;
        return -1;
    }

    fh2 = fs_verify_handler_ref(rfn2);

    if(!fh2) {
        errno = ENOENT;
        return -1;
    }

    if(fh1 != fh2) {
        errno = EXDEV;
        return -1;
    }

    if(fh1->link) {
        return fh1->link(fh1, rfn1 + strlen(fh1->nmmgr.pathname),
                         rfn2 + strlen(fh1->nmmgr.pathname));
    }
    else {
        errno = EMLINK;
        return -1;
    }
}

int fs_symlink(const char *path1, const char *path2) {
    vfs_handler_t *vfs __attribute__((cleanup(fs_vfs_cleanup))) = NULL;
    char rfn[PATH_MAX];

    if(!fs_normalize_path(path2, rfn))
        return -1;

    /* Look for the handler */
    vfs = fs_verify_handler_ref(rfn);

    if(!vfs) {
        errno = ENOENT;
        return -1;
    }

    if(vfs->symlink) {
        return vfs->symlink(vfs, path1, rfn + strlen(vfs->nmmgr.pathname));
    }
    else {
        errno = ENOSYS;
        return -1;
    }
}

ssize_t fs_readlink(const char *path, char *buf, size_t bufsize) {
    vfs_handler_t *vfs __attribute__((cleanup(fs_vfs_cleanup))) = NULL;
    char fullpath[PATH_MAX];

    if(!fs_normalize_path(path, fullpath))
        return -1;

    /* Look for the handler */
    vfs = fs_verify_handler_ref(fullpath);

    if(!vfs) {
        errno = ENOENT;
        return -1;
    }

    if(vfs->readlink) {
        return vfs->readlink(vfs, fullpath + strlen(vfs->nmmgr.pathname), buf,
                             bufsize);
    }
    else {
        errno = ENOSYS;
        return -1;
    }
}

int fs_stat(const char *path, struct stat *st, int flag) {
    vfs_handler_t *vfs __attribute__((cleanup(fs_vfs_cleanup))) = NULL;
    char fullpath[PATH_MAX];

    /* Verify the input... */
    if(!st || !path) {
        errno = EFAULT;
        return -1;
    }
    else if(flag & (~AT_SYMLINK_NOFOLLOW)) {
        errno = EINVAL;
        return -1;
    }

    if(!fs_normalize_path(path, fullpath))
        return -1;

    /* The VFS root has no backing handler so stat it directly as a directory */
    if(!strcmp(fullpath, "/")) {
        *st = (struct stat) {
            .st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO,
            .st_size = -1,
            .st_nlink = 2
        };
        return 0;
    }

    /* Look for the handler */
    vfs = fs_verify_handler_ref(fullpath);

    if(!vfs) {
        errno = ENOENT;
        return -1;
    }

    if(vfs->stat) {
        return vfs->stat(vfs, fullpath + strlen(vfs->nmmgr.pathname), st,
                         flag);
    }
    else if(!strcmp(path, vfs->nmmgr.pathname)) {
        /* no vfs->stat - handle stat() on the mount folder */
        *st = (struct stat) {
            .st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO,
            .st_size = -1,
            .st_nlink = 2
        };
        return 0;
    }
    else {
        errno = ENOSYS;
        return -1;
    }
}

int fs_rewinddir(file_t fd) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(h->handler == NULL) {
        h->hnd = (void *)0;
        return 0;
    }

    if(h->handler->rewinddir == NULL) {
        errno = ENOSYS;
        return -1;
    }

    h->idx = -2;

    return h->handler->rewinddir(h->hnd);
}

int fs_fstat(file_t fd, struct stat *st) {
    fs_hnd_t *h __attribute__((cleanup(fs_hnd_cleanup))) =
        fs_map_hnd_ref(fd);

    if(!h) return -1;

    if(!st) {
        errno = EFAULT;
        return -1;
    }

    /* The VFS root has no backing handler so stat it directly as a directory */
    if(h->handler == NULL) {
        *st = (struct stat) {
            .st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO,
            .st_size = -1,
            .st_nlink = 2
        };
        return 0;
    }

    if(h->handler->fstat == NULL) {
        errno = ENOSYS;
        return -1;
    }

    return h->handler->fstat(h->hnd, st);
}

/* Initialize FS structures */
void fs_init(void) {
    mutex_lock(&fd_mutex);
    fs_accepting_descriptors = true;
    mutex_unlock(&fd_mutex);
}

void fs_shutdown(void) {
    fs_fdtbl_drain(true);
}
