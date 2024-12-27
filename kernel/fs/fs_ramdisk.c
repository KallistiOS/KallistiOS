/* KallistiOS ##version##

   fs_ramdisk.c
   Copyright (C) 2002, 2003 Megan Potter
   Copyright (C) 2012, 2013, 2014, 2016 Lawrence Sebald

*/

/*

This module implements a very simple file-based ramdisk file system. What this means
is that instead of setting up a block of memory as a virtual block device like
many operating systems would do, this file system keeps the directory structure
and file data in allocated chunks of RAM. This also means that the ramdisk can
get as big as the memory available, there's no arbitrary limit.

A note of warning about thread usage here as well. This FS is protected against
thread contention at a file handle and data structure level. This means that the
directory structures and the file handles will never become inconsistent. However,
it is not protected at the individual file level. Because of this limitation, only
one file handle may be open to an individual file for writing at any given time.
If the file is already open for reading, it cannot be written to. Likewise, if
the file is open for writing, you can't open it for reading or writing.

So for example, if you wanted to cache an MP3 in the ramdisk, you'd copy the data
to the ramdisk in write mode, then close the file and let the library re-open it
in read-only mode. You'd then be safe.

So at the moment this is mainly useful as a scratch space for temp files or to
cache data from disk rather than as a general purpose file system.

*/

#include <kos/thread.h>
#include <kos/mutex.h>
#include <kos/fs_ramdisk.h>
#include <kos/opts.h>
#include <malloc.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#ifdef __STRICT_ANSI__
/* Newlib doesn't prototype this function in strict standards compliant mode, so
   we'll do it here. It is still provided either way, but it isn't prototyped if
   we use -std=c99 (or any other non-gnuXX value). */
char *strdup(const char *);
#endif

/* File definition */
typedef struct rd_file {
    char    * name;     /* File name -- allocated */
    uint32  size;       /* Actual file size */
    int type;       /* File type */
    int openfor;    /* Lock constant */
    int usage;      /* Usage count (unopened is 0) */

    /* For the following two members:
      - In files, this is a block of allocated memory containing the
        actual file data. Each time we need to expand it beyond its
        current capacity, we realloc() with enough to hold the new
        data plus 4k (to avoid realloc thrashing). All files start
        out with a 1K block of space.
      - In directories, this is just a pointer to an rd_dir struct,
        which is defined below. datasize has no meaning for a
        directory. */
    void    * data;     /* Data block pointer */
    uint32  datasize;   /* Size of data block pointer */

    LIST_ENTRY(rd_file) dirlist;    /* Directory list entry */
} rd_file_t;

/* Lock constants */
#define OPENFOR_NOTHING 0   /* Not opened */
#define OPENFOR_READ    1   /* Opened read-only */
#define OPENFOR_WRITE   2   /* Opened read-write */

/* Directory definition -- just basically a list of files we contain */
typedef LIST_HEAD(rd_dir, rd_file) rd_dir_t;

/* Pointer to the root diretctory */
static rd_file_t *root = NULL;
static rd_dir_t  *rootdir = NULL;

/********************************************************************************/
/* File primitives */

/* File handles.. I could probably do this with a linked list, but I'm just
   too lazy right now. =) */
static struct {
    rd_file_t   *file;      /* ramdisk file struct */
    int         dir;        /* >0 if a directory */
    uint32      ptr;        /* Current read position in bytes */
    dirent_t    dirent;     /* A static dirent to pass back to clients */
    int         omode;      /* Open mode */
} fh[FS_RAMDISK_MAX_FILES];

/* Mutex for file system structs */
static mutex_t rd_mutex;

/* Search a directory for the named file; return the struct if
   we find it. Assumes we hold rd_mutex. */
static rd_file_t *ramdisk_find(rd_dir_t *parent, const char *name, size_t namelen) {
    rd_file_t   *f;

    LIST_FOREACH(f, parent, dirlist) {
        if((strlen(f->name) == namelen) && !strncasecmp(name, f->name, namelen))
            return f;
    }

    return NULL;
}

