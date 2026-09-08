/* KallistiOS ##version##

   kernel/kosload_net.c

   Copyright (C) 2007, 2008, 2012, 2013, 2015 Lawrence Sebald

   Based on fs_dclnative.c and related files
   Copyright (C) 2003 Megan Potter

   Copyright (C) 2026 Andy Barajas

   Portions of various supporting modules are
   Copyright (C) 2001 Andrew Kieschnick, imported
   from the GPL'd dc-load-ip sources to a BSD-compatible
   license with permission.
*/

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

#include <kos/kosload.h>
#include <kos/fs_kosload.h>

/* Default transport, defined per-arch in kosload_syscalls.c */
int kosload_syscall_native(kosload_cmd_t cmd, void *p1, void *p2, void *p3);

#define KOSLOAD_PORT 53535
#define KOSLOAD_PACKET_SIZE 1440

/* Number of KOSLOAD_PACKET_SIZE chunks needed to cover n bytes. */
#define KOSLOAD_PACKETS(n)  (((n) + KOSLOAD_PACKET_SIZE - 1) / KOSLOAD_PACKET_SIZE)

/* PARTBIN receive map: one bit per chunk, sized to cover 16 MB of RAM. */
#define KOSLOAD_MAP_ENTRIES KOSLOAD_PACKETS(16 * 1024 * 1024)
#define KOSLOAD_MAP_BYTES   (((KOSLOAD_MAP_ENTRIES) + 7) / 8)

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
    unsigned char map[KOSLOAD_MAP_BYTES];
} bin_info;

extern int kosload_type;
static bool initted = false;
static bool escape = false;
static int retval = 0;
static mutex_t mutex;
static uint8_t pktbuf[KOSLOAD_PACKET_SIZE + sizeof(command_t)];

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
    memset(bin_info.map, 0, KOSLOAD_MAP_BYTES);

    send(dcls_socket, cmd, sizeof(command_t), 0);
}

static void dcls_handle_pbin(command_t *cmd) {
    int index = (ntohl(cmd->address) - bin_info.addr) / KOSLOAD_PACKET_SIZE;

    memcpy((uint8_t *)ntohl(cmd->address), cmd->data, ntohl(cmd->size));

    if(index >= 0 && index < KOSLOAD_MAP_ENTRIES)
        bin_map_set(index);
}

