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
#include <kos/opts.h>
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

Define VMUFS_DEBUG in kos/opts.h, in your CFLAGS, or here if you want copious
debug output.
*/

#define VMU_DIR     0
#define VMU_FILE    1
#define VMU_ANY     -1  /* Used for checking validity */

/* File header */
typedef struct vmufs_rawhdr_str {
    uint8_t *buffer;                    /* buffer holding the header */
    size_t buffer_length;               /* buffer length in bytes */
} vmufs_rawhdr_t;

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
    uint32 blks;                        /* block count from dirent (each blk is 512-byte) */
    uint32 filelength;                  /* file length in bytes */
    uint8 *data;                        /* copy of the whole file */
    bool raw;                           /* file opened as raw */
    vmu_hdr_status_t header_status;     /* parse status of VMU file header */
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

/* Default VMU header for new files */
static vmufs_rawhdr_t default_header = { NULL, 0 };


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

                dbglog(DBG_SOURCE(VMUFS_DEBUG), "vmu_open_vmu_dir: found memcard (%c%d)\n",
                       'a' + p, u);
            }
        }
    }

    dbglog(DBG_SOURCE(VMUFS_DEBUG), "# of memcards found: %d\n", num);

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

    errno = 0;
    return (vmu_fh_t *)dh;
}

/* opendir function */
static vmu_fh_t *vmu_open_dir(maple_device_t * dev) {
    vmu_dir_t   * dirents;
    int     dircnt;
    vmu_dh_t    * dh;

    /* Read the VMU's directory */
    if(vmufs_readdir(dev, &dirents, &dircnt) < 0) {
        errno = ENOENT;
        return NULL;
    }

    /* Allocate a handle for the dir blocks */
    if(!(dh = malloc(sizeof(vmu_dh_t))))
        return NULL;

    dh->strtype = VMU_DIR;
    dh->dirblocks = dirents;
    dh->rootdir = 0;
    dh->entry = 0;
    dh->dircnt = dircnt;
    dh->dev = dev;

    errno = 0;
    return (vmu_fh_t *)dh;
}

/* openfile function */
static vmu_fh_t *vmu_open_file(maple_device_t * dev, const char *path, int mode) {
    vmu_fh_t    * fd;       /* file descriptor */
    int     realmode, rv;
    void        * data = NULL;
    int     datasize;
    vmu_pkg_t vmu_pkg;
    uint8   filetype;

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
    fd->raw = mode & O_META;
    fd->header_status = VMUHDR_STATUS_NEWFILE;

    /* What mode are we opening in? If we're reading or writing without O_TRUNC
       then we need to read the old file if there is one. */
    realmode = mode & O_MODE_MASK;

    if(realmode == O_RDONLY && (mode & O_TRUNC)) {
        errno = EINVAL;
        goto error;
    }
    //if(!(mode & O_META) && strcmp(fd->name, "ICONDATA_VMS") == 0) {
    //    dbglog(DBG_SOURCE(VMUFS_DEBUG), "vmu_open_file: attempt to open ICONDATA_VMS without raw mode\n");
    //}

    if(realmode == O_RDONLY || ((realmode == O_RDWR || realmode == O_WRONLY) && !(mode & O_TRUNC))) {
        /* Try to open it */
        rv = vmufs_read(dev, fd->name, &data, &datasize, &filetype);

        if(rv != 0 && rv != -2) {
            /* Invalid device, read error or truncated file */
            errno = EIO;
            goto error;
        }
        else if(rv == -2) {
            if(realmode == O_RDONLY) {
                errno = ENOENT;
                goto error;
            }

            //if(!(mode & O_CREAT)) {
            //    //dbglog(DBG_INFO, "vmu_open_file: file %s not found\n", path);
            //    errno = ENOENT;
            //    goto error;
            //}

            /* File not found, flag to setup a blank first block. */
            datasize = -1;
        }
        else if(filetype != 0x33) {
            dbglog(DBG_WARNING, "VMUFS: file %s isn't DATA type\n", path);
            errno = EFTYPE;
            goto error;
        }
    }
    else {
        /* We're writing with truncate... flag to setup a blank first block. */
        datasize = -1;
    }

    /* We were flagged to set up a blank first block */
    if(datasize == -1) {
        fd->blks = 1;
        fd->filelength = 0;

        data = malloc(fd->blks * 512);
        if(data == NULL) {
            goto error;
        }
        memset(data, 0, 512);
    } else if(fd->raw)  {
        fd->filelength = datasize;
        fd->blks = datasize / 512;
    } else {
        /* Parse header but ignore any bad checksum */
        rv = vmu_pkg_parse_ex(data, datasize, &vmu_pkg, true);
        if(rv == -2) {
            /* vmufs_read() doesn't lie about the file size, file is totally corrupted */
            errno = EIO;
            goto error;
        }

        fd->header_status = rv == -1 ? VMUHDR_STATUS_BADCRC : VMUHDR_STATUS_OK;
        fd->start = (unsigned int)vmu_pkg.data - (unsigned int)data;
        fd->filelength = vmu_pkg.data_len;
        fd->blks = datasize / 512;
    }

    fd->data = (uint8 *)data;

    if(fd->blks == 0) {
        dbglog(DBG_WARNING, "VMUFS: can't open zero-length file %s\n", path);
        errno = ENODATA;
        goto error;
    }

    return fd;

error:
    free(data);
    free(fd);
    return NULL;
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
        if(dev == NULL) {
            errno = ENODEV;
            return NULL;
        }

        /* Check for open as dir */
        if(strlen(path) == 3 || (strlen(path) == 4 && path[3] == '/')) {
            if(!(mode & O_DIR)) {
                errno = EISDIR;
                return NULL;
            }

            fh = vmu_open_dir(dev);
        }
        else {
            if(mode & O_DIR) {
                errno = ENOTDIR;
                return NULL;
            }

            fh = vmu_open_file(dev, path, mode);
        }
    }

    if(fh == NULL) return NULL;

    /* link the fh onto the top of the list */
    mutex_lock(&fh_mutex);
    TAILQ_INSERT_TAIL(&vmu_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    return (void *)fh;
}