/* Find a path-named file in the ramdisk. There should not be a
   slash at the beginning, nor at the end. Assumes we hold rd_mutex. */
static rd_file_t * ramdisk_find_path(rd_dir_t * parent, const char * fn, int dir) {
    rd_file_t * f = NULL;
    char * cur;

    /* If the object is in a sub-tree, traverse the tree looking
       for the right directory */
    while((cur = strchr(fn, '/'))) {
        /* We've got another part to look at */
        if(cur != fn) {
            /* Look for it in the parent dir.. if it's not a dir
               itself, something is wrong. */
            f = ramdisk_find(parent, fn, cur - fn);

            if(f == NULL || f->type != STAT_TYPE_DIR)
                return NULL;

            /* Pull out the rd_dir_t pointer */
            parent = (rd_dir_t *)f->data;
            assert(parent != NULL);
        }

        /* Skip the last piece of the pathname */
        fn = cur + 1;
    }

    /* Ok, no more directories */

    /* If there was a remaining file part, then look for it
       in the dir. */
    if(fn[0] != 0) {
        f = ramdisk_find(parent, fn, strlen(fn));

        if((f == NULL) || (!dir && f->type == STAT_TYPE_DIR) || (dir && f->type != STAT_TYPE_DIR))
            return NULL;
    }
    else {
        /* We must have been looking for the dir itself */
        if(!dir)
            return NULL;
    }

    return f;
}

/* Find the parent directory and file name in the path-named file */
static int ramdisk_get_parent(rd_dir_t * parent, const char * fn, rd_dir_t ** dout, const char **fnout) {
    const char  * p;
    char        * pname;
    rd_file_t   * f;

    p = strrchr(fn, '/');

    if(p == NULL) {
        *dout = parent;
        *fnout = fn;
    }
    else {
        pname = (char *)malloc((p - fn) + 1);
        strncpy(pname, fn, p - fn);
        pname[p - fn] = 0;

        f = ramdisk_find_path(parent, pname, 1);
        free(pname);

        if(!f)
            return -1;

        *dout = (rd_dir_t *)f->data;
        *fnout = p + 1;
        assert(*dout != NULL);
    }

    return 0;
}

/* Create a path-named file in the ramdisk. There should not be a
   slash at the beginning, nor at the end. Assumes we hold rd_mutex. */
static rd_file_t * ramdisk_create_file(rd_dir_t * parent, const char * fn, int dir) {
    rd_file_t   * f;
    rd_dir_t    * pdir;
    const char  * p;

    /* First, find the parent dir */
    if(ramdisk_get_parent(parent, fn, &pdir, &p) < 0)
        return NULL;

    /* Now add a file to the parent */
    if(!(f = (rd_file_t *)malloc(sizeof(rd_file_t))))
        return NULL;

    f->name = strdup(p);
    if(f->name == NULL) {
        free(f);
        return NULL;
    }

    f->size = 0;
    f->type = dir ? STAT_TYPE_DIR : STAT_TYPE_FILE;
    f->openfor = OPENFOR_NOTHING;
    f->usage = 0;

    if(!dir) {
        f->data = malloc(1024);
        f->datasize = 1024;
    }
    else {
        f->data = malloc(sizeof(rd_dir_t));
        f->datasize = 0;
    }

    if(f->data == NULL) {
        free(f->name);
        free(f);
        return NULL;
    }

    LIST_INSERT_HEAD(pdir, f, dirlist);

    return f;
}

