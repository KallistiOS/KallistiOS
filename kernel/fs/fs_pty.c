/* KallistiOS ##version##

   fs_pty.c
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2012, 2014, 2016 Lawrence Sebald

*/

/*

This module implements a pseudo-terminal filesystem (like Linux's /dev/pty).
When you want a pty, call fs_pty_create(x). This will generate two file entries
in the VFS -- /pty/maXX and /pty/slXX. This is more or less like a dry
copper loop in the phone system. Anyone can open up the master or slave ends
and start talking, and it comes out the other end.

A small amount of buffering is done on each pty. If O_NONBLOCK is set, then we
return -1 and set errno to EAGAIN when the buffers are full (or when there is
nothing to read). If O_NONBLOCK is not set (normal) then the caller blocks until
space or data is available (respectively). Like Unix sockets, the returned
data may be less than the requested data if there is not enough data
or space present.

*/

#include <kos/dbgio.h>
#include <kos/thread.h>
#include <kos/mutex.h>
#include <kos/cond.h>
#include <kos/fs_pty.h>
#include <sys/queue.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/ioctl.h>

/* pty buffer size */
#define PTY_BUFFER_SIZE 1024

/* Forward-declare some stuff */
struct ptyhalf;
typedef LIST_HEAD(ptylist, ptyhalf) ptylist_t;

/* This struct represents one half of a pty. Each end is openable as a
   separate file. */
typedef struct ptyhalf {
    LIST_ENTRY(ptyhalf) list;

    struct ptyhalf * other;         /* Other end of the pipe */
    int master;             /* Non-zero if we are master */

    uint8   buffer[PTY_BUFFER_SIZE];    /* Our _receive_ buffer */
    int head, tail;         /* Insert at head, remove at tail */
    size_t cnt;             /* Byte count in the queue */

    int refcnt;             /* When this reaches zero, we close */

    int id;

    mutex_t     mutex;
    condvar_t   ready_read, ready_write;
} ptyhalf_t;

/* Our global pty list */
static ptylist_t ptys;
static int pty_id_highest;
static mutex_t list_mutex;

/* This struct is used for traversing the directory listing */
typedef struct diritem {
    char    name[32];
    int size;
} diritem_t;

typedef struct dirlist {
    diritem_t   * items;
    int     cnt;
    int     ptr;

    dirent_t    dirent;

    mutex_t     mutex;
} dirlist_t;

/* We'll have one of these for each opened pipe */
typedef struct pipefd {
    /* Our directory or pty */
    union {
        ptyhalf_t   * p;
        dirlist_t   * d;
    } d;

    int type;       /* 0 = ptyhalf, 1 = dirlist */

    /* Opened mode */
    int mode;
} pipefd_t;

/* Here incase fs_pty_create() fails */
static void pty_destroy_unused(void);

#define PF_PTY  0
#define PF_DIR  1

