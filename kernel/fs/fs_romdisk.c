/* KallistiOS ##version##

   fs_romdisk.c
   Copyright (C) 2001, 2002, 2003 Megan Potter
   Copyright (C) 2012, 2013, 2014, 2016 Lawrence Sebald

*/

/*

This module implements the Linux ROMFS file system for reading a pre-made boot romdisk.

To create a romdisk image, you will need the program "genromfs". This program was designed
for Linux but ought to compile under Cygwin. The source for this utility can be found
on sunsite.unc.edu in /pub/Linux/system/recovery/, or as a package under Debian "genromfs".

*/

#include <kos/thread.h>
#include <kos/mutex.h>
#include <kos/fs_romdisk.h>
#include <kos/opts.h>
#include <kos/dbglog.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#define ROMFS_MAXFN 128
#define ROMFH_HRD 0
#define ROMFH_DIR 1
#define ROMFH_REG 2
#define ROMFH_LNK 3
#define ROMFH_BLK 4
#define ROMFH_CHR 5
#define ROMFH_SCK 6
#define ROMFH_FIF 7
#define ROMFH_EXEC 8

#define RD_VN_MAX 16
#define RD_FN_MAX 16

/* Header definitions from Linux ROMFS documentation; all integer quantities are
   expressed in big-endian notation. Unfortunately the ROMFS guys were being
   clever and made this header a variable length depending on the size of
   the volume name *groan*. Its size will be a multiple of 16 bytes though. */
typedef struct {
    char        magic[8];               /* Should be "-rom1fs-" */
    uint32_t    full_size;              /* Full size of the file system */
    uint32_t    checksum;               /* Checksum */
    char        volume_name[RD_VN_MAX]; /* Volume name (zero-terminated) */
} romdisk_hdr_t;

/* File header info; note that this header plus filename must be a multiple of
   16 bytes, and the following file data must also be a multiple of 16 bytes. */
typedef struct {
    uint32_t  next_header;            /* Offset of next header */
    uint32_t  spec_info;              /* Spec info */
    uint32_t  size;                   /* Data size */
    uint32_t  checksum;               /* File checksum */
    char      filename[RD_FN_MAX];    /* File name (zero-terminated) */
} romdisk_file_t;


/* Util function to reverse the byte order of a uint32_t */
static uint32_t ntohl_32(const void *data) {
    const uint8_t *d = (const uint8_t *)data;
    return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
}

/********************************************************************************/

/* A list of the following */
struct rd_image;
typedef LIST_HEAD(rdi_list, rd_image) rdi_list_t;

/* A single mounted romdisk image; a pointer to one of these will be in our
   VFS struct for each mount. */
typedef struct rd_image {
    LIST_ENTRY(rd_image) list_ent;  /* List entry */

    bool                own_buffer; /* Do we own the memory? */
    const uint8_t       *image;     /* The actual image */
    uint32_t            files;      /* Offset in the image to the files area */
    vfs_handler_t       *vfsh;      /* Our VFS mount struct */
} rd_image_t;

/* Global list of mounted romdisks */
static rdi_list_t romdisks;

/********************************************************************************/
/* File primitives */

/* File handles.. I could probably do this with a linked list, but I'm just
   too lazy right now. =) */
static struct {
    uint32_t    index;      /* romfs image index */
    bool        dir;        /* true if a directory */
    uint32_t    ptr;        /* Current read position in bytes */
    uint32_t    size;       /* Length of file in bytes */
    dirent_t    dirent;     /* A static dirent to pass back to clients */
    rd_image_t  *mnt;       /* Which mount instance are we using? */
} fh[FS_ROMDISK_MAX_FILES];

#define FH_INDEX_FREE 0
#define FH_INDEX_RESERVED -1

/* File type */
#define ROMFH_DIR   1

#define ROMFH_MASK  3

/* Mutex for file handles */
static mutex_t fh_mutex;

/* Given a filename and a starting romdisk directory listing (byte offset),
   search for the entry in the directory and return the byte offset to its
   entry. */
static uint32_t romdisk_find_object(rd_image_t *mnt, const char *fn, size_t fnlen, bool dir, uint32_t offset) {
    uint32_t          i, ni, type;
    const romdisk_file_t    *fhdr;

    i = offset;

    do {
        /* Locate the entry, next pointer, and type info */
        fhdr = (const romdisk_file_t *)(mnt->image + i);
        ni = ntohl_32(&fhdr->next_header);
        type = ni & 0x0f;
        ni = ni & 0xfffffff0;

        /* Check the type */
        if(!dir) {
            if((type & 3) != 2) {
                i = ni;

                if(!i)
                    break;
                else
                    continue;
            }
        }
        else {
            if((type & 3) != 1) {
                i = ni;

                if(!i)
                    break;
                else
                    continue;
            }
        }

        /* Check filename */
        if((strlen(fhdr->filename) == fnlen) && (!strncasecmp(fhdr->filename, fn, fnlen))) {
            /* Match: return this index */
            return i;
        }

        i = ni;
    }
    while(i != 0);

    /* Didn't find it */
    return 0;
}

