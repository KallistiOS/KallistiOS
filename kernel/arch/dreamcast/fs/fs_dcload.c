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

#include <dc/fifo.h>
#include <dc/fs_dcload.h>
#include <arch/spinlock.h>
#include <kos/dbgio.h>
#include <kos/dbglog.h>
#include <kos/fs.h>
#include <kos/init.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

/* A linked list of dir entries. */
typedef struct dcl_dir {
    LIST_ENTRY(dcl_dir) fhlist;
    int hnd;
    char *path;
    dirent_t dirent;
} dcl_dir_t;

LIST_HEAD(dcl_de, dcl_dir);

static struct dcl_de dir_head = LIST_HEAD_INITIALIZER(0);

static dcl_dir_t *hnd_is_dir(int hnd) {
    dcl_dir_t *i;

    if(!hnd) return NULL;

    LIST_FOREACH(i, &dir_head, fhlist) {
        if(i->hnd == (int)hnd)
            break;
    }

    return i;
}

static spinlock_t mutex = SPINLOCK_INITIALIZER;

#define dclsc(...) ({ \
        irq_disable_scoped(); \
        while(FIFO_STATUS & FIFO_SH4) \
            ; \
        dcloadsyscall(__VA_ARGS__); \
    })

/* Printk replacement */

int dcload_write_buffer(const uint8 *data, int len, int xlat) {
    (void)xlat;

    spinlock_lock_scoped(&mutex);
    dclsc(DCLOAD_WRITE, 1, data, len);

    return len;
}

int dcload_read_cons(void) {
    return -1;
}

size_t dcload_gdbpacket(const char* in_buf, size_t in_size, char* out_buf, size_t out_size) {

    spinlock_lock_scoped(&mutex);

    /* we have to pack the sizes together because the dcloadsyscall handler
       can only take 4 parameters */
    return dclsc(DCLOAD_GDBPACKET, in_buf, (in_size << 16) | (out_size & 0xffff), out_buf);
}

static void *dcload_open(vfs_handler_t * vfs, const char *fn, int mode) {
    char *dcload_path = NULL;
    dcl_dir_t *entry;
    int hnd = 0;
    int dcload_mode = 0;
    int mm = (mode & O_MODE_MASK);
    size_t fn_len = 0;

    (void)vfs;

    spinlock_lock_scoped(&mutex);

    if(mode & O_DIR) {
        if(fn[0] == '\0') {
            fn = "/";
        }

        hnd = dclsc(DCLOAD_OPENDIR, fn);

        if(!hnd) {
            /* It could be caused by other issues, such as
            pathname being too long or symlink loops, but
            ENOTDIR seems to be the best generic and we should
            set something */
            errno = ENOTDIR;
            return (void *)NULL;
        }

        /* We got something back so create an dir list entry for it */
        entry = malloc(sizeof(dcl_dir_t));
        if(!entry) {
            errno = ENOMEM;
            return (void *)NULL;
        }

        fn_len = strlen(fn);
        if(fn[fn_len - 1] == '/') fn_len--;

        dcload_path = malloc(fn_len + 2);
        if(!dcload_path) {
            errno = ENOMEM;
            free(entry);
            return (void *)NULL;
        }

        memcpy(dcload_path, fn, fn_len);
        dcload_path[fn_len]   = '/';
        dcload_path[fn_len+1] = '\0';

        /* Now that everything is ready, add to list */
        entry->hnd = hnd;
        entry->path = dcload_path;
        LIST_INSERT_HEAD(&dir_head, entry, fhlist);
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

        hnd = dclsc(DCLOAD_OPEN, fn, dcload_mode, 0644);
        hnd++; /* KOS uses 0 for error, not -1 */
    }

    return (void *)hnd;
}

static int dcload_close(void * h) {
    uint32 hnd = (uint32)h;
    dcl_dir_t *i;

    spinlock_lock_scoped(&mutex);

    if(hnd) {
        /* Check if it's a dir */
        i = hnd_is_dir(hnd);

        /* We found it in the list, so it's a dir */
        if(i) {
            dclsc(DCLOAD_CLOSEDIR, hnd);
            LIST_REMOVE(i, fhlist);
            free(i->path);
            free(i);
        }
        else {
            hnd--; /* KOS uses 0 for error, not -1 */
            dclsc(DCLOAD_CLOSE, hnd);
        }
    }

    return 0;
}

static ssize_t dcload_read(void * h, void *buf, size_t cnt) {
    ssize_t ret = -1;
    uint32 hnd = (uint32)h;

    spinlock_lock_scoped(&mutex);

    if(hnd) {
        hnd--; /* KOS uses 0 for error, not -1 */
        ret = dclsc(DCLOAD_READ, hnd, buf, cnt);
    }

    return ret;
}

static ssize_t dcload_write(void * h, const void *buf, size_t cnt) {
    ssize_t ret = -1;
    uint32 hnd = (uint32)h;

    spinlock_lock_scoped(&mutex);

    if(hnd) {
        hnd--; /* KOS uses 0 for error, not -1 */
        ret = dclsc(DCLOAD_WRITE, hnd, buf, cnt);
    }

    return ret;
}