static void dcls_handle_dbin(command_t *cmd) {
    unsigned int i;

    for(i = 0; i < KOSLOAD_PACKETS(bin_info.size); ++i) {
        if(!bin_map_test(i))
            break;
    }

    if(i == KOSLOAD_PACKETS(bin_info.size)) {
        cmd->address = 0;
        cmd->size = 0;
    }
    else {
        cmd->address = htonl(bin_info.addr + i * KOSLOAD_PACKET_SIZE);

        if(i == KOSLOAD_PACKETS(bin_info.size) - 1) {
            cmd->size = htonl(bin_info.size % KOSLOAD_PACKET_SIZE);
        }
        else {
            cmd->size = htonl(KOSLOAD_PACKET_SIZE);
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
    count = KOSLOAD_PACKETS(left);

    memcpy(resp->id, "SBIN", 4);

    for(i = 0; i < count; ++i) {
        size = left > KOSLOAD_PACKET_SIZE ? KOSLOAD_PACKET_SIZE : left;
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

static int kosload_net_open(const char *fn, uint32_t flags, uint32_t umask) {
    command_t *cmd = (command_t *)pktbuf;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DC04", 4);
    cmd->address = htonl(flags);     /* open flags */
    cmd->size = htonl(umask);        /* umask */
    strcpy((char *)cmd->data, fn);   /* NUL-terminated path in data[] */

    send(dcls_socket, cmd, sizeof(command_t) + strlen(fn) + 1, 0);
    dcls_recv_loop();

    int rv = retval;
    mutex_unlock(&mutex);
    return rv;
}

static int kosload_net_close(uint32_t hnd) {
    command_int_t *cmd = (command_int_t *)pktbuf;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DC05", 4);
    cmd->value0 = htonl(hnd);  /* file handle */

    send(dcls_socket, cmd, sizeof(command_int_t), 0);
    dcls_recv_loop();

    mutex_unlock(&mutex);
    return 0;
}

static ssize_t kosload_net_read(uint32_t hnd, void *buf, size_t cnt) {
    command_3int_t *cmd = (command_3int_t *)pktbuf;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DC03", 4);
    cmd->value0 = htonl(hnd);  /* file handle */
    cmd->value1 = htonl((uint32_t) buf);  /* buffer address */
    cmd->value2 = htonl((uint32_t) cnt);  /* count */

    send(dcls_socket, cmd, sizeof(command_3int_t), 0);
    dcls_recv_loop();

    ssize_t rv = retval;
    mutex_unlock(&mutex);
    return rv;
}

static ssize_t kosload_net_write(uint32_t hnd, const void *buf, size_t cnt) {
    command_3int_t *cmd = (command_3int_t *)pktbuf;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DD02", 4);
    cmd->value0 = htonl(hnd);  /* file handle */
    cmd->value1 = htonl((uint32_t) buf);  /* buffer address */
    cmd->value2 = htonl((uint32_t) cnt);  /* count */

    send(dcls_socket, cmd, sizeof(command_3int_t), 0);
    dcls_recv_loop();

    ssize_t rv = retval;
    mutex_unlock(&mutex);
    return rv;
}

static off_t kosload_net_seek(uint32_t hnd, off_t offset, int whence) {
    command_3int_t *command = (command_3int_t *)pktbuf;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(command->id, "DC11", 4);
    command->value0 = htonl(hnd);
    command->value1 = htonl((uint32_t)offset);
    command->value2 = htonl((uint32_t)whence);

    send(dcls_socket, command, sizeof(command_3int_t), 0);
    dcls_recv_loop();

    off_t rv = retval;
    mutex_unlock(&mutex);
    return rv;
}

static int kosload_net_stat(const char *path, kosload_stat_t *filestat) {
    command_t *cmd = (command_t *)pktbuf;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DC13", 4);
    cmd->address = htonl((uint32_t) filestat);
    cmd->size = htonl(sizeof(kosload_stat_t));
    strcpy((char *)(cmd->data), path);

    send(dcls_socket, cmd, sizeof(command_t) + strlen(path) + 1, 0);
    dcls_recv_loop();

    int rv = retval;
    mutex_unlock(&mutex);
    return rv;
}

static int kosload_net_link(const char *fn1, const char *fn2) {
    int len1 = strlen(fn1), len2 = strlen(fn2);

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(pktbuf, "DC07", 4);
    strcpy((char *)(pktbuf + 4), fn1);
    strcpy((char *)(pktbuf + 5 + len1), fn2);

    send(dcls_socket, pktbuf, 6 + len1 + len2, 0);
    dcls_recv_loop();

    int rv = retval;
    mutex_unlock(&mutex);
    return rv;
}

static int kosload_net_unlink(const char *fn) {
    int len = strlen(fn) + 5;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(pktbuf, "DC08", 4);
    strcpy((char *)(pktbuf + 4), fn);

    send(dcls_socket, pktbuf, len, 0);
    dcls_recv_loop();

    int rv = retval;
    mutex_unlock(&mutex);
    return rv;
}

static int kosload_net_opendir(const char *fn) {
    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(pktbuf, "DC16", 4);
    strcpy((char *)(pktbuf + 4), fn);

    send(dcls_socket, pktbuf, 5 + strlen(fn), 0);
    dcls_recv_loop();

    int rv = retval;
    mutex_unlock(&mutex);
    return rv;
}

static int kosload_net_closedir(uint32_t hnd) {
    command_int_t *cmd = (command_int_t *)pktbuf;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DC17", 4);
    cmd->value0 = htonl(hnd);  /* dir handle */

    send(dcls_socket, cmd, sizeof(command_int_t), 0);
    dcls_recv_loop();

    mutex_unlock(&mutex);
    return 0;
}

/* The host fills the readdir reply as a POSIX struct dirent, not a KOS
   dirent_t, so receive it as one */
static union {
    struct dirent de;
    char storage[sizeof(struct dirent) + NAME_MAX + 1];
} our_dir;

static struct dirent *kosload_net_readdir(uint32_t hnd) {
    command_3int_t *cmd = (command_3int_t *)pktbuf;

    if(mutex_lock_irqsafe(&mutex))
        return NULL;

    memcpy(cmd->id, "DC18", 4);
    cmd->value0 = htonl(hnd);                    /* dir handle */
    cmd->value1 = htonl((uint32_t)&our_dir.de);  /* buffer address */
    cmd->value2 = htonl(sizeof(our_dir));        /* buffer size */

    send(dcls_socket, cmd, sizeof(command_3int_t), 0);
    dcls_recv_loop();

    struct dirent *rv = retval ? &our_dir.de : NULL;
    mutex_unlock(&mutex);
    return rv;
}

static int kosload_net_rewinddir(uint32_t hnd) {
    command_int_t *cmd = (command_int_t *)pktbuf;

    if(mutex_lock_irqsafe(&mutex))
        return -1;

    memcpy(cmd->id, "DC21", 4);
    cmd->value0 = htonl(hnd);  /* dir handle */

    send(dcls_socket, cmd, sizeof(command_int_t), 0);
    dcls_recv_loop();

    int rv = retval;
    mutex_unlock(&mutex);
    return rv;
}

static int kosload_syscall_net(kosload_cmd_t cmd, void *p1, void *p2, void *p3) {
    switch(cmd) {
        case KOSLOAD_OPEN:
            return kosload_net_open((const char *)p1, (uint32_t)p2, (uint32_t)p3);
        case KOSLOAD_CLOSE:
            return kosload_net_close((uint32_t)p1);
        case KOSLOAD_READ:
            return kosload_net_read((uint32_t)p1, p2, (size_t)p3);
        case KOSLOAD_WRITE:
            return kosload_net_write((uint32_t)p1, p2, (size_t)p3);
        case KOSLOAD_LSEEK:
            return kosload_net_seek((uint32_t)p1, (off_t)p2, (int)p3);
        case KOSLOAD_STAT:
            return kosload_net_stat((const char *)p1, (kosload_stat_t *)p2);
        case KOSLOAD_LINK:
            return kosload_net_link((const char *)p1, (const char *)p2);
        case KOSLOAD_UNLINK:
            return kosload_net_unlink((const char *)p1);
        case KOSLOAD_OPENDIR:
            return kosload_net_opendir((const char *)p1);
        case KOSLOAD_CLOSEDIR:
            return kosload_net_closedir((uint32_t)p1);
        case KOSLOAD_READDIR:
            return (int)(uintptr_t)kosload_net_readdir((uint32_t)p1);
        case KOSLOAD_REWINDDIR:
            return kosload_net_rewinddir((uint32_t)p1);

        /* ----------- Native-only ----------- */
        case KOSLOAD_ASSIGNWRKMEM:
        case KOSLOAD_GETHOSTINFO:
        case KOSLOAD_EXIT:
            return kosload_syscall_native(cmd, p1, p2, p3);

        /* ----------- Not Used ----------- */
        case KOSLOAD_CREAT:
        case KOSLOAD_CHDIR:
        case KOSLOAD_CHMOD:
        case KOSLOAD_FSTAT:
        case KOSLOAD_TIME:
        case KOSLOAD_UTIME:
        case KOSLOAD_GDBPACKET:
        default:
            errno = ENOSYS;
            return -1;
    }
}

int kosload_syscall_net_init(void) {
    struct sockaddr_in addr;
    uint8_t ipaddr[4], mac[6];
    uint32_t ip, port;
    int err;

    /* Make sure we've initted the console */
    if(initted)
        return 0;

    if(!net_default_dev || kosload_type != KOSLOAD_TYPE_IP)
        return -1;

    /* Determine where dctool is running, and set up our variables for that */
    kosload_gethostinfo(&ip, &port);

    /* Put dc-tool's info into our ARP cache */
    net_ipv4_parse_address(ip, ipaddr);

    err = net_arp_lookup(net_default_dev, ipaddr, mac, NULL, NULL, 0);
    while(err == -1 || err == -2)
        err = net_arp_lookup(net_default_dev, ipaddr, mac, NULL, NULL, 0);

    /* Make the entry permanent */
    net_arp_insert(net_default_dev, mac, ipaddr, 0);

    /* Ok, now create our socket, and set it up properly */
    dcls_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if(dcls_socket == -1)
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(KOSLOAD_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(dcls_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        goto error;

    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(ip);

    if(connect(dcls_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        goto error;

    if(mutex_init(&mutex, MUTEX_TYPE_NORMAL))
        goto error;

    initted = true;
    kosload_syscall_set(kosload_syscall_net);

    return 0;

error:
    close(dcls_socket);
    dcls_socket = -1;
    return -1;
}

/* Restore the native backend and tear down the socket */
void kosload_syscall_net_shutdown(void) {
    /* Nothing to undo if the socket backend was never installed. */
    if(!initted)
        return;

    /* Point kosload syscalls back at the native backend before the socket that
       the net backend depends on goes away. */
    kosload_syscall_set(NULL);
    mutex_destroy(&mutex);

    close(dcls_socket);
    dcls_socket = -1;
    initted = false;
}