/* Locate an object anywhere in the image, starting at the root, and
   expecting a fully qualified path name. This is analogous to the
   find_object_path in iso9660.

   fn:      object filename (absolute path)
   dir:     false if looking for a file, true if looking for a dir

   It will return an offset in the romdisk image for the object. */
static uint32_t romdisk_find(rd_image_t *mnt, const char *fn, bool dir) {
    const char      *cur;
    uint32_t        i;
    const romdisk_file_t    *fhdr;

    /* If the object is in a sub-tree, traverse the trees looking
       for the right directory. */
    i = mnt->files;

    while((cur = strchr(fn, '/'))) {
        if(cur != fn) {
            i = romdisk_find_object(mnt, fn, cur - fn, true, i);

            if(i == 0) return 0;

            fhdr = (const romdisk_file_t *)(mnt->image + i);
            i = ntohl_32(&fhdr->spec_info);
        }

        fn = cur + 1;
    }

    /* Locate the file in the resulting directory */
    if(*fn) {
        i = romdisk_find_object(mnt, fn, strlen(fn), dir, i);
        return i;
    }
    else {
        if(!dir)
            return 0;
        else
            return i;
    }
}

/* Open a file or directory */
static void * romdisk_open(vfs_handler_t *vfs, const char *fn, int mode) {
    file_t          fd;
    uint32_t        filehdr;
    const romdisk_file_t    *fhdr;
    rd_image_t      *mnt = (rd_image_t *)vfs->privdata;

    /* Make sure they don't want to open things as writeable */
    if((mode & O_MODE_MASK) != O_RDONLY) {
        errno = EPERM;
        return NULL;
    }

    /* No blank filenames */
    if(fn[0] == 0)
        fn = "";

    /* Look for the file */
    filehdr = romdisk_find(mnt, fn + 1, mode & O_DIR);

    if(filehdr == 0) {
        errno = ENOENT;
        return NULL;
    }

    /* Find a free file handle */
    mutex_lock(&fh_mutex);

    for(fd = 0; fd < FS_ROMDISK_MAX_FILES; fd++)
        if(fh[fd].index == FH_INDEX_FREE) {
            fh[fd].index = FH_INDEX_RESERVED;
            break;
        }

    mutex_unlock(&fh_mutex);

    if(fd >= FS_ROMDISK_MAX_FILES) {
        errno = ENFILE;
        return NULL;
    }

    /* Fill the fd structure */
    fhdr = (const romdisk_file_t *)(mnt->image + filehdr);
    fh[fd].index = filehdr + sizeof(romdisk_file_t) + (strlen(fhdr->filename) / RD_FN_MAX) * RD_FN_MAX;
    fh[fd].dir = ((mode & O_DIR) != 0);
    fh[fd].ptr = 0;
    fh[fd].size = ntohl_32(&fhdr->size);
    fh[fd].mnt = mnt;

    return (void *)fd;
}

/* Close a file or directory */
static int romdisk_close(void *h) {
    file_t fd = (file_t)h;

    /* Check that the fd is valid */
    if(fd < FS_ROMDISK_MAX_FILES) {
        /* No need to lock the mutex: this is an atomic op */
        fh[fd].index = FH_INDEX_FREE;
    }
    return 0;
}

/* Read from a file */
static ssize_t romdisk_read(void *h, void *buf, size_t bytes) {
    file_t fd = (file_t)h;

    /* Check that the fd is valid */
    if(fd >= FS_ROMDISK_MAX_FILES || fh[fd].index == FH_INDEX_FREE || fh[fd].dir) {
        errno = EINVAL;
        return -1;
    }

    /* Is there enough left? */
    if((fh[fd].ptr + bytes) > fh[fd].size)
        bytes = fh[fd].size - fh[fd].ptr;

    /* Copy out the requested amount */
    memcpy(buf, fh[fd].mnt->image + fh[fd].index + fh[fd].ptr, bytes);
    fh[fd].ptr += bytes;

    return bytes;
}

/* Just to get the errno that might be better recognized upstream. */
static ssize_t romdisk_write(void *h, const void *buf, size_t bytes) {
    (void)h;
    (void)buf;
    (void)bytes;

    errno = ENXIO;
    return -1;
}