/* Ovewrite, remove or insert a raw header */
static int vmu_overwrite_header(vmu_fh_t *fh, const uint8_t *hdr, size_t hdr_size) {
    uint8_t *data = fh->data;
    size_t old_hdr_size = (size_t)fh->start;
    size_t new_blks;

    if(hdr_size > old_hdr_size) {
        new_blks = fh->filelength + hdr_size;
        new_blks = (new_blks + 511) / 512;

        if(new_blks > fh->blks) {
            dbglog(DBG_SOURCE(VMUFS_DEBUG), "VMUFS: extending file's blocks by %d\n", new_blks);

            data = realloc(data, new_blks * 512);
            if(!data) {
                dbglog(DBG_ERROR, "VMUFS: unable to realloc another %d bytes\n", new_blks * 512);
                return -1;
            }

            fh->data = data;
            fh->blks = new_blks;
        }
    }

    /* Move payload data */
    memmove(data + hdr_size, data + old_hdr_size, fh->filelength);

    /* Write new header */
    fh->start = hdr_size;
    memcpy(data, hdr, hdr_size);

    fh->header_status = hdr_size < 1 ? VMUHDR_STATUS_NEWFILE : VMUHDR_STATUS_OK;
    return 0;
}

/* Ovewrite, remove or insert a raw header from vmu_pkg */
static int vmu_overwrite_header_from_pkg(vmu_fh_t *fh, const vmu_pkg_t *new_hdr) {
    int rv;
    int buffer_length;
    uint8_t *buffer;
    vmu_pkg_t pkg;

    if(fh->raw) {
        dbglog(DBG_ERROR, "VMUFS: can't set header in raw mode\n");
        return -1;
    }

    if((fh->mode & O_MODE_MASK) != O_WRONLY && (fh->mode & O_MODE_MASK) != O_RDWR) {
        /* File is read-only */
        return -1;
    }

    if(!new_hdr) {
        /* Remove header (this should be illegal) */
        rv = vmu_overwrite_header(fh, NULL, 0);
        return rv;
    }

    /* Don't write payload data twice, temporally set to zero. */
    memcpy(&pkg, new_hdr, sizeof(vmu_pkg_t));
    pkg.data_len = 0;

    if(new_hdr->data_len > 0) {
        dbglog(DBG_SOURCE(VMUFS_DEBUG), "VMUFS: Field vmu_pkg_t::data_len will be ignored\n");
    }

    rv = vmu_pkg_build(&pkg, &buffer, &buffer_length);
    if(rv) {
        /* Build failed */
        return -1;
    }

    rv = vmu_overwrite_header(fh, buffer, (size_t)buffer_length);
    free(buffer);

    return rv;
}

