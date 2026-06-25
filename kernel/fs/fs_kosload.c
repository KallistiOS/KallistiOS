/* KallistiOS ##version##

   kernel/fs/fs_kosload.c
   Copyright (C) 2002 Andrew Kieschnick
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2012 Lawrence Sebald
   Copyright (C) 2025 Donald Haase
   Copyright (C) 2026 Andy Barajas

*/

/*
   VFS driver for the kos-tool host filesystem (formerly dc-load/fs_dcload).

   printf goes to the kos-tool console
   /pc corresponds to / on the system running kos-tool

   The low-level loader syscall driver lives in kernel/kosload.c; this file
   only implements the /pc VFS on top of it and is fully portable.
*/

#include <kos/kosload.h>
#include <kos/fs_kosload.h>
#include <kos/dbgio.h>
#include <kos/dbglog.h>
#include <kos/fs.h>
#include <kos/init.h>
#include <kos/mutex.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/* ---- VFS handler --------------------------------------------------------- */

typedef struct kl_obj {
    int    hnd;
    char  *path;
    dirent_t dirent;
} kl_obj_t;

static mutex_t mutex = MUTEX_INITIALIZER;

static void *fs_kosload_open(vfs_handler_t *vfs, const char *fn, int mode) {
    kl_obj_t *entry;
    int hnd = 0;
    int kosload_mode = 0;
    int mm = (mode & O_MODE_MASK);
    size_t fn_len = 0;

    (void)vfs;

    entry = calloc(1, sizeof(kl_obj_t));
    if(!entry) {
        errno = ENOMEM;
        return (void *)NULL;
    }

    if(mode & O_DIR) {
        if(fn[0] == '\0') {
            fn = "/";
        }

        hnd = kosload_opendir(fn);
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
            kosload_mode = 0;
        else if((mm & O_RDWR) == O_RDWR)
            kosload_mode = 0x0202;
        else if((mm & O_WRONLY) == O_WRONLY)
            kosload_mode = 0x0201;

        if(mode & O_APPEND)
            kosload_mode |= 0x0008;

        if(mode & O_TRUNC)
            kosload_mode |= 0x0400;

        hnd = kosload_open(fn, kosload_mode, 0644);

        if(hnd == -1) {
            errno = ENOENT;
            free(entry);
            return (void *)NULL;
        }
    }

    entry->hnd = hnd;
    return (void *)entry;
}

static int fs_kosload_close(void *h) {
    kl_obj_t *obj = h;

    if(!obj) return 0;

    /* It has a path so it's a dir */
    if(obj->path) {
        kosload_closedir(obj->hnd);

        free(obj->path);
    }
    else
        kosload_close(obj->hnd);

    free(obj);
    return 0;
}

static ssize_t fs_kosload_read(void *h, void *buf, size_t cnt) {
    ssize_t ret = -1;
    kl_obj_t *obj = h;

    if(obj)
        ret = kosload_read(obj->hnd, buf, cnt);

    return ret;
}

static ssize_t fs_kosload_write(void *h, const void *buf, size_t cnt) {
    ssize_t ret = -1;
    kl_obj_t *obj = h;

    if(obj)
        ret = kosload_write(obj->hnd, buf, cnt);

    return ret;
}

static off_t fs_kosload_seek(void *h, off_t offset, int whence) {
    off_t ret = -1;
    kl_obj_t *obj = h;

    if(obj)
        ret = kosload_lseek(obj->hnd, offset, whence);

    return ret;
}

static off_t fs_kosload_tell(void *h) {
    off_t ret = -1;
    kl_obj_t *obj = h;

    if(obj)
        ret = kosload_lseek(obj->hnd, 0, SEEK_CUR);

    return ret;
}

static size_t fs_kosload_total(void *h) {
    size_t ret = -1;
    off_t cur;
    kl_obj_t *obj = h;

    if(obj) {
        /* Lock to ensure commands are sent sequentially. */
        mutex_lock_scoped(&mutex);

        cur = kosload_lseek(obj->hnd, 0, SEEK_CUR);
        ret = kosload_lseek(obj->hnd, 0, SEEK_END);
        kosload_lseek(obj->hnd, cur, SEEK_SET);
    }

    return ret;
}

