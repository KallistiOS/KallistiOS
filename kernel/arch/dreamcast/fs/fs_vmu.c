/* KallistiOS ##version##

   fs_vmu.c
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2012, 2013, 2014, 2016 Lawrence Sebald

*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <arch/types.h>
#include <kos/mutex.h>
#include <kos/dbglog.h>
#include <dc/fs_vmu.h>
#include <dc/vmufs.h>
#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <dc/vmu_pkg.h>
#include <sys/queue.h>

/*

This is the vmu filesystem module.  Because there are no directories on vmu's
it's pretty simple, however the filesystem uses a separate directory for each
of the vmu slots, so if vmufs were mounted on /vmu, /vmu/a1/ is the dir for
slot 1 on port a, and /vmu/c2 is slot 2 on port c, etc.

At the moment this FS is kind of a hack because of the simplicity (and weirdness)
of the VMU file system. For one, all files must be pretty small, so it loads
and caches the entire file on open. For two, all files are a multiple of 512
bytes in size (no way around this one). On top of it all, files may have an
obnoxious header and you can't just read and write them with abandon like
a normal file system. We'll have to find ways around this later on, but for
now it gives the file data to you raw.

Note: this new version now talks directly to the vmufs module and doesn't do
any block-level I/O anymore. This layer and that one are interchangeable
and may be used pretty much simultaneously in the same program.

*/

/* Enable this if you want copious debug output */
/* #define VMUFS_DEBUG */

#define VMU_DIR     0
#define VMU_FILE    1
#define VMU_ANY     -1  /* Used for checking validity */

/* File handles */
typedef struct vmu_fh_str {
    uint32 strtype;                     /* 0==dir, 1==file */
    TAILQ_ENTRY(vmu_fh_str) listent;    /* list entry */

    int mode;                           /* mode the file was opened with */
    char path[17];                      /* full path of the file */
    char name[13];                      /* name of the file */
    off_t loc;                          /* current position from the start in the file (bytes) */
    off_t start;                        /* start of the data in the file (bytes) */
    maple_device_t *dev;                /* maple address of the vmu to use */
    uint32 filesize;                    /* file length from dirent (in 512-byte blks) */
    uint8 *data;                        /* copy of the whole file */
    vmu_pkg_t *header;                  /* VMU file header */
    bool raw;                           /* file opened as raw */
} vmu_fh_t;

/* Directory handles */
typedef struct vmu_dh_str {
    uint32 strtype;                     /* 0==dir, 1==file */
    TAILQ_ENTRY(vmu_dh_str) listent;    /* list entry */

    int rootdir;                        /* 1 if we're reading /vmu */
    dirent_t dirent;                    /* Dirent to pass back */
    vmu_dir_t *dirblocks;               /* Copy of all directory blocks */
    uint16 entry;                       /* Current dirent */
    uint16 dircnt;                      /* Count of dir entries */
    maple_device_t *dev;                /* VMU address */
} vmu_dh_t;

/* Linked list of open files (controlled by "mutex") */
TAILQ_HEAD(vmu_fh_list, vmu_fh_str) vmu_fh;

/* Thread mutex for vmu_fh access */
static mutex_t fh_mutex;

static vmu_pkg_t *dft_header;

static vmu_pkg_t * vmu_pkg_dup(const vmu_pkg_t *old_hdr) {
    size_t ec_size, icon_size;
    vmu_pkg_t *hdr;

    hdr = malloc(sizeof(*hdr));
    if(!hdr)
        return NULL;

    memcpy(hdr, old_hdr, sizeof(*hdr));

    if(old_hdr->eyecatch_type && old_hdr->eyecatch_data) {
        ec_size = (72 * 56 / 2) << (3 - old_hdr->eyecatch_type);

        hdr->eyecatch_data = malloc(ec_size);
        if(!hdr->eyecatch_data)
            goto err_free_hdr;

        memcpy(hdr->eyecatch_data, old_hdr->eyecatch_data, ec_size);
    } else {
        hdr->eyecatch_data = NULL;
    }

    if(old_hdr->icon_cnt) {
        icon_size = 512 * old_hdr->icon_cnt;

        hdr->icon_data = malloc(icon_size);
        if(!hdr->icon_data)
            goto err_free_ec_data;

        memcpy(hdr->icon_data, old_hdr->icon_data, icon_size);
    } else {
        hdr->icon_data = NULL;
    }

    return hdr;

err_free_ec_data:
    free(hdr->eyecatch_data);
err_free_hdr:
    free(hdr);
    return NULL;
}

