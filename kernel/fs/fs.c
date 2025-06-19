/* KallistiOS ##version##

   fs.c
   Copyright (C) 2000, 2001, 2002, 2003 Megan Potter
   Copyright (C) 2012, 2013, 2014, 2015, 2016 Lawrence Sebald

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
#include <kos/dbgio.h>
#include <kos/dbglog.h>

/* File handle structure; this is an entirely internal structure so it does
   not go in a header file. */
typedef struct fs_hnd {
    vfs_handler_t *handler;   /* Handler */
    void *hnd;   /* Handler-internal */
    int refcnt;  /* Reference count */
    int idx;     /* Current index for readdir */
} fs_hnd_t;

/* The global file descriptor table */
fs_hnd_t * fd_table[FD_SETSIZE] = { NULL };

/* Internal file commands for root dir reading */
static fs_hnd_t * fs_root_opendir(void) {
    return calloc(1, sizeof(fs_hnd_t));
}

/* Not thread-safe right now */
static dirent_t root_readdir_dirent;
static dirent_t *fs_root_readdir(fs_hnd_t * handle) {
    nmmgr_handler_t *nmhnd;
    nmmgr_list_t    *nmhead;
    int         cnt;

    cnt = (int)handle->hnd;

    nmhead = nmmgr_get_list();

    LIST_FOREACH(nmhnd, nmhead, list_ent) {
        if((nmhnd->flags & NMMGR_FLAGS_INDEV))
            continue;

        if(nmhnd->type != NMMGR_TYPE_VFS)
            continue;

        if(!(cnt--))
            break;
    }

    if(nmhnd == NULL)
        return NULL;

    root_readdir_dirent.attr = O_DIR;
    root_readdir_dirent.size = -1;

    if(nmhnd->pathname[0] == '/')
        strcpy(root_readdir_dirent.name, nmhnd->pathname + 1);
    else
        strcpy(root_readdir_dirent.name, nmhnd->pathname);

    handle->hnd = (void *)((int)handle->hnd + 1);

    return &root_readdir_dirent;
}

/* This version of open deals with raw handles only. This is below the level
   of file descriptors. It is used by the standard fs_open below. The
   returned handle will have no references attached to it. */
static fs_hnd_t * fs_hnd_open(const char *fn, int mode) {
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
    nmhnd = nmmgr_lookup(rfn);

    if(nmhnd == NULL || nmhnd->type != NMMGR_TYPE_VFS) {
        errno = ENOENT;
        return NULL;
    }

    cur = (vfs_handler_t *)nmhnd;

    /* Found one -- get the "canonical" path name */
    cname = rfn + strlen(nmhnd->pathname);

    /* Invoke the handler */
    if(cur->open == NULL) {
        errno = ENOSYS;
        return NULL;
    }

    h = cur->open(cur, cname, mode);

    if(h == NULL) return NULL;

    /* Wrap it up in a structure */
    hnd = malloc(sizeof(fs_hnd_t));

    if(hnd == NULL) {
        cur->close(h);
        errno = ENOMEM;
        return NULL;
    }

    hnd->handler = cur;
    hnd->hnd = h;
    hnd->refcnt = 0;

    return hnd;
}

/* Reference a file handle. This should be called when a persistent reference
   to a raw handle is created somewhere. */
static void fs_hnd_ref(fs_hnd_t * ref) {
    assert(ref);
    assert(ref->refcnt < (1 << 30));
    ref->refcnt++;
}

/* Unreference a file handle. Should be called when a persistent reference
   to a raw handle is no longer applicable. This function may destroy the
   file handle, so under no circumstances should you presume that it will
   still exist later. */
static int fs_hnd_unref(fs_hnd_t *ref) {
    int retval = 0;
    assert(ref);
    assert(ref->refcnt > 0);

    if(--ref->refcnt > 0)
        return retval; /* Still references left, nothing to do */

    if(ref->handler && ref->handler->close)
        retval = ref->handler->close(ref->hnd);

    free(ref);
    return retval;
}

/* Assigns a file descriptor (index) to a file handle (pointer). Will auto-
   reference the handle, and unrefs on error. */
static int fs_hnd_assign(fs_hnd_t *hnd) {
    int i;

    fs_hnd_ref(hnd);

    /* XXX Not thread-safe! */
    for(i = 0; i < FD_SETSIZE; i++)
        if(!fd_table[i])
            break;

    if(i >= FD_SETSIZE) {
        dbglog(DBG_ERROR, "fs_hnd_assign: Update FD_SETSIZE definition in \
              opts.h to support additional files being opened. Current \
              limit is %d\n", FD_SETSIZE);

        fs_hnd_unref(hnd);
        errno = EMFILE;
        return -1;
    }

    fd_table[i] = hnd;

    return i;
}

