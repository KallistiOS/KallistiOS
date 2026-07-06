/* KallistiOS ##version##

   kernel/arch/dreamcast/fs/fs_dclsocket.c
   Copyright (C) 2007, 2008, 2012, 2013, 2015 Lawrence Sebald

   Based on fs_dclnative.c and related files
   Copyright (C) 2003 Megan Potter

   Portions of various supporting modules are
   Copyright (C) 2001 Andrew Kieschnick, imported
   from the GPL'd dc-load-ip sources to a BSD-compatible
   license with permission.

*/

/* This file is basically a rewrite of the old fs_dclnative that uses KOS'
   internal sockets library. This fs module is basically designed for debugging
   networked code with dcload-ip, rather than requiring a serial cable and
   dcload-serial. */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include <kos/irq.h>
#include <kos/mutex.h>
#include <kos/fs.h>
#include <kos/net.h>
#include <kos/dbgio.h>
#include <kos/dbglog.h>
#include <kos/regfield.h>

#include <dc/fs_dclsocket.h>
#include <dc/fs_dcload.h>
#include <dc/dcload.h>

#define DCLOAD_PORT 53535
#define DCLOAD_PACKET_SIZE 1440

/* Number of DCLOAD_PACKET_SIZE chunks needed to cover n bytes. */
#define DCLOAD_PACKETS(n)  (((n) + DCLOAD_PACKET_SIZE - 1) / DCLOAD_PACKET_SIZE)

/* PARTBIN receive map: one bit per chunk, sized to cover 16 MB of RAM. */
#define DCLOAD_MAP_ENTRIES DCLOAD_PACKETS(16 * 1024 * 1024)
#define DCLOAD_MAP_BYTES   (((DCLOAD_MAP_ENTRIES) + 7) / 8)


typedef struct {
    unsigned char id[4];
    unsigned int address;
    unsigned int size;
    unsigned char data[];
} command_t;

typedef struct {
    unsigned char id[4];
    unsigned int value0;
} command_int_t;

typedef struct {
    unsigned char id[4];
    unsigned int value0;
    unsigned int value1;
    unsigned int value2;
} command_3int_t;

static struct {
    unsigned int addr;
    unsigned int size;
    unsigned char map[DCLOAD_MAP_BYTES];
} bin_info;

typedef struct dcl_obj {
    int hnd;
    char *path;
    dirent_t dirent;
} dcl_obj_t;

extern int dcload_type;
static int initted = 0;
static bool escape = false;
static int retval = 0;
static mutex_t mutex;
static uint8_t pktbuf[DCLOAD_PACKET_SIZE + sizeof(command_t)];

static int dcls_socket = -1;

/* Mark / test receipt of chunk `index` in the PARTBIN map. */
static inline void bin_map_set(unsigned int index) {
    bin_info.map[index >> 3] |= BIT(index & 7);
}

static inline int bin_map_test(unsigned int index) {
    return bin_info.map[index >> 3] & BIT(index & 7);
}

static void dcls_handle_lbin(command_t *cmd) {
    bin_info.addr = ntohl(cmd->address);
    bin_info.size = ntohl(cmd->size);
    memset(bin_info.map, 0, DCLOAD_MAP_BYTES);

    send(dcls_socket, cmd, sizeof(command_t), 0);
}

static void dcls_handle_pbin(command_t *cmd) {
    int index = (ntohl(cmd->address) - bin_info.addr) / DCLOAD_PACKET_SIZE;

    memcpy((uint8_t *)ntohl(cmd->address), cmd->data, ntohl(cmd->size));

    if(index >= 0 && index < DCLOAD_MAP_ENTRIES)
        bin_map_set(index);
}

static void dcls_handle_dbin(command_t *cmd) {
    unsigned int i;

    for(i = 0; i < DCLOAD_PACKETS(bin_info.size); ++i) {
        if(!bin_map_test(i))
            break;
    }

    if(i == DCLOAD_PACKETS(bin_info.size)) {
        cmd->address = 0;
        cmd->size = 0;
    }
    else {
        cmd->address = htonl(bin_info.addr + i * DCLOAD_PACKET_SIZE);

        if(i == DCLOAD_PACKETS(bin_info.size) - 1) {
            cmd->size = htonl(bin_info.size % DCLOAD_PACKET_SIZE);
        }
        else {
            cmd->size = htonl(DCLOAD_PACKET_SIZE);
        }
    }

    send(dcls_socket, cmd, sizeof(command_t), 0);
}

