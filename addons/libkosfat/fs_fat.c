/* KallistiOS ##version##

   fs_fat.c
   Copyright (C) 2012, 2013, 2014, 2016, 2019 Lawrence Sebald
*/

#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/queue.h>

#include <kos/fs.h>
#include <kos/mutex.h>
#include <kos/dbglog.h>

#include <fat/fs_fat.h>

#include "fatfs.h"
#include "directory.h"
#include "bpb.h"
#include "ucs.h"

#ifdef __STRICT_ANSI__
/* These don't necessarily get prototyped in string.h in standard-compliant mode
   as they are extensions to the standard. Declaring them this way shouldn't
   hurt. */
char *strtok_r(char *, const char *, char **);
char *strdup(const char *);
#endif

#define MAX_FAT_FILES 16

typedef struct fs_fat_fs {
    LIST_ENTRY(fs_fat_fs) entry;

    vfs_handler_t *vfsh;
    fat_fs_t *fs;
    uint32_t mount_flags;
} fs_fat_fs_t;

LIST_HEAD(fat_list, fs_fat_fs);
static struct fat_list fat_fses;
static mutex_t fat_mutex;

static struct {
    int opened;
    fat_dentry_t dentry;
    uint32_t dentry_cluster;
    uint32_t dentry_offset;
    uint32_t dentry_lcl;
    uint32_t dentry_loff;
    uint32_t cluster;
    uint32_t cluster_order;
    int mode;
    uint32_t ptr;
    dirent_t dent;
    fs_fat_fs_t *fs;
} fh[MAX_FAT_FILES];

static uint16_t longname_buf[256];

static int fat_create_entry(fat_fs_t *fs, const char *fn, uint8_t attr,
                            uint32_t *cl2, uint32_t *off, uint32_t *lcl,
                            uint32_t *loff, uint8_t **buf, uint32_t *pcl) {
    char *parent_fn, *newdir_fn;
    fat_dentry_t p_ent, n_ent;
    int err;
    uint32_t cl;

    /* Make a copy of the filename, as we're gonna split it into two... */
    if(!(parent_fn = strdup(fn))) {
        return -ENOMEM;
    }

    /* Figure out where the new directory's name starts in the string... */
    newdir_fn = strrchr(parent_fn, '/');
    if(!newdir_fn) {
        free(parent_fn);
        return -EEXIST;
    }

    /* Split the string. */
    *newdir_fn++ = 0;

    /* Find the parent's dentry. */
    if((err = fat_find_dentry(fs, parent_fn, &p_ent, &cl, off, lcl,
                              loff)) < 0) {
        free(parent_fn);
        return err;
    }

    /* Make sure the parent is actually a directory. */
    if(!(p_ent.attr & FAT_ATTR_DIRECTORY)) {
        free(parent_fn);
        return -ENOTDIR;
    }

    /* Make sure the child doeesn't exist. */
    if((err = fat_find_child(fs, newdir_fn, &p_ent, &n_ent, &cl, off, lcl,
                             loff)) != -ENOENT) {
        free(parent_fn);
        if(!err)
            err = -EEXIST;
        return err;
    }

    /* Allocate a cluster to store the first block of the new thing in. */
    if((cl = fat_allocate_cluster(fs, &err)) == FAT_INVALID_CLUSTER) {
        free(parent_fn);
        return -err;
    }

    /* Clear the target cluster on the disk (well, in the cache, anyway). */
    if(!(*buf = fat_cluster_clear(fs, cl, &err))) {
        /* Uh oh... Now things start becoming bad if things fail... */
        fat_erase_chain(fs, cl);
        free(parent_fn);
        return -err;
    }

    /* Add the dentry to the parent. */
    if((err = fat_add_dentry(fs, newdir_fn, &p_ent, attr, cl, cl2, off, lcl,
                             loff)) < 0) {
        fat_erase_chain(fs, cl);
        free(parent_fn);
        return err;
    }

    /* Clean up, because we're done. */
    *pcl = p_ent.cluster_low | (p_ent.cluster_high << 16);
    free(parent_fn);
    return 0;
}

static int advance_cluster(fat_fs_t *fs, int fd, uint32_t order, int write) {
    uint32_t clo, cl, cl2;
    int err;

    cl = fh[fd].cluster;
    clo = fh[fd].cluster_order;

    /* Are we moving forward or backward? */
    if(clo > order) {
        /* If moving backward, we have to start from the beginning of the file
           and advance forward. */
        clo = 0;
        cl = fh[fd].dentry.cluster_low | (fh[fd].dentry.cluster_high << 16);
        fh[fd].cluster = cl;
        fh[fd].cluster_order = clo;
    }

    /* At this point, we're definitely moving forward, if at all... */
    while(clo < order) {
        /* Read the FAT for the current cluster to see where we're going
           next... */
        cl2 = fat_read_fat(fs, cl, &err);

        if(cl2 == FAT_INVALID_CLUSTER) {
            return -err;
        }
        else if(fat_is_eof(fs, cl2)) {
            /* If we've hit the EOF and we're writing, we need to allocate a new
               cluster to the file. If we're reading, then return error. */
            if(!write) {
                fh[fd].cluster = cl2;
                fh[fd].cluster_order = clo;
                fh[fd].mode &= ~0x80000000;
                return -EDOM;
            }
            else {
                /* Allocate a new cluster */
                cl2 = fat_allocate_cluster(fs, &err);

                if(cl2 == FAT_INVALID_CLUSTER) {
                    return -err;
                }

                /* Clear it. */
                if(!fat_cluster_clear(fs, cl2, &err)) {
                    fat_write_fat(fs, cl2, 0);
                    return -err;
                }

                /* Write it to the file's FAT chain. */
                if((err = fat_write_fat(fs, cl, cl2)) < 0) {
                    fat_write_fat(fs, cl2, 0);
                    return err;
                }
            }
        }

        cl = cl2;
        ++clo;
    }

    fh[fd].cluster = cl;
    fh[fd].cluster_order = clo;
    fh[fd].mode &= ~0x80000000;
    return 0;
}

