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

#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdint.h>
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
    char      *name;    /* File name -- allocated */
    uint32_t  size;     /* Actual file size */
    bool      isdir;    /* Dir or not */
    int       openfor;  /* Lock constant */
    int       usage;    /* Usage count (unopened is 0) */

    /* For the following two members:
      - In files, this is a block of allocated memory containing the
        actual file data. Each time we need to expand it beyond its
        current capacity, we realloc() with enough to hold the new
        data plus 4k (to avoid realloc thrashing). All files start
        out with a 1K block of space.
      - In directories, this is just a pointer to an rd_dir struct,
        which is defined below. datasize has no meaning for a
        directory. */
    void      *data;    /* Data block pointer */
    uint32_t  datasize; /* Size of data block pointer */

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

/* Entries in the fd list. Pointers of this type are passed out to ref files. */
typedef struct rd_fd {
    rd_file_t   *file;      /* ramdisk file struct */
    bool        dir;        /* true if a directory */
    uint32_t    ptr;        /* Current read position in bytes */
    dirent_t    dirent;     /* A static dirent to pass back to clients */
    int         omode;      /* Open mode */
    TAILQ_ENTRY(rd_fd)  next;   /* Next handle in the linked list */
} rd_fd_t;

static TAILQ_HEAD(rd_fd_queue, rd_fd) rd_fd_queue;

/* Mutex for file system structs */
static mutex_t rd_mutex;

/* Data used for stat->st_dev's dev_t */
static const dev_t rd_dev = (dev_t)('r' | ('a' << 8) | ('m' << 16));

/* Data used for stat->st_blksize's blksize_t */
static const blksize_t rd_blksize = 1024;