static off_t dcload_seek(void * h, off_t offset, int whence) {
    off_t ret = -1;
    uint32 hnd = (uint32)h;

    spinlock_lock_scoped(&mutex);

    if(hnd) {
        hnd--; /* KOS uses 0 for error, not -1 */
        ret = dclsc(DCLOAD_LSEEK, hnd, offset, whence);
    }

    return ret;
}

static off_t dcload_tell(void * h) {
    off_t ret = -1;
    uint32 hnd = (uint32)h;

    spinlock_lock_scoped(&mutex);

    if(hnd) {
        hnd--; /* KOS uses 0 for error, not -1 */
        ret = dclsc(DCLOAD_LSEEK, hnd, 0, SEEK_CUR);
    }

    return ret;
}

static size_t dcload_total(void * h) {
    size_t ret = -1;
    size_t cur;
    uint32 hnd = (uint32)h;

    spinlock_lock_scoped(&mutex);

    if(hnd) {
        hnd--; /* KOS uses 0 for error, not -1 */
        cur = dclsc(DCLOAD_LSEEK, hnd, 0, SEEK_CUR);
        ret = dclsc(DCLOAD_LSEEK, hnd, 0, SEEK_END);
        dclsc(DCLOAD_LSEEK, hnd, cur, SEEK_SET);
    }

    return ret;
}

static dirent_t *dcload_readdir(void * h) {
    dirent_t *rv = NULL;
    dcload_dirent_t *dcld;
    dcload_stat_t filestat;
    char *fn;
    uint32 hnd = (uint32)h;
    dcl_dir_t *entry;

    spinlock_lock_scoped(&mutex);

    if(!(entry = hnd_is_dir(hnd))) {
        errno = EBADF;
        return NULL;
    }

    dcld = (dcload_dirent_t *)dclsc(DCLOAD_READDIR, hnd);

    if(dcld) {
        rv = &(entry->dirent);
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

        if(!dclsc(DCLOAD_STAT, fn, &filestat)) {
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

static int dcload_rename(vfs_handler_t * vfs, const char *fn1, const char *fn2) {
    int ret;

    (void)vfs;

    spinlock_lock_scoped(&mutex);

    /* really stupid hack, since I didn't put rename() in dcload */

    ret = dclsc(DCLOAD_LINK, fn1, fn2);

    if(!ret)
        ret = dclsc(DCLOAD_UNLINK, fn1);

    return ret;
}

static int dcload_unlink(vfs_handler_t * vfs, const char *fn) {
    (void)vfs;

    spinlock_lock_scoped(&mutex);

    return dclsc(DCLOAD_UNLINK, fn);
}

static int dcload_stat(vfs_handler_t *vfs, const char *path, struct stat *st,
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

    spinlock_lock(&mutex);
    retval = dclsc(DCLOAD_STAT, path, &filestat);
    spinlock_unlock(&mutex);

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

static int dcload_fcntl(void *h, int cmd, va_list ap) {
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

static int dcload_rewinddir(void *h) {
    uint32_t hnd = (uint32_t)h;

    spinlock_lock_scoped(&mutex);

    if(!hnd_is_dir(hnd))
        return -1;

    return dclsc(DCLOAD_REWINDDIR, hnd);
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

    dcload_open,
    dcload_close,
    dcload_read,
    dcload_write,
    dcload_seek,
    dcload_tell,
    dcload_total,
    dcload_readdir,
    NULL,               /* ioctl */
    dcload_rename,
    dcload_unlink,
    NULL,               /* mmap */
    NULL,               /* complete */
    dcload_stat,
    NULL,               /* mkdir */
    NULL,               /* rmdir */
    dcload_fcntl,
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    dcload_rewinddir,
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

int fs_dcload_detected(void) {
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
    dbgio_dcload.detected = fs_dcload_detected;
    dbgio_dcload.write_buffer = dcload_write_buffer;
    // dbgio_dcload.read = dcload_read_cons;

    /* We actually need to detect here to make sure we're not on
       dcload-serial, or scif_init must not proceed. */
    if(*DCLOADMAGICADDR != DCLOADMAGICVALUE)
        return;


    /* dcload IP will always return -1 here. Serial will return 0 and make
      no change since it already holds 0 as 'no mem assigned */
    if(dclsc(DCLOAD_ASSIGNWRKMEM, 0) == -1) {
        dcload_type = DCLOAD_TYPE_IP;
    }
    else {
        dcload_type = DCLOAD_TYPE_SER;

        /* Give dcload the 64k it needs to compress data (if on serial) */
        dcload_wrkmem = malloc(65536);
        if(dcload_wrkmem) {
            if(dclsc(DCLOAD_ASSIGNWRKMEM, dcload_wrkmem) == -1)
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
    if(*DCLOADMAGICADDR != DCLOADMAGICVALUE)
        return;

    /* Free dcload wrkram */
    if(dcload_wrkmem) {
        dclsc(DCLOAD_ASSIGNWRKMEM, 0);
        free(dcload_wrkmem);
    }

    nmmgr_handler_remove(&vh.nmmgr);
}
