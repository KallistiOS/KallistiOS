/* KallistiOS ##version##

   kernel/arch/dreamcast/fs/fs_dcload.c
   Copyright (C) 2002 Andrew Kieschnick
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2025 Donald Haase

*/

/*

This is a rewrite of Megan Potter's fs_serconsole to use the dcload / dc-tool
fileserver and console.

printf goes to the dc-tool console
/pc corresponds to / on the system running dc-tool

*/

#include <dc/dcload.h>
#include <dc/fs_dcload.h>
#include <kos/dbgio.h>
#include <kos/dbglog.h>
#include <kos/fs.h>
#include <kos/init.h>
#include <kos/mutex.h>
#include <kos/rwsem.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/queue.h>

typedef struct dcl_obj {
    int hnd;
    char *path;
    dirent_t dirent;
} dcl_obj_t;

static mutex_t mutex = MUTEX_INITIALIZER;

int dcload_write_buffer(const uint8_t *data, int len, int xlat) {
    (void)xlat;

    dcload_write(STDOUT_FILENO, data, len);

    return len;
}

int dcload_read_cons(void) {
    return -1;
}

static void *fs_dcload_open(vfs_handler_t *vfs, const char *fn, int mode) {
    dcl_obj_t *entry;
    int hnd = 0;
    int dcload_mode = 0;
    int mm = (mode & O_MODE_MASK);
    size_t fn_len = 0;

    (void)vfs;

    entry = calloc(1, sizeof(dcl_obj_t));
    if(!entry) {
        errno = ENOMEM;
        return (void *)NULL;
    }

    if(mode & O_DIR) {
        if(fn[0] == '\0') {
            fn = "/";
        }

        hnd = dcload_opendir(fn);

        if(!hnd) {
            /* It could be caused by other issues, such as
            pathname being too long or symlink loops, but
            ENOTDIR seems to be the best generic and we should
            set something */
            errno = ENOTDIR;
            free(entry);
            return (void *)NULL;
        }

        fn_len = strlen(fn);
        if(fn[fn_len - 1] == '/') fn_len--;

        entry->path = malloc(fn_len + 2);
        if(!entry->path) {
            errno = ENOMEM;
            free(entry);
            return (void *)NULL;
        }

        memcpy(entry->path, fn, fn_len);
        entry->path[fn_len]   = '/';
        entry->path[fn_len+1] = '\0';
    }
    else {
        if(mm == O_RDONLY)
            dcload_mode = 0;
        else if((mm & O_RDWR) == O_RDWR)
            dcload_mode = 0x0202;
        else if((mm & O_WRONLY) == O_WRONLY)
            dcload_mode = 0x0201;

        if(mode & O_APPEND)
            dcload_mode |= 0x0008;

        if(mode & O_TRUNC)
            dcload_mode |= 0x0400;

        hnd = dcload_open(fn, dcload_mode, 0644);

        if(hnd == -1) {
            errno = ENOENT;
            free(entry);
            return (void *)NULL;
        }
    }

    entry->hnd = hnd;
    return (void *)entry;
}

static int fs_dcload_close(void *h) {
    dcl_obj_t *obj = h;

    if(!obj) return 0;

    /* It has a path so it's a dir */
    if(obj->path) {
        dcload_closedir(obj->hnd);

        free(obj->path);
    }
    else
        dcload_close(obj->hnd);

    free(obj);
    return 0;
}

static ssize_t fs_dcload_read(void *h, void *buf, size_t cnt) {
    ssize_t ret = -1;
    dcl_obj_t *obj = h;

    if(obj)
        ret = dcload_read(obj->hnd, buf, cnt);

    return ret;
}

static ssize_t fs_dcload_write(void *h, const void *buf, size_t cnt) {
    ssize_t ret = -1;
    dcl_obj_t *obj = h;

    if(obj)
        ret = dcload_write(obj->hnd, buf, cnt);

    return ret;
}

static off_t fs_dcload_seek(void *h, off_t offset, int whence) {
    off_t ret = -1;
    dcl_obj_t *obj = h;

    if(obj)
        ret = dcload_lseek(obj->hnd, offset, whence);

    return ret;
}