/* Open a file or directory */
static void * ramdisk_open(vfs_handler_t * vfs, const char *fn, int mode) {
    file_t      fd = -1;
    rd_file_t   *f;
    int     mm = mode & O_MODE_MASK;

    (void)vfs;

    if(fn[0] == '/')
        fn++;

    mutex_lock_scoped(&rd_mutex);

    /* Are we trying to do something stupid? */
    if((mode & O_DIR) && mm != O_RDONLY)
        goto error_out;

    /* Look for the file */
    assert(root != NULL);

    if(fn[0] == 0) {
        f = root;
    }
    else {
        f = ramdisk_find_path(rootdir, fn, mode & O_DIR);

        if(f == NULL) {
            /* Are we planning to write anyway? */
            if(mm != O_RDONLY && !(mode & O_DIR)) {
                /* Create a new file */
                f = ramdisk_create_file(rootdir, fn, mode & O_DIR);

                if(f == NULL)
                    goto error_out;
            }
            else
                goto error_out;
        }
    }

    /* Check for more stupid things */
    if(f->type == STAT_TYPE_DIR && (!(mode & O_DIR) || mm != O_RDONLY))
        goto error_out;

    /* Find a free file handle */
    for(fd = 1; fd < FS_RAMDISK_MAX_FILES; fd++)
        if(fh[fd].file == NULL)
            break;

    /* Did we find it? */
    if(fd >= FS_RAMDISK_MAX_FILES) {
        fd = -1;
        goto error_out;
    }

    /* Is the file already open for write? */
    if(f->openfor == OPENFOR_WRITE)
        goto error_out;

    /* Fill the basic fd structure */
    fh[fd].file = f;
    fh[fd].dir = mode & O_DIR;
    fh[fd].omode = mode;

    /* The rest require a bit more thought */
    if(mm == O_RDONLY) {
        f->openfor = OPENFOR_READ;
        fh[fd].ptr = 0;
    }
    else if((mm & O_RDWR) || (mm & O_WRONLY)) {
        if(f->openfor == OPENFOR_READ)
            goto error_out;

        f->openfor = OPENFOR_WRITE;

        if(mode & O_APPEND)
            fh[fd].ptr = f->size;
        /* If we're opening with O_TRUNC, kill the existing contents */
        else if(mode & O_TRUNC) {
            free(f->data);
            f->data = malloc(1024);
            f->datasize = 1024;
            f->size = 0;
            fh[fd].ptr = 0;
        }
        else
            fh[fd].ptr = 0;
    }
    else {
        assert_msg(0, "Unknown file mode");
    }

    /* If we opened a dir, then ptr is actually a pointer to the first
       file entry. */
    if(mode & O_DIR) {
        fh[fd].ptr = (uint32)LIST_FIRST((rd_dir_t *)f->data);
    }

    /* Increase the usage count */
    f->usage++;

    /* Should do it... */
    return (void *)fd;

error_out:

    if(fd != -1)
        fh[fd].file = NULL;

    return NULL;
}

/* Close a file or directory */
static int ramdisk_close(void * h) {
    rd_file_t   *f;
    file_t      fd = (file_t)h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is valid */
    if(fd < FS_RAMDISK_MAX_FILES && fh[fd].file != NULL) {
        f = fh[fd].file;
        fh[fd].file = NULL;

        /* Decrease the usage count */
        f->usage--;
        assert(f->usage >= 0);

        /* If the usage count is back to 0, then no one has the file
           open. Remove the openfor status. */
        if(f->usage == 0)
            f->openfor = OPENFOR_NOTHING;
    }

    return 0;
}

/* Read from a file */
static ssize_t ramdisk_read(void * h, void *buf, size_t bytes) {
    ssize_t rv = -1;
    file_t  fd = (file_t)h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is valid */
    if(fd < FS_RAMDISK_MAX_FILES && fh[fd].file != NULL && !fh[fd].dir) {
        /* Is there enough left? */
        if((fh[fd].ptr + bytes) > fh[fd].file->size)
            bytes = fh[fd].file->size - fh[fd].ptr;

        /* Copy out the requested amount */
        memcpy(buf, ((uint8 *)fh[fd].file->data) + fh[fd].ptr, bytes);
        fh[fd].ptr += bytes;

        rv = bytes;
    }

    return rv;
}

