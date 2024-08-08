/* KallistiOS ##version##

   dev_fb.c
   Copyright (C) 2024 Donald Haase
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <errno.h>
#include <arch/types.h>
#include <kos/mutex.h>
#include <sys/queue.h>
#include <linux/fb.h>
#include <dc/video.h>
#include <dc/pvr.h>

#include <kos/dbgio.h>

/* File handles */
typedef struct fb_fh_str {
    int mode;                           /* mode the file was opened with */
    off_t pos;                          /* Current read position */

    TAILQ_ENTRY(fb_fh_str) listent;     /* list entry */
} fb_fh_t;

/* Linked list of open files (controlled by "mutex") */
TAILQ_HEAD(fb_fh_list, fb_fh_str) fb_fh;

/* Thread mutex for fb_fh access */
static mutex_t fh_mutex;

/* Screen Fixinfo struct. */
struct fb_fix_screeninfo fb_fscreeninfo = {
    .id         = "KOS Framebuffer",
    .smem_start = PVR_RAM_BASE, 
    .smem_len   = PVR_RAM_SIZE,
    .type       = FB_TYPE_PACKED_PIXELS,
    .visual     = FB_VISUAL_TRUECOLOR,
    .line_length= 640 * vid_pmode_bpp[PM_RGB565],
    .accel      = FB_ACCEL_NONE
};

/* openfile function */
static fb_fh_t *fb_open_file(vfs_handler_t *vfs, const char *fn, int mode) {
    (void) vfs;
    (void) fn;

    fb_fh_t    * fd;       /* file descriptor */

    /* Malloc a new fh struct */
    fd = malloc(sizeof(fb_fh_t));
    if(!fd) {
        errno = ENOMEM;
        return NULL;
    }

    /* Fill in the filehandle struct */
    fd->mode = mode;
    fd->pos = 0;

    return fd;
}