static off_t fs_dcload_tell(void *h) {
    off_t ret = -1;
    dcl_obj_t *obj = h;

    if(obj)
        ret = dcload_lseek(obj->hnd, 0, SEEK_CUR);

    return ret;
}

static size_t fs_dcload_total(void *h) {
    size_t ret = -1;
    off_t cur;
    dcl_obj_t *obj = h;

    if(obj) {
        /* Lock to ensure commands are sent sequentially. */
        mutex_lock_scoped(&mutex);

        cur = dcload_lseek(obj->hnd, 0, SEEK_CUR);
        ret = dcload_lseek(obj->hnd, 0, SEEK_END);
        dcload_lseek(obj->hnd, cur, SEEK_SET);
    }

    return ret;
}

static dirent_t *fs_dcload_readdir(void *h) {
    dirent_t *rv = NULL;
    struct dirent *dcld;
    dcload_stat_t filestat;
    char *fn;
    dcl_obj_t *entry = h;

    /* Lock to ensure commands are sent sequentially. */
    mutex_lock_scoped(&mutex);

    /* Check if it's a dir */
    if(!entry || !entry->path) {
        errno = EBADF;
        return NULL;
    }

    dcld = dcload_readdir(entry->hnd);

    if(dcld) {
        rv = &(entry->dirent);

        /* Verify dcload won't overflow us */
        if(strlen(dcld->d_name) + 1 > NAME_MAX) {
            errno = EOVERFLOW;
            return NULL;
        }

        strcpy(rv->name, dcld->d_name);
        rv->size = 0;
        rv->time = 0;
        rv->attr = 0; /* what the hell is attr supposed to be anyways? */

        fn = malloc(strlen(entry->path) + strlen(dcld->d_name) + 1);

        if(!fn) {
            errno = ENOMEM;
            return NULL;
        }

        strcpy(fn, entry->path);
        strcat(fn, dcld->d_name);

        if(!dcload_stat(fn, &filestat)) {
            if(filestat.st_mode & S_IFDIR) {
                rv->size = -1;
                rv->attr = O_DIR;
            }
            else
                rv->size = filestat.st_size;

            rv->time = filestat.mtime;

        }

        free(fn);
    }

    return rv;
}

static int fs_dcload_rename(vfs_handler_t *vfs, const char *fn1, const char *fn2) {
    int ret;

    (void)vfs;

    /* Lock to ensure commands are sent sequentially. */
    mutex_lock_scoped(&mutex);

    /* really stupid hack, since I didn't put rename() in dcload */

    ret = dcload_link(fn1, fn2);

    if(!ret)
        ret = dcload_unlink(fn1);

    return ret;
}

static int fs_dcload_unlink(vfs_handler_t *vfs, const char *fn) {
    (void)vfs;

    return dcload_unlink(fn);
}

static int fs_dcload_stat(vfs_handler_t *vfs, const char *path, struct stat *st,
                       int flag) {
    dcload_stat_t filestat;
    size_t len = strlen(path);
    int retval;

    (void)flag;

    /* Root directory '/pc' */
    if(len == 0 || (len == 1 && *path == '/')) {
        memset(st, 0, sizeof(struct stat));
        st->st_dev = (dev_t)((uintptr_t)vfs);
        st->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        st->st_size = -1;
        st->st_nlink = 2;

        return 0;
    }

    retval = dcload_stat(path, &filestat);

    if(!retval) {
        memset(st, 0, sizeof(struct stat));
        st->st_dev = (dev_t)((uintptr_t)vfs);
        st->st_ino = filestat.st_ino;
        st->st_mode = filestat.st_mode;
        st->st_nlink = filestat.st_nlink;
        st->st_uid = filestat.st_uid;
        st->st_gid = filestat.st_gid;
        st->st_rdev = filestat.st_rdev;
        st->st_size = filestat.st_size;
        st->st_atime = filestat.atime;
        st->st_mtime = filestat.mtime;
        st->st_ctime = filestat.ctime;
        st->st_blksize = filestat.st_blksize;
        st->st_blocks = filestat.st_blocks;

        return 0;
    }

    errno = ENOENT;
    return -1;
}