/* Write to a file */
static ssize_t ramdisk_write(void * h, const void *buf, size_t bytes) {
    ssize_t rv = -1;
    file_t  fd = (file_t)h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is valid */
    if(fd < FS_RAMDISK_MAX_FILES && fh[fd].file != NULL && !fh[fd].dir && fh[fd].file->openfor == OPENFOR_WRITE) {
        /* Is there enough left? */
        if((fh[fd].ptr + bytes) > fh[fd].file->datasize) {
            /* We need to realloc the block */
            void * np = realloc(fh[fd].file->data, (fh[fd].ptr + bytes) + 4096);

            if(np == NULL)
                return -1;

            fh[fd].file->data = np;
            fh[fd].file->datasize = (fh[fd].ptr + bytes) + 4096;
        }

        /* Copy out the requested amount */
        memcpy(((uint8 *)fh[fd].file->data) + fh[fd].ptr, buf, bytes);
        fh[fd].ptr += bytes;

        if(fh[fd].file->size < fh[fd].ptr) {
            fh[fd].file->size = fh[fd].ptr;
        }

        rv = bytes;
    }

    return rv;
}

/* Seek elsewhere in a file */
static off_t ramdisk_seek(void * h, off_t offset, int whence) {
    file_t  fd = (file_t)h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is valid */
    if(fd >= FS_RAMDISK_MAX_FILES || !fh[fd].file || fh[fd].dir) {
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
            if(offset < 0 && ((uint32)-offset) > fh[fd].ptr) {
                errno = EINVAL;
                return -1;
            }

            fh[fd].ptr += offset;
            break;

        case SEEK_END:
            if(offset < 0 && ((uint32)-offset) > fh[fd].file->size) {
                errno = EINVAL;
                return -1;
            }

            fh[fd].ptr = fh[fd].file->size + offset;
            break;

        default:
            errno = EINVAL;
            return -1;
    }

    /* Check bounds */
    // XXXX: Technically this isn't correct. Fix it sometime.
    if(fh[fd].ptr > fh[fd].file->size) fh[fd].ptr = fh[fd].file->size;

    return fh[fd].ptr;
}

/* Tell where in the file we are */
static off_t ramdisk_tell(void * h) {
    file_t  fd = (file_t)h;

    mutex_lock_scoped(&rd_mutex);

    if(fd < FS_RAMDISK_MAX_FILES && fh[fd].file != NULL && !fh[fd].dir)
        return fh[fd].ptr;

    return -1;
}

/* Tell how big the file is */
static size_t ramdisk_total(void * h) {
    file_t  fd = (file_t)h;

    mutex_lock_scoped(&rd_mutex);

    if(fd < FS_RAMDISK_MAX_FILES && fh[fd].file != NULL && !fh[fd].dir)
        return fh[fd].file->size;

    return -1;
}

/* Read a directory entry */
static dirent_t *ramdisk_readdir(void * h) {
    rd_file_t   * f;
    file_t      fd = (file_t)h;

    mutex_lock_scoped(&rd_mutex);

    if(fd < FS_RAMDISK_MAX_FILES && fh[fd].file != NULL && fh[fd].ptr != 0 && fh[fd].dir) {
        /* Find the current file and advance to the next */
        f = (rd_file_t *)fh[fd].ptr;
        fh[fd].ptr = (uint32)LIST_NEXT(f, dirlist);

        /* Copy out the requested data */
        strcpy(fh[fd].dirent.name, f->name);
        fh[fd].dirent.time = 0;

        if(f->type == STAT_TYPE_DIR) {
            fh[fd].dirent.attr = O_DIR;
            fh[fd].dirent.size = -1;
        }
        else {
            fh[fd].dirent.attr = 0;
            fh[fd].dirent.size = f->size;
        }

        return &fh[fd].dirent;
    }
    else {
        errno = EBADF;
        return NULL;
    }
}

static int ramdisk_unlink(vfs_handler_t * vfs, const char *fn) {
    rd_file_t   * f;
    int     rv = -1;

    (void)vfs;

    mutex_lock_scoped(&rd_mutex);

    /* Find the file */
    f = ramdisk_find_path(rootdir, fn, 0);

    if(f) {
        /* Make sure it's not in use */
        if(f->usage == 0) {
            /* Free its data */
            free(f->name);
            free(f->data);

            /* Remove it from the parent list */
            LIST_REMOVE(f, dirlist);

            /* Free the entry itself */
            free(f);
            rv = 0;
        }
    }

    return rv;
}