/* Creates a pty pair */
int fs_pty_create(char *buffer, int maxbuflen, file_t *master_out, file_t *slave_out) {
    ptyhalf_t *master, *slave;
    int boot;
    char mname[16], sname[16];

    (void)buffer;
    (void)maxbuflen;

    /* Check basics */
    if(!master_out || !slave_out)
        return -1;

    /* Initialize outputs to invalid values */
    *master_out = -1;
    *slave_out = -1;

    /* Are we bootstrapping? */
    boot = LIST_EMPTY(&ptys);

    /* Alloc new structs */
    master = calloc(1, sizeof(ptyhalf_t));
    if(!master) {
        errno = ENOMEM;
        return -1;
    }

    slave = calloc(1, sizeof(ptyhalf_t));
    if(!slave) {
        free(master);
        errno = ENOMEM;
        return -1;
    }

    /* Hook 'em up */
    master->other = slave;
    master->master = 1;
    slave->other = master;
    slave->master = 0;

    /* Reset their queue pointers */
    master->head = master->tail = 0;
    slave->head = slave->tail = 0;
    master->cnt = slave->cnt = 0;

    /* Reset their refcnts (these will get increased in a minute) */
    master->refcnt = slave->refcnt = 0;

    /* Allocate a mutex for each for multiple readers or writers */
    mutex_init(&master->mutex, MUTEX_TYPE_NORMAL);
    cond_init(&master->ready_read);
    cond_init(&master->ready_write);
    mutex_init(&slave->mutex, MUTEX_TYPE_NORMAL);
    cond_init(&slave->ready_read);
    cond_init(&slave->ready_write);

    /* Add it to the list */
    mutex_lock(&list_mutex);
    master->id = ++pty_id_highest;
    slave->id = master->id;
    LIST_INSERT_HEAD(&ptys, master, list);
    LIST_INSERT_HEAD(&ptys, slave, list);
    mutex_unlock(&list_mutex);

    /* Call back up to fs to open two file descriptors */
    sprintf(mname, "/pty/ma%02x", master->id);
    sprintf(sname, "/pty/sl%02x", slave->id);
    *slave_out = fs_open(sname, O_RDWR);
    if(*slave_out < 0)
        goto cleanup;

    if(boot) {
        /* Get the slave channel setup first, and dup it across
           our stdout and stderr. */
        fs_dup2(*slave_out, STDOUT_FILENO);
        fs_dup2(*slave_out, STDERR_FILENO);
    }

    *master_out = fs_open(mname, O_RDWR);
    if(*master_out < 0)
        goto cleanup;

    return 0;

cleanup:
    
    if(*slave_out > 0)
        fs_close(*slave_out);
    else
        pty_destroy_unused();

    return -1;
}

/* Autoclean totally unreferenced PTYs (zero refcnt). */
/* XXX This is a kinda nasty piece of code... goto!! */
static void pty_destroy_unused(void) {
    ptyhalf_t *c, *n;
    int old;

    /* Make sure no one else is messing with the list and then disable
       everything for a bit */
    mutex_lock_irqsafe(&list_mutex);

    old = irq_disable();

again:
    c = LIST_FIRST(&ptys);

    while(c) {
        n = LIST_NEXT(c, list);

        /* Don't mess with the kernel console or locked items */
        if((c->id != 0) && (!mutex_is_locked(&c->mutex))) {

            /* Make sure neither is in use */
            if((c->refcnt <= 0) && (c->other->refcnt <= 0)) {

                /* Free all our structs */
                cond_destroy(&c->ready_read);
                cond_destroy(&c->ready_write);
                mutex_destroy(&c->mutex);

                /* Remove us from the list */
                LIST_REMOVE(c, list);

                /* Now to deal with our partner... */
                cond_destroy(&c->other->ready_read);
                cond_destroy(&c->other->ready_write);
                mutex_destroy(&c->other->mutex);

                /* Remove it from the list */
                LIST_REMOVE(c->other, list);

                /* Free the structs */
                free(c->other);
                free(c);

                /* Need to restart */
                goto again;
            }
        }
        c = n;
    }

    irq_restore(old);
    mutex_unlock(&list_mutex);
}

static void * pty_open_dir(const char * fn, int mode) {
    ptyhalf_t   * ph;
    dirlist_t   * dl;
    int     cnt;
    pipefd_t    * fdobj = NULL;

    (void)fn;

    mutex_lock_scoped(&list_mutex);

    /* Go through and count the number of items */
    cnt = 0;
    LIST_FOREACH(ph, &ptys, list) {
        cnt++;
    }

    /* Allow a dir struct */
    dl = malloc(sizeof(dirlist_t));

    if(!dl) {
        errno = ENOMEM;
        return NULL;
    }

    memset(dl, 0, sizeof(dirlist_t));
    dl->items = malloc(sizeof(diritem_t) * cnt);

    if(!dl->items) {
        free(dl);
        errno = ENOMEM;
        return NULL;
    }

    memset(dl->items, 0, sizeof(diritem_t) * cnt);
    dl->cnt = cnt;
    dl->ptr = 0;

    /* Now fill it in */
    cnt = 0;
    LIST_FOREACH(ph, &ptys, list) {
        if(ph->master)
            sprintf(dl->items[cnt].name, "ma%02x", ph->id);
        else
            sprintf(dl->items[cnt].name, "sl%02x", ph->id);

        dl->items[cnt].size = ph->cnt;
        cnt++;
    }

    /* Now return that as our handle item */
    fdobj = malloc(sizeof(pipefd_t));

    if(fdobj == NULL) {
        free(dl->items);
        free(dl);
        errno = ENOMEM;
        return NULL;
    }

    memset(fdobj, 0, sizeof(pipefd_t));
    fdobj->d.d = dl;
    fdobj->type = PF_DIR;
    fdobj->mode = mode;

    return (void *)fdobj;
}