/* Take a VMUFS path and return the requested address */
static maple_device_t * vmu_path_to_addr(const char *p) {
    char port;

    if(p[0] != '/') return NULL;            /* Only absolute paths */

    port = p[1] | 32;               /* Lowercase the port */

    if(port < 'a' || port > 'd') return NULL;   /* Unit A-D, device 0-5 */

    if(p[2] < '0' || p[2] > '5') return NULL;

    return maple_enum_dev(port - 'a', p[2] - '0');
}

/* Open the fake vmu root dir /vmu */
static vmu_fh_t *vmu_open_vmu_dir(void) {
    unsigned int p, u;
    unsigned int num = 0;
    char names[MAPLE_PORT_COUNT * MAPLE_UNIT_COUNT][2];
    vmu_dh_t *dh;
    maple_device_t * dev;

    /* Determine how many VMUs are connected */
    for(p = 0; p < MAPLE_PORT_COUNT; p++) {
        for(u = 0; u < MAPLE_UNIT_COUNT; u++) {
            dev = maple_enum_dev(p, u);

            if(!dev) continue;

            if(dev->info.functions & MAPLE_FUNC_MEMCARD) {
                names[num][0] = p + 'a';
                names[num][1] = u + '0';
                num++;

                if(__is_defined(VMUFS_DEBUG)) {
                    dbglog(DBG_KDEBUG, "vmu_open_vmu_dir: found memcard (%c%d)\n",
                           'a' + p, u);
                }
            }
        }
    }

    if(__is_defined(VMUFS_DEBUG))
        dbglog(DBG_KDEBUG, "# of memcards found: %d\n", num);

    if(!(dh = malloc(sizeof(vmu_dh_t))))
        return NULL;
    memset(dh, 0, sizeof(vmu_dh_t));
    dh->strtype = VMU_DIR;
    dh->dirblocks = malloc(num * sizeof(vmu_dir_t));

    if(!dh->dirblocks) {
        free(dh);
        return NULL;
    }

    dh->rootdir = 1;
    dh->entry = 0;
    dh->dircnt = num;
    dh->dev = NULL;

    /* Create the directory entries */
    for(u = 0; u < num; u++) {
        memset(dh->dirblocks + u, 0, sizeof(vmu_dir_t));    /* Start in a clean room */
        memcpy(dh->dirblocks[u].filename, names + u, 2);
        dh->dirblocks[u].filetype = 0xff;
    }

    return (vmu_fh_t *)dh;
}

/* opendir function */
static vmu_fh_t *vmu_open_dir(maple_device_t * dev) {
    vmu_dir_t   * dirents;
    int     dircnt;
    vmu_dh_t    * dh;

    /* Read the VMU's directory */
    if(vmufs_readdir(dev, &dirents, &dircnt) < 0)
        return NULL;

    /* Allocate a handle for the dir blocks */
    if(!(dh = malloc(sizeof(vmu_dh_t))))
        return NULL;
    dh->strtype = VMU_DIR;
    dh->dirblocks = dirents;
    dh->rootdir = 0;
    dh->entry = 0;
    dh->dircnt = dircnt;
    dh->dev = dev;

    return (vmu_fh_t *)dh;
}