static void dcls_handle_sbin(command_t *cmd) {
    uint32_t left, size;
    uint8_t *ptr;
    int count, i;
    command_t *resp = (command_t *)pktbuf;

    left = ntohl(cmd->size);
    ptr = (uint8_t *)ntohl(cmd->address);
    count = DCLOAD_PACKETS(left);

    memcpy(resp->id, "SBIN", 4);

    for(i = 0; i < count; ++i) {
        size = left > DCLOAD_PACKET_SIZE ? DCLOAD_PACKET_SIZE : left;
        left -= size;

        resp->address = htonl((uint32_t)ptr);
        resp->size = htonl(size);
        memcpy(resp->data, ptr, size);

        send(dcls_socket, resp, sizeof(command_t) + size, 0);
        ptr += size;
    }

    memcpy(resp->id, "DBIN", 4);

    resp->address = 0;
    resp->size = 0;
    send(dcls_socket, resp, sizeof(command_t), 0);
}

static void dcls_handle_retv(command_t *cmd) {
    send(dcls_socket, cmd, sizeof(command_t), 0);
    retval = ntohl(cmd->address);
    escape = true;
}

static void dcls_recv_loop(void) {
    uint8_t pkt[1514];
    command_t *cmd = (command_t *)pkt;

    while(!escape) {
        /* If we're in an interrupt, this works differently.... */
        if(irq_inside_int()) {
            if(!net_default_dev)
                break;
            /* Since we can't count on an interrupt happening, handle it
               manually, and poll the default device... */
            net_default_dev->if_rx_poll(net_default_dev);

            if(recv(dcls_socket, pkt, 1514, 0) == -1)
                continue;
        }
        else if(recv(dcls_socket, pkt, 1514, 0) == -1) {
            break;
        }

        if(!memcmp(cmd->id, "RETV", 4)) {
            dcls_handle_retv(cmd);
        }
        else if(!memcmp(cmd->id, "SBIN", 4) || !memcmp(cmd->id, "SBIQ", 4)) {
            dcls_handle_sbin(cmd);
        }
        else if(!memcmp(cmd->id, "LBIN", 4)) {
            dcls_handle_lbin(cmd);
        }
        else if(!memcmp(cmd->id, "PBIN", 4)) {
            dcls_handle_pbin(cmd);
        }
        else if(!memcmp(cmd->id, "DBIN", 4)) {
            dcls_handle_dbin(cmd);
        }
    }

    escape = false;
}

static void *dcls_open(struct vfs_handler *vfs, const char *fn, int mode) {
    int hnd, dcload_mode = 0;
    int mm = (mode & O_MODE_MASK);
    dcl_obj_t *entry;
    command_t *cmd = (command_t *)pktbuf;

    (void)vfs;

    entry = calloc(1, sizeof(dcl_obj_t));
    if(!entry) {
        errno = ENOMEM;
        return NULL;
    }

    if(mutex_lock_irqsafe(&mutex)) {
        free(entry);
        return NULL;
    }

    if(mode & O_DIR) {
        char realfn[fn[0] ? strlen(fn) + 1 : 2];

        if(fn[0] == '\0') {
            strcpy(realfn, "/");
        }
        else {
            strcpy(realfn, fn);
        }

        memcpy(pktbuf, "DC16", 4);
        strcpy((char *)(pktbuf + 4), realfn);

        send(dcls_socket, pktbuf, 5 + strlen(realfn), 0);

        dcls_recv_loop();
        hnd = retval;

        if(!hnd)                        /* opendir failed */
            goto fail;

        entry->hnd = hnd;

        /* Keep a trailing-slash copy of the path for readdir(). */
        size_t plen = strlen(realfn);
        int add_slash = realfn[plen - 1] != '/';

        entry->path = malloc(plen + add_slash + 1);
        if(!entry->path) {
            errno = ENOMEM;
            goto fail;
        }

        strcpy(entry->path, realfn);
        if(add_slash)
            strcat(entry->path, "/");
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

        memcpy(cmd->id, "DC04", 4);
        cmd->address = htonl(dcload_mode); /* Open flags */
        cmd->size = htonl(0644);           /* umask */
        strcpy((char *)cmd->data, fn);

        send(dcls_socket, pktbuf, sizeof(command_t) + strlen(fn) + 1, 0);
        dcls_recv_loop();

        if(retval < 0)                  /* open failed */
            goto fail;

        entry->hnd = retval;
    }

    mutex_unlock(&mutex);

    return entry;

fail:
    mutex_unlock(&mutex);
    free(entry);
    return NULL;
}