/* open function */
static void * fb_open(vfs_handler_t *vfs, const char *path, int mode) {
    fb_fh_t *fh;

    if(mode & O_DIR) {
        errno = ENOTDIR;
        return NULL;
    }
    
    fh = fb_open_file(vfs, path, mode);
    if(!fh) {
        return NULL;
    }

    /* link the fh onto the top of the list */
    mutex_lock(&fh_mutex);
    TAILQ_INSERT_TAIL(&fb_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    return (void *)fh;
}

/* Verify that a given hnd is actually in the list */
static int fb_verify_hnd(void *hnd) {
    fb_fh_t    *cur;
    int     rv = 0;

    mutex_lock(&fh_mutex);
    TAILQ_FOREACH(cur, &fb_fh, listent) {
        if((void *)cur == hnd) {
            rv = 1;
            break;
        }
    }
    mutex_unlock(&fh_mutex);

    return rv;
}

/* close a file */
static int fb_close(void *hnd) {
    fb_fh_t *fh;

    /* Check the handle */
    if(!fb_verify_hnd(hnd)) {
        errno = EBADF;
        return -1;
    }

    fh = (fb_fh_t *)hnd;

    /* Look for the one to get rid of */
    mutex_lock(&fh_mutex);
    TAILQ_REMOVE(&fb_fh, fh, listent);
    mutex_unlock(&fh_mutex);

    free(fh);
    return 0;
}

/* read function */
static ssize_t fb_read(void *hnd, void *buffer, size_t cnt) {
    fb_fh_t *fh;

    /* Check the handle */
    if(!fb_verify_hnd(hnd)) {
        errno = EBADF;
        return -1;
    }

    fh = (fb_fh_t *)hnd;

    /* make sure we're opened for reading */
    if((fh->mode & O_MODE_MASK) != O_RDONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return 0;
    
    /* Limit count to total fb size. */
    if((fh->pos + cnt) > vid_mode->fb_size)
        cnt = vid_mode->fb_size - fh->pos;

    /* Copy out the requested amount */
    memcpy(buffer, ((uint8 *)vram_l) + fh->pos, cnt);
    fh->pos += cnt;

    return cnt;
}

/* write function */
static ssize_t fb_write(void *hnd, const void *buffer, size_t cnt) {
    fb_fh_t    *fh;

    /* Check the handle */
    if(!fb_verify_hnd(hnd)) {
        errno = EBADF;
        return -1;
    }

    fh = (fb_fh_t *)hnd;

    /* Make sure we're opened for writing */
    if((fh->mode & O_MODE_MASK) != O_WRONLY && (fh->mode & O_MODE_MASK) != O_RDWR)
        return -1;
    
    /* Limit count to total fb size */
    if((fh->pos + cnt) > vid_mode->fb_size)
        cnt = vid_mode->fb_size - fh->pos;
    
    /* Copy out the requested amount */
    memcpy( ((uint8 *)vram_l) + fh->pos, buffer, cnt);
    fh->pos += cnt;

    return cnt;
}

/* Seek elsewhere in a file */
static off_t fb_seek(void *hnd, off_t offset, int whence) {
    off_t       tmp;
    fb_fh_t    *fh;

    /* Check the handle */
    if(!fb_verify_hnd(hnd)) {
        errno = EBADF;
        return -1;
    }

    fh = (fb_fh_t *)hnd;
    tmp = fh->pos;
    
    /* Update current position according to arguments */
    switch(whence) {
        case SEEK_SET:
            if(offset < 0) {
                errno = EINVAL;
                return -1;
            }

            tmp = offset;
            break;

        case SEEK_CUR:
            if(offset < 0 && ((off_t)-offset) > tmp) {
                errno = EINVAL;
                return -1;
            }

            tmp += offset;
            break;

        case SEEK_END:
            if(offset < 0 && ((off_t)-offset) > vid_mode->fb_size) {
                errno = EINVAL;
                return -1;
            }

            tmp = vid_mode->fb_size + offset;
            break;

        default:
            errno = EINVAL;
            return -1;
    }

    /* Limit pos to total fb size */
    if(tmp > vid_mode->fb_size) {
        errno = EFBIG;
        return -1;
    }
    
    fh->pos = tmp;

    return 0;
}

/* tell the current position in the file */
static off_t fb_tell(void *hnd) {
    fb_fh_t    *fh;

    /* Check the handle */
    if(!fb_verify_hnd(hnd)) {
        errno = EBADF;
        return (off_t)-1;
    }

    fh = (fb_fh_t *)hnd;
    return fh->pos;
}

/* return the filesize */
static size_t fb_total(void *hnd) {

    /* Check the handle */
    if(!fb_verify_hnd(hnd)) {
        errno = EBADF;
        return (size_t)-1;
    }

    return vid_mode->fb_size;
}

static int fb_ioctl(void *hnd, int cmd, va_list ap) {
    fb_fh_t *fd = (fb_fh_t *)hnd;
    
    void *arg = va_arg(ap, void*);

    if(!fb_verify_hnd(hnd)) {
        errno = EBADF;
        return -1;
    }

    switch (cmd) {
        case FBIOGET_FSCREENINFO:
            if(arg == NULL) {
                errno = EINVAL;
                return -1;
            }
            memcpy(arg, &fb_fscreeninfo, sizeof(struct fb_fix_screeninfo));
            return 0;

        /* Add other ioctl cases here */

        default:
            errno = EINVAL;
            return -1;
    }
}

static int fb_stat(vfs_handler_t *vfs, const char *fn, struct stat *st,
                    int flag) {
    (void)vfs;
    (void)fn;
    (void)flag;

    memset(st, 0, sizeof(struct stat));
    st->st_mode =   S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    st->st_nlink = 1;
    st->st_size = vid_mode->fb_size;

    return 0;
}

static int fb_fstat(void *fd, struct stat *st) {

    /* Check the handle */
    if(!fb_verify_hnd(fd)) {
        errno = EBADF;
        return -1;
    }

    return fb_stat(NULL, NULL, st, 0);
}

static void *fb_mmap(void *fd) {

    /* Check the handle */
    if(!fb_verify_hnd(fd)) {
        errno = EBADF;
        return NULL;
    }

    return (void *)vram_l;
}

/* handler interface */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/dev/fb0",          /* name */
        0,                   /* tbfi */
        0x00010000,          /* Version 1.0 */
        NMMGR_FLAGS_INDEV,   /* flags */
        NMMGR_TYPE_VFS,      /* VFS handler */
        NMMGR_LIST_INIT
    },
    0, NULL,            /* In-kernel, privdata */

    fb_open,
    fb_close,
    fb_read,
    fb_write,
    fb_seek,
    fb_tell,
    fb_total,
    NULL,               /* readdir */
    fb_ioctl,
    NULL,               /* rename/move */
    NULL,               /* unlink */
    fb_mmap,
    NULL,               /* complete */
    fb_stat,          /* stat */
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
    NULL,
    fb_fstat
};

int fs_fb_init(void) {
    int rv = 0;
    TAILQ_INIT(&fb_fh);
    mutex_init(&fh_mutex, MUTEX_TYPE_NORMAL);

    nmmgr_handler_add(&vh.nmmgr);

    return rv;
}

int fs_fb_shutdown(void) {
    fb_fh_t * c, * n;

    /* First, clean up any open files */
    c = TAILQ_FIRST(&fb_fh);

    while(c) {
        n = TAILQ_NEXT(c, listent);
        free(c);
        c = n;
    }

    mutex_destroy(&fh_mutex);

    nmmgr_handler_remove(&vh.nmmgr);

    return 0;
}