static void *fs_fat_open(vfs_handler_t *vfs, const char *fn, int mode) {
    file_t fd;
    fs_fat_fs_t *mnt = (fs_fat_fs_t *)vfs->privdata;
    int rv;
    uint32_t cl, cl2;

    /* Make sure if we're going to be writing to the file that the fs is mounted
       read/write. */
    if((mode & (O_TRUNC | O_WRONLY | O_RDWR)) &&
       !(mnt->mount_flags & FS_FAT_MOUNT_READWRITE)) {
        errno = EROFS;
        return NULL;
    }

    /* Find a free file handle */
    mutex_lock(&fat_mutex);

    for(fd = 0; fd < MAX_FAT_FILES; ++fd) {
        if(fh[fd].opened == 0) {
            break;
        }
    }

    if(fd >= MAX_FAT_FILES) {
        errno = ENFILE;
        mutex_unlock(&fat_mutex);
        return NULL;
    }

    /* Find the object in question... */
    if((rv = fat_find_dentry(mnt->fs, fn, &fh[fd].dentry,
                             &fh[fd].dentry_cluster, &fh[fd].dentry_offset,
                             &fh[fd].dentry_lcl, &fh[fd].dentry_loff))) {
        if(rv == -ENOENT) {
            if((mode & O_CREAT)) {
                uint32_t off, lcl, loff, pcl;
                uint8_t *buf;

                if((rv = fat_create_entry(mnt->fs, fn, FAT_ATTR_ARCHIVE,
                                           &cl, &off, &lcl, &loff, &buf,
                                           &pcl)) < 0) {
                    mutex_unlock(&fat_mutex);
                    errno = -rv;
                    return NULL;
                }

                /* Fill in the file descriptor... */
                fat_get_dentry(mnt->fs, cl, off, &fh[fd].dentry);
                fh[fd].dentry_cluster = cl;
                fh[fd].dentry_offset = off;
                fh[fd].dentry_lcl = lcl;
                fh[fd].dentry_loff = loff;
                goto created;
            }
        }

        mutex_unlock(&fat_mutex);
        errno = -rv;
        return NULL;
    }

    /* Make sure we're not trying to open a directory for writing */
    if((fh[fd].dentry.attr & FAT_ATTR_DIRECTORY) &&
       ((mode & O_WRONLY) || !(mode & O_DIR))) {
        errno = EISDIR;
        fh[fd].dentry_cluster = fh[fd].dentry_offset = 0;
        mutex_unlock(&fat_mutex);
        return NULL;
    }

    /* Make sure if we're trying to open a directory that we have a directory */
    if((mode & O_DIR) && !(fh[fd].dentry.attr & FAT_ATTR_DIRECTORY)) {
        errno = ENOTDIR;
        fh[fd].dentry_cluster = fh[fd].dentry_offset = 0;
        fh[fd].dentry_lcl = fh[fd].dentry_loff = 0;
        mutex_unlock(&fat_mutex);
        return NULL;
    }

    /* Handle truncating the file if we need to (for writing). */
    if((mode & (O_WRONLY | O_RDWR)) && (mode & O_TRUNC)) {
        /* Read the FAT for the first cluster of the file and clear the entire
           chain after that point. Then, blank the first cluster and fix up
           the directory entry. */
        cl = fh[fd].dentry.cluster_low | (fh[fd].dentry.cluster_high << 16);
        cl2 = fat_read_fat(mnt->fs, cl, &rv);

        if(cl2 == FAT_INVALID_CLUSTER) {
            errno = rv;
            fh[fd].dentry_cluster = fh[fd].dentry_offset = 0;
            fh[fd].dentry_lcl = fh[fd].dentry_loff = 0;
            mutex_unlock(&fat_mutex);
            return NULL;
        }
        else if(!fat_is_eof(mnt->fs, cl2)) {
            /* Erase all but the first block. */
            if((rv = fat_erase_chain(mnt->fs, cl2)) < 0) {
                /* Uh oh... this could be really bad... */
                errno = -rv;
                fh[fd].dentry_cluster = fh[fd].dentry_offset = 0;
                fh[fd].dentry_lcl = fh[fd].dentry_loff = 0;
                mutex_unlock(&fat_mutex);
                return NULL;
            }

            /* Set the first block's fat value to the end of chain marker. */
            if((rv = fat_write_fat(mnt->fs, cl, 0x0FFFFFFF)) < 0) {
                /* Uh oh... this could be really bad... */
                errno = -rv;
                fh[fd].dentry_cluster = fh[fd].dentry_offset = 0;
                fh[fd].dentry_lcl = fh[fd].dentry_loff = 0;
                mutex_unlock(&fat_mutex);
                return NULL;
            }
        }

        /* Set the size to 0. */
        fat_cluster_clear(mnt->fs, cl, &rv);
        fh[fd].dentry.size = 0;

        if((rv = fat_update_dentry(mnt->fs, &fh[fd].dentry,
                                   fh[fd].dentry_cluster,
                                   fh[fd].dentry_offset)) < 0) {
            errno = -rv;
            mutex_unlock(&fat_mutex);
            return NULL;
        }
    }

    /* Fill in the rest of the handle */
created:
    fh[fd].mode = mode;
    fh[fd].ptr = 0;
    fh[fd].fs = mnt;
    fh[fd].cluster = fh[fd].dentry.cluster_low |
        (fh[fd].dentry.cluster_high << 16);
    fh[fd].cluster_order = 0;
    fh[fd].opened = 1;

    mutex_unlock(&fat_mutex);
    return (void *)(fd + 1);
}