/* Seek elsewhere in a file */
static off_t romdisk_seek(void *h, off_t offset, int whence) {
    file_t fd = (file_t)h;

    /* Check that the fd is valid */
    if(fd >= FS_ROMDISK_MAX_FILES || fh[fd].index == FH_INDEX_FREE || fh[fd].dir) {
        errno = EBADF;
        return -1;
    }

    /* Update current position according to arguments */
    switch(whence) {
        case SEEK_SET:
            if(offset < 0) {
                errno = EINVAL;
                return -1;
            }

            fh[fd].ptr = offset;
            break;

        case SEEK_CUR:
            if(offset < 0 && ((uint32_t)-offset) > fh[fd].ptr) {
                errno = EINVAL;
                return -1;
            }

            fh[fd].ptr += offset;
            break;

        case SEEK_END:
            if(offset < 0 && ((uint32_t)-offset) > fh[fd].size) {
                errno = EINVAL;
                return -1;
            }

            fh[fd].ptr = fh[fd].size + offset;
            break;

        default:
            errno = EINVAL;
            return -1;
    }

    /* Check bounds */
    if(fh[fd].ptr > fh[fd].size) fh[fd].ptr = fh[fd].size;

    return fh[fd].ptr;
}

/* Tell where in the file we are */
static off_t romdisk_tell(void *h) {
    file_t fd = (file_t)h;

    if(fd >= FS_ROMDISK_MAX_FILES || fh[fd].index == FH_INDEX_FREE || fh[fd].dir) {
        errno = EINVAL;
        return -1;
    }

    return fh[fd].ptr;
}

/* Tell how big the file is */
static size_t romdisk_total(void *h) {
    file_t fd = (file_t)h;

    if(fd >= FS_ROMDISK_MAX_FILES || fh[fd].index == FH_INDEX_FREE || fh[fd].dir) {
        errno = EINVAL;
        return -1;
    }

    return fh[fd].size;
}

/* Read a directory entry */
static dirent_t *romdisk_readdir(void *h) {
    romdisk_file_t *fhdr;
    int type;
    file_t fd = (file_t)h;

    if(fd >= FS_ROMDISK_MAX_FILES || fh[fd].index == FH_INDEX_FREE || !fh[fd].dir) {
        errno = EBADF;
        return NULL;
    }

    /* This happens if we hit the end of the directory on advancing the pointer
       last time through. */
    if(fh[fd].ptr == (uint32_t)-1)
        return NULL;

    /* Get the current file header */
    fhdr = (romdisk_file_t *)(fh[fd].mnt->image + fh[fd].index + fh[fd].ptr);

    /* Update the pointer */
    fh[fd].ptr = ntohl_32(&fhdr->next_header);
    type = fh[fd].ptr & 0x0f;
    fh[fd].ptr = fh[fd].ptr & 0xfffffff0;

    if(fh[fd].ptr != 0)
        fh[fd].ptr = fh[fd].ptr - fh[fd].index;
    else
        fh[fd].ptr = (uint32_t)-1;

    /* Copy out the requested data */
    strcpy(fh[fd].dirent.name, fhdr->filename);
    fh[fd].dirent.time = 0;

    if((type & ROMFH_MASK) == ROMFH_DIR ||
            strcmp(fh[fd].dirent.name, ".") == 0 ||
            strcmp(fh[fd].dirent.name, "..") == 0) {
        fh[fd].dirent.attr = O_DIR;
        fh[fd].dirent.size = -1;
    }
    else {
        fh[fd].dirent.attr = 0;
        fh[fd].dirent.size = ntohl_32(&fhdr->size);
    }

    return &fh[fd].dirent;
}

/* Just to get the errno that might be better recognized upstream. */
static int romdisk_unlink(vfs_handler_t *vfs, const char *fn) {
    (void)vfs;
    (void)fn;

    errno = EROFS;
    return -1;
}

static void *romdisk_mmap(void *h) {
    file_t fd = (file_t)h;

    if(fd >= FS_ROMDISK_MAX_FILES || fh[fd].index == FH_INDEX_FREE) {
        errno = EINVAL;
        return NULL;
    }

    /* Can't really help the loss of "const" here */
    return (void *)(fh[fd].mnt->image + fh[fd].index);
}

