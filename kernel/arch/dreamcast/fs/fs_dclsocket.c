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
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include <kos/mutex.h>
#include <kos/fs.h>
#include <kos/net.h>
#include <kos/dbgio.h>
#include <kos/dbglog.h>

#include <dc/fs_dclsocket.h>

#define DCLOAD_PORT 31313
#define NAME "dcload-ip over KOS sockets"

#define PACKED __attribute__((packed))
typedef struct {
    unsigned char id[4];
    unsigned int address;
    unsigned int size;
    unsigned char data[];
} PACKED command_t;

typedef struct {
    unsigned char id[4];
    unsigned int value0;
} PACKED command_int_t;

typedef struct {
    unsigned char id[4];
    unsigned int value0;
    unsigned int value1;
    unsigned int value2;
} PACKED command_3int_t;
#undef PACKED

static struct {
    unsigned int addr;
    unsigned int size;
    unsigned char map[16384];
} bin_info;

extern int dcload_type;
static int initted = 0;
static int escape = 0;
static int retval = 0;
static mutex_t mutex;
static char *dcload_path = NULL;
static uint8 pktbuf[1024 + sizeof(command_t)];

static int dcls_socket = -1;

static void dcls_handle_lbin(command_t *cmd) {
    bin_info.addr = ntohl(cmd->address);
    bin_info.size = ntohl(cmd->size);
    memset(bin_info.map, 0, 16384);

    send(dcls_socket, cmd, sizeof(command_t), 0);
}

static void dcls_handle_pbin(command_t *cmd) {
    int index = (ntohl(cmd->address) - bin_info.addr) >> 10;

    memcpy((uint8 *)ntohl(cmd->address), cmd->data, ntohl(cmd->size));
    bin_info.map[index] = 1;
}

static void dcls_handle_dbin(command_t *cmd) {
    unsigned int i;

    for(i = 0; i < (bin_info.size + 1023) / 1024; ++i) {
        if(!bin_info.map[i])
            break;
    }

    if(i == (bin_info.size + 1023) / 1024) {
        cmd->address = 0;
        cmd->size = 0;
    }
    else {
        cmd->address = htonl(bin_info.addr + i * 1024);

        if(i == (bin_info.size + 1023) / 1024 - 1) {
            cmd->size = htonl(bin_info.size % 1024);
        }
        else {
            cmd->size = htonl(1024);
        }
    }

    send(dcls_socket, cmd, sizeof(command_t), 0);
}