static int fs_fat_close(void *h) {
    file_t fd = ((file_t)h) - 1;
    int rv = 0;

    mutex_lock(&fat_mutex);

    if(fd < MAX_FAT_FILES && fh[fd].opened) {
        fh[fd].opened = 0;
        fh[fd].dentry_offset = fh[fd].dentry_cluster = 0;
        fh[fd].dentry_lcl = fh[fd].dentry_loff = 0;
    }
    else {
        rv = -1;
        errno = EBADF;
    }

    mutex_unlock(&fat_mutex);
    return rv;
}

static ssize_t fs_fat_read(void *h, void *buf, size_t cnt) {
    file_t fd = ((file_t)h) - 1;
    fat_fs_t *fs;
    uint32_t bs, bo;
    uint8_t *block;
    uint8_t *bbuf = (uint8_t *)buf;
    ssize_t rv;
    uint64_t sz, cl;
    int mode;

    mutex_lock(&fat_mutex);

    /* Check that the fd is valid */
    if(fd >= MAX_FAT_FILES || !fh[fd].opened) {
        mutex_unlock(&fat_mutex);
        errno = EBADF;
        return -1;
    }

    fs = fh[fd].fs->fs;

    /* Make sure the fd is open for reading */
    mode = fh[fd].mode & O_MODE_MASK;
    if(mode != O_RDONLY && mode != O_RDWR) {
        mutex_unlock(&fat_mutex);
        errno = EBADF;
        return -1;
    }

    /* Make sure we're not trying to read a directory with read */
    if(fh[fd].mode & O_DIR) {
        mutex_unlock(&fat_mutex);
        errno = EISDIR;
        return -1;
    }

    /* Did we hit the end of the file? */
    sz = fh[fd].dentry.size;

    if(fat_is_eof(fs, fh[fd].cluster) || fh[fd].ptr >= sz) {
        mutex_unlock(&fat_mutex);
        return 0;
    }

    /* Do we have enough left? */
    if((fh[fd].ptr + cnt) > sz)
        cnt = sz - fh[fd].ptr;

    bs = fat_cluster_size(fs);
    rv = (ssize_t)cnt;
    bo = fh[fd].ptr & (bs - 1);

    /* Have we had an intervening seek call? */
    if((fh[fd].mode & 0x80000000)) {
        mode = advance_cluster(fs, fd, fh[fd].ptr / bs, 0);

        if(mode == -EDOM) {
            mutex_unlock(&fat_mutex);
            return 0;
        }
        else if(mode < 0) {
            mutex_unlock(&fat_mutex);
            errno = -mode;
            return -1;
        }
    }

    /* Handle the first block specially if we are offset within it. */
    if(bo) {
        if(!(block = fat_cluster_read(fs, fh[fd].cluster, &errno))) {
            mutex_unlock(&fat_mutex);
            return -1;
        }

        /* Is there still more to read? */
        if(cnt > bs - bo) {
            memcpy(bbuf, block + bo, bs - bo);
            fh[fd].ptr += bs - bo;
            cnt -= bs - bo;
            bbuf += bs - bo;
            cl = fat_read_fat(fs, fh[fd].cluster, &errno);

            if(cl == FAT_INVALID_CLUSTER) {
                mutex_unlock(&fat_mutex);
                return -1;
            }
            else if(fat_is_eof(fs, cl)) {
                mutex_unlock(&fat_mutex);
                errno = EIO;
                return -1;
            }

            fh[fd].cluster = cl;
            ++fh[fd].cluster_order;
        }
        else {
            memcpy(bbuf, block + bo, cnt);
            fh[fd].ptr += cnt;

            /* Did we hit the end of the cluster? */
            if(cnt + bo == bs) {
                cl = fat_read_fat(fs, fh[fd].cluster, &errno);

                if(cl == FAT_INVALID_CLUSTER) {
                    mutex_unlock(&fat_mutex);
                    return -1;
                }

                fh[fd].cluster = cl;
                ++fh[fd].cluster_order;
            }

            cnt = 0;
        }
    }

    /* While we still have more to read, do it. */
    while(cnt) {
        if(!(block = fat_cluster_read(fs, fh[fd].cluster, &errno))) {
            mutex_unlock(&fat_mutex);
            return -1;
        }

        /* Is there still more to read? */
        if(cnt > bs) {
            memcpy(bbuf, block, bs);
            fh[fd].ptr += bs;
            cnt -= bs;
            bbuf += bs;
            cl = fat_read_fat(fs, fh[fd].cluster, &errno);

            if(cl == FAT_INVALID_CLUSTER) {
                mutex_unlock(&fat_mutex);
                return -1;
            }
            else if(fat_is_eof(fs, cl)) {
                mutex_unlock(&fat_mutex);
                errno = EIO;
                return -1;
            }

            fh[fd].cluster = cl;
            ++fh[fd].cluster_order;
        }
        else {
            memcpy(bbuf, block, cnt);
            fh[fd].ptr += cnt;

            /* Did we hit the end of the cluster? */
            if(cnt == bs) {
                cl = fat_read_fat(fs, fh[fd].cluster, &errno);

                if(cl == FAT_INVALID_CLUSTER) {
                    mutex_unlock(&fat_mutex);
                    return -1;
                }

                fh[fd].cluster = cl;
                ++fh[fd].cluster_order;
            }

            cnt = 0;
        }
    }

    /* We're done, clean up and return. */
    mutex_unlock(&fat_mutex);
    return rv;
}