static int dcls_close(void *hnd) {
    dcl_obj_t *obj = hnd;
    command_int_t *cmd = (command_int_t *)pktbuf;

    if(!obj)
        return 0;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    if(obj->path) {
        memcpy(cmd->id, "DC17", 4);
        free(obj->path);
    }
    else {
        memcpy(cmd->id, "DC05", 4);
    }

    cmd->value0 = htonl(obj->hnd);

    send(dcls_socket, cmd, sizeof(command_int_t), 0);
    dcls_recv_loop();

    free(obj);

    mutex_unlock(&mutex);
    return 0;
}

static ssize_t dcls_read(void *hnd, void *buf, size_t cnt) {
    dcl_obj_t *obj = hnd;
    command_3int_t *cmd = (command_3int_t *)pktbuf;

    if(!obj)
        return -1;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DC03", 4);
    cmd->value0 = htonl(obj->hnd);
    cmd->value1 = htonl((uint32_t) buf);
    cmd->value2 = htonl((uint32_t) cnt);

    send(dcls_socket, cmd, sizeof(command_3int_t), 0);
    dcls_recv_loop();

    mutex_unlock(&mutex);

    return retval;
}

static ssize_t dcls_write(void *hnd, const void *buf, size_t cnt) {
    dcl_obj_t *obj = hnd;
    command_3int_t *cmd = (command_3int_t *)pktbuf;

    if(!obj)
        return -1;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DD02", 4);
    cmd->value0 = htonl(obj->hnd);
    cmd->value1 = htonl((uint32_t) buf);
    cmd->value2 = htonl(cnt);

    send(dcls_socket, cmd, sizeof(command_3int_t), 0);
    dcls_recv_loop();

    mutex_unlock(&mutex);

    return retval;
}

static off_t dcls_seek(void *hnd, off_t offset, int whence) {
    dcl_obj_t *obj = hnd;
    command_3int_t *command = (command_3int_t *)pktbuf;

    if(!obj)
        return -1;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(command->id, "DC11", 4);
    command->value0 = htonl(obj->hnd);
    command->value1 = htonl((uint32_t)offset);
    command->value2 = htonl((uint32_t)whence);

    send(dcls_socket, command, sizeof(command_3int_t), 0);
    dcls_recv_loop();

    mutex_unlock(&mutex);

    return retval;
}

static off_t dcls_tell(void *hnd) {
    return dcls_seek(hnd, 0, SEEK_CUR);
}

static size_t dcls_total(void *hnd) {
    size_t cur, ret;

    cur = dcls_tell(hnd);
    ret = dcls_seek(hnd, 0, SEEK_END);
    dcls_seek(hnd, cur, SEEK_SET);

    return ret;
}

/* The host fills the readdir reply as a POSIX struct dirent, not a KOS
   dirent_t, so receive it as one */
static union {
    struct dirent de;
    char storage[sizeof(struct dirent) + NAME_MAX + 1];
} our_dir;