int fs_fdtbl_destroy(void) {
    int i;

    for(i = 0; i < FD_SETSIZE; i++) {
        if(fd_table[i])
            fs_hnd_unref(fd_table[i]);

        fd_table[i] = NULL;
    }

    return 0;
}

/* Attempt to open a file, given a path name. Follows the process described
   in the above comments. */
file_t fs_open(const char *fn, int mode) {
    fs_hnd_t * hnd;

    /* First try to open the file handle */
    hnd = fs_hnd_open(fn, mode);

    if(!hnd)
        return -1;

    /* Ok, that succeeded -- now look for a file descriptor. */
    return fs_hnd_assign(hnd);
}

/* See header for comments */
file_t fs_open_handle(vfs_handler_t * vfs, void * vhnd) {
    fs_hnd_t * hnd;

    /* Wrap it up in a structure */
    hnd = malloc(sizeof(fs_hnd_t));

    if(hnd == NULL) {
        errno = ENOMEM;
        return -1;
    }

    hnd->handler = vfs;
    hnd->hnd = vhnd;
    hnd->refcnt = 0;

    /* Ok, that succeeded -- now look for a file descriptor. */
    return fs_hnd_assign(hnd);
}

vfs_handler_t * fs_get_handler(file_t fd) {
    /* Make sure it exists */
    if(!fd_table[fd]) {
        errno = EBADF;
        return NULL;
    }

    return fd_table[fd]->handler;
}

void * fs_get_handle(file_t fd) {
    /* Make sure it exists */
    if(!fd_table[fd]) {
        errno = EBADF;
        return NULL;
    }

    return fd_table[fd]->hnd;
}

file_t fs_dup(file_t oldfd) {
    /* Make sure it exists */
    if(oldfd < 0 || oldfd >= FD_SETSIZE) {
        errno = EBADF;
        return -1;
    }
    else if(!fd_table[oldfd]) {
        errno = EBADF;
        return -1;
    }

    return fs_hnd_assign(fd_table[oldfd]);
}

file_t fs_dup2(file_t oldfd, file_t newfd) {
    /* Make sure the descriptors are valid */
    if(oldfd < 0 || oldfd >= FD_SETSIZE || newfd < 0 || newfd >= FD_SETSIZE) {
        errno = EBADF;
        return -1;
    }
    else if(!fd_table[oldfd]) {
        errno = EBADF;
        return -1;
    }

    if(fd_table[newfd])
        fs_close(newfd);

    fd_table[newfd] = fd_table[oldfd];
    fs_hnd_ref(fd_table[newfd]);

    return newfd;
}

/* Returns a file handle for a given fd, or NULL if the parameters
   are not valid. */
static fs_hnd_t * fs_map_hnd(file_t fd) {
    if(fd < 0 || fd >= FD_SETSIZE) {
        errno = EBADF;
        return NULL;
    }

    if(!fd_table[fd]) {
        errno = EBADF;
        return NULL;
    }

    return fd_table[fd];
}

/* Close a file and clean up the handle */
int fs_close(file_t fd) {
    int retval;
    fs_hnd_t *h = fs_map_hnd(fd);

    if(!h) return -1;

    /* Deref it and remove it from our table */
    retval = fs_hnd_unref(h);

    fd_table[fd] = NULL;
    return retval ? -1 : 0;
}

/* The rest of these pretty much map straight through */
ssize_t fs_read(file_t fd, void *buffer, size_t cnt) {
    fs_hnd_t *h = fs_map_hnd(fd);

    if(!h) return -1;

    if(h->handler == NULL || h->handler->read == NULL) {
        errno = EINVAL;
        return -1;
    }

    return h->handler->read(h->hnd, buffer, cnt);
}

ssize_t fs_write(file_t fd, const void *buffer, size_t cnt) {
    fs_hnd_t *h;

    h = fs_map_hnd(fd);

    if(!h) return -1;

    if(h->handler == NULL || h->handler->write == NULL) {
        errno = EINVAL;
        return -1;
    }

    return h->handler->write(h->hnd, buffer, cnt);
}