static ssize_t fs_fat_write(void *h, const void *buf, size_t cnt) {
    file_t fd = ((file_t)h) - 1;
    fat_fs_t *fs;
    uint32_t bs, bo;
    uint8_t *block;
    uint8_t *bbuf = (uint8_t *)buf;
    ssize_t rv;
    int mode, err;

    mutex_lock(&fat_mutex);

    /* Check that the fd is valid */
    if(fd >= MAX_FAT_FILES || !fh[fd].opened) {
        mutex_unlock(&fat_mutex);
        errno = EBADF;
        return -1;
    }

    /* Make sure the fd is open for reading */
    mode = fh[fd].mode & O_MODE_MASK;
    if(mode != O_WRONLY && mode != O_RDWR) {
        mutex_unlock(&fat_mutex);
        errno = EBADF;
        return -1;
    }

    if(!cnt) {
        mutex_unlock(&fat_mutex);
        return 0;
    }

    fs = fh[fd].fs->fs;
    bs = fat_cluster_size(fs);
    rv = (ssize_t)cnt;
    bo = fh[fd].ptr & (bs - 1);

    /* Have we had an intervening seek call (or a write that ended exactly on
       a cluster boundary)? */
    if((fh[fd].mode & 0x80000000)) {
        if((err = advance_cluster(fs, fd, fh[fd].ptr / bs, 1)) < 0) {
            mutex_unlock(&fat_mutex);
            errno = -err;
            return -1;
        }
    }

    /* Are we starting our write in the middle of a block? */
    if(bo) {
        if(!(block = fat_cluster_read(fs, fh[fd].cluster, &err))) {
            mutex_unlock(&fat_mutex);
            errno = err;
            return -1;
        }

        /* Are we writing past the end of this block, or not? */
        if(cnt > bs - bo) {
            memcpy(block + bo, bbuf, bs - bo);
            fat_cluster_mark_dirty(fs, fh[fd].cluster);

            fh[fd].ptr += bs - bo;
            bbuf += bs - bo;
            cnt -= bs - bo;

            if((err = advance_cluster(fs, fd, fh[fd].cluster_order + 1,
                                      1)) < 0) {
                mutex_unlock(&fat_mutex);
                errno = -err;
                return -1;
            }
        }
        else {
            memcpy(block + bo, bbuf, cnt);
            fat_cluster_mark_dirty(fs, fh[fd].cluster);
            fh[fd].ptr += cnt;
            cnt = 0;

            /* We don't want to advance the cluster even if we've hit the end of
               the current one here, as that may extend the file unnecessarily
               into a new cluster. Set the seek flag and it'll be dealt with
               on the next write, if needed. */
            fh[fd].mode |= 0x80000000;
        }
    }

    /* While we still have more to write, do it. */
    while(cnt) {
        if(!(block = fat_cluster_read(fs, fh[fd].cluster, &err))) {
            mutex_unlock(&fat_mutex);
            errno = err;
            return -1;
        }

        /* Is there still more to write after this cluster? */
        if(cnt > bs) {
            memcpy(block, bbuf, bs);
            fat_cluster_mark_dirty(fs, fh[fd].cluster);
            fh[fd].ptr += bs;
            cnt -= bs;
            bbuf += bs;

            if((err = advance_cluster(fs, fd, fh[fd].cluster_order + 1,
                                      1)) < 0) {
                mutex_unlock(&fat_mutex);
                errno = -err;
                return -1;
            }
        }
        else {
            memcpy(block, bbuf, cnt);
            fat_cluster_mark_dirty(fs, fh[fd].cluster);
            fh[fd].ptr += cnt;
            cnt = 0;

            /* We don't want to advance the cluster even if we've hit the end of
               the current one here, as that may extend the file unnecessarily
               into a new cluster. Set the seek flag and it'll be dealt with
               on the next write, if needed. */
            fh[fd].mode |= 0x80000000;
        }
    }

    /* If the file pointer is past the end of the file as recorded in its
       directory entry, update the directory entry with the new size. */
    if(fh[fd].ptr > fh[fd].dentry.size || mode == O_WRONLY) {
        fh[fd].dentry.size = fh[fd].ptr;

        if((err = fat_update_dentry(fs, &fh[fd].dentry,
                                    fh[fd].dentry_cluster,
                                    fh[fd].dentry_offset)) < 0) {
            rv = -1;
            errno = -err;
        }
    }

    /* Update the file's modification timestamp. */
    fat_update_mtime(&fh[fd].dentry);

    /* We're done, clean up and return. */
    mutex_unlock(&fat_mutex);
    return rv;
}

static _off64_t fs_fat_seek64(void *h, _off64_t offset, int whence) {
    file_t fd = ((file_t)h) - 1;
    off_t rv;
    uint32_t pos;

    mutex_lock(&fat_mutex);

    /* Check that the fd is valid */
    if(fd >= MAX_FAT_FILES || !fh[fd].opened || (fh[fd].mode & O_DIR)) {
        mutex_unlock(&fat_mutex);
        errno = EINVAL;
        return -1;
    }

    /* Update current position according to arguments */
    switch(whence) {
        case SEEK_SET:
            pos = offset;
            break;

        case SEEK_CUR:
            pos = fh[fd].ptr + offset;
            break;

        case SEEK_END:
            pos = fh[fd].dentry.size + offset;
            break;

        default:
            mutex_unlock(&fat_mutex);
            errno = EINVAL;
            return -1;
    }

    /* Update the file pointer and set the flag so that we know that we have
       done a seek. */
    fh[fd].ptr = pos;
    fh[fd].mode |= 0x80000000;

    rv = (_off64_t)pos;
    mutex_unlock(&fat_mutex);
    return rv;
}

static _off64_t fs_fat_tell64(void *h) {
    file_t fd = ((file_t)h) - 1;
    off_t rv;

    mutex_lock(&fat_mutex);

    if(fd >= MAX_FAT_FILES || !fh[fd].opened || (fh[fd].mode & O_DIR)) {
        mutex_unlock(&fat_mutex);
        errno = EINVAL;
        return -1;
    }

    rv = (_off64_t)fh[fd].ptr;
    mutex_unlock(&fat_mutex);
    return rv;
}

static uint64 fs_fat_total64(void *h) {
    file_t fd = ((file_t)h) - 1;
    size_t rv;

    mutex_lock(&fat_mutex);

    if(fd >= MAX_FAT_FILES || !fh[fd].opened || (fh[fd].mode & O_DIR)) {
        mutex_unlock(&fat_mutex);
        errno = EINVAL;
        return -1;
    }

    rv = fh[fd].dentry.size;
    mutex_unlock(&fat_mutex);
    return rv;
}