static const dirent_t *dcls_readdir(void *hnd) {
    dcl_obj_t *entry = hnd;
    command_3int_t *cmd = (command_3int_t *)pktbuf;

    if(!entry || !entry->path) {
        errno = EBADF;
        return NULL;
    }

    if(mutex_lock_irqsafe(&mutex))
        return NULL;

    memcpy(cmd->id, "DC18", 4);
    cmd->value0 = htonl(entry->hnd);
    cmd->value1 = htonl((uint32_t)(&our_dir.de));
    cmd->value2 = htonl(sizeof(our_dir));

    send(dcls_socket, cmd, sizeof(command_3int_t), 0);

    dcls_recv_loop();

    if(retval) {
        command_t *cmd2 = (command_t *)pktbuf;
        dcload_stat_t filestat;
        dirent_t *rv = &entry->dirent;

        /* Verify dcload won't overflow us */
        if(strlen(our_dir.de.d_name) + 1 > NAME_MAX) {
            mutex_unlock(&mutex);
            errno = EOVERFLOW;
            return NULL;
        }

        char fn[strlen(entry->path) + strlen(our_dir.de.d_name) + 1];

        strcpy(rv->name, our_dir.de.d_name);
        rv->size = 0;
        rv->time = 0;
        rv->attr = 0;

        strcpy(fn, entry->path);
        strcat(fn, our_dir.de.d_name);

        memcpy(cmd2->id, "DC13", 4);
        cmd2->address = htonl((uint32_t) &filestat);
        cmd2->size = htonl(sizeof(dcload_stat_t));
        strcpy((char *)cmd2->data, fn);

        send(dcls_socket, cmd2, sizeof(command_t) + strlen(fn) + 1, 0);

        dcls_recv_loop();

        if(!retval) {
            if(filestat.st_mode & S_IFDIR) {
                rv->size = -1;
                rv->attr = O_DIR;
            }
            else {
                rv->size = filestat.st_size;
            }

            rv->time = filestat.mtime;
        }

        mutex_unlock(&mutex);
        return rv;
    }

    mutex_unlock(&mutex);
    return NULL;
}

static int dcls_rename(vfs_handler_t *vfs, const char *fn1, const char *fn2) {
    int len1 = strlen(fn1), len2 = strlen(fn2);

    (void)vfs;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(pktbuf, "DC07", 4);
    strcpy((char *)(pktbuf + 4), fn1);
    strcpy((char *)(pktbuf + 5 + len1), fn2);

    send(dcls_socket, pktbuf, 6 + len1 + len2, 0);

    dcls_recv_loop();

    if(retval == 0) {
        memcpy(pktbuf, "DC08", 4);
        strcpy((char *)(pktbuf + 4), fn1);

        send(dcls_socket, pktbuf, len1 + 5, 0);

        dcls_recv_loop();
    }

    mutex_unlock(&mutex);

    return retval;
}

static int dcls_unlink(vfs_handler_t *vfs, const char *fn) {
    int len = strlen(fn) + 5;

    (void)vfs;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(pktbuf, "DC08", 4);
    strcpy((char *)(pktbuf + 4), fn);

    send(dcls_socket, pktbuf, len, 0);

    dcls_recv_loop();

    mutex_unlock(&mutex);

    return retval;
}

static int dcls_stat(vfs_handler_t *vfs, const char *path, struct stat *st, int flag) {
    command_t *cmd = (command_t *)pktbuf;
    dcload_stat_t filestat = { 0 };
    size_t len = strlen(path);

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

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DC13", 4);
    cmd->address = htonl((uint32_t) &filestat);
    cmd->size = htonl(sizeof(dcload_stat_t));
    strcpy((char *)(cmd->data), path);

    send(dcls_socket, cmd, sizeof(command_t) + strlen(path) + 1, 0);

    dcls_recv_loop();

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

        mutex_unlock(&mutex);
        return 0;
    }

    mutex_unlock(&mutex);
    return -1;
}

static int dcls_rewinddir(void *hnd) {
    dcl_obj_t *obj = hnd;
    command_int_t *cmd = (command_int_t *)pktbuf;

    if(!obj || !obj->path) {
        errno = EBADF;
        return -1;
    }

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DC21", 4);
    cmd->value0 = htonl(obj->hnd);

    send(dcls_socket, cmd, sizeof(command_int_t), 0);

    dcls_recv_loop();

    mutex_unlock(&mutex);

    return retval;
}

/* dbgio interface */
static int dcls_detected(void) {
    return initted > 0;
}

static int dcls_fake_init(void) {
    return 0;
}

static int dcls_fake_shutdown(void) {
    return 0;
}

static int dcls_writebuf(const uint8_t *buf, int len, int xlat) {
    command_3int_t cmd;

    (void)xlat;

    if(initted < 2)
        return -1;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd.id, "DD02", 4);
    cmd.value0 = htonl(1);
    cmd.value1 = htonl((uint32_t) buf);
    cmd.value2 = htonl(len);

    send(dcls_socket, &cmd, sizeof(cmd), 0);

    dcls_recv_loop();

    mutex_unlock(&mutex);

    return retval;
}