static void dcls_handle_sbin(command_t *cmd) {
    uint32 left, size;
    uint8 *ptr;
    int count, i;
    command_t *resp = (command_t *)pktbuf;

    left = ntohl(cmd->size);
    ptr = (uint8 *)ntohl(cmd->address);
    count = (left + 1023) / 1024;

    memcpy(resp->id, "SBIN", 4);

    for(i = 0; i < count; ++i) {
        size = left > 1024 ? 1024 : left;
        left -= size;

        resp->address = htonl((uint32)ptr);
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
    escape = 1;
}

static void dcls_handle_vers(command_t *cmd) {
    command_t *resp = (command_t *)pktbuf;
    int size = strlen(NAME) + 1 + sizeof(command_t);

    memcpy(resp, cmd, sizeof(command_t));
    strcpy((char *)resp->data, NAME);

    send(dcls_socket, pktbuf, size, 0);
}

static void dcls_recv_loop(void) {
    uint8 pkt[1514];
    command_t *cmd = (command_t *)pkt;

    while(!escape) {
        /* If we're in an interrupt, this works differently.... */
        if(irq_inside_int()) {
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
        else if(!memcmp(cmd->id, "VERS", 4)) {
            dcls_handle_vers(cmd);
        }
    }

    escape = 0;
}

static void *dcls_open(struct vfs_handler *vfs, const char *fn, int mode) {
    int hnd, dcload_mode = 0;
    int mm = (mode & O_MODE_MASK);
    command_t *cmd = (command_t *)pktbuf;

    (void)vfs;

    if(mutex_lock_irqsafe(&mutex))
        return NULL;

    if(mode & O_DIR) {
        char realfn[fn[0] ? strlen(fn) + 1 : 2];

        if(fn[0] == '\0') {
            strcpy(realfn, "/");
        }
        else    {
            strcpy(realfn, fn);
        }

        memcpy(pktbuf, "DC16", 4);
        strcpy((char *)(pktbuf + 4), fn);

        send(dcls_socket, pktbuf, 5 + strlen(realfn), 0);

        dcls_recv_loop();
        hnd = retval;

        if(hnd) {
            if(dcload_path)
                free(dcload_path);

            if(fn[strlen(realfn) - 1] == '/') {
                dcload_path = (char *)malloc(strlen(realfn) + 1);
                strcpy(dcload_path, realfn);
            }
            else {
                dcload_path = (char *)malloc(strlen(realfn) + 2);
                strcpy(dcload_path, realfn);
                strcat(dcload_path, "/");
            }
        }
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
        hnd = retval + 1;
    }

    mutex_unlock(&mutex);

    return (void *)hnd;
}

static int dcls_close(void *hnd) {
    int fd = (int) hnd;
    command_int_t *cmd = (command_int_t *)pktbuf;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    if(fd > 100) {
        memcpy(cmd->id, "DC17", 4);
        cmd->value0 = htonl(fd);

        send(dcls_socket, cmd, sizeof(command_int_t), 0);
        dcls_recv_loop();
    }
    else if(fd) {
        --fd;

        memcpy(cmd->id, "DC05", 4);
        cmd->value0 = htonl(fd);

        send(dcls_socket, cmd, sizeof(command_int_t), 0);
        dcls_recv_loop();
    }

    mutex_unlock(&mutex);
    return 0;
}

static ssize_t dcls_read(void *hnd, void *buf, size_t cnt) {
    uint32 fd = (uint32) hnd;
    command_3int_t *cmd = (command_3int_t *)pktbuf;

    if(!fd)
        return -1;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    --fd;

    memcpy(cmd->id, "DC03", 4);
    cmd->value0 = htonl(fd);
    cmd->value1 = htonl((uint32) buf);
    cmd->value2 = htonl((uint32) cnt);

    send(dcls_socket, cmd, sizeof(command_3int_t), 0);
    dcls_recv_loop();

    mutex_unlock(&mutex);

    return retval;
}

static ssize_t dcls_write(void *hnd, const void *buf, size_t cnt) {
    uint32 fd = (uint32) hnd;
    command_3int_t *cmd = (command_3int_t *)pktbuf;

    if(!fd)
        return -1;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    --fd;

    memcpy(cmd->id, "DD02", 4);
    cmd->value0 = htonl(fd);
    cmd->value1 = htonl((uint32) buf);
    cmd->value2 = htonl(cnt);

    send(dcls_socket, cmd, sizeof(command_3int_t), 0);
    dcls_recv_loop();

    mutex_unlock(&mutex);

    return retval;
}

static off_t dcls_seek(void *hnd, off_t offset, int whence) {
    uint32 fd = (uint32)hnd;
    command_3int_t *command = (command_3int_t *)pktbuf;

    if(!hnd)
        return -1;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    --fd;

    memcpy(command->id, "DC11", 4);
    command->value0 = htonl(fd);
    command->value1 = htonl((uint32)offset);
    command->value2 = htonl((uint32)whence);

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

static dirent_t their_dir;
static dcload_dirent_t our_dir;

static dirent_t *dcls_readdir(void *hnd) {
    uint32 fd = (uint32) hnd;
    command_3int_t *cmd = (command_3int_t *)pktbuf;

    if(fd < 100) {
        errno = EBADF;
        return NULL;
    }

    if(mutex_lock_irqsafe(&mutex))
        return NULL;

    memcpy(cmd->id, "DC18", 4);
    cmd->value0 = htonl(fd);
    cmd->value1 = htonl((uint32)(&our_dir));
    cmd->value2 = htonl(sizeof(dcload_dirent_t));

    send(dcls_socket, cmd, sizeof(command_3int_t), 0);

    dcls_recv_loop();

    if(retval) {
        char fn[strlen(dcload_path) + strlen(our_dir.d_name) + 1];
        command_t *cmd2 = (command_t *)pktbuf;
        dcload_stat_t filestat;

        strcpy(their_dir.name, our_dir.d_name);
        their_dir.size = 0;
        their_dir.time = 0;
        their_dir.attr = 0;

        strcpy(fn, dcload_path);
        strcat(fn, our_dir.d_name);

        memcpy(cmd2->id, "DC13", 4);
        cmd2->address = htonl((uint32) &filestat);
        cmd2->size = htonl(sizeof(dcload_stat_t));
        strcpy((char *)cmd2->data, fn);

        send(dcls_socket, cmd2, sizeof(command_t) + strlen(fn) + 1, 0);

        dcls_recv_loop();

        if(!retval) {
            if(filestat.st_mode & S_IFDIR) {
                their_dir.size = -1;
            }
            else {
                their_dir.size = filestat.st_size;
            }

            their_dir.time = filestat.mtime;
        }

        mutex_unlock(&mutex);
        return &their_dir;
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

static int dcls_stat(vfs_handler_t *vfs, const char *fn, struct stat *rv,
                     int flag) {
    command_t *cmd = (command_t *)pktbuf;
    dcload_stat_t filestat;

    (void)flag;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DC13", 4);
    cmd->address = htonl((uint32) &filestat);
    cmd->size = htonl(sizeof(dcload_stat_t));
    strcpy((char *)(cmd->data), fn);

    send(dcls_socket, cmd, sizeof(command_t) + strlen(fn) + 1, 0);

    dcls_recv_loop();

    if(!retval) {
        memset(rv, 0, sizeof(struct stat));
        rv->st_dev = (dev_t)((ptr_t)vfs);
        rv->st_ino = filestat.st_ino;
        rv->st_mode = filestat.st_mode;
        rv->st_nlink = filestat.st_nlink;
        rv->st_uid = filestat.st_uid;
        rv->st_gid = filestat.st_gid;
        rv->st_rdev = filestat.st_rdev;
        rv->st_size = filestat.st_size;
        rv->st_atime = filestat.atime;
        rv->st_mtime = filestat.mtime;
        rv->st_ctime = filestat.ctime;
        rv->st_blksize = filestat.st_blksize;
        rv->st_blocks = filestat.st_blocks;

        mutex_unlock(&mutex);
        return 0;
    }

    mutex_unlock(&mutex);
    return -1;
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

static int dcls_writebuf(const uint8 *buf, int len, int xlat) {
    command_3int_t cmd;

    (void)xlat;

    if(initted < 2)
        return -1;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd.id, "DD02", 4);
    cmd.value0 = htonl(1);
    cmd.value1 = htonl((uint32) buf);
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
    NULL,               /* rewinddir */
    NULL                /* fstat */
};

/* dbgio handler */
dbgio_handler_t dbgio_dcls = {
    "fs_dclsocket",
    dcls_detected,
    dcls_fake_init,
    dcls_fake_shutdown,
    NULL,
    NULL,
    NULL,
    NULL,
    dcls_writebuf,
    NULL
};

/* This function must be called prior to calling fs_dclsocket_init() */
void fs_dclsocket_init_console(void) {
    /* Make sure networking is up first of all */
    if(!net_default_dev) {
        return;
    }

    dbgio_dcls.set_irq_usage = dbgio_null.set_irq_usage;
    dbgio_dcls.read = dbgio_null.read;
    dbgio_dcls.write = dbgio_null.write;
    dbgio_dcls.flush = dbgio_null.flush;
    dbgio_dcls.read_buffer = dbgio_null.read_buffer;

    initted = 1;
}

uint32 _fs_dclsocket_get_ip(void) {
    uint32 ip, port;

    return dcloadsyscall(DCLOAD_GETHOSTINFO, &ip, &port);
}

int fs_dclsocket_init(void) {
    struct sockaddr_in addr;
    int err;
    uint8 ipaddr[4], mac[6];
    uint32 ip, port;

    /* Make sure we've initted the console */
    if(initted != 1)
        return -1;

    /* Make sure we're actually on dcload-ip */
    if(dcload_type != DCLOAD_TYPE_IP)
        return -1;

    /* Determine where dctool is running, and set up our variables for that */
    dcloadsyscall(DCLOAD_GETHOSTINFO, &ip, &port);

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

    addr.sin_port = htons((uint16)port);
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