static void * pty_open_file(const char * fn, int mode) {
    /* Ok, they want an actual pty. We always give them one RDWR, no
       matter what is asked for (it's much simpler). Also we don't have to
       handle fds of our own here thanks to the VFS layer, just reference
       counting so a pty can be opened by more than one process. */
    int     master;
    int     id;
    ptyhalf_t   * ph;
    pipefd_t    * fdobj;

    /* Parse out the name we got */
    if(strlen(fn) != 4) {
        errno = ENOENT;
        return NULL;
    }

    if(fn[0] == 'm' && fn[1] == 'a')
        master = 1;
    else if(fn[0] == 's' && fn[1] == 'l')
        master = 0;
    else {
        errno = ENOENT;
        return NULL;
    }

    id = strtol(fn + 2, NULL, 16);

    /* Do we have that pty? */
    mutex_lock(&list_mutex);
    LIST_FOREACH(ph, &ptys, list) {
        if(ph->id == id)
            break;
    }
    mutex_unlock(&list_mutex);

    if(!ph) {
        errno = ENOENT;
        return NULL;
    }

    /* Which one did we get? If we got the wrong one, swap. */
    if(!ph->master) {
        if(master)
            ph = ph->other;
    }
    else {
        if(!master)
            ph = ph->other;
    }

    fdobj = malloc(sizeof(pipefd_t));

    if(fdobj == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    memset(fdobj, 0, sizeof(pipefd_t));

    /* Now add a refcnt and return it */
    mutex_lock(&ph->mutex);
    ph->refcnt++;
    mutex_unlock(&ph->mutex);

    fdobj->d.p = ph;
    fdobj->type = PF_PTY;
    fdobj->mode = mode;
    return (void *)fdobj;
}

static void * pty_open(vfs_handler_t * vfs, const char * fn, int mode) {
    (void)vfs;

    /* Skip any preceding slash */
    if(*fn == '/') fn++;

    /* Are they opening the root? As a dir? */
    if(*fn == 0) {
        if(!(mode & O_DIR)) {
            errno = EISDIR;
            return NULL;
        }
        else
            return pty_open_dir(fn, mode);
    }
    else {
        if(mode & O_DIR) {
            errno = ENOTDIR;
            return NULL;
        }
        else
            return pty_open_file(fn, mode);
    }
}

/* Close pty or dirlist */
static int pty_close(void *h) {
    pipefd_t *fdobj;

    assert(h);
    fdobj = (pipefd_t *)h;

    if(fdobj->type == PF_PTY) {
        /* De-ref this end of it */
        mutex_lock_irqsafe(&fdobj->d.p->mutex);

        fdobj->d.p->refcnt--;

        if(fdobj->d.p->refcnt <= 0) {
            /* Unblock anyone who might be waiting on the other end */
            cond_broadcast(&fdobj->d.p->other->ready_read);
            cond_broadcast(&fdobj->d.p->ready_write);
        }

        mutex_unlock(&fdobj->d.p->mutex);

        pty_destroy_unused();
    }
    else {
        free(fdobj->d.d->items);
        free(fdobj->d.d);
    }

    free(fdobj);
    return 0;
}

/* Read from a pty endpoint, kernel console special case */
static ssize_t pty_read_serial(pipefd_t * fdobj, ptyhalf_t * ph, void * buf, size_t bytes) {
    int c, r = 0;

    (void)ph;

    while(bytes > 0) {
    again:
        /* Try to read a char */
        c = dbgio_read();

        /* Get anything? */
        if(c == -1) {
            /* If we are in non-block, we give up now */
            if(fdobj->mode & O_NONBLOCK) {
                if(r == 0) {
                    errno = EAGAIN;
                    r = -1;
                }

                break;
            }

            /* Have we read anything at all? */
            if(r == 0) {
                /* Nope -- sleep a bit and try again */
                thd_sleep(10);
                goto again;
            }
            else
                /* Yep -- that's enough */
                break;
        }

        /* Add the obtained char to the buffer and echo it */
        ((uint8 *)buf)[r] = c;
        dbgio_write(c);
        r++;
    }

    /* Flush any remaining echoed chars */
    dbgio_flush();

    /* Return the number we got */
    return r;
}

/* Read from a pty endpoint */
static ssize_t pty_read(void * h, void * buf, size_t bytes) {
    size_t avail;
    pipefd_t *fdobj;
    ptyhalf_t *ph;

    fdobj = (pipefd_t *)h;
    ph = fdobj->d.p;

    if(fdobj->type != PF_PTY) {
        errno = EINVAL;
        return -1;
    }

    /* Special case the unattached console */
    if(ph->id == 0 && !ph->master && ph->other->refcnt == 0)
        return pty_read_serial(fdobj, ph, buf, bytes);

    /* Lock the ptyhalf */
    mutex_lock(&ph->mutex);

    /* Is there anything to read? */
    while(!ph->cnt && ph->other->refcnt > 0) {
        /* If we're in non-block, give up now */
        if(fdobj->mode & O_NONBLOCK) {
            errno = EAGAIN;
            bytes = -1;
            goto done;
        }

        cond_wait(&ph->ready_read, &ph->mutex);
    }

    /* If the buffer is empty and the other end is closed, return 0 */
    if(!ph->cnt && ph->other->refcnt == 0) {
        bytes = 0;
        goto done;
    }

    /* Figure out how much to read */
    avail = ph->cnt;

    if(avail < bytes)
        bytes = avail;

    /* Copy out the data and remove it from the buffer */
    if((ph->head + bytes) > PTY_BUFFER_SIZE) {
        avail = PTY_BUFFER_SIZE - ph->head;
        memcpy(buf, ph->buffer + ph->head, avail);
        memcpy(((uint8 *)buf) + avail, ph->buffer, bytes - avail);
    }
    else
        memcpy(buf, ph->buffer + ph->head, bytes);

    ph->head = (ph->head + bytes) % PTY_BUFFER_SIZE;
    ph->cnt -= bytes;

    /* Wake anyone waiting for write space */
    cond_broadcast(&ph->ready_write);

done:
    mutex_unlock(&ph->mutex);
    return bytes;
}

/* Write to a pty endpoint */
static ssize_t pty_write(void * h, const void * buf, size_t bytes) {
    size_t avail;
    pipefd_t *fdobj;
    ptyhalf_t *ph;

    fdobj = (pipefd_t *)h;
    ph = fdobj->d.p;

    if(fdobj->type != PF_PTY) {
        errno = EINVAL;
        return -1;
    }

    /* Special case the unattached console */
    if(ph->id == 0 && !ph->master && ph->other->refcnt == 0) {
        /* This actually blocks, but fooey.. :) */
        dbgio_write_buffer_xlat((const uint8 *)buf, bytes);
        return bytes;
    }

    /* Get the other end of the pipe */
    ph = ph->other;
    assert(ph);

    mutex_lock(&ph->mutex);

    /* Is there any room to write? */
    while(ph->cnt >= PTY_BUFFER_SIZE && ph->refcnt > 0) {
        /* If we're in non-block, give up now */
        if(fdobj->mode & O_NONBLOCK) {
            errno = EAGAIN;
            bytes = -1;
            goto done;
        }

        cond_wait(&ph->ready_write, &ph->mutex);
    }

    /* If the buffer is full and the other end is closed, return 0 */
    if(ph->cnt >= PTY_BUFFER_SIZE && ph->refcnt == 0) {
        bytes = 0;
        goto done;
    }

    /* Figure out how much to write */
    avail = PTY_BUFFER_SIZE - ph->cnt;
    if(avail < bytes)
        bytes = avail;

    /* Copy in the data and add it from the buffer */
    if((ph->tail + bytes) > PTY_BUFFER_SIZE) {
        avail = PTY_BUFFER_SIZE - ph->tail;
        memcpy(ph->buffer + ph->tail, buf, avail);
        memcpy(ph->buffer, ((const uint8 *)buf) + avail, bytes - avail);
    }
    else
        memcpy(ph->buffer + ph->tail, buf, bytes);

    ph->tail = (ph->tail + bytes) % PTY_BUFFER_SIZE;
    ph->cnt += bytes;
    assert(ph->cnt <= PTY_BUFFER_SIZE);

    /* Wake anyone waiting on read */
    cond_broadcast(&ph->ready_read);

done:
    mutex_unlock(&ph->mutex);
    return bytes;
}

/* Get total size. For this we return the number of bytes available for reading. */
static size_t pty_total(void * h) {
    pipefd_t    * fdobj;
    ptyhalf_t   * ph;

    fdobj = (pipefd_t *)h;
    ph = fdobj->d.p;

    if(fdobj->type != PF_PTY) {
        errno = EINVAL;
        return -1;
    }

    return ph->cnt;
}

/* Read a directory entry */
static dirent_t * pty_readdir(void * h) {
    pipefd_t * fdobj = (pipefd_t *)h;
    dirlist_t * dl;

    assert(h);

    if(fdobj->type != PF_DIR) {
        errno = EBADF;
        return NULL;
    }

    dl = fdobj->d.d;

    if(dl->ptr >= dl->cnt) {
        return NULL;
    }

    dl->dirent.size = dl->items[dl->ptr].size;
    strcpy(dl->dirent.name, dl->items[dl->ptr].name);
    dl->dirent.time = 0;
    dl->dirent.attr = STAT_ATTR_RW;
    dl->ptr++;

    return &dl->dirent;
}

static int pty_stat(vfs_handler_t *vfs, const char *path, struct stat *st,
                    int flag) {
    ptyhalf_t *ph;
    int id;
    int master;
    size_t len = strlen(path);

    (void)vfs;
    (void)flag;

    /* Root directory '/pty' */
    if(len == 0 || (len == 1 && *path == '/')) {
        memset(st, 0, sizeof(struct stat));
        st->st_dev = (dev_t)('p' | ('t' << 8) | ('y' << 16));
        st->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        st->st_size = -1;
        st->st_nlink = 2;

        return 0;
    }

    /* Handle paths that start directly with '/maXX' or '/slXX' */
    if(sscanf(path, "/%2c%02x", (char[3]){}, &id) != 2) {
        errno = ENOENT;
        return -1;
    }

    /* Check if it's a master (ma) or slave (sl) */
    master = (path[0] == 'm' && path[1] == 'a');

    /* Find the corresponding PTY half */
    mutex_lock(&list_mutex);
    LIST_FOREACH(ph, &ptys, list) {
        if(ph->id == id) break;
    }
    mutex_unlock(&list_mutex);

    /* If PTY is not found, return error */
    if(!ph) {
        errno = ENOENT;
        return -1;
    }

    /* If the file requested doesn't match the master/slave role, switch */
    if(master != ph->master) {
        ph = ph->other;
    }

    /* Fill in the stat structure */
    memset(st, 0, sizeof(struct stat));
    st->st_dev = (dev_t)('p' | ('t' << 8) | ('y' << 16));
    st->st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    st->st_nlink = 1;
    st->st_size = ph->cnt;
    st->st_blksize = PTY_BUFFER_SIZE;

    return 0;
}

static int pty_fcntl(void *h, int cmd, va_list ap) {
    pipefd_t *fd = (pipefd_t *)h;
    int rv = -1;
    long val;

    if(!fd) {
        errno = EBADF;
        return -1;
    }

    switch(cmd) {
        case F_GETFL:
            rv = fd->mode;
            break;

        case F_SETFL:
            val = va_arg(ap, long);

            if(val & O_NONBLOCK)
                fd->mode |= O_NONBLOCK;
            else
                fd->mode &= ~O_NONBLOCK;

            rv = 0;
            break;

        case F_GETFD:
        case F_SETFD:
            rv = 0;
            break;

        default:
            errno = EINVAL;
    }

    return rv;
}

static int pty_rewinddir(void *h) {
    pipefd_t *fdobj = (pipefd_t *)h;
    dirlist_t *dl;

    assert(h);

    if(fdobj->type != PF_DIR) {
        errno = EBADF;
        return -1;
    }

    dl = fdobj->d.d;
    dl->ptr = 0;
    return 0;
}

static int pty_fstat(void *h, struct stat *st) {
    pipefd_t *fd = (pipefd_t *)h;

    if(!fd) {
        errno = EBADF;
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_dev = (dev_t)('p' | ('t' << 8) | ('y' << 16));
    st->st_mode = (fd->mode & O_DIR) ? 
        (S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO) : 
        (S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    st->st_size = (fd->mode & O_DIR) ? -1 : (off_t)fd->d.p->cnt;
    st->st_blksize = (fd->mode & O_DIR) ? 0 : 1;

    return 0;
}

static vfs_handler_t vh = {
    /* Name Handler */
    {
        { "/pty" },     /* name */
        0,              /* in-kernel */
        0x00010000,     /* Version 1.0 */
        0,              /* flags */
        NMMGR_TYPE_VFS, /* VFS handler */
        NMMGR_LIST_INIT /* list */
    },

    0, NULL,            /* no caching, privdata */

    pty_open,
    pty_close,
    pty_read,
    pty_write,
    NULL,
    NULL,
    pty_total,
    pty_readdir,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    pty_stat,
    NULL,
    NULL,
    pty_fcntl,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    pty_rewinddir,
    pty_fstat
};

/* Are we initialized? */
static int initted = 0;

/* Initialize the file system */
int fs_pty_init(void) {
    int cm, cs;
    int tm, ts;

    if(initted)
        return 0;

    /* Init our list of ptys */
    LIST_INIT(&ptys);
    pty_id_highest = -1;

    if(nmmgr_handler_add(&vh.nmmgr) < 0)
        return -1;

    mutex_init(&list_mutex, MUTEX_TYPE_NORMAL);
    initted = 1;

    /* Start out with a console pty */
    fs_pty_create(NULL, 0, &cm, &cs);

    /* Close the master end, we want dbgio by default */
    fs_close(cm);

    fs_pty_create(NULL, 0, &tm, &ts);
    fs_close(tm);
    fs_close(ts);

    return 0;
}

/* De-init the file system */
int fs_pty_shutdown(void) {
    ptyhalf_t *n, *c;

    if(!initted)
        return 0;

    mutex_lock_irqsafe(&list_mutex);

    /* Go through and free all the pty entries */
    c = LIST_FIRST(&ptys);

    while(c != NULL) {
        n = LIST_NEXT(c, list);

        cond_destroy(&c->ready_read);
        cond_destroy(&c->ready_write);
        mutex_destroy(&c->mutex);
        free(c);

        c = n;
    }

    nmmgr_handler_remove(&vh.nmmgr);

    mutex_destroy(&list_mutex);

    initted = 0;

    return 0;
}
