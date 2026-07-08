/* KallistiOS ##version##

   dcload_syscalls_net.c

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

#include <dc/dcload.h>

/* Default backend, defined in dcload_syscall.c */
int dcload_syscall_native(dcload_cmd_t cmd, void *p1, void *p2, void *p3);

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

static int dcload_net_open(const char *fn, uint32_t flags, uint32_t umask) {
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

int dcload_syscall_net(dcload_cmd_t cmd, void *p1, void *p2, void *p3) {
    switch(cmd) {
        case DCLOAD_OPEN:
            return dcload_net_open((const char *)p1, (uint32_t)p2, (uint32_t)p3);
        case DCLOAD_CLOSE:
        case DCLOAD_READ:
        case DCLOAD_WRITE:
        case DCLOAD_LSEEK:
        case DCLOAD_STAT:
        case DCLOAD_LINK:
        case DCLOAD_UNLINK:
        case DCLOAD_OPENDIR:
        case DCLOAD_CLOSEDIR:
        case DCLOAD_READDIR:
        case DCLOAD_REWINDDIR:
            return -1;

        /* ----------- Native-only ----------- */
        case DCLOAD_ASSIGNWRKMEM:
        case DCLOAD_GETHOSTINFO:
        case DCLOAD_EXIT:
            return dcload_syscall_native(cmd, p1, p2, p3);

        /* ----------- Not Used ----------- */
        case DCLOAD_CREAT:
        case DCLOAD_CHDIR:
        case DCLOAD_CHMOD:
        case DCLOAD_FSTAT:
        case DCLOAD_TIME:
        case DCLOAD_UTIME:
        case DCLOAD_GDBPACKET:
        default:
            errno = ENOSYS;
            return -1;
    }
}