off_t fs_seek(file_t fd, off_t offset, int whence) {
    fs_hnd_t *h = fs_map_hnd(fd);

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
    fs_hnd_t *h = fs_map_hnd(fd);

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
    fs_hnd_t *h = fs_map_hnd(fd);

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
    fs_hnd_t *h = fs_map_hnd(fd);

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

size_t fs_total(file_t fd) {
    fs_hnd_t *h = fs_map_hnd(fd);

    if(!h) return -1;

    if(h->handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Prefer the 32-bit version, but fall back if needed to the 64-bit one. */
    if(h->handler->total)
        return h->handler->total(h->hnd);
    else if(h->handler->total64)
        return (size_t)h->handler->total64(h->hnd);

    errno = EINVAL;
    return -1;
}

uint64 fs_total64(file_t fd) {
    fs_hnd_t *h = fs_map_hnd(fd);

    if(!h) return -1;

    if(h->handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Prefer the 64-bit version, but fall back if needed to the 32-bit one. */
    if(h->handler->total64)
        return h->handler->total64(h->hnd);
    else if(h->handler->total)
        return (uint64)h->handler->total(h->hnd);

    errno = EINVAL;
    return -1;
}

dirent_t *fs_readdir(file_t fd) {
    static dirent_t dot_dirent;
    static dirent_t *temp_dirent;
    fs_hnd_t *h = fs_map_hnd(fd);

    if(!h) return NULL;

    if(h->handler == NULL)
        return fs_root_readdir(h);

    if(h->handler->readdir == NULL) {
        errno = ENOSYS;
        return NULL;
    }

    switch (h->idx) {
        case 0:
            temp_dirent = h->handler->readdir(h->hnd);
            h->idx++;

            /* Does fs provide its own . directory? */
            if(temp_dirent && (strcmp(temp_dirent->name, ".") == 0)) {
                return temp_dirent;
            } else {
                /* Send . directory first */
                strcpy(dot_dirent.name, ".");
                dot_dirent.attr = O_DIR;
                dot_dirent.size = -1;
                dot_dirent.time = 0;
                return &dot_dirent;
            }
        case 1:
            h->idx++;

            /* Did fs provide its own . directory? */
            if(temp_dirent && (strcmp(temp_dirent->name, ".") == 0)) {
                /* Read a new entry */
                temp_dirent = h->handler->readdir(h->hnd);
            }

            /* Does fs provide its own .. directory? */
            if(temp_dirent && (strcmp(temp_dirent->name, "..") == 0)) {
                h->idx++;
                return temp_dirent;
            } else {
                /* Send .. directory second */
                strcpy(dot_dirent.name, "..");
                dot_dirent.attr = O_DIR;
                dot_dirent.size = -1;
                dot_dirent.time = 0;
                return &dot_dirent;
            }
        case 2:
            h->idx++;
            /* FS didnt provide a . or .. directory. 
               Return what we read first */
            return temp_dirent;
        default:
            return h->handler->readdir(h->hnd);
    }
}

int fs_vioctl(file_t fd, int cmd, va_list ap) {
    fs_hnd_t *h = fs_map_hnd(fd);
    int rv;

    if(!h) return -1;

    if(!h->handler || !h->handler->ioctl) {
        errno = EINVAL;
        return -1;
    }

    rv = h->handler->ioctl(h->hnd, cmd, ap);

    return rv;
}

int fs_ioctl(file_t fd, int cmd, ...) {
    va_list ap;
    int rv;

    va_start(ap, cmd);
    rv = fs_vioctl(fd, cmd, ap);
    va_end(ap);
    return rv;
}

static vfs_handler_t * fs_verify_handler(const char * fn) {
    nmmgr_handler_t *nh;

    nh = nmmgr_lookup(fn);

    if(nh == NULL || nh->type != NMMGR_TYPE_VFS)
        return NULL;
    else
        return (vfs_handler_t *)nh;
}

int fs_rename(const char *fn1, const char *fn2) {
    vfs_handler_t   *fh1, *fh2;
    char        rfn1[PATH_MAX], rfn2[PATH_MAX];

    if(!fs_normalize_path(fn1, rfn1) || !fs_normalize_path(fn2, rfn2))
        return -1;

    /* Look for handlers */
    fh1 = fs_verify_handler(rfn1);

    if(fh1 == NULL) {
        errno = ENOENT;
        return -1;
    }

    fh2 = fs_verify_handler(rfn2);

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
    vfs_handler_t   *cur;
    char        rfn[PATH_MAX];

    if(!fs_normalize_path(fn, rfn))
        return -1;

    /* Look for a handler */
    cur = fs_verify_handler(rfn);

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
    fs_hnd_t *h = fs_map_hnd(fd);

    if(!h) return NULL;

    if(h->handler == NULL || h->handler->mmap == NULL) {
        errno = EINVAL;
        return NULL;
    }

    return h->handler->mmap(h->hnd);
}

int fs_complete(file_t fd, ssize_t * rv) {
    fs_hnd_t *h = fs_map_hnd(fd);

    if(!h) return -1;

    if(h->handler == NULL || h->handler->complete == NULL) {
        errno = EINVAL;
        return -1;
    }

    return h->handler->complete(h->hnd, rv);
}

int fs_mkdir(const char * fn) {
    vfs_handler_t   *cur;
    char        rfn[PATH_MAX];

    if(!fs_normalize_path(fn, rfn))
        return -1;

    /* Look for a handler */
    cur = fs_verify_handler(rfn);

    if(cur == NULL) return -1;

    if(cur->mkdir)
        return cur->mkdir(cur, rfn + strlen(cur->nmmgr.pathname));
    else {
        errno = EINVAL;
        return -1;
    }
}

int fs_rmdir(const char * fn) {
    vfs_handler_t   *cur;
    char        rfn[PATH_MAX];

    if(!fs_normalize_path(fn, rfn))
        return -1;

    /* Look for a handler */
    cur = fs_verify_handler(rfn);

    if(cur == NULL) return -1;

    if(cur->rmdir)
        return cur->rmdir(cur, rfn + strlen(cur->nmmgr.pathname));
    else {
        errno = EINVAL;
        return -1;
    }
}

static int fs_vfcntl(file_t fd, int cmd, va_list ap) {
    fs_hnd_t *h = fs_map_hnd(fd);
    int rv;

    if(!h) return -1;

    if(!h->handler || !h->handler->fcntl) {
        errno = ENOSYS;
        return -1;
    }

    rv = h->handler->fcntl(h->hnd, cmd, ap);

    return rv;
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
    vfs_handler_t *fh1, *fh2;
    char rfn1[PATH_MAX], rfn2[PATH_MAX];

    if(!fs_normalize_path(path1, rfn1) || !fs_normalize_path(path2, rfn2))
        return -1;

    /* Look for handlers */
    fh1 = fs_verify_handler(rfn1);

    if(!fh1) {
        errno = ENOENT;
        return -1;
    }

    fh2 = fs_verify_handler(rfn2);

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
    vfs_handler_t *vfs;
    char rfn[PATH_MAX];

    if(!fs_normalize_path(path2, rfn))
        return -1;

    /* Look for the handler */
    vfs = fs_verify_handler(rfn);

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

int fs_readlink(const char *path, char *buf, size_t bufsize) {
    vfs_handler_t *vfs;
    char fullpath[PATH_MAX];

    /* Prepend the current working directory if we have to. */
    if(path[0] == '/') {
        strcpy(fullpath, path);
    }
    else {
        strcpy(fullpath, fs_getwd());
        strcat(fullpath, "/");
        strcat(fullpath, path);
    }

    /* Look for the handler */
    vfs = fs_verify_handler(fullpath);

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

int fs_stat(const char *path, struct stat *buf, int flag) {
    vfs_handler_t *vfs;
    char fullpath[PATH_MAX];

    /* Verify the input... */
    if(!buf || !path) {
        errno = EFAULT;
        return -1;
    }
    else if(flag & (~AT_SYMLINK_NOFOLLOW)) {
        errno = EINVAL;
        return -1;
    }

    /* Prepend the current working directory if we have to. */
    if(path[0] == '/') {
        strcpy(fullpath, path);
    }
    else {
        strcpy(fullpath, fs_getwd());
        strcat(fullpath, "/");
        strcat(fullpath, path);
    }

    /* Look for the handler */
    vfs = fs_verify_handler(fullpath);

    if(!vfs) {
        errno = ENOENT;
        return -1;
    }

    if(vfs->stat) {
        return vfs->stat(vfs, fullpath + strlen(vfs->nmmgr.pathname), buf,
                         flag);
    }
    else {
        errno = ENOSYS;
        return -1;
    }
}

int fs_rewinddir(file_t fd) {
    fs_hnd_t *h = fs_map_hnd(fd);

    if(!h) return -1;

    if(h->handler == NULL) {
        h->hnd = (void *)0;
        return 0;
    }

    if(h->handler->rewinddir == NULL) {
        errno = ENOSYS;
        return -1;
    }

    h->idx = 0;

    return h->handler->rewinddir(h->hnd);
}

int fs_fstat(file_t fd, struct stat *st) {
    fs_hnd_t *h = fs_map_hnd(fd);

    if(!h) return -1;

    if(!st) {
        errno = EFAULT;
        return -1;
    }

    if(h->handler == NULL) {
        h->hnd = (void *)0;
        return 0;
    }

    if(h->handler->fstat == NULL) {
        errno = ENOSYS;
        return -1;
    }

    return h->handler->fstat(h->hnd, st);
}

/* Initialize FS structures */
int fs_init(void) {
    return 0;
}

void fs_shutdown(void) {
    fs_fdtbl_destroy();
}