static time_t fat_time_to_stat(uint16_t date, uint16_t time) {
    struct tm tmv;

    /* The MS-DOS epoch is January 1, 1980, not January 1, 1970... */
    tmv.tm_year = (date >> 9) + 80;
    tmv.tm_mon = ((date >> 5) & 0x0F) - 1;
    tmv.tm_mday = date & 0x1F;

    tmv.tm_hour = (time >> 11) & 0x1F;
    tmv.tm_min = (time >> 5) & 0x3F;
    tmv.tm_sec = (time & 0x1F) << 1;

    return mktime(&tmv);
}

static void fill_stat_timestamps(const fat_dentry_t *ent, struct stat *buf) {
    if(!ent->cdate) {
        buf->st_ctime = 0;
    }
    else {
        buf->st_ctime = fat_time_to_stat(ent->cdate, ent->ctime);
    }

    if(!ent->adate) {
        buf->st_atime = 0;
    }
    else {
        buf->st_atime = fat_time_to_stat(ent->adate, 0);
    }

    buf->st_mtime = fat_time_to_stat(ent->mdate, ent->mtime);
}

static void copy_shortname(fat_dentry_t *dent, char *fn) {
    int i, j = 0;

    for(i = 0; i < 8 && dent->name[i] != ' '; ++i) {
        fn[i] = dent->name[i];
    }

    /* Only add a dot if there's actually an extension. */
    if(dent->name[8] != ' ') {
        fn[i++] = '.';

        for(; j < 3 && dent->name[8 + j] != ' '; ++j) {
            fn[i + j] = dent->name[8 + j];
        }
    }

    fn[i + j] = '\0';
}

static void copy_longname(fat_dentry_t *dent) {
    fat_longname_t *lent;
    int fnlen;

    lent = (fat_longname_t *)dent;

    /* We've got our expected long name block... Deal with it. */
    fnlen = ((lent->order - 1) & 0x3F) * 13;

    /* Build out the filename component we have. */
    memcpy(&longname_buf[fnlen], lent->name1, 10);
    memcpy(&longname_buf[fnlen + 5], lent->name2, 12);
    memcpy(&longname_buf[fnlen + 11], lent->name3, 4);
}

static dirent_t *fs_fat_readdir(void *h) {
    file_t fd = ((file_t)h) - 1;
    fat_fs_t *fs;
    uint32_t bs, cl;
    uint8_t *block;
    int err, has_longname = 0;
    fat_dentry_t *dent;

    mutex_lock(&fat_mutex);

    /* Check that the fd is valid */
    if(fd >= MAX_FAT_FILES || !fh[fd].opened || !(fh[fd].mode & O_DIR)) {
        mutex_unlock(&fat_mutex);
        errno = EBADF;
        return NULL;
    }

    fs = fh[fd].fs->fs;

    /* The block size we use here requires a bit of thought...
       If the filesystem is FAT12/FAT16, we use the raw sector size if we're
       reading the root directory. In all other cases (a non-root directory or
       a FAT32 filesystem), we use the cluster size. This is because FAT12 and
       FAT16 do not store their root directory in the data clusters of the
       volume (but all other directories are stored in the data clusters). FAT32
       stores all of its directories in the data area of the volume. */
    if(fat_fs_type(fs) == FAT_FS_FAT32 || fh[fd].dentry_cluster)
        bs = fat_cluster_size(fs);
    else
        bs = fat_block_size(fs);

    /* Make sure we're not at the end of the directory. */
    if(fat_is_eof(fs, fh[fd].cluster)) {
        mutex_unlock(&fat_mutex);
        return NULL;
    }

    /* Read the block we're looking at... */
    if(!(block = fat_cluster_read(fs, fh[fd].cluster, &err))) {
        errno = err;
        mutex_unlock(&fat_mutex);
        return NULL;
    }

    memset(&fh[fd].dent, 0, sizeof(dirent_t));
    memset(longname_buf, 0, sizeof(uint16_t) * 256);

    /* Grab the entry. */
    do {
        dent = (fat_dentry_t *)(block + (fh[fd].ptr & (bs - 1)));
        fh[fd].ptr += 32;

        /* If this is a long name entry, copy the name out... */
        if(FAT_IS_LONG_NAME(dent)) {
            has_longname = 1;
            copy_longname(dent);
        }

        /* Did we hit the end? */
        if(dent->name[0] == FAT_ENTRY_EOD) {
            /* This will work for all versions of FAT, because of how the
               fat_is_eof() function works. */
            fh[fd].cluster = 0x0FFFFFF8;
            mutex_unlock(&fat_mutex);
            return NULL;
        }
        /* This entry is empty, so move onto the next one... */
        else if(dent->name[0] == FAT_ENTRY_FREE || FAT_IS_LONG_NAME(dent)) {
            /* Are we at the end of this block/cluster? */
            if((fh[fd].ptr & (bs - 1)) == 0) {
                if(fat_fs_type(fs) == FAT_FS_FAT32 || fh[fd].dentry_cluster) {
                    cl = fat_read_fat(fs, fh[fd].cluster, &err);

                    if(cl == FAT_INVALID_CLUSTER) {
                        errno = err;
                        mutex_unlock(&fat_mutex);
                        return NULL;
                    }
                    else if(fat_is_eof(fs, cl)) {
                        /* We've actually hit the end of the directory... */
                        mutex_unlock(&fat_mutex);
                        return NULL;
                    }

                    fh[fd].cluster = cl;
                    ++fh[fd].cluster_order;
                }
                else {
                    /* Are we at the end of the directory? */
                    if((fh[fd].ptr >> 5) >= fat_rootdir_length(fs)) {
                        fh[fd].cluster = 0x0FFFFFFF;
                        mutex_unlock(&fat_mutex);
                        return NULL;
                    }

                    ++fh[fd].cluster;
                    ++fh[fd].cluster_order;
                }
            }
        }
    } while(dent->name[0] == FAT_ENTRY_FREE || FAT_IS_LONG_NAME(dent));

    /* We now have a dentry to work with... Fill in the static dirent_t. */
    if(!has_longname)
        copy_shortname(dent, fh[fd].dent.name);
    else
        fat_ucs2_to_utf8((uint8_t *)fh[fd].dent.name, longname_buf, 256,
                         fat_strlen_ucs2(longname_buf));

    fh[fd].dent.size = dent->size;
    fh[fd].dent.time = fat_time_to_stat(dent->mdate, dent->mtime);

    if(dent->attr & FAT_ATTR_DIRECTORY) {
        fh[fd].dent.attr = O_DIR;
        fh[fd].dent.size = -1;
    }

    /* We're done. Return the static dirent_t. */
    mutex_unlock(&fat_mutex);
    return &fh[fd].dent;
}