static int fs_dcload_fcntl(void *h, int cmd, va_list ap) {
    int rv = -1;

    (void)h;
    (void)ap;

    switch(cmd) {
        case F_GETFL:
            /* XXXX: Not the right thing to do... */
            rv = O_RDWR;
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

static int fs_dcload_rewinddir(void *h) {
    dcl_obj_t *obj = h;

    /* Check if it's a dir */
    if(!obj || !obj->path)
        return -1;

    return dcload_rewinddir(obj->hnd);
}

/* Pull all that together */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/pc",          /* name */
        0,              /* tbfi */
        0x00010000,     /* Version 1.0 */
        0,              /* flags */
        NMMGR_TYPE_VFS,
        NMMGR_LIST_INIT
    },

    0, NULL,            /* no cache, privdata */

    fs_dcload_open,
    fs_dcload_close,
    fs_dcload_read,
    fs_dcload_write,
    fs_dcload_seek,
    fs_dcload_tell,
    fs_dcload_total,
    fs_dcload_readdir,
    NULL,               /* ioctl */
    fs_dcload_rename,
    fs_dcload_unlink,
    NULL,               /* mmap */
    NULL,               /* complete */
    fs_dcload_stat,
    NULL,               /* mkdir */
    NULL,               /* rmdir */
    fs_dcload_fcntl,
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    fs_dcload_rewinddir,
    NULL                /* fstat */
};

/* We have to provide a minimal interface in case dcload usage is
   disabled through init flags. */
static int never_detected(void) {
    return 0;
}

dbgio_handler_t dbgio_dcload = {
    "fs_dcload_uninit",
    never_detected,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

int syscall_dcload_detected(void) {
    /* Check for dcload */
    if(*DCLOADMAGICADDR == DCLOADMAGICVALUE)
        return 1;
    else
        return 0;
}

static int *dcload_wrkmem = NULL;
static const char * dbgio_dcload_name = "fs_dcload";
int dcload_type = DCLOAD_TYPE_NONE;

/* Call this before arch_init_all (or any call to dbgio_*) to use dcload's
   console output functions. */
void fs_dcload_init_console(void) {
    /* Setup our dbgio handler */
    memcpy(&dbgio_dcload, &dbgio_null, sizeof(dbgio_dcload));
    dbgio_dcload.name = dbgio_dcload_name;
    dbgio_dcload.detected = syscall_dcload_detected;
    dbgio_dcload.write_buffer = dcload_write_buffer;
    // dbgio_dcload.read = dcload_read_cons;

    /* We actually need to detect here to make sure we're on
       dcload-serial, or scif_init must not proceed. */
    if(!syscall_dcload_detected())
        return;


    /* dcload IP will always return -1 here. Serial will return 0 and make
      no change since it already holds 0 as 'no mem assigned */
    if(dcload_assignwrkmem(0) == -1) {
        dcload_type = DCLOAD_TYPE_IP;
    }
    else {
        dcload_type = DCLOAD_TYPE_SER;

        /* Give dcload the 64k it needs to compress data (if on serial) */
        dcload_wrkmem = malloc(65536);
        if(dcload_wrkmem) {
            if(dcload_assignwrkmem(dcload_wrkmem) == -1)
                free(dcload_wrkmem);
        }
    }
}

/* Call fs_dcload_init_console() before calling fs_dcload_init() */
void fs_dcload_init(void) {
    /* This was already done in init_console. */
    if(dcload_type == DCLOAD_TYPE_NONE)
        return;

    /* Check for combination of KOS networking and dcload-ip */
    if((dcload_type == DCLOAD_TYPE_IP) && (__kos_init_flags & INIT_NET)) {
        dbglog(DBG_INFO, "dc-load console+kosnet, fs_dcload unavailable.\n");
        return;
    }

    /* Register with VFS */
    nmmgr_handler_add(&vh.nmmgr);
}

void fs_dcload_shutdown(void) {
    /* Check for dcload */
    if(!syscall_dcload_detected())
        return;

    /* Free dcload wrkram */
    if(dcload_wrkmem) {
        dcload_assignwrkmem(0);
        free(dcload_wrkmem);
    }

    nmmgr_handler_remove(&vh.nmmgr);
}