/* openfile function */
static vmu_fh_t *vmu_open_file(maple_device_t * dev, const char *path, int mode) {
    vmu_fh_t    * fd;       /* file descriptor */
    int     realmode, rv;
    void        * data;
    int     datasize;
    vmu_pkg_t vmu_pkg;

    /* Malloc a new fh struct */
    if(!(fd = malloc(sizeof(vmu_fh_t))))
        return NULL;

    /* Fill in the filehandle struct */
    fd->strtype = VMU_FILE;
    fd->mode = mode;
    strncpy(fd->path, path, 16);
    strncpy(fd->name, path + 4, 12);
    fd->loc = 0;
    fd->start = 0;
    fd->dev = dev;
    fd->header = NULL;
    fd->raw = mode & O_META;

    /* What mode are we opening in? If we're reading or writing without O_TRUNC
       then we need to read the old file if there is one. */
    realmode = mode & O_MODE_MASK;

    if(realmode == O_RDONLY || ((realmode == O_RDWR || realmode == O_WRONLY) && !(mode & O_TRUNC))) {
        /* Try to open it */
        rv = vmufs_read(dev, fd->name, &data, &datasize);

        if(rv < 0) {
            if(realmode == O_RDWR || realmode == O_WRONLY) {
                /* In some modes failure is ok -- flag to setup a blank first block. */
                datasize = -1;
            }
            else {
                free(fd);
                return NULL;
            }
        }
    }
    else {
        /* We're writing with truncate... flag to setup a blank first block. */
        datasize = -1;
    }

    /* We were flagged to set up a blank first block */
    if(datasize == -1) {
        data = malloc(512);
        if(data == NULL) {
            free(fd);
            return NULL;
        }
        datasize = 512;
        memset(data, 0, 512);
    } else if(!fd->raw && !vmu_pkg_parse(data, datasize, &vmu_pkg)) {
        fd->header = vmu_pkg_dup(&vmu_pkg);
        fd->start = (unsigned int)vmu_pkg.data - (unsigned int)data;
    }

    fd->data = (uint8 *)data;
    fd->filesize = datasize / 512;

    if(fd->filesize == 0) {
        dbglog(DBG_WARNING, "VMUFS: can't open zero-length file %s\n", path);
        free(fd->data);
        free(fd);
        return NULL;
    }

    return fd;
}