static int dcls_fcntl(void *h, int cmd, va_list ap) {
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

/* VFS handler */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/pc",      /* name */
        0,      /* tbfi */
        0x00010000, /* Version 1.0 */
        0,      /* flags */
        NMMGR_TYPE_VFS,
        NMMGR_LIST_INIT
    },

    0, NULL,    /* no cache, privdata */

    dcls_open,
    dcls_close,
    dcls_read,
    dcls_write,
    dcls_seek,
    dcls_tell,
    dcls_total,
    dcls_readdir,
    NULL,               /* ioctl */
    dcls_rename,
    dcls_unlink,
    NULL,               /* mmap */
    NULL,               /* complete */
    dcls_stat,
    NULL,               /* mkdir */
    NULL,               /* rmdir */
    dcls_fcntl,
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    dcls_rewinddir,     /* rewinddir */
    NULL                /* fstat */
};

/* dbgio handler */
dbgio_handler_t dbgio_dcls = {
    .name = "fs_dclsocket",
    .detected = dcls_detected,
    .init = dcls_fake_init,
    .shutdown = dcls_fake_shutdown,
    .write_buffer = dcls_writebuf
};

/* This function must be called prior to calling fs_dclsocket_init() */
void fs_dclsocket_init_console(void) {
    /* Make sure networking is up first of all */
    if(!net_default_dev) {
        return;
    }

    dbgio_dcls.set_irq_usage = dbgio_null.set_irq_usage;
    dbgio_dcls.flush = dbgio_null.flush;
    dbgio_dcls.read_buffer = dbgio_null.read_buffer;

    initted = 1;
}

uint32_t _fs_dclsocket_get_ip(void) {
    uint32_t ip, port;

    return dcload_gethostinfo(&ip, &port);
}

int fs_dclsocket_init(void) {
    struct sockaddr_in addr;
    int err;
    uint8_t ipaddr[4], mac[6];
    uint32_t ip, port;

    /* Make sure we've initted the console */
    if(initted != 1)
        return -1;

    /* Make sure we're actually on dcload-ip */
    if(dcload_type != DCLOAD_TYPE_IP)
        return -1;

    /* Determine where dctool is running, and set up our variables for that */
    dcload_gethostinfo(&ip, &port);

    /* Put dc-tool's info into our ARP cache */
    net_ipv4_parse_address(ip, ipaddr);

    err = net_arp_lookup(net_default_dev, ipaddr, mac, NULL, NULL, 0);

    while(err == -1 || err == -2) {
        err = net_arp_lookup(net_default_dev, ipaddr, mac, NULL, NULL, 0);
    }

    /* Make the entry permanent */
    net_arp_insert(net_default_dev, mac, ipaddr, 0);

    /* Ok, now create our socket, and set it up properly */
    dcls_socket = socket(PF_INET, SOCK_DGRAM, 0);

    if(dcls_socket == -1)
        return -1;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DCLOAD_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    err = bind(dcls_socket, (struct sockaddr *)&addr,
               sizeof(struct sockaddr_in));

    if(err == -1)
        goto error;

    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(ip);

    err = connect(dcls_socket, (struct sockaddr *)&addr,
                  sizeof(struct sockaddr_in));

    if(err == -1)
        goto error;

    if(mutex_init(&mutex, MUTEX_TYPE_NORMAL))
        goto error;

    initted = 2;

    return nmmgr_handler_add(&vh.nmmgr);

error:
    close(dcls_socket);
    return -1;
}

void fs_dclsocket_shutdown(void) {
    int old;
    command_t cmd;

    if(initted != 2)
        return;

    dbglog(DBG_INFO, "fs_dclsocket: About to disable console\n");

    /* Disable the console first of all */
    if(!strcmp(dbgio_dev_get(), "fs_dclsocket"))
        dbgio_disable();

    /* Send dc-tool an exit packet */
    memcpy(cmd.id, "DC00", 4);

    cmd.address = 0;
    cmd.size = 0;

    send(dcls_socket, &cmd, sizeof(command_t), 0);

    old = irq_disable();

    /* Destroy our mutex, and set us as uninitted */
    mutex_destroy(&mutex);
    initted = 0;

    irq_restore(old);

    /* Finally, clean up the socket */
    close(dcls_socket);

    nmmgr_handler_remove(&vh.nmmgr);
}