static int fs_fat_fcntl(void *h, int cmd, va_list ap) {
    file_t fd = ((file_t)h) - 1;
    int rv = -1;

    (void)ap;

    mutex_lock(&fat_mutex);

    if(fd >= MAX_FAT_FILES || !fh[fd].opened) {
        mutex_unlock(&fat_mutex);
        errno = EBADF;
        return -1;
    }

    switch(cmd) {
        case F_GETFL:
            rv = fh[fd].mode;
            break;

        case F_SETFL:
        case F_GETFD:
        case F_SETFD:
            rv = 0;
            break;

        default:
            errno = EINVAL;
    }

    mutex_unlock(&fat_mutex);
    return rv;
}

static int fs_fat_unlink(vfs_handler_t *vfs, const char *fn) {
    fs_fat_fs_t *fs = (fs_fat_fs_t *)vfs->privdata;
    fat_dentry_t ent;
    int irv = 0, err;
    uint32_t cl, off, lcl, loff, cluster;

    mutex_lock(&fat_mutex);

    /* Make sure the filesystem isn't mounted read-only. */
    if(!(fs->mount_flags & FS_FAT_MOUNT_READWRITE)) {
        mutex_unlock(&fat_mutex);
        errno = EROFS;
        return -1;
    }

    /* Find the object in question */
    if((irv = fat_find_dentry(fs->fs, fn, &ent, &cl, &off, &lcl, &loff)) < 0) {
        mutex_unlock(&fat_mutex);
        errno = -irv;
        return -1;
    }

    /* Make sure that the user isn't trying to delete a directory. */
    if((ent.attr & FAT_ATTR_DIRECTORY)) {
        mutex_unlock(&fat_mutex);
        errno = EISDIR;
        return -1;
    }

    if((ent.attr & FAT_ATTR_VOLUME_ID)) {
        mutex_unlock(&fat_mutex);
        errno = ENOENT;
        return -1;
    }

    /* First clean up the clusters of the file... (if any) */
    cluster = ent.cluster_low | (ent.cluster_high << 16);
    if(cluster != FAT_FREE_CLUSTER) {
        if((err = fat_erase_chain(fs->fs, cluster))) {
            /* Uh oh... This is really bad... */
            dbglog(DBG_ERROR, "fs_fat: Error erasing FAT chain for file %s\n",
	           fn);
            irv = -1;
            errno = -err;
        }
    }

    /* Next, erase the directory entry (and long name, if applicable). */
    if((irv = fat_erase_dentry(fs->fs, cl, off, lcl, loff)) < 0) {
        dbglog(DBG_ERROR, "fs_fat: Error erasing directory entry for file %s\n",
               fn);
        errno = -irv;
        irv = -1;
    }

    mutex_unlock(&fat_mutex);
    return irv;
}

static int fs_fat_stat(vfs_handler_t *vfs, const char *path, struct stat *st,
                       int flag) {
    fs_fat_fs_t *fs = (fs_fat_fs_t *)vfs->privdata;
    uint32_t sz, bs;
    int irv = 0;
    fat_dentry_t ent;
    uint32_t cl, off, lcl, loff;
    size_t len = strlen(path);

    (void)flag;

    /* Root directory fat device */
    if(len == 0 || (len == 1 && *path == '/')) {
        memset(st, 0, sizeof(struct stat));
        st->st_dev = (dev_t)((uintptr_t)fs->vfsh);
        st->st_mode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR |
            S_IXGRP | S_IXOTH | S_IWUSR | S_IWGRP | S_IWOTH;
        st->st_size = -1;
        st->st_nlink = 2;

        return 0;
    }

    mutex_lock(&fat_mutex);

    /* Find the object in question */
    if((irv = fat_find_dentry(fs->fs, path, &ent, &cl, &off, &lcl,
                              &loff)) < 0) {
        errno = -irv;
        mutex_unlock(&fat_mutex);
        return -1;
    }

    /* Fill in the structure */
    memset(st, 0, sizeof(struct stat));
    irv = 0;
    st->st_dev = (dev_t)((uintptr_t)fs->vfsh);
    st->st_ino = ent.cluster_low | (ent.cluster_high << 16);
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_blksize = fat_cluster_size(fs->fs);

    /* Read the mode bits... */
    st->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH;
    if(!(ent.attr & FAT_ATTR_READ_ONLY)) {
        st->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
    }

    /* Fill in the timestamps... */
    fill_stat_timestamps(&ent, st);

    /* The rest depends on what type of object this is... */
    if(ent.attr & FAT_ATTR_DIRECTORY) {
        st->st_mode |= S_IFDIR;
        st->st_size = 0;
        st->st_blocks = 0;
    }
    else {
        st->st_mode |= S_IFREG;
        sz = ent.size;

        if(sz > LONG_MAX) {
            errno = EOVERFLOW;
            irv = -1;
        }

        st->st_size = sz;
        bs = fat_cluster_size(fs->fs);
        st->st_blocks = sz / bs;

        if(sz & (bs - 1))
            ++st->st_blocks;
    }

    mutex_unlock(&fat_mutex);

    return irv;
}