/* open function */
static void * vmu_open(vfs_handler_t * vfs, const char *path, int mode) {
    maple_device_t  * dev;      /* maple bus address of the vmu unit */
    vmu_fh_t    *fh;

    (void)vfs;

    if(!*path || (path[0] == '/' && !path[1])) {
        /* /vmu should be opened */
        fh = vmu_open_vmu_dir();
    }
    else {
        /* Figure out which vmu slot is being opened */
        dev = vmu_path_to_addr(path);

        /* printf("VMUFS: card address is %02x\n", addr); */
        if(dev == NULL) return 0;

        /* Check for open as dir */
        if(strlen(path) == 3 || (strlen(path) == 4 && path[3] == '/')) {
            if(!(mode & O_DIR)) return 0;

            fh = vmu_open_dir(dev);
        }
        else {
            if(mode & O_DIR) return 0;

            fh = vmu_open_file(dev, path, mode);
        }
    }

    if(fh == NULL) return 0;

    /* link the fh onto the top of the list */
    mutex_lock(&fh_mutex);
    TAILQ_INSERT_TAIL(&vmu_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    return (void *)fh;
}

/* Verify that a given hnd is actually in the list */
static int vmu_verify_hnd(void * hnd, int type) {
    vmu_fh_t    *cur;
    int     rv;

    rv = 0;

    mutex_lock(&fh_mutex);
    TAILQ_FOREACH(cur, &vmu_fh, listent) {
        if((void *)cur == hnd) {
            rv = 1;
            break;
        }
    }
    mutex_unlock(&fh_mutex);

    if(rv)
        return type == VMU_ANY ? 1 : ((int)cur->strtype == type);
    else
        return 0;
}

/* write a file out before closing it: we aren't perfect on error handling here */
static int vmu_write_close(void * hnd) {
    vmu_fh_t    *fh = (vmu_fh_t*)hnd;
    uint8_t     *data = fh->data + fh->start;
    int         ret, data_len = fh->filesize * 512;
    vmu_pkg_t   *hdr = fh->header ?: dft_header;

    if(!fh->raw) {
        if(!hdr) {
            dbglog(DBG_WARNING, "VMUFS: file written without header\n");
        } else {
            hdr->data_len = data_len;
            hdr->data = data;

            ret = vmu_pkg_build(hdr, &data, &data_len);
            if(ret < 0)
                return ret;
        }
    }

    ret = vmufs_write(fh->dev, fh->name, data, data_len, VMUFS_OVERWRITE);

    if(hdr)
        free(data);

    return ret;
}

/* close a file */
static int vmu_close(void * hnd) {
    vmu_fh_t *fh;
    int st, retval = 0;

    /* Check the handle */
    if(!vmu_verify_hnd(hnd, VMU_ANY)) {
        errno = EBADF;
        return -1;
    }

    fh = (vmu_fh_t *)hnd;

    switch(fh->strtype) {
        case VMU_DIR: {
            vmu_dh_t * dir = (vmu_dh_t *)hnd;

            if(dir->dirblocks)
                free(dir->dirblocks);

            break;
        }

        case VMU_FILE:
            if((fh->mode & O_MODE_MASK) == O_WRONLY ||
                    (fh->mode & O_MODE_MASK) == O_RDWR) {
                if ((st = vmu_write_close(hnd))) {
                    if (st == -7)
                        errno = ENOSPC;
                    else
                        errno = EIO;
                    retval = -1;
                }
            }

            if(fh->header) {
                free(fh->header->eyecatch_data);
                free(fh->header->icon_data);
                free(fh->header);
            }
            free(fh->data);
            break;

    }

    /* Look for the one to get rid of */
    mutex_lock(&fh_mutex);
    TAILQ_REMOVE(&vmu_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    free(fh);
    return retval;
}

/* read function */
static ssize_t vmu_read(void * hnd, void *buffer, size_t cnt) {
    vmu_fh_t *fh;

    /* Check the handle */
    if(!vmu_verify_hnd(hnd, VMU_FILE))
        return -1;

    fh = (vmu_fh_t *)hnd;

    /* make sure we're opened for reading */
    if((fh->mode & O_MODE_MASK) != O_RDONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return 0;

    /* Check size */
    cnt = (fh->loc + cnt) > (fh->filesize * 512) ?
          (fh->filesize * 512 - fh->loc) : cnt;

    /* Reads past EOF return 0 */
    if((long)cnt < 0)
        return 0;

    /* Copy out the data */
    memcpy(buffer, fh->data + fh->loc + fh->start, cnt);
    fh->loc += cnt;

    return cnt;
}

/* write function */
static ssize_t vmu_write(void * hnd, const void *buffer, size_t cnt) {
    vmu_fh_t    *fh;
    void        *tmp;
    int     n;

    /* Check the handle we were given */
    if(!vmu_verify_hnd(hnd, VMU_FILE))
        return -1;

    fh = (vmu_fh_t *)hnd;

    /* Make sure we're opened for writing */
    if((fh->mode & O_MODE_MASK) != O_WRONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return -1;

    /* Check to make sure we have enough room in data */
    if(fh->loc + fh->start + cnt > fh->filesize * 512) {
        /* Figure out the new block count */
        n = ((fh->loc + fh->start + cnt) - (fh->filesize * 512));

        if(n & 511)
            n = (n + 512) & ~511;

        n = n / 512;

        if(__is_defined(VMUFS_DEBUG))
            dbglog(DBG_KDEBUG, "VMUFS: extending file's filesize by %d\n", n);

        /* We alloc another 512*n bytes for the file */
        tmp = realloc(fh->data, (fh->filesize + n) * 512);

        if(!tmp) {
            dbglog(DBG_ERROR, "VMUFS: unable to realloc another 512 bytes\n");
            return -1;
        }

        /* Assign the new pointer and clear out the new space */
        fh->data = tmp;
        memset(fh->data + fh->filesize * 512, 0, 512 * n);
        fh->filesize += n;
    }

    /* insert the data in buffer into fh->data at fh->loc */
    if(__is_defined(VMUFS_DEBUG)) {
        dbglog(DBG_KDEBUG, "VMUFS: adding %d bytes of data at loc %ld (%ld avail)\n",
               cnt, fh->loc, fh->filesize * 512);
    }

    memcpy(fh->data + fh->loc + fh->start, buffer, cnt);
    fh->loc += cnt;

    return cnt;
}

/* mmap a file */
/* note: writing past EOF will invalidate your pointer */
static void *vmu_mmap(void * hnd) {
    vmu_fh_t *fh;

    /* Check the handle */
    if(!vmu_verify_hnd(hnd, VMU_FILE))
        return NULL;

    fh = (vmu_fh_t *)hnd;

    return fh->data + fh->start;
}

/* Seek elsewhere in a file */
static off_t vmu_seek(void * hnd, off_t offset, int whence) {
    vmu_fh_t *fh;

    /* Check the handle */
    if(!vmu_verify_hnd(hnd, VMU_FILE))
        return -1;

    fh = (vmu_fh_t *)hnd;

    /* Update current position according to arguments */
    switch(whence) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            offset += fh->loc;
            break;
        case SEEK_END:
            offset = fh->filesize * 512 - offset;
            break;
        default:
            return -1;
    }

    /* Check bounds; allow seek past EOF. */
    if(offset < 0)
        offset = 0;

    fh->loc = offset;

    return fh->loc;
}

/* tell the current position in the file */
static off_t vmu_tell(void * hnd) {
    /* Check the handle */
    if(!vmu_verify_hnd(hnd, VMU_FILE))
        return -1;

    return ((vmu_fh_t *) hnd)->loc;
}

/* return the filesize */
static size_t vmu_total(void * fd) {
    /* Check the handle */
    if(!vmu_verify_hnd(fd, VMU_FILE))
        return -1;

    /* note that all filesizes are multiples of 512 for the vmu */
    return (((vmu_fh_t *) fd)->filesize) * 512;
}

/* read a directory handle */
static dirent_t *vmu_readdir(void * fd) {
    vmu_dh_t    *dh;
    vmu_dir_t   *dir;

    /* Check the handle */
    if(!vmu_verify_hnd(fd, VMU_DIR)) {
        errno = EBADF;
        return NULL;
    }

    dh = (vmu_dh_t*)fd;

    /* printf("VMUFS: readdir on entry %d of %d\n", dh->entry, dh->dircnt); */

    /* Check if we have any entries left */
    if(dh->entry >= dh->dircnt)
        return NULL;

    /* printf("VMUFS: reading non-null entry %d\n", dh->entry); */

    /* Ok, extract it and fill the dirent struct */
    dir = dh->dirblocks + dh->entry;

    if(dh->rootdir) {
        dh->dirent.size = -1;
        dh->dirent.attr = O_DIR;
    }
    else {
        dh->dirent.size = dir->filesize * 512;
        dh->dirent.attr = 0;
    }

    strncpy(dh->dirent.name, dir->filename, 12);
    dh->dirent.name[12] = 0;
    dh->dirent.time = 0;    /* FIXME */

    /* Move to the next entry */
    dh->entry++;

    return &dh->dirent;
}

static int vmu_ioctl(void *fd, int cmd, va_list ap) {
    vmu_fh_t *fh = (vmu_fh_t*)fd;
    vmu_dh_t *dh = (vmu_dh_t*)fd;
    vmu_pkg_t *old_hdr, *hdr = NULL;
    const vmu_pkg_t *new_hdr;

    if(!dh || (dh->strtype == VMU_DIR && !dh->rootdir)) {
        errno = EBADF;
        return -1;
    }

    switch(cmd) {
    case IOCTL_VMU_SET_HDR:
        new_hdr = va_arg(ap, const vmu_pkg_t *);
        if(new_hdr) {
            hdr = vmu_pkg_dup(new_hdr);
            if(!hdr)
                return -1;
        }

        if(fh->strtype == VMU_FILE) {
            old_hdr = fh->header;
            fh->header = hdr;
        } else {
            old_hdr = dft_header;
            dft_header = hdr;
        }

        if(old_hdr) {
            free(old_hdr->icon_data);
            free(old_hdr->eyecatch_data);
            free(old_hdr);
        }
        break;
    }

    return 0;
}

/* Delete a file */
static int vmu_unlink(vfs_handler_t * vfs, const char *path) {
    maple_device_t  * dev = NULL;   /* address of VMU */

    (void)vfs;

    /* convert path to valid VMU address */
    dev = vmu_path_to_addr(path);

    if(dev == NULL) {
        dbglog(DBG_ERROR, "VMUFS: vmu_unlink on invalid path '%s'\n", path);
        return -1;
    }

    return vmufs_delete(dev, path + 4);
}

static int vmu_stat(vfs_handler_t *vfs, const char *path, struct stat *st,
                    int flag) {
    maple_device_t *dev;
    size_t len = strlen(path);

    (void)vfs;
    (void)flag;

    /* Root directory '/vmu' */
    if(len == 0 || (len == 1 && *path == '/')) {
        memset(st, 0, sizeof(struct stat));
        st->st_dev = (dev_t)('v' | ('m' << 8) | ('u' << 16));
        st->st_mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | 
            S_IXGRP | S_IROTH | S_IXOTH;
        st->st_size = -1;
        st->st_nlink = 2;

        return 0;
    }
    else if(len > 4) {
            /* The only thing we can stat right now is full VMUs, and what that
       will get you is a count of free blocks in "size". */
        /* XXXX: This isn't right, but it'll keep the old functionality of this
           function, at least. */
        errno = ENOTDIR;
        return -1;
    }

    dev = vmu_path_to_addr(path);

    if(!dev) {
        errno = ENOENT;
        return -1;
    }

    /* Get the number of free blocks */
    memset(st, 0, sizeof(struct stat));
    st->st_dev = (dev_t)((uintptr_t)dev);
    st->st_mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | 
        S_IXGRP | S_IROTH | S_IXOTH;
    st->st_size = vmufs_free_blocks(dev);
    st->st_nlink = 1;
    st->st_blksize = 512;

    return 0;
}

static int vmu_fcntl(void *fd, int cmd, va_list ap) {
    vmu_fh_t *fh;
    int rv = -1;

    (void)ap;

    /* Check the handle */
    if(!vmu_verify_hnd(fd, VMU_ANY)) {
        errno = EBADF;
        return -1;
    }

    fh = (vmu_fh_t *)fd;

    switch(cmd) {
        case F_GETFL:

            if(fh->strtype)
                rv = fh->mode;
            else
                rv = O_RDONLY | O_DIR;

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

static int vmu_rewinddir(void * fd) {
    vmu_dh_t *dh;

    /* Check the handle */
    if(!vmu_verify_hnd(fd, VMU_DIR)) {
        errno = EBADF;
        return -1;
    }

    /* Rewind to the beginning of the directory. */
    dh = (vmu_dh_t*)fd;
    dh->entry = 0;

    /* TODO: Technically, we need to re-scan the directory here, but for now we
       will punt on that requirement. */

    return 0;
}

static int vmu_fstat(void *fd, struct stat *st) {
    vmu_fh_t *fh;

    /* Check the handle */
    if(!vmu_verify_hnd(fd, VMU_ANY)) {
        errno = EBADF;
        return -1;
    }

    fh = (vmu_fh_t *)fd;
    memset(st, 0, sizeof(struct stat));
    st->st_dev = (dev_t)((uintptr_t)fh->dev);
    st->st_mode =  S_IRWXU | S_IRWXG | S_IRWXO;
    st->st_mode |= (fh->strtype == VMU_DIR) ? S_IFDIR : S_IFREG;
    st->st_size = (fh->strtype == VMU_DIR) ? 
        vmufs_free_blocks(((vmu_dh_t *)fh)->dev) : (int)(fh->filesize * 512);
    st->st_nlink = (fh->strtype == VMU_DIR) ? 2 : 1;
    st->st_blksize = 512;

    return 0;
}

/* handler interface */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/vmu",         /* name */
        0,              /* tbfi */
        0x00010000,     /* Version 1.0 */
        0,              /* flags */
        NMMGR_TYPE_VFS, /* VFS handler */
        NMMGR_LIST_INIT
    },
    0, NULL,            /* In-kernel, privdata */

    vmu_open,
    vmu_close,
    vmu_read,
    vmu_write,
    vmu_seek,
    vmu_tell,
    vmu_total,
    vmu_readdir,
    vmu_ioctl,
    NULL,               /* rename/move */
    vmu_unlink,
    vmu_mmap,
    NULL,               /* complete */
    vmu_stat,
    NULL,               /* mkdir */
    NULL,               /* rmdir */
    vmu_fcntl,
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    vmu_rewinddir,
    vmu_fstat
};

int fs_vmu_init(void) {
    TAILQ_INIT(&vmu_fh);
    mutex_init(&fh_mutex, MUTEX_TYPE_NORMAL);
    return nmmgr_handler_add(&vh.nmmgr);
}

int fs_vmu_shutdown(void) {
    vmu_fh_t * c, * n;

    mutex_lock(&fh_mutex);

    TAILQ_FOREACH_SAFE(c, &vmu_fh, listent, n) {

        switch(c->strtype) {
            case VMU_DIR: {
                vmu_dh_t * dir = (vmu_dh_t *)c;
                free(dir->dirblocks);
                break;
            }

            case VMU_FILE:

                if((c->mode & O_MODE_MASK) == O_WRONLY ||
                        (c->mode & O_MODE_MASK) == O_RDWR) {
                    dbglog(DBG_ERROR, "fs_vmu_shutdown: still-open file '%s' not written!\n", c->path);
                }

                free(c->data);
                break;
        }

        free(c);
    }

    mutex_unlock(&fh_mutex);
    mutex_destroy(&fh_mutex);

    if(dft_header) {
        free(dft_header->eyecatch_data);
        free(dft_header->icon_data);
        free(dft_header);
    }

    return nmmgr_handler_remove(&vh.nmmgr);
}