static void * ramdisk_mmap(void * h) {
    file_t  fd = (file_t)h;

    mutex_lock_scoped(&rd_mutex);

    if(fd < FS_RAMDISK_MAX_FILES && fh[fd].file != NULL && !fh[fd].dir)
        return fh[fd].file->data;

    return NULL;
}

static int ramdisk_stat(vfs_handler_t *vfs, const char *path, struct stat *st,
                        int flag) {
    rd_file_t *f;
    size_t len = strlen(path);

    (void)vfs;
    (void)flag;

    /* Root directory of ramdisk */
    if(len == 0 || (len == 1 && *path == '/')) {
        memset(st, 0, sizeof(struct stat));
        st->st_dev = (dev_t)('r' | ('a' << 8) | ('m' << 16));
        st->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        st->st_size = -1;
        st->st_nlink = 2;

        return 0;
    }

    mutex_lock_scoped(&rd_mutex);

    /* Find the file */
    f = ramdisk_find_path(rootdir, path, 0);
    if(!f) {
        errno = ENOENT;
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_dev = (dev_t)('r' | ('a' << 8) | ('m' << 16));
    st->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    st->st_mode |= (f->type == STAT_TYPE_DIR) ? 
        (S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH) : S_IFREG;
    st->st_size = (f->type == STAT_TYPE_DIR) ? -1 : (int)f->datasize;
    st->st_nlink = (f->type == STAT_TYPE_DIR) ? 2 : 1;
    st->st_blksize = 1024;
    st->st_blocks = f->datasize >> 10;

    if(f->datasize & 0x3ff)
        ++st->st_blocks;

    return 0;
}

static int ramdisk_fcntl(void *h, int cmd, va_list ap) {
    file_t fd = (file_t)h;

    (void)ap;

    mutex_lock_scoped(&rd_mutex);

    if(fd >= FS_RAMDISK_MAX_FILES || !fh[fd].file) {
        errno = EBADF;
        return -1;
    }

    switch(cmd) {
        case F_GETFL:
            return fh[fd].omode;

        case F_SETFL:
        case F_GETFD:
        case F_SETFD:
            return 0;

        default:
            errno = EINVAL;
            return -1;
    }
}

static int ramdisk_rewinddir(void * h) {
    file_t fd = (file_t)h;

    mutex_lock_scoped(&rd_mutex);

    if(fd >= FS_RAMDISK_MAX_FILES || !fh[fd].file || !fh[fd].dir) {
        errno = EBADF;
        return -1;
    }

    /* Rewind to the first file. */
    fh[fd].ptr = (uint32)LIST_FIRST((rd_dir_t *)fh[fd].file->data);

    return 0;
}

static int ramdisk_fstat(void *h, struct stat *st) {
    file_t fd = (file_t)h;
    rd_file_t *f;

    mutex_lock_scoped(&rd_mutex);

    if(fd >= FS_RAMDISK_MAX_FILES || !fh[fd].file) {
        errno = EBADF;
        return -1;
    }

    /* Grab the file itself... */
    f = fh[fd].file;

    /* Fill in the structure. */
    memset(st, 0, sizeof(struct stat));
    st->st_dev = (dev_t)('r' | ('a' << 8) | ('m' << 16));
    st->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    st->st_mode |= (f->type == STAT_TYPE_DIR) ? S_IFDIR : S_IFREG;
    st->st_size = (f->type == STAT_TYPE_DIR) ? -1 : (int)f->datasize;
    st->st_nlink = (f->type == STAT_TYPE_DIR) ? 2 : 1;
    st->st_blksize = 1024;
    st->st_blocks = f->datasize >> 10;

    if(f->datasize & 0x3ff)
        ++st->st_blocks;

    return 0;
}

/* Put everything together */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/ram",         /* name */
        0,              /* tbfi */
        0x00010000,     /* Version 1.0 */
        0,              /* flags */
        NMMGR_TYPE_VFS, /* VFS handler */
        NMMGR_LIST_INIT
    },

    0, NULL,            /* no caching, privdata */

    ramdisk_open,
    ramdisk_close,
    ramdisk_read,
    ramdisk_write,
    ramdisk_seek,
    ramdisk_tell,
    ramdisk_total,
    ramdisk_readdir,
    NULL,               /* ioctl */
    NULL,               /* rename XXX */
    ramdisk_unlink,
    ramdisk_mmap,
    NULL,               /* complete */
    ramdisk_stat,
    NULL,               /* mkdir XXX */
    NULL,               /* rmdir XXX */
    ramdisk_fcntl,
    NULL,               /* poll XXX */
    NULL,               /* link XXX */
    NULL,               /* symlink XXX */
    NULL,               /* seek64 XXX */
    NULL,               /* tell64 XXX */
    NULL,               /* total64 XXX */
    NULL,               /* readlink XXX */
    ramdisk_rewinddir,
    ramdisk_fstat
};