static int romdisk_stat(vfs_handler_t *vfs, const char *path, struct stat *st,
                        int flag) {
    mode_t md;
    uint32_t filehdr;
    const romdisk_file_t *fhdr;
    rd_image_t *mnt = (rd_image_t *)vfs->privdata;
    size_t len = strlen(path);

    (void)flag;

    /* Root directory of romdisk */
    if(len == 0 || (len == 1 && *path == '/')) {
        memset(st, 0, sizeof(struct stat));
        st->st_dev = (dev_t)((uintptr_t)mnt);
        st->st_mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP |
            S_IROTH | S_IXOTH;
        st->st_size = -1;
        st->st_nlink = 2;

        return 0;
    }

    /* First try opening as a file */
    filehdr = romdisk_find(mnt, path + 1, 0);
    md = S_IFREG;

    /* If we couldn't get it as a file, try as a directory */
    if(filehdr == 0) {
        filehdr = romdisk_find(mnt, path + 1, 1);
        md = S_IFDIR;
    }

    /* If we still don't have it, then we're not going to get it. */
    if(filehdr == 0) {
        errno = ENOENT;
        return -1;
    }
       
    memset(st, 0, sizeof(struct stat));
    st->st_dev = (dev_t)((uintptr_t)mnt);
    st->st_mode = md | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    st->st_size = -1;
    st->st_nlink = 2;
    st->st_blksize = 1024;

    if(md == S_IFREG) {
        fhdr = (const romdisk_file_t *)(mnt->image + filehdr);
        st->st_size = ntohl_32(&fhdr->size);
        st->st_nlink = 1;
        st->st_blocks = st->st_size >> 10;

        /* Add one more block if there's a remainder */
        if (st->st_size & 0x3ff)
            ++st->st_blocks;
    }

    return 0;
}