static const dirent_t *fs_kosload_readdir(void *h) {
    dirent_t *rv = NULL;
    struct dirent *kld;
    kosload_stat_t filestat;
    char *fn;
    kl_obj_t *entry = h;

    /* Lock to ensure commands are sent sequentially. */
    mutex_lock_scoped(&mutex);

    /* Check if it's a dir */
    if(!entry || !entry->path) {
        errno = EBADF;
        return NULL;
    }

    kld = kosload_readdir(entry->hnd);

    if(kld) {
        rv = &(entry->dirent);

        /* Verify kosload won't overflow us */
        if(strlen(kld->d_name) + 1 > NAME_MAX) {
            errno = EOVERFLOW;
            return NULL;
        }

        strcpy(rv->name, kld->d_name);
        rv->size = 0;
        rv->time = 0;
        rv->attr = 0; /* what the hell is attr supposed to be anyways? */

        fn = malloc(strlen(entry->path) + strlen(kld->d_name) + 1);
        if(!fn) {
            errno = ENOMEM;
            return NULL;
        }

        strcpy(fn, entry->path);
        strcat(fn, kld->d_name);

        if(!kosload_stat(fn, &filestat)) {
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

static int fs_kosload_rename(vfs_handler_t *vfs, const char *fn1, const char *fn2) {
    int ret;

    (void)vfs;

    /* Lock to ensure commands are sent sequentially. */
    mutex_lock_scoped(&mutex);

    /* really stupid hack, since I didn't put rename() in dcload */
    ret = kosload_link(fn1, fn2);
    if(!ret)
        ret = kosload_unlink(fn1);

    return ret;
}

static int fs_kosload_unlink(vfs_handler_t *vfs, const char *fn) {
    (void)vfs;

    return kosload_unlink(fn);
}

static int fs_kosload_stat(vfs_handler_t *vfs, const char *path, struct stat *st,
                           int flag) {
    kosload_stat_t filestat;
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

    retval = kosload_stat(path, &filestat);

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

static int fs_kosload_fcntl(void *h, int cmd, va_list ap) {
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

static int fs_kosload_rewinddir(void *h) {
    kl_obj_t *obj = h;

    /* Check if it's a dir */
    if(!obj || !obj->path)
        return -1;

    return kosload_rewinddir(obj->hnd);
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

    fs_kosload_open,
    fs_kosload_close,
    fs_kosload_read,
    fs_kosload_write,
    fs_kosload_seek,
    fs_kosload_tell,
    fs_kosload_total,
    fs_kosload_readdir,
    NULL,               /* ioctl */
    fs_kosload_rename,
    fs_kosload_unlink,
    NULL,               /* mmap */
    NULL,               /* complete */
    fs_kosload_stat,
    NULL,               /* mkdir */
    NULL,               /* rmdir */
    fs_kosload_fcntl,
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    fs_kosload_rewinddir,
    NULL                /* fstat */
};

/* ---- Console (dbgio) ----------------------------------------------------- */

static int fs_kosload_write_buffer(const uint8_t *data, int len, int xlat) {
    (void)xlat;

    kosload_write(STDOUT_FILENO, data, len);

    return len;
}

static int fs_kosload_read_buffer(uint8_t *data, int len) {
    return kosload_read(STDIN_FILENO, data, len);
}

/* Minimal stub so the dbgio handler table entry is valid before init. */
static int never_detected(void) {
    return 0;
}

dbgio_handler_t dbgio_kosload = {
    .name = "fs_kosload_uninit",
    .detected = never_detected
};

static int *kosload_wrkmem = NULL;
static const char *dbgio_kosload_name = "fs_kosload";
int kosload_type = KOSLOAD_TYPE_NONE;

/* Call this before arch_init_all (or any call to dbgio_*) to use dc-tool's
   console output functions. */
void fs_kosload_init_console(void) {
    /* Setup our dbgio handler */
    memcpy(&dbgio_kosload, &dbgio_null, sizeof(dbgio_kosload));
    dbgio_kosload.name = dbgio_kosload_name;
    dbgio_kosload.detected = syscall_kosload_detected;
    dbgio_kosload.write_buffer = fs_kosload_write_buffer;
    dbgio_kosload.read_buffer = fs_kosload_read_buffer;

    /* We actually need to detect here to make sure we're on
       kosload-serial, or scif_init must not proceed. */
    if(!syscall_kosload_detected())
        return;

    /* kosload IP will always return -1 here. Serial will return 0 and make
       no change since it already holds 0 as 'no mem assigned */
    if(kosload_assignwrkmem(0) == -1) {
        kosload_type = KOSLOAD_TYPE_IP;
    }
    else {
        kosload_type = KOSLOAD_TYPE_SER;

        /* Give kosload the 64k it needs to compress data (if on serial) */
        kosload_wrkmem = malloc(65536);
        if(kosload_wrkmem) {
            if(kosload_assignwrkmem(kosload_wrkmem) == -1)
                free(kosload_wrkmem);
        }
    }
}

/* ---- Initialization ------------------------------------------------------ */

/* Call fs_kosload_init_console() before calling fs_kosload_init() */
void fs_kosload_init(void) {
    /* This was already done in init_console. */
    if(kosload_type == KOSLOAD_TYPE_NONE)
        return;

    /* Don't register the /pc filesystem if KOS networking is active and we're
       on IP mode — the network socket driver takes over in that case. */
    if((kosload_type == KOSLOAD_TYPE_IP) && (__kos_init_flags & INIT_NET)) {
        dbglog(DBG_INFO, "kos-load console+kosnet, fs_kosload unavailable.\n");
        return;
    }

    /* Register with VFS */
    nmmgr_handler_add(&vh.nmmgr);
}

void fs_kosload_shutdown(void) {
    /* Check for kosload */
    if(!syscall_kosload_detected())
        return;

    /* Free kosload wrkram */
    if(kosload_wrkmem) {
        kosload_assignwrkmem(0);
        free(kosload_wrkmem);
        kosload_wrkmem = NULL;
    }

    nmmgr_handler_remove(&vh.nmmgr);
}