/* Attach a piece of memory to a file. This works somewhat like open for
   writing, but it doesn't actually attach the file to an fd, and it starts
   out with data instead of being blank. */
int fs_ramdisk_attach(const char * fn, void * obj, size_t size) {
    void        *fd;
    rd_file_t   *f;

    /* First of all, open a file for writing. This'll save us a bunch
       of duplicated code. */
    fd = ramdisk_open(&vh, fn, O_WRONLY | O_TRUNC);

    if(fd == NULL)
        return -1;

    /* Ditch the data block we had and replace it with the user one. */
    f = fh[(int)fd].file;
    free(f->data);
    f->data = obj;
    f->datasize = size;
    f->size = size;

    /* Close the file */
    ramdisk_close(fd);

    return 0;
}

/* Does the opposite of attach. This again piggybacks on open. */
int fs_ramdisk_detach(const char * fn, void ** obj, size_t * size) {
    void        *fd;
    rd_file_t   *f;

    /* First of all, open a file for reading. This'll save us a bunch
       of duplicated code. */
    fd = ramdisk_open(&vh, fn, O_RDONLY);

    if(fd == NULL)
        return -1;

    /* Pull the data block and put it in the user parameters. */
    assert(obj != NULL);
    assert(size != NULL);

    f = fh[(int)fd].file;
    *obj = f->data;
    *size = f->size;

    /* Ditch the data block we had and replace it with a fake one. */
    f->data = malloc(64);
    f->datasize = 64;
    f->size = 64;

    /* Close the file */
    ramdisk_close(fd);

    /* Unlink the file */
    ramdisk_unlink(&vh, fn);

    return 0;
}

/* Initialize the file system */
void fs_ramdisk_init(void) {
    /* Test if initted */
    if(rootdir != NULL)
        return;

    /* Create an empty root dir */
    if(!(rootdir = (rd_dir_t *)malloc(sizeof(rd_dir_t))))
        return;

    root = (rd_file_t *)malloc(sizeof(rd_file_t));
    if(root == NULL) {
        free(rootdir);
        return;
    }

    root->name = strdup("/");
    if(root->name == NULL) {
        free(root);
        free(rootdir);
        return;
    }

    root->size = 0;
    root->type = STAT_TYPE_DIR;
    root->openfor = OPENFOR_NOTHING;
    root->usage = 0;
    root->data = rootdir;
    root->datasize = 0;

    LIST_INIT(rootdir);

    /* Reset fd's */
    memset(fh, 0, sizeof(fh));

    /* Init thread mutexes */
    mutex_init(&rd_mutex, MUTEX_TYPE_NORMAL);

    /* Register with VFS */
    nmmgr_handler_add(&vh.nmmgr);
}

/* De-init the file system */
void fs_ramdisk_shutdown(void) {
    rd_file_t *f1, *f2;

    /* Test if initted */
    if(rootdir == NULL)
        return;

    /* For now assume there's only the root dir, since mkdir and
       rmdir aren't even implemented... */
    f1 = LIST_FIRST(rootdir);

    while(f1) {
        f2 = LIST_NEXT(f1, dirlist);
        free(f1->name);
        free(f1->data);
        free(f1);
        f1 = f2;
    }

    free(rootdir);
    free(root->name);
    free(root);

    mutex_destroy(&rd_mutex);
    nmmgr_handler_remove(&vh.nmmgr);
}