/* Test if an file_t is invalid, presumes mutex is already held. */
static inline bool ramdisk_fd_invalid(rd_fd_t *fd) {
    return(!fd || (!fd->file));
}

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
static rd_file_t *ramdisk_find_path(rd_dir_t *parent, const char *fn, bool dir) {
    rd_file_t *f = NULL;
    char *cur;

    /* If the object is in a sub-tree, traverse the tree looking
       for the right directory */
    while((cur = strchr(fn, '/'))) {
        /* We've got another part to look at */
        if(cur != fn) {
            /* Look for it in the parent dir.. if it's not a dir
               itself, something is wrong. */
            f = ramdisk_find(parent, fn, cur - fn);

            if(f == NULL || !f->isdir)
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

        if((f == NULL) || (!dir && f->isdir) || (dir && !f->isdir))
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
static int ramdisk_get_parent(rd_dir_t *parent, const char *fn, rd_dir_t **dout, const char **fnout) {
    const char  *p;
    char        *pname;
    rd_file_t   *f;

    p = strrchr(fn, '/');

    if(p == NULL) {
        *dout = parent;
        *fnout = fn;
    }
    else {
        pname = (char *)malloc((p - fn) + 1);

        if(!pname) {
            errno = ENOMEM;
            return -2;
        }

        strncpy(pname, fn, p - fn);
        pname[p - fn] = 0;

        f = ramdisk_find_path(parent, pname, true);
        free(pname);

        if(!f) {
            errno = ENOENT;
            return -1;
        }

        *dout = (rd_dir_t *)f->data;
        *fnout = p + 1;
        assert(*dout != NULL);
    }

    return 0;
}

/* Create a path-named file in the ramdisk. There should not be a
   slash at the beginning, nor at the end. Assumes we hold rd_mutex. */
static rd_file_t *ramdisk_create_file(rd_dir_t *parent, const char *fn, bool dir) {
    rd_file_t   *f;
    rd_dir_t    *pdir;
    const char  *p;

    /* First, find the parent dir */
    if(ramdisk_get_parent(parent, fn, &pdir, &p) < 0)
        return NULL;

    /* Now add a file to the parent */
    if(!(f = (rd_file_t *)malloc(sizeof(rd_file_t)))) {
        errno = ENOMEM;
        return NULL;
    }

    f->name = strdup(p);
    if(f->name == NULL) {
        free(f);
        errno = ENOMEM;
        return NULL;
    }

    f->size = 0;
    f->isdir = dir;
    f->openfor = OPENFOR_NOTHING;
    f->usage = 0;

    if(!dir) {
        /* Initial file is one block */
        f->datasize = rd_blksize;
        f->data = malloc(f->datasize);
    }
    else {
        f->data = malloc(sizeof(rd_dir_t));
        f->datasize = 0;
    }

    if(f->data == NULL) {
        free(f->name);
        free(f);
        errno = ENOMEM;
        return NULL;
    }

    LIST_INSERT_HEAD(pdir, f, dirlist);

    return f;
}

/* Open a file or directory */
static void *ramdisk_open(vfs_handler_t *vfs, const char *fn, int mode) {
    rd_fd_t     *fd;
    rd_file_t   *f;
    int     mm = mode & O_MODE_MASK;

    (void)vfs;

    if(fn[0] == '/')
        fn++;

    mutex_lock_scoped(&rd_mutex);

    /* Are we trying to do something stupid? */
    if((mode & O_DIR) && mm != O_RDONLY) {
        errno = EISDIR;
        return NULL;
    }

    /* Look for the file */
    assert(root != NULL);

    if(fn[0] == 0) {
        f = root;
    }
    else {
        f = ramdisk_find_path(rootdir, fn, mode & O_DIR);

        if(f == NULL) {
            /* Are we planning to write to a file anyway? */
            if(mm != O_RDONLY && !(mode & O_DIR)) {
                /* Create a new file */
                f = ramdisk_create_file(rootdir, fn, mode & O_DIR);

                if(f == NULL)
                    return NULL;
            }
            /* Must be read only mode as we tested for non-read DIR earlier. */
            else /* if(mm == O_RDONLY) */ {
                errno = ENOENT;
                return NULL;
            }
        }
    }

    /* Did we ask for a dir as a file? */
    if(f->isdir && !(mode & O_DIR)) {
        errno = EINVAL;
        return NULL;
    }

    /* Is the file already open for write? */
    if(f->openfor == OPENFOR_WRITE)
        return NULL;

    /* Create a new file handle */
    fd = (rd_fd_t *)calloc(1, sizeof(*fd));

    /* Did we get memory? */
    if(!fd) {
        errno = ENOMEM;
        return NULL;
    }

    /* Fill the basic fd structure */
    fd->file = f;
    fd->dir = !!(mode & O_DIR);
    fd->omode = mode;

    /* The rest require a bit more thought */
    if(mm == O_RDONLY) {
        f->openfor = OPENFOR_READ;
        fd->ptr = 0;
    }
    else if((mm & O_RDWR) || (mm & O_WRONLY)) {
        if(f->openfor == OPENFOR_READ)
            return NULL;

        f->openfor = OPENFOR_WRITE;

        if(mode & O_APPEND)
            fd->ptr = f->size;
        /* If we're opening with O_TRUNC, kill the existing contents */
        else if(mode & O_TRUNC) {
            free(f->data);
            f->datasize = rd_blksize;
            f->data = malloc(f->datasize);
            f->size = 0;
            fd->ptr = 0;
        }
        else
            fd->ptr = 0;
    }
    else {
        assert_msg(0, "Unknown file mode");
    }

    /* If we opened a dir, then ptr is actually a pointer to the first
       file entry. */
    if(mode & O_DIR) {
        fd->ptr = (uint32_t)LIST_FIRST((rd_dir_t *)f->data);
    }

    /* Increase the usage count */
    f->usage++;

    /* Now insert the fd into our fd list */
    TAILQ_INSERT_TAIL(&rd_fd_queue, fd, next);

    /* Should do it... */
    return fd;
}

/* Close a file or directory */
static int ramdisk_close(void *h) {
    rd_file_t   *f;
    rd_fd_t     *fd = h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid */
    if(ramdisk_fd_invalid(fd)) {
        errno = EBADF;
        return 0;
    }

    f = fd->file;
    fd->file = NULL;

    /* Decrease the usage count */
    f->usage--;
    assert(f->usage >= 0);

    /* If the usage count is back to 0, then no one has the file
       open. Remove the openfor status. */
    if(f->usage == 0)
        f->openfor = OPENFOR_NOTHING;

    /* Remove fd from the queue, and free it. */
    TAILQ_REMOVE(&rd_fd_queue, fd, next);
    free(fd);

    return 0;
}

/* Read from a file */
static ssize_t ramdisk_read(void *h, void *buf, size_t bytes) {
    rd_fd_t     *fd = h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid or a dir */
    if(ramdisk_fd_invalid(fd) || fd->dir) {
        errno = EBADF;
        return (ssize_t)-1;
    }

    /* Is there enough left? */
    if((fd->ptr + bytes) > fd->file->size)
        bytes = fd->file->size - fd->ptr;

    /* Copy out the requested amount */
    memcpy(buf, ((uint8_t *)fd->file->data) + fd->ptr, bytes);
    fd->ptr += bytes;

    return bytes;
}

/* Write to a file */
static ssize_t ramdisk_write(void *h, const void *buf, size_t bytes) {
    rd_fd_t     *fd = h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid or a dir or not open for writing */
    if(ramdisk_fd_invalid(fd) || fd->dir ||
        (fd->file->openfor != OPENFOR_WRITE)) {
        errno = EBADF;
        return (ssize_t)-1;
    }

    /* Is there enough left? */
    if((fd->ptr + bytes) > fd->file->datasize) {
        /* We need to realloc the block */
        void *np = realloc(fd->file->data, (fd->ptr + bytes) + (rd_blksize * 4));

        if(np == NULL) {
            errno = ENOSPC;
            return -1;
        }

        fd->file->data = np;
        fd->file->datasize = (fd->ptr + bytes) + (rd_blksize * 4);
    }

    /* Copy out the requested amount */
    memcpy(((uint8_t *)fd->file->data) + fd->ptr, buf, bytes);
    fd->ptr += bytes;

    if(fd->file->size < fd->ptr) {
        fd->file->size = fd->ptr;
    }

    return bytes;
}

/* Seek elsewhere in a file */
static off_t ramdisk_seek(void *h, off_t offset, int whence) {
    rd_fd_t     *fd = h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid or a dir */
    if(ramdisk_fd_invalid(fd) || fd->dir) {
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

            fd->ptr = offset;
            break;

        case SEEK_CUR:
            if(offset < 0 && ((uint32_t)-offset) > fd->ptr) {
                errno = EINVAL;
                return -1;
            }

            fd->ptr += offset;
            break;

        case SEEK_END:
            if(offset < 0 && ((uint32_t)-offset) > fd->file->size) {
                errno = EINVAL;
                return -1;
            }

            fd->ptr = fd->file->size + offset;
            break;

        default:
            errno = EINVAL;
            return -1;
    }

    /* Check bounds */
    // XXXX: Technically this isn't correct. Fix it sometime.
    if(fd->ptr > fd->file->size) fd->ptr = fd->file->size;

    return fd->ptr;
}

/* Tell where in the file we are */
static off_t ramdisk_tell(void *h) {
    rd_fd_t     *fd = h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid or a dir */
    if(ramdisk_fd_invalid(fd) || fd->dir) {
        errno = EBADF;
        return -1;
    }

    return fd->ptr;
}

/* Tell how big the file is */
static size_t ramdisk_total(void *h) {
    rd_fd_t     *fd = h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid or a dir */
    if(ramdisk_fd_invalid(fd) || fd->dir) {
        errno = EBADF;
        return -1;
    }

    return fd->file->size;
}

/* Read a directory entry */
static dirent_t *ramdisk_readdir(void *h) {
    rd_file_t   *f;
    rd_fd_t     *fd = h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid or NOT a dir */
    if(ramdisk_fd_invalid(fd) || !fd->dir) {
        errno = EBADF;
        return NULL;
    }

    /* Find the current file and advance to the next */
    f = (rd_file_t *)fd->ptr;
    fd->ptr = (uint32_t)LIST_NEXT(f, dirlist);

    /* Copy out the requested data */
    strcpy(fd->dirent.name, f->name);
    fd->dirent.time = 0;

    if(f->isdir) {
        fd->dirent.attr = O_DIR;
        fd->dirent.size = -1;
    }
    else {
        fd->dirent.attr = 0;
        fd->dirent.size = f->size;
    }

    return &fd->dirent;
}

static int ramdisk_unlink(vfs_handler_t *vfs, const char *fn) {
    rd_file_t    *f;

    (void)vfs;

    mutex_lock_scoped(&rd_mutex);

    /* Find the file */
    f = ramdisk_find_path(rootdir, fn, false);

    /* No entry found to unlink */
    if(!f) {
        errno = ENOENT;
        return -1;
    }

    /* Make sure it's not in use */
    if(f->usage) {
        errno = EBUSY;
        return -1;
    }

    /* Free its data */
    free(f->name);
    free(f->data);

    /* Remove it from the parent list */
    LIST_REMOVE(f, dirlist);

    /* Free the entry itself */
    free(f);

    return 0;
}

static void *ramdisk_mmap(void *h) {
    rd_fd_t     *fd = h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid or a dir */
    if(ramdisk_fd_invalid(fd) || fd->dir) return NULL;

    return fd->file->data;
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
        st->st_dev = rd_dev;
        st->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        st->st_size = -1;
        st->st_nlink = 2;

        return 0;
    }

    mutex_lock_scoped(&rd_mutex);

    /* Find the file */
    f = ramdisk_find_path(rootdir, path, false);
    if(!f) {
        errno = ENOENT;
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_dev = rd_dev;
    st->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    st->st_mode |= (f->isdir) ?
        (S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH) : S_IFREG;
    st->st_size = (f->isdir) ? -1 : (int)f->datasize;
    st->st_nlink = (f->isdir) ? 2 : 1;
    st->st_blksize = rd_blksize;
    st->st_blocks = __align_up(f->datasize, rd_blksize) / rd_blksize;

    return 0;
}

static int ramdisk_fcntl(void *h, int cmd, va_list ap) {
    rd_fd_t     *fd = h;

    (void)ap;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid */
    if(ramdisk_fd_invalid(fd)) {
        errno = EBADF;
        return -1;
    }

    switch(cmd) {
        case F_GETFL:
            return fd->omode;

        case F_SETFL:
        case F_GETFD:
        case F_SETFD:
            return 0;

        default:
            errno = EINVAL;
            return -1;
    }
}

static int ramdisk_rewinddir(void *h) {
    rd_fd_t     *fd = h;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid or NOT a dir */
    if(ramdisk_fd_invalid(fd) || !fd->dir) {
        errno = EBADF;
        return -1;
    }

    /* Rewind to the first file. */
    fd->ptr = (uint32_t)LIST_FIRST((rd_dir_t *)fd->file->data);

    return 0;
}

static int ramdisk_fstat(void *h, struct stat *st) {
    rd_fd_t     *fd = h;
    rd_file_t *f;

    mutex_lock_scoped(&rd_mutex);

    /* Check that the fd is invalid */
    if(ramdisk_fd_invalid(fd)) {
        errno = EBADF;
        return -1;
    }

    /* Grab the file itself... */
    f = fd->file;

    /* Fill in the structure. */
    memset(st, 0, sizeof(struct stat));
    st->st_dev = rd_dev;
    st->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    st->st_mode |= (f->isdir) ? S_IFDIR : S_IFREG;
    st->st_size = (f->isdir) ? -1 : (int)f->datasize;
    st->st_nlink = (f->isdir) ? 2 : 1;
    st->st_blksize = rd_blksize;
    st->st_blocks = __align_up(f->datasize, rd_blksize) / rd_blksize;

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
int fs_ramdisk_attach(const char *fn, void *obj, size_t size) {
    rd_fd_t     *fd;
    rd_file_t   *f;

    /* First of all, open a file for writing. This'll save us a bunch
       of duplicated code. */
    fd = ramdisk_open(&vh, fn, O_WRONLY | O_TRUNC);

    if(fd == NULL)
        return -1;

    /* Ditch the data block we had and replace it with the user one. */
    f = fd->file;
    free(f->data);
    f->data = obj;
    f->datasize = size;
    f->size = size;

    /* Close the file */
    ramdisk_close(fd);

    return 0;
}

/* Does the opposite of attach. This again piggybacks on open. */
int fs_ramdisk_detach(const char *fn, void **obj, size_t *size) {
    rd_fd_t     *fd;
    rd_file_t   *f;

    /* First of all, open a file for reading. This'll save us a bunch
       of duplicated code. */
    fd = ramdisk_open(&vh, fn, O_RDONLY);

    if(fd == NULL)
        return -1;

    /* Pull the data block and put it in the user parameters. */
    assert(obj != NULL);
    assert(size != NULL);

    f = fd->file;
    *obj = f->data;
    *size = f->size;

    /* Ditch the data block we had and replace it with a fake one. */
    f->data = NULL;
    f->datasize = 0;
    f->size = 0;

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
    root->isdir = true;
    root->openfor = OPENFOR_NOTHING;
    root->usage = 0;
    root->data = rootdir;
    root->datasize = 0;

    LIST_INIT(rootdir);

    /* Init the list of file descriptors */
    TAILQ_INIT(&rd_fd_queue);

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
    LIST_FOREACH_SAFE(f1, rootdir, dirlist, f2) {
        LIST_REMOVE(f1, dirlist);
        free(f1->name);
        free(f1->data);
        free(f1);
    }

    free(rootdir);
    free(root->name);
    free(root);

    mutex_destroy(&rd_mutex);
    nmmgr_handler_remove(&vh.nmmgr);
}