/* Compile and replace default header */
static int vmu_change_default_header(const vmu_pkg_t *new_hdr) {
    int rv;
    int buffer_length;
    uint8_t *buffer, *old_buffer;
    vmu_pkg_t pkg;

    old_buffer = default_header.buffer;

    if(new_hdr) {
        /* Don't write payload data twice, temporally set to zero. */
        memcpy(&pkg, new_hdr, sizeof(vmu_pkg_t));
        pkg.data_len = 0;

        rv = vmu_pkg_build(&pkg, &buffer, &buffer_length);
        if(rv) {
            /* Build failed */
            return rv;
        }

        default_header.buffer = buffer;
        default_header.buffer_length = (size_t)buffer_length;
    }
    else {
        memset(&default_header, 0x00, sizeof(vmufs_rawhdr_t));
    }

    free(old_buffer);
    return 0;
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
    int         ret;
    uint32      buffer_used, blks_bytes_used;

    int flags = VMUFS_OVERWRITE;
    uint8_t *def_hdr = default_header.buffer;
    size_t def_len = default_header.buffer_length;

    if(fh->raw) {
        /* In raw mode new files are written as DATA */
        goto perform_write;
    }

    if(def_hdr && fh->start < 1) {
        /* Write in buffer the default header */
        ret = vmu_overwrite_header(fh, def_hdr, def_len);
        if(ret) {
            /* Write failed */
            return ret;
        }
    }

    if(fh->start < 1) {
        dbglog(DBG_WARNING, "VMUFS: file written without header\n");
    } else {
        vmu_pkg_crc_set(fh->data, (int)fh->filelength);
    }

perform_write:
    /* Count the amount blocks required */
    buffer_used = fh->start + fh->filelength;
    blks_bytes_used = ((buffer_used + 511) / 512) * 512;

    /* Write everything */
    ret = vmufs_write(fh->dev, fh->name, fh->data, blks_bytes_used, flags);

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
                if((st = vmu_write_close(hnd))) {
                    if(st == -7)
                        errno = ENOSPC;
                    else
                        errno = EIO;
                    retval = -1;
                }
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
    if((fh->loc + cnt) > fh->filelength)
        cnt = fh->filelength - fh->loc;

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

    /* Check to make sure we have enough room in buffer */
    if(fh->loc + fh->start + cnt > fh->blks * 512) {
        /* Figure out the new block count */
        n = ((fh->loc + fh->start + cnt) - (fh->blks * 512));

        if(n & 511)
            n = (n + 512) & ~511;

        n = n / 512;

        dbglog(DBG_SOURCE(VMUFS_DEBUG), "VMUFS: extending file's blocks by %d\n", n);

        /* We alloc another 512*n bytes for the file */
        tmp = realloc(fh->data, (fh->blks + n) * 512);

        if(!tmp) {
            dbglog(DBG_ERROR, "VMUFS: unable to realloc another %d bytes\n", n * 512);
            return -1;
        }

        /* Assign the new pointer */
        fh->data = tmp;
        memset(fh->data + fh->blks * 512, 0, 512 * n);
        fh->blks += n;
    }

    /* insert the data in buffer into fh->data at fh->loc */
    dbglog(DBG_SOURCE(VMUFS_DEBUG), "VMUFS: adding %d bytes of data at loc %ld (%ld avail)\n",
           cnt, fh->loc, fh->blks * 512);

    memcpy(fh->data + fh->loc + fh->start, buffer, cnt);
    fh->loc += cnt;

    if((uint32_t)fh->loc > fh->filelength)
        fh->filelength = (uint32_t)fh->loc;

    return cnt;
}

/* mmap a file */
/* notes:
    - writing past EOF will invalidate your pointer
    - buffer can be invalidated when is reallocated
*/
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
            offset = (off_t)fh->filelength - offset;
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

    return ((vmu_fh_t *)fd)->filelength;
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
    const vmu_pkg_t *new_hdr;

    if(!dh || (dh->strtype == VMU_DIR && !dh->rootdir)) {
        errno = EBADF;
        return -1;
    }

    switch(cmd) {
        case IOCTL_VMU_SET_HDR:
            new_hdr = va_arg(ap, const vmu_pkg_t *);

            if(fh->strtype == VMU_FILE) {
                return vmu_overwrite_header_from_pkg(fh, new_hdr);
            }
            return vmu_change_default_header(new_hdr);
        case IOCTL_VMU_GET_HDR_STATE:
            if(fh->strtype != VMU_FILE) {
                /* Not applicable */
                return VMUHDR_STATUS_OK;
            }
            return fh->header_status;
        case IOCTL_VMU_GET_REALFSIZE:
            if(fh->strtype == VMU_FILE) {
                _Static_assert(sizeof(int) >= sizeof(fh->blks));
                return (int)fh->blks * 512;
            } else {
                errno = EISDIR;
                return -1;
            }
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
        vmufs_free_blocks(((vmu_dh_t *)fh)->dev) : (int)(fh->blks * 512);
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

    /* Deallocate default header */
    vmu_change_default_header(NULL);

    return nmmgr_handler_remove(&vh.nmmgr);
}