static int fs_fat_mkdir(vfs_handler_t *vfs, const char *fn) {
    fs_fat_fs_t *fs = (fs_fat_fs_t *)vfs->privdata;
    int err;
    uint32_t cl, off, lcl, loff, cl2 = 0;
    uint8_t *buf = NULL;

    mutex_lock(&fat_mutex);

    /* Make sure the filesystem isn't mounted read-only. */
    if(!(fs->mount_flags & FS_FAT_MOUNT_READWRITE)) {
        mutex_unlock(&fat_mutex);
        errno = EROFS;
        return -1;
    }

    if((err = fat_create_entry(fs->fs, fn, FAT_ATTR_DIRECTORY, &cl, &off, &lcl,
                               &loff, &buf, &cl2)) < 0) {
        mutex_unlock(&fat_mutex);
        errno = -err;
        return -1;
    }

    /* Add entries for "." and ".." */
    fat_add_raw_dentry((fat_dentry_t *)buf, ".          ", FAT_ATTR_DIRECTORY,
                       cl);
    fat_add_raw_dentry((fat_dentry_t *)(buf + sizeof(fat_dentry_t)),
                       "..         ", FAT_ATTR_DIRECTORY, cl2);

    /* And we're done... Clean up. */
    mutex_unlock(&fat_mutex);
    return 0;
}

static int fs_fat_rmdir(vfs_handler_t *vfs, const char *fn) {
    fs_fat_fs_t *fs = (fs_fat_fs_t *)vfs->privdata;
    fat_dentry_t ent;
    int irv = 0, err;
    uint32_t cl, off, lcl, loff, cluster;

    mutex_lock(&fat_mutex);

    /* Make sure the filesystem isn't mounted read-only. */
    if(!(fs->mount_flags & FS_FAT_MOUNT_READWRITE)) {
        mutex_unlock(&fat_mutex);
        errno = EROFS;
        return -1;
    }

    /* Find the object in question */
    if((irv = fat_find_dentry(fs->fs, fn, &ent, &cl, &off, &lcl, &loff)) < 0) {
        mutex_unlock(&fat_mutex);
        errno = -irv;
        return -1;
    }

    /* Make sure that the user isn't trying to rmdir a file. */
    if(!(ent.attr & FAT_ATTR_DIRECTORY)) {
        mutex_unlock(&fat_mutex);
        errno = ENOTDIR;
        return -1;
    }

    /* Make sure they're not trying to delete the root directory... */
    if(!cl) {
        mutex_unlock(&fat_mutex);
        errno = EPERM;
        return -1;
    }

    /* Make sure the directory is empty... */
    cluster = ent.cluster_low | (ent.cluster_high << 16);
    irv = fat_is_dir_empty(fs->fs, cluster);

    if(irv < 0) {
        mutex_unlock(&fat_mutex);
        errno = -irv;
        return -1;
    }
    else if(irv == 0) {
        mutex_unlock(&fat_mutex);
        errno = ENOTEMPTY;
        return -1;
    }

    /* First clean up the clusters of the directory... */
    if((err = fat_erase_chain(fs->fs, cluster))) {
        /* Uh oh... This is really bad... */
        dbglog(DBG_ERROR, "fs_fat: Error erasing FAT chain for directory %s\n",
               fn);
        irv = -1;
        errno = -err;
    }

    /* Next, erase the directory entry (and long name, if applicable). */
    if((irv = fat_erase_dentry(fs->fs, cl, off, lcl, loff)) < 0) {
        dbglog(DBG_ERROR, "fs_fat: Error erasing directory entry for directory "
               "%s\n", fn);
        irv = -1;
        errno = -err;
    }

    mutex_unlock(&fat_mutex);
    return irv;
}

static int fs_fat_rewinddir(void *h) {
    file_t fd = ((file_t)h) - 1;

    mutex_lock(&fat_mutex);

    /* Check that the fd is valid */
    if(fd >= MAX_FAT_FILES || !fh[fd].opened || !(fh[fd].mode & O_DIR)) {
        mutex_unlock(&fat_mutex);
        errno = EBADF;
        return -1;
    }

    /* Rewind to the beginning of the directory. */
    fh[fd].ptr = 0;
    fh[fd].cluster = fh[fd].dentry.cluster_low |
        (fh[fd].dentry.cluster_high << 16);
    fh[fd].cluster_order = 0;

    mutex_unlock(&fat_mutex);
    return 0;
}

static int fs_fat_fstat(void *h, struct stat *buf) {
    fs_fat_fs_t *fs;
    uint32_t sz, bs;
    file_t fd = ((file_t)h) - 1;
    int irv = 0;
    fat_dentry_t *ent;

    mutex_lock(&fat_mutex);

    if(fd >= MAX_FAT_FILES || !fh[fd].opened) {
        mutex_unlock(&fat_mutex);
        errno = EBADF;
        return -1;
    }

    /* Find the object in question */
    ent = &fh[fd].dentry;
    fs = fh[fd].fs;

    /* Fill in the structure */
    memset(buf, 0, sizeof(struct stat));
    buf->st_dev = (dev_t)((uintptr_t)fs->vfsh);
    buf->st_ino = ent->cluster_low | (ent->cluster_high << 16);
    buf->st_nlink = 1;
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_blksize = fat_cluster_size(fs->fs);

    /* Read the mode bits... */
    buf->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH;
    if(!(ent->attr & FAT_ATTR_READ_ONLY)) {
        buf->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
    }

    /* Fill in the timestamps... */
    fill_stat_timestamps(ent, buf);

    /* The rest depends on what type of object this is... */
    if(ent->attr & FAT_ATTR_DIRECTORY) {
        buf->st_mode |= S_IFDIR;
        buf->st_size = 0;
        buf->st_blocks = 0;
    }
    else {
        buf->st_mode |= S_IFREG;
        sz = ent->size;

        if(sz > LONG_MAX) {
            errno = EOVERFLOW;
            irv = -1;
        }

        buf->st_size = sz;
        bs = fat_cluster_size(fs->fs);
        buf->st_blocks = sz / bs;

        if(sz & (bs - 1))
            ++buf->st_blocks;
    }

    mutex_unlock(&fat_mutex);

    return irv;
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

    fs_fat_open,                /* open */
    fs_fat_close,               /* close */
    fs_fat_read,                /* read */
    fs_fat_write,               /* write */
    NULL,                       /* seek */
    NULL,                       /* tell */
    NULL,                       /* total */
    fs_fat_readdir,             /* readdir */
    NULL,                       /* ioctl */
    NULL,                       /* rename */
    fs_fat_unlink,              /* unlink */
    NULL,                       /* mmap */
    NULL,                       /* complete */
    fs_fat_stat,                /* stat */
    fs_fat_mkdir,               /* mkdir */
    fs_fat_rmdir,               /* rmdir */
    fs_fat_fcntl,               /* fcntl */
    NULL,                       /* poll */
    NULL,                       /* link */
    NULL,                       /* symlink */
    fs_fat_seek64,              /* seek64 */
    fs_fat_tell64,              /* tell64 */
    fs_fat_total64,             /* total64 */
    NULL,                       /* readlink */
    fs_fat_rewinddir,           /* rewinddir */
    fs_fat_fstat                /* fstat */
};