static int romdisk_fcntl(void *h, int cmd, va_list ap) {
    file_t fd = (file_t)h;
    int rv = -1;

    (void)ap;

    if(fd >= FS_ROMDISK_MAX_FILES || fh[fd].index == FH_INDEX_FREE) {
        errno = EBADF;
        return -1;
    }

    switch(cmd) {
        case F_GETFL:
            rv = O_RDONLY;

            if(fh[fd].dir)
                rv |= O_DIR;

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

static int romdisk_rewinddir(void *h) {
    file_t fd = (file_t)h;

    if(fd >= FS_ROMDISK_MAX_FILES || fh[fd].index == FH_INDEX_FREE || !fh[fd].dir) {
        errno = EBADF;
        return -1;
    }

    fh[fd].ptr = 0;
    return 0;
}

static int romdisk_fstat(void *h, struct stat *st) {
    file_t fd = (file_t)h;

    if(fd >= FS_ROMDISK_MAX_FILES || !fh[fd].index) {
        errno = EBADF;
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_dev = (dev_t)((uintptr_t)fh[fd].mnt);
    st->st_mode = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    st->st_mode |= (fh[fd].dir) ? S_IFDIR : S_IFREG;
    st->st_size = fh[fd].size;
    st->st_nlink = (fh[fd].dir) ? 2 : 1;
    st->st_blksize = 1024;
    st->st_blocks = fh[fd].size >> 10;

    if(fh[fd].size & 0x3ff)
        ++st->st_blocks;

    return 0;
}

/* This is a template that will be used for each mount */
static vfs_handler_t vh = {
    /* Name Handler */
    {
        { 0 },                  /* name */
        0,                      /* in-kernel */
        0x00010000,             /* Version 1.0 */
        NMMGR_FLAGS_NEEDSFREE,  /* We malloc each VFS struct */
        NMMGR_TYPE_VFS,         /* VFS handler */
        NMMGR_LIST_INIT         /* list */
    },

    0, NULL,                    /* no caching, privdata */

    romdisk_open,
    romdisk_close,
    romdisk_read,
    romdisk_write,
    romdisk_seek,
    romdisk_tell,
    romdisk_total,
    romdisk_readdir,
    NULL,                       /* ioctl */
    NULL,                       /* rename */
    romdisk_unlink,
    romdisk_mmap,
    NULL,                       /* complete */
    romdisk_stat,
    NULL,                       /* mkdir */
    NULL,                       /* rmdir */
    romdisk_fcntl,
    NULL,                       /* poll */
    NULL,                       /* link */
    NULL,                       /* symlink */
    NULL,                       /* seek64 */
    NULL,                       /* tell64 */
    NULL,                       /* total64 */
    NULL,                       /* readlink */
    romdisk_rewinddir,
    romdisk_fstat
};

/* Are we initialized? */
static int initted = 0;

/* Initialize the file system */
void fs_romdisk_init(void) {
    if(initted)
        return;

    /* Init our list of mounted images */
    LIST_INIT(&romdisks);

    /* Reset fd's */
    memset(fh, 0, sizeof(fh));

    /* Mark the first as active so we can have an error FD of zero */
    fh[0].index = FH_INDEX_RESERVED;

    /* Init thread mutexes */
    mutex_init(&fh_mutex, MUTEX_TYPE_NORMAL);

    initted = 1;
}

/* De-init the file system; also unmounts any mounted images. */
void fs_romdisk_shutdown(void) {
    rd_image_t *n, *c;

    if(!initted)
        return;

    /* Go through and free all the romdisk mount entries */
    c = LIST_FIRST(&romdisks);

    while(c != NULL) {
        n = LIST_NEXT(c, list_ent);

        dbglog(DBG_DEBUG, "fs_romdisk: unmounting image at %p from %s\n",
               c->image, c->vfsh->nmmgr.pathname);

        if(c->own_buffer)
            dbglog(DBG_DEBUG, "   (and also freeing its image buffer)\n");

        assert((void *)&c->vfsh->nmmgr == (void *)c->vfsh);

        if(c->own_buffer)
            free((void *)c->image);

        nmmgr_handler_remove(&c->vfsh->nmmgr);
        free(c->vfsh);
        free(c);

        c = n;
    }

    /* Free mutex */
    mutex_destroy(&fh_mutex);

    initted = 0;
}

/* Mount a romdisk image; must have called fs_romdisk_init() earlier.
   Also note that we do _not_ take ownership of the image data if
   own_buffer is false, so if you malloc'd that buffer, you must
   also free it after the unmount. If own_buffer is true, then
   we free the buffer when it is unmounted. */
int fs_romdisk_mount(const char *mountpoint, const uint8_t *img, bool own_buffer) {
    const romdisk_hdr_t *hdr = (const romdisk_hdr_t *)img;
    rd_image_t          *mnt;
    vfs_handler_t       *vfsh;

    /* Are we initted? */
    if(!initted)
        return -1;

    /* Check the image and print some info about it */
    if(strncmp(hdr->magic, "-rom1fs-", sizeof(hdr->magic))) {
        dbglog(DBG_ERROR, "fs_romdisk: image at %p is not a ROMFS image\n", img);
        return -2;
    }
    else {
        dbglog(DBG_DEBUG, "fs_romdisk: mounting image at %p at %s\n", img, mountpoint);
    }

    /* Create a mount struct */
    mnt = (rd_image_t *)malloc(sizeof(rd_image_t));

    if(mnt == NULL) {
        errno=ENOMEM;
        return -3;
    }
    mnt->own_buffer = own_buffer;
    mnt->image = img;
    mnt->files = sizeof(romdisk_hdr_t)
                 + (strlen(hdr->volume_name) / RD_VN_MAX) * RD_VN_MAX;

    /* Make a VFS struct */
    vfsh = (vfs_handler_t *)malloc(sizeof(vfs_handler_t));

    if(vfsh == NULL) {
        free(mnt);
        errno=ENOMEM;
        return -3;
    }
    memcpy(vfsh, &vh, sizeof(vfs_handler_t));
    strcpy(vfsh->nmmgr.pathname, mountpoint);
    vfsh->privdata = (void *)mnt;
    mnt->vfsh = vfsh;

    assert((void *)&mnt->vfsh->nmmgr == (void *)mnt->vfsh);

    /* Add it to our mount list */
    mutex_lock(&fh_mutex);
    LIST_INSERT_HEAD(&romdisks, mnt, list_ent);
    mutex_unlock(&fh_mutex);

    /* Register with VFS */
    return nmmgr_handler_add(&vfsh->nmmgr);
}

/* Unmount a romdisk image */
int fs_romdisk_unmount(const char *mountpoint) {
    rd_image_t  *n;

    mutex_lock_scoped(&fh_mutex);

    LIST_FOREACH(n, &romdisks, list_ent) {
        if(!strcmp(mountpoint, n->vfsh->nmmgr.pathname)) {
            break;
        }
    }

    /* The LIST_FOREACH got to the end and didn't find anything */
    if(!n) {
        errno = ENOENT;
        return -1;
    }

    /* Remove it from the mount list */
    LIST_REMOVE(n, list_ent);

    dbglog(DBG_DEBUG, "fs_romdisk: unmounting image at %p from %s\n",
           n->image, n->vfsh->nmmgr.pathname);

    /* Unmount it */
    assert((void *)&n->vfsh->nmmgr == (void *)n->vfsh);
    nmmgr_handler_remove(&n->vfsh->nmmgr);

    /* If we own the buffer, free it */
    if(n->own_buffer) {
        dbglog(DBG_DEBUG, "   (and also freeing its image buffer)\n");
        free((void *)n->image);
    }

    /* Free the structs */
    free(n->vfsh);
    free(n);

    return 0;
}