static int initted = 0;

/* These two functions borrow heavily from the same functions in fs_romdisk */
int fs_fat_mount(const char *mp, kos_blockdev_t *dev, uint32_t flags) {
    fat_fs_t *fs;
    fs_fat_fs_t *mnt;
    vfs_handler_t *vfsh;

    if(!initted)
        return -1;

    if((flags & FS_FAT_MOUNT_READWRITE) && !dev->write_blocks) {
        dbglog(DBG_DEBUG, "fs_fat: device does not support writing, cannot "
               "mount filesystem as read-write\n");
        return -1;
    }

    mutex_lock(&fat_mutex);

    /* Try to initialize the filesystem */
    if(!(fs = fat_fs_init(dev, flags))) {
        mutex_unlock(&fat_mutex);
        dbglog(DBG_DEBUG, "fs_fat: device does not contain a valid FAT FS.\n");
        return -1;
    }

    /* Create a mount structure */
    if(!(mnt = (fs_fat_fs_t *)malloc(sizeof(fs_fat_fs_t)))) {
        dbglog(DBG_DEBUG, "fs_fat: out of memory creating fs structure\n");
        fat_fs_shutdown(fs);
        mutex_unlock(&fat_mutex);
        return -1;
    }

    mnt->fs = fs;
    mnt->mount_flags = flags;

    /* Create a VFS structure */
    if(!(vfsh = (vfs_handler_t *)malloc(sizeof(vfs_handler_t)))) {
        dbglog(DBG_DEBUG, "fs_fat: out of memory creating vfs handler\n");
        free(mnt);
        fat_fs_shutdown(fs);
        mutex_unlock(&fat_mutex);
        return -1;
    }

    memcpy(vfsh, &vh, sizeof(vfs_handler_t));
    strcpy(vfsh->nmmgr.pathname, mp);
    vfsh->privdata = mnt;
    mnt->vfsh = vfsh;

    /* Add it to our list */
    LIST_INSERT_HEAD(&fat_fses, mnt, entry);

    /* Register with the VFS */
    if(nmmgr_handler_add(&vfsh->nmmgr)) {
        dbglog(DBG_DEBUG, "fs_fat: couldn't add fs to nmmgr\n");
        free(vfsh);
        free(mnt);
        fat_fs_shutdown(fs);
        mutex_unlock(&fat_mutex);
        return -1;
    }

    mutex_unlock(&fat_mutex);
    return 0;
}

int fs_fat_unmount(const char *mp) {
    fs_fat_fs_t *i;
    int found = 0, rv = 0;

    /* Find the fs in question */
    mutex_lock(&fat_mutex);
    LIST_FOREACH(i, &fat_fses, entry) {
        if(!strcmp(mp, i->vfsh->nmmgr.pathname)) {
            found = 1;
            break;
        }
    }

    if(found) {
        LIST_REMOVE(i, entry);

        /* XXXX: We should probably do something with open files... */
        nmmgr_handler_remove(&i->vfsh->nmmgr);
        fat_fs_shutdown(i->fs);
        free(i->vfsh);
        free(i);
    }
    else {
        errno = ENOENT;
        rv = -1;
    }

    mutex_unlock(&fat_mutex);
    return rv;
}

int fs_fat_sync(const char *mp) {
    fs_fat_fs_t *i;
    int found = 0, rv = 0;

    /* Find the fs in question */
    mutex_lock(&fat_mutex);
    LIST_FOREACH(i, &fat_fses, entry) {
        if(!strcmp(mp, i->vfsh->nmmgr.pathname)) {
            found = 1;
            break;
        }
    }

    if(found) {
        /* fat_fs_sync() will set errno if there's a problem. */
        rv = fat_fs_sync(i->fs);
    }
    else {
        errno = ENOENT;
        rv = -1;
    }

    mutex_unlock(&fat_mutex);
    return rv;
}

int fs_fat_init(void) {
    if(initted)
        return 0;

    LIST_INIT(&fat_fses);
    mutex_init(&fat_mutex, MUTEX_TYPE_NORMAL);
    initted = 1;

    memset(fh, 0, sizeof(fh));

    return 0;
}

int fs_fat_shutdown(void) {
    fs_fat_fs_t *i, *next;

    if(!initted)
        return 0;

    /* Clean up the mounted filesystems */
    i = LIST_FIRST(&fat_fses);
    while(i) {
        next = LIST_NEXT(i, entry);

        /* XXXX: We should probably do something with open files... */
        nmmgr_handler_remove(&i->vfsh->nmmgr);
        fat_fs_shutdown(i->fs);
        free(i->vfsh);
        free(i);

        i = next;
    }

    mutex_destroy(&fat_mutex);
    initted = 0;

    return 0;
}
