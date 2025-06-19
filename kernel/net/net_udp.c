/* KallistiOS ##version##

   kernel/net/net_udp.c
   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2012, 2013, 2014 Lawrence Sebald

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <kos/net.h>
#include <kos/mutex.h>
#include <kos/genwait.h>
#include <sys/queue.h>
#include <kos/fs_socket.h>
#include <arch/irq.h>
#include <sys/socket.h>
#include <netinet/udp.h>
#include <netinet/udplite.h>

#include "net_ipv4.h"
#include "net_ipv6.h"

#if __GNUC__ >= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#endif

/* Default hop limit (or ttl for IPv4) for new sockets */
#define UDP_DEFAULT_HOPS    64

typedef struct {
    uint16 src_port __packed;
    uint16 dst_port __packed;
    uint16 length __packed;
    uint16 checksum __packed;
} udp_hdr_t;

struct udp_pkt {
    TAILQ_ENTRY(udp_pkt) pkt_queue;
    struct sockaddr_in6 from;
    uint8 *data;
    uint16 datasize;
};

TAILQ_HEAD(udp_pkt_queue, udp_pkt);

#define UDPSOCK_NO_CHECKSUM 0x00000001
#define UDPSOCK_LITE_RCVCOV 0x00000002

struct udp_sock {
    LIST_ENTRY(udp_sock) sock_list;
    struct sockaddr_in6 local_addr;
    struct sockaddr_in6 remote_addr;

    uint32 flags;
    uint32 int_flags;
    int domain;
    int proto;
    int hop_limit;
    file_t sock;

    struct {
        uint16_t send_cscov;
        uint16_t recv_cscov;
    } udp_lite;

    struct udp_pkt_queue packets;
};

LIST_HEAD(udp_sock_list, udp_sock);

static struct udp_sock_list net_udp_sockets = LIST_HEAD_INITIALIZER(0);
static mutex_t udp_mutex = MUTEX_INITIALIZER;
static net_udp_stats_t udp_stats = { 0 };

static int net_udp_send_raw(netif_t *net, const struct sockaddr_in6 *src,
                            const struct sockaddr_in6 *dst, const uint8 *data,
                            size_t size, uint32_t flags, int hops,
                            uint32_t iflags, int proto, uint16_t cscov);

static int net_udp_accept(net_socket_t *hnd, struct sockaddr *addr,
                          socklen_t *addr_len) {
    (void)hnd;
    (void)addr;
    (void)addr_len;
    errno = EOPNOTSUPP;
    return -1;
}

static int net_udp_bind(net_socket_t *hnd, const struct sockaddr *addr,
                        socklen_t addr_len) {
    struct udp_sock *udpsock, *iter;
    struct sockaddr_in *realaddr4;
    struct sockaddr_in6 realaddr6;

    /* Verify the parameters sent in first */
    if(addr == NULL) {
        errno = EDESTADDRREQ;
        return -1;
    }

    switch(addr->sa_family) {
        case AF_INET:

            if(addr_len != sizeof(struct sockaddr_in)) {
                errno = EINVAL;
                return -1;
            }

            /* Grab the IPv4 address struct and convert it to IPv6 */
            realaddr4 = (struct sockaddr_in *)addr;
            memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
            realaddr6.sin6_family = AF_INET6;
            realaddr6.sin6_port = realaddr4->sin_port;

            if(realaddr4->sin_addr.s_addr != INADDR_ANY) {
                realaddr6.sin6_addr.__s6_addr.__s6_addr16[5] = 0xFFFF;
                realaddr6.sin6_addr.__s6_addr.__s6_addr32[3] =
                    realaddr4->sin_addr.s_addr;
            }
            else {
                realaddr6.sin6_addr = in6addr_any;
            }

            break;

        case AF_INET6:

            if(addr_len != sizeof(struct sockaddr_in6)) {
                errno = EINVAL;
                return -1;
            }

            realaddr6 = *((struct sockaddr_in6 *)addr);
            break;

        default:
            errno = EAFNOSUPPORT;
            return -1;
    }

    if(mutex_lock_irqsafe(&udp_mutex))
        return -1;

    udpsock = (struct udp_sock *)hnd->data;

    if(udpsock == NULL) {
        mutex_unlock(&udp_mutex);
        errno = EBADF;
        return -1;
    }

    /* Make sure the address family we're binding to matches that which is set
       on the socket itself */
    if(addr->sa_family != udpsock->domain) {
        mutex_unlock(&udp_mutex);
        errno = EINVAL;
        return -1;
    }

    /* See if we requested a specific port or not */
    if(realaddr6.sin6_port != 0) {
        /* Make sure we don't already have a socket bound to the port
           specified */
        LIST_FOREACH(iter, &net_udp_sockets, sock_list) {
            if(iter == udpsock)
                continue;

            if(iter->local_addr.sin6_port == realaddr6.sin6_port) {
                mutex_unlock(&udp_mutex);
                errno = EADDRINUSE;
                return -1;
            }
        }

        udpsock->local_addr = realaddr6;
    }
    else {
        uint16 port = 1024, tmp = 0;

        /* Grab the first unused port >= 1024. This is, unfortunately, O(n^2) */
        while(tmp != port) {
            tmp = port;

            LIST_FOREACH(iter, &net_udp_sockets, sock_list) {
                if(iter->local_addr.sin6_port == port) {
                    ++port;
                    break;
                }
            }
        }

        udpsock->local_addr = realaddr6;
        udpsock->local_addr.sin6_port = htons(port);
    }

    udpsock->sock = hnd->fd;

    mutex_unlock(&udp_mutex);

    return 0;
}

static int net_udp_connect(net_socket_t *hnd, const struct sockaddr *addr,
                           socklen_t addr_len) {
    struct udp_sock *udpsock;
    struct sockaddr_in *realaddr4;
    struct sockaddr_in6 realaddr6;

    if(addr == NULL) {
        errno = EDESTADDRREQ;
        return -1;
    }

    switch(addr->sa_family) {
        case AF_INET:

            if(addr_len != sizeof(struct sockaddr_in)) {
                errno = EINVAL;
                return -1;
            }

            /* Grab the IPv4 address struct and convert it to IPv6 */
            realaddr4 = (struct sockaddr_in *)addr;

            if(realaddr4->sin_addr.s_addr == INADDR_ANY) {
                errno = EADDRNOTAVAIL;
                return -1;
            }

            memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
            realaddr6.sin6_family = AF_INET6;
            realaddr6.sin6_port = realaddr4->sin_port;
            realaddr6.sin6_addr.__s6_addr.__s6_addr16[5] = 0xFFFF;
            realaddr6.sin6_addr.__s6_addr.__s6_addr32[3] =
                realaddr4->sin_addr.s_addr;
            break;

        case AF_INET6:

            if(addr_len != sizeof(struct sockaddr_in6)) {
                errno = EINVAL;
                return -1;
            }

            realaddr6 = *((struct sockaddr_in6 *)addr);
            break;

        default:
            errno = EAFNOSUPPORT;
            return -1;
    }

    if(mutex_lock_irqsafe(&udp_mutex))
        return -1;

    udpsock = (struct udp_sock *)hnd->data;

    if(udpsock == NULL) {
        mutex_unlock(&udp_mutex);
        errno = EBADF;
        return -1;
    }

    /* Make sure the address family we're binding to matches that which is set
       on the socket itself */
    if(addr->sa_family != udpsock->domain) {
        mutex_unlock(&udp_mutex);
        errno = EINVAL;
        return -1;
    }

    /* Make sure the socket isn't already connected */
    if(!IN6_IS_ADDR_UNSPECIFIED(&udpsock->remote_addr.sin6_addr)) {
        mutex_unlock(&udp_mutex);
        errno = EISCONN;
        return -1;
    }

    /* Make sure we have a valid address to connect to */
    if(IN6_IS_ADDR_UNSPECIFIED(&realaddr6.sin6_addr) ||
            realaddr6.sin6_port == 0) {
        mutex_unlock(&udp_mutex);
        errno = EADDRNOTAVAIL;
        return -1;
    }

    /* "Connect" to the specified address */
    udpsock->remote_addr = realaddr6;

    mutex_unlock(&udp_mutex);

    return 0;
}

static int net_udp_listen(net_socket_t *hnd, int backlog) {
    (void)hnd;
    (void)backlog;
    errno = EOPNOTSUPP;
    return -1;
}

static ssize_t net_udp_recvfrom(net_socket_t *hnd, void *buffer, size_t length,
                                int flags, struct sockaddr *addr,
                                socklen_t *addr_len) {
    struct udp_sock *udpsock;
    struct udp_pkt *pkt;

    if(mutex_lock_irqsafe(&udp_mutex))
        return -1;

    udpsock = (struct udp_sock *)hnd->data;

    if(udpsock == NULL) {
        mutex_unlock(&udp_mutex);
        errno = EBADF;
        return -1;
    }

    if(udpsock->flags & (SHUT_RD << 24)) {
        mutex_unlock(&udp_mutex);
        return 0;
    }

    if(buffer == NULL || (addr != NULL && addr_len == NULL)) {
        mutex_unlock(&udp_mutex);
        errno = EFAULT;
        return -1;
    }

    if(TAILQ_EMPTY(&udpsock->packets) &&
       ((udpsock->flags & FS_SOCKET_NONBLOCK) || (flags & MSG_DONTWAIT) ||
        irq_inside_int())) {
        mutex_unlock(&udp_mutex);
        errno = EWOULDBLOCK;
        return -1;
    }

    while(TAILQ_EMPTY(&udpsock->packets)) {
        mutex_unlock(&udp_mutex);
        genwait_wait(udpsock, "net_udp_recvfrom", 0, NULL);
        mutex_lock(&udp_mutex);
    }

    pkt = TAILQ_FIRST(&udpsock->packets);

    if(pkt->datasize > length) {
        memcpy(buffer, pkt->data, length);
    }
    else {
        memcpy(buffer, pkt->data, pkt->datasize);
        length = pkt->datasize;
    }

    if(addr != NULL) {
        if(udpsock->domain == AF_INET) {
            struct sockaddr_in realaddr;

            memset(&realaddr, 0, sizeof(struct sockaddr_in));
            realaddr.sin_family = AF_INET;
            realaddr.sin_addr.s_addr =
                pkt->from.sin6_addr.__s6_addr.__s6_addr32[3];
            realaddr.sin_port = pkt->from.sin6_port;

            if(*addr_len < sizeof(struct sockaddr_in)) {
                memcpy(addr, &realaddr, *addr_len);
            }
            else {
                memcpy(addr, &realaddr, sizeof(struct sockaddr_in));
                *addr_len = sizeof(struct sockaddr_in);
            }
        }
        else if(udpsock->domain == AF_INET6) {
            struct sockaddr_in6 realaddr6;

            memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
            realaddr6.sin6_family = AF_INET6;
            realaddr6.sin6_addr = pkt->from.sin6_addr;
            realaddr6.sin6_port = pkt->from.sin6_port;

            if(*addr_len < sizeof(struct sockaddr_in6)) {
                memcpy(addr, &realaddr6, *addr_len);
            }
            else {
                memcpy(addr, &realaddr6, sizeof(struct sockaddr_in6));
                *addr_len = sizeof(struct sockaddr_in6);
            }
        }
    }

    /* Remove the packet if we're pulling data out of the queue. */
    if(!(flags & MSG_PEEK)) {
        TAILQ_REMOVE(&udpsock->packets, pkt, pkt_queue);
        free(pkt->data);
        free(pkt);
    }

    mutex_unlock(&udp_mutex);

    return length;
}

static ssize_t net_udp_sendto(net_socket_t *hnd, const void *message,
                              size_t length, int flags,
                              const struct sockaddr *addr, socklen_t addr_len) {
    struct udp_sock *udpsock;
    struct sockaddr_in *realaddr;
    struct sockaddr_in6 realaddr6;
    uint32_t sflags, iflags;
    int hops, proto;
    uint16_t cscov;
    struct sockaddr_in6 local_addr;

    (void)flags;

    if(mutex_lock_irqsafe(&udp_mutex))
        return -1;

    udpsock = (struct udp_sock *)hnd->data;

    if(udpsock == NULL) {
        errno = EBADF;
        goto err;
    }

    if(udpsock->flags & (SHUT_WR << 24)) {
        errno = EPIPE;
        goto err;
    }

    if(!IN6_IS_ADDR_UNSPECIFIED(&udpsock->remote_addr.sin6_addr) &&
       udpsock->remote_addr.sin6_port != 0) {
        if(addr) {
            errno = EISCONN;
            goto err;
        }

        realaddr6 = udpsock->remote_addr;
    }
    else if(addr == NULL) {
        errno = EDESTADDRREQ;
        goto err;
    }
    else if(addr->sa_family != udpsock->domain) {
        errno = EAFNOSUPPORT;
        goto err;
    }
    else if(udpsock->domain == AF_INET6) {
        if(addr_len != sizeof(struct sockaddr_in6)) {
            errno = EINVAL;
            goto err;
        }

        realaddr6 = *((struct sockaddr_in6 *)addr);
    }
    else if(udpsock->domain == AF_INET) {
        if(addr_len != sizeof(struct sockaddr_in)) {
            errno = EINVAL;
            goto err;
        }

        realaddr = (struct sockaddr_in *)addr;
        memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
        realaddr6.sin6_family = AF_INET6;
        realaddr6.sin6_addr.__s6_addr.__s6_addr16[5] = 0xFFFF;
        realaddr6.sin6_addr.__s6_addr.__s6_addr32[3] =
            realaddr->sin_addr.s_addr;
        realaddr6.sin6_port = realaddr->sin_port;
    }
    else {
        /* Shouldn't be able to get here... */
        errno = EBADF;
        goto err;
    }

    if(message == NULL) {
        errno = EFAULT;
        goto err;
    }

    if(udpsock->local_addr.sin6_port == 0) {
        uint16 port = 1024, tmp = 0;
        struct udp_sock *iter;

        /* Grab the first unused port >= 1024. This is, unfortunately, O(n^2) */
        while(tmp != port) {
            tmp = port;

            LIST_FOREACH(iter, &net_udp_sockets, sock_list) {
                if(iter->local_addr.sin6_port == port) {
                    ++port;
                    break;
                }
            }
        }

        udpsock->local_addr.sin6_port = htons(port);
    }

    local_addr = udpsock->local_addr;
    sflags = udpsock->flags;
    iflags = udpsock->int_flags;
    hops = udpsock->hop_limit;
    proto = udpsock->proto;
    cscov = udpsock->udp_lite.send_cscov;
    mutex_unlock(&udp_mutex);

    return net_udp_send_raw(NULL, &local_addr, &realaddr6,
                            (const uint8 *)message, length, sflags, hops,
                            iflags, proto, cscov);
err:
    mutex_unlock(&udp_mutex);
    return -1;
}

static int net_udp_shutdownsock(net_socket_t *hnd, int how) {
    struct udp_sock *udpsock;

    if(mutex_lock_irqsafe(&udp_mutex))
        return -1;

    udpsock = (struct udp_sock *)hnd->data;

    if(udpsock == NULL) {
        mutex_unlock(&udp_mutex);
        errno = EBADF;
        return -1;
    }

    if(how & 0xFFFFFFFC) {
        mutex_unlock(&udp_mutex);
        errno = EINVAL;
        return -1;
    }

    udpsock->flags |= (how << 24);

    mutex_unlock(&udp_mutex);

    return 0;
}

static int net_udp_socket(net_socket_t *hnd, int domain, int type, int proto) {
    struct udp_sock *udpsock;

    (void)type;
    (void)proto;

    udpsock = (struct udp_sock *)malloc(sizeof(struct udp_sock));

    if(udpsock == NULL) {
        errno = ENOMEM;
        return -1;
    }

    if(!proto) {
        proto = IPPROTO_UDP;
    }
    else if(proto != IPPROTO_UDP && proto != IPPROTO_UDPLITE) {
        free(udpsock);
        errno = EPROTONOSUPPORT;
        return -1;
    }

    memset(udpsock, 0, sizeof(struct udp_sock));
    TAILQ_INIT(&udpsock->packets);
    udpsock->domain = domain;
    udpsock->proto = proto;
    udpsock->hop_limit = UDP_DEFAULT_HOPS;

    if(mutex_lock_irqsafe(&udp_mutex)) {
        free(udpsock);
        return -1;
    }

    LIST_INSERT_HEAD(&net_udp_sockets, udpsock, sock_list);
    hnd->data = udpsock;
    mutex_unlock(&udp_mutex);

    return 0;
}

static void net_udp_close(net_socket_t *hnd) {
    struct udp_sock *udpsock;
    struct udp_pkt *pkt;
    struct udp_pkt *it;

    if(mutex_lock_irqsafe(&udp_mutex))
        return;

    udpsock = (struct udp_sock *)hnd->data;

    if(udpsock == NULL) {
        mutex_unlock(&udp_mutex);
        errno = EBADF;
        return;
    }

    it = udpsock->packets.tqh_first;
    while(it) {
        pkt = it;
        it = it->pkt_queue.tqe_next;

        free(pkt->data);
        TAILQ_REMOVE(&udpsock->packets, pkt, pkt_queue);
        free(pkt);
    }

    LIST_REMOVE(udpsock, sock_list);

    free(udpsock);
    mutex_unlock(&udp_mutex);
}

static int net_udp_getsockopt(net_socket_t *hnd, int level, int option_name,
                              void *option_value, socklen_t *option_len) {
    struct udp_sock *sock;
    int tmp;

    if(mutex_lock_irqsafe(&udp_mutex))
        return -1;

    if(!(sock = (struct udp_sock *)hnd->data)) {
        mutex_unlock(&udp_mutex);
        errno = EBADF;
        return -1;
    }

    switch(level) {
        case SOL_SOCKET:
            switch(option_name) {
                case SO_ACCEPTCONN:
                    tmp = 0;
                    goto copy_int;

                case SO_TYPE:
                    tmp = SOCK_DGRAM;
                    goto copy_int;
            }

            break;

        case IPPROTO_IP:
            if(sock->domain != AF_INET)
                goto ret_inval;

            switch(option_name) {
                case IP_TTL:
                    tmp = sock->hop_limit;
                    goto copy_int;
            }

            break;

        case IPPROTO_IPV6:
            if(sock->domain != AF_INET6)
                goto ret_inval;

            switch(option_name) {
                case IPV6_UNICAST_HOPS:
                    tmp = sock->hop_limit;
                    goto copy_int;

                case IPV6_V6ONLY:
                    tmp = !!(sock->flags & FS_SOCKET_V6ONLY);
                    goto copy_int;
            }

            break;

        case IPPROTO_UDP:
            if(sock->proto != IPPROTO_UDP)
                goto ret_inval;

            switch(option_name) {
                case UDP_NOCHECKSUM:
                    /* UDP/IPv6 packets must always have a checksum. */
                    if(sock->domain == AF_INET6) {
                        tmp = 0;
                        goto copy_int;
                    }

                    tmp = !!(sock->int_flags & UDPSOCK_NO_CHECKSUM);
                    goto copy_int;
            }

            break;

        case IPPROTO_UDPLITE:
            if(sock->proto != IPPROTO_UDPLITE)
                goto ret_inval;

            switch(option_name) {
                case UDPLITE_SEND_CSCOV:
                    tmp = sock->udp_lite.send_cscov;
                    goto copy_int;

                case UDPLITE_RECV_CSCOV:
                    tmp = sock->udp_lite.recv_cscov;
                    goto copy_int;
            }

            break;
    }

    /* If it wasn't handled, return that error. */
    mutex_unlock(&udp_mutex);
    errno = ENOPROTOOPT;
    return -1;

ret_inval:
    mutex_unlock(&udp_mutex);
    errno = EINVAL;
    return -1;

copy_int:
    if(*option_len >= sizeof(int)) {
        memcpy(option_value, &tmp, sizeof(int));
        *option_len = sizeof(int);
    }
    else {
        memcpy(option_value, &tmp, *option_len);
    }

    mutex_unlock(&udp_mutex);
    return 0;
}

static int net_udp_setsockopt(net_socket_t *hnd, int level, int option_name,
                              const void *option_value, socklen_t option_len) {
    struct udp_sock *sock;
    int tmp;

    if(mutex_lock_irqsafe(&udp_mutex))
        return -1;

    if(!(sock = (struct udp_sock *)hnd->data)) {
        mutex_unlock(&udp_mutex);
        errno = EBADF;
        return -1;
    }

    switch(level) {
        case SOL_SOCKET:
            switch(option_name) {
                case SO_ACCEPTCONN:
                case SO_ERROR:
                case SO_TYPE:
                    goto ret_inval;
            }

            break;

        case IPPROTO_IP:
            if(sock->domain != AF_INET)
                goto ret_inval;

            switch(option_name) {
                case IP_TTL:
                    if(option_len != sizeof(int))
                        goto ret_inval;

                    tmp = *((int *)option_value);

                    if(tmp < -1 || tmp > 255)
                        goto ret_inval;
                    else if(tmp == -1)
                        sock->hop_limit = UDP_DEFAULT_HOPS;
                    else
                        sock->hop_limit = tmp;

                    goto ret_success;
            }

            break;

        case IPPROTO_IPV6:
            if(sock->domain != AF_INET6)
                goto ret_inval;

            switch(option_name) {
                case IPV6_UNICAST_HOPS:
                    if(option_len != sizeof(int))
                        goto ret_inval;

                    tmp = *((int *)option_value);

                    if(tmp < -1 || tmp > 255)
                        goto ret_inval;
                    else if(tmp == -1)
                        sock->hop_limit = UDP_DEFAULT_HOPS;
                    else
                        sock->hop_limit = tmp;

                    goto ret_success;

                case IPV6_V6ONLY:
                    if(option_len != sizeof(int))
                        goto ret_inval;

                    tmp = *((int *)option_value);

                    if(tmp)
                        sock->flags |= FS_SOCKET_V6ONLY;
                    else
                        sock->flags &= ~FS_SOCKET_V6ONLY;

                    goto ret_success;
            }

            break;

        case IPPROTO_UDP:
            if(sock->proto != IPPROTO_UDP)
                goto ret_inval;

            switch(option_name) {
                case UDP_NOCHECKSUM:
                    /* UDP/IPv6 packets must always have a checksum. */
                    if(sock->domain == AF_INET6)
                        goto ret_inval;

                    if(option_len != sizeof(int))
                        goto ret_inval;

                    tmp = *((int *)option_value);

                    if(tmp)
                        sock->int_flags |= UDPSOCK_NO_CHECKSUM;
                    else
                        sock->int_flags &= ~(UDPSOCK_NO_CHECKSUM);

                    goto ret_success;
            }

            break;

        case IPPROTO_UDPLITE:
            if(sock->proto != IPPROTO_UDPLITE)
                goto ret_inval;

            switch(option_name) {
                case UDPLITE_SEND_CSCOV:
                    if(option_len != sizeof(int))
                        goto ret_inval;

                    tmp = *((int *)option_value);

                    if(tmp && tmp < 8)
                        goto ret_inval;
                    else if(tmp > 0xFFFF)
                        tmp = 0xFFFF;

                    sock->udp_lite.send_cscov = (uint16_t)tmp;
                    goto ret_success;

                case UDPLITE_RECV_CSCOV:
                    if(option_len != sizeof(int))
                        goto ret_inval;

                    tmp = *((int *)option_value);

                    if(tmp && tmp < 8)
                        goto ret_inval;
                    else if(tmp > 0xFFFF)
                        tmp = 0xFFFF;

                    sock->udp_lite.recv_cscov = (uint16_t)tmp;
                    sock->int_flags |= UDPSOCK_LITE_RCVCOV;
                    goto ret_success;
            }

            break;
    }

    /* If it wasn't handled, return that error. */
    mutex_unlock(&udp_mutex);
    errno = ENOPROTOOPT;
    return -1;

ret_inval:
    mutex_unlock(&udp_mutex);
    errno = EINVAL;
    return -1;

ret_success:
    mutex_unlock(&udp_mutex);
    return 0;
}

static int net_udp_getsockname(net_socket_t *hnd, struct sockaddr *name,
                               socklen_t *name_len) {
    struct udp_sock *sock;
    struct sockaddr_in realaddr;
    struct sockaddr_in6 realaddr6;

    if(!name || !name_len) {
        errno = EFAULT;
        return -1;
    }

    if(mutex_lock_irqsafe(&udp_mutex))
        return -1;

    if(!(sock = (struct udp_sock *)hnd->data)) {
        mutex_unlock(&udp_mutex);
        errno = EBADF;
        return -1;
    }

    if(sock->domain == AF_INET) {
        memset(&realaddr, 0, sizeof(struct sockaddr_in));
        realaddr.sin_family = AF_INET;
        realaddr.sin_addr.s_addr =
            sock->local_addr.sin6_addr.__s6_addr.__s6_addr32[3];
        realaddr.sin_port = sock->local_addr.sin6_port;

        if(*name_len <= sizeof(struct sockaddr_in)) {
            /* Passed in a structure not big enough so truncate*/
            memcpy(name, &realaddr, *name_len);
        } else {
            memcpy(name, &realaddr, sizeof(struct sockaddr_in));
        }

        *name_len = sizeof(struct sockaddr_in);

        goto ret_success;

    } else if(sock->domain == AF_INET6) {
        memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
        realaddr6.sin6_family = AF_INET6;
        realaddr6.sin6_addr = sock->local_addr.sin6_addr;
        realaddr6.sin6_port = sock->local_addr.sin6_port;

        if(*name_len <= sizeof(struct sockaddr_in6)) {
            /* Passed in a structure not big enough */
            memcpy(name, &realaddr6, *name_len);
        } else {
            memcpy(name, &realaddr6, sizeof(struct sockaddr_in6));
        }

        *name_len = sizeof(struct sockaddr_in6);

        goto ret_success;
    }

    mutex_unlock(&udp_mutex);
    errno = ENOTSOCK;
    return -1;

ret_success:
    mutex_unlock(&udp_mutex);
    return 0;
}

static int net_udp_getpeername(net_socket_t *hnd, struct sockaddr *name, socklen_t *name_len) {
    struct udp_sock *sock;
    struct sockaddr_in realaddr;
    struct sockaddr_in6 realaddr6;

    if(!name || !name_len) {
        errno = EFAULT;
        return -1;
    }

    if(irq_inside_int()) {
        if(mutex_trylock(&udp_mutex) == -1) {
            errno = EWOULDBLOCK;
            return -1;
        }
    } else {
        mutex_lock(&udp_mutex);
    }

    if(!(sock = (struct udp_sock *)hnd->data)) {
        mutex_unlock(&udp_mutex);
        errno = EBADF;
        return -1;
    }

    /* If the socket is not connected, return an error */
    if(IN6_IS_ADDR_UNSPECIFIED(&sock->remote_addr.sin6_addr) || sock->remote_addr.sin6_port == 0) {
        mutex_unlock(&udp_mutex);
        errno = ENOTCONN;
        return -1;
    }

    /* Depending on the socket domain, return the appropriate address structure */
    if(sock->domain == AF_INET) {
        memset(&realaddr, 0, sizeof(struct sockaddr_in));
        realaddr.sin_family = AF_INET;
        realaddr.sin_addr.s_addr = sock->remote_addr.sin6_addr.__s6_addr.__s6_addr32[3];
        realaddr.sin_port = sock->remote_addr.sin6_port;

        if(*name_len <= sizeof(struct sockaddr_in)) {
            /* Passed in a structure not big enough so truncate*/
            memcpy(name, &realaddr, *name_len);
            errno = ENOBUFS;
        } else {
            memcpy(name, &realaddr, sizeof(struct sockaddr_in));
            *name_len = sizeof(struct sockaddr_in);
            goto ret_success;
        }
    } else if(sock->domain == AF_INET6) {
        memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
        realaddr6.sin6_family = AF_INET6;
        realaddr6.sin6_addr = sock->remote_addr.sin6_addr;
        realaddr6.sin6_port = sock->remote_addr.sin6_port;

        if(*name_len <= sizeof(struct sockaddr_in6)) {
            /* Passed in a structure not big enough */
            memcpy(name, &realaddr6, *name_len);
            errno = ENOBUFS;
        } else {
            memcpy(name, &realaddr6, sizeof(struct sockaddr_in6));
            *name_len = sizeof(struct sockaddr_in6);
            goto ret_success;
        }
    }

    mutex_unlock(&udp_mutex);
    errno = ENOTSOCK;
    return -1;

ret_success:
    mutex_unlock(&udp_mutex);
    return 0;
}

static int net_udp_fcntl(net_socket_t *hnd, int cmd, va_list ap) {
    struct udp_sock *sock;
    long val;
    int rv = -1;

    if(mutex_lock_irqsafe(&udp_mutex))
        return -1;

    if(!(sock = (struct udp_sock *)hnd->data)) {
        mutex_unlock(&udp_mutex);
        errno = EBADF;
        return -1;
    }

    switch(cmd) {
        case F_SETFL:
            val = va_arg(ap, long);

            if(val & O_NONBLOCK)
                sock->flags |= FS_SOCKET_NONBLOCK;
            else
                sock->flags &= ~FS_SOCKET_NONBLOCK;

            rv = 0;
            goto out;

        case F_GETFL:
            rv = O_RDWR;

            if(sock->flags & FS_SOCKET_NONBLOCK)
                rv |= O_NONBLOCK;

            goto out;

        case F_GETFD:
        case F_SETFD:
            rv = 0;
            goto out;
    }

    errno = EINVAL;

out:
    mutex_unlock(&udp_mutex);
    return rv;
}

static short net_udp_poll(net_socket_t *hnd, short events) {
    struct udp_sock *sock;
    short rv = POLLWRNORM;

    if(mutex_lock_irqsafe(&udp_mutex))
        return 0;

    if(!(sock = (struct udp_sock *)hnd->data)) {
        mutex_unlock(&udp_mutex);
        return POLLNVAL;
    }

    if(!TAILQ_EMPTY(&sock->packets))
        rv |= POLLRDNORM;

    mutex_unlock(&udp_mutex);

    return rv & events;
}

extern void __poll_event_trigger(int fd, short event);

static int net_udp_input4(netif_t *src, const ip_hdr_t *ip, const uint8 *data,
                          size_t size) {
    udp_hdr_t *hdr = (udp_hdr_t *)data;
    uint16 cs, cscov = 0;
    int partial = 1;
    struct udp_sock *sock;
    struct udp_pkt *pkt;

    (void)src;

    if(size <= sizeof(udp_hdr_t)) {
        /* Discard the packet, since it is too short to be of any interest. */
        ++udp_stats.pkt_recv_bad_size;
        return -1;
    }

    if(ip->protocol == IPPROTO_UDP) {
        /* Calculate the checksum if one was computed by the sender.
           Unfortunately, with IPv4, we don't know if a zero checksum means that
           the sender didn't calculate the checksum or if it actually came out
           as 0xFFFF. We pretty much have to assume the former option though. */
        if(hdr->checksum != 0) {
            cs = net_ipv4_checksum_pseudo(ip->src, ip->dest, IPPROTO_UDP, size);

            /* If the checksum is right, we'll get zero back from the checksum
               function */
            if(net_ipv4_checksum(data, size, cs)) {
                /* The checksum was wrong, bail out */
                ++udp_stats.pkt_recv_bad_chksum;
                return -1;
            }
        }
    }
    else {
        cscov = ntohs(hdr->length);

        /* Make sure the checksum coverage is sane. */
        if(cscov && (cscov < 8 || cscov > size)) {
            ++udp_stats.pkt_recv_bad_chksum;
            return -1;
        }
        else if(!cscov) {
            cscov = size;
            partial = 0;
        }
        else if(cscov == size) {
            partial = 0;
        }

        cs = net_ipv4_checksum_pseudo(ip->src, ip->dest, IPPROTO_UDPLITE, size);

        /* If the checksum is right, we'll get zero back from the checksum
           function. */
        if(net_ipv4_checksum(data, cscov, cs)) {
            ++udp_stats.pkt_recv_bad_chksum;
            return -1;
        }
    }

    if(mutex_lock_irqsafe(&udp_mutex))
        /* If the mutex is locked, there isn't much that can be done. */
        return -1;

    LIST_FOREACH(sock, &net_udp_sockets, sock_list) {
        /* Don't even bother looking at IPv6-only sockets */
        if(sock->domain == AF_INET6 && (sock->flags & FS_SOCKET_V6ONLY))
            continue;

        /* If the ports don't match, don't look any further */
        if(sock->local_addr.sin6_port != hdr->dst_port)
            continue;

        /* If the socket has a remote port set and it isn't the one that this
           packet came from, bail */
        if(sock->remote_addr.sin6_port != 0 &&
           sock->remote_addr.sin6_port != hdr->src_port)
            continue;

        /* If we have a address specified, and its not v4-mapped or its not the
           address this packet came from, bail out */
        if(!IN6_IS_ADDR_UNSPECIFIED(&sock->remote_addr.sin6_addr) &&
           (!IN6_IS_ADDR_V4MAPPED(&sock->remote_addr.sin6_addr) ||
            sock->remote_addr.sin6_addr.__s6_addr.__s6_addr32[3] != ip->src))
            continue;

        /* Make sure we have the right protocol */
        if(sock->proto != ip->protocol)
            continue;

        /* If this packet is UDP-Lite, make sure the checksum coverage is valid
           for the socket. We have to be careful here not to reject packets with
           full coverage that just happen to be smaller than the coverage set by
           the userspace program. Note that failing this check DOES NOT change
           any of the statistics counters at all, by design. */
        if((sock->int_flags & UDPSOCK_LITE_RCVCOV) && partial &&
           cscov < sock->udp_lite.recv_cscov) {
            /* Silently drop packets that fail the partial coverage check. */
            mutex_unlock(&udp_mutex);
            return 0;
        }

        if(!(pkt = (struct udp_pkt *)malloc(sizeof(struct udp_pkt)))) {
            mutex_unlock(&udp_mutex);
            return -1;
        }

        memset(pkt, 0, sizeof(struct udp_pkt));

        pkt->datasize = size - sizeof(udp_hdr_t);

        if(!(pkt->data = (uint8 *)malloc(pkt->datasize))) {
            free(pkt);
            mutex_unlock(&udp_mutex);
            return -1;
        }

        pkt->from.sin6_family = AF_INET6;
        pkt->from.sin6_addr.__s6_addr.__s6_addr16[5] = 0xFFFF;
        pkt->from.sin6_addr.__s6_addr.__s6_addr32[3] = ip->src;
        pkt->from.sin6_port = hdr->src_port;

        memcpy(pkt->data, data + sizeof(udp_hdr_t), pkt->datasize);

        TAILQ_INSERT_TAIL(&sock->packets, pkt, pkt_queue);

        ++udp_stats.pkt_recv;
        __poll_event_trigger(sock->sock, POLLRDNORM);
        genwait_wake_one(sock);
        mutex_unlock(&udp_mutex);

        return 0;
    }

    ++udp_stats.pkt_recv_no_sock;
    mutex_unlock(&udp_mutex);

    return -1;
}

static int net_udp_input6(netif_t *src, const ipv6_hdr_t *ip, const uint8 *data,
                          size_t size) {
    udp_hdr_t *hdr = (udp_hdr_t *)data;
    uint16 cs, cscov = 0;
    int partial = 1;
    struct udp_sock *sock;
    struct udp_pkt *pkt;

    (void)src;

    if(size <= sizeof(udp_hdr_t)) {
        /* Discard the packet, since it is too short to be of any interest. */
        ++udp_stats.pkt_recv_bad_size;
        return -1;
    }

    if(ip->next_header == IPPROTO_UDP) {
        /* Calculate the checksum of the packet. Note that this is optional for
           IPv4 but required for IPv6. */
        cs = net_ipv6_checksum_pseudo(&ip->src_addr, &ip->dst_addr, size,
                                      IPPROTO_UDP);

        /* If the checksum is right, we'll get zero back from the checksum
           function. */
        if(net_ipv4_checksum(data, size, cs)) {
            /* The checksum was wrong, bail out */
            ++udp_stats.pkt_recv_bad_chksum;
            return -1;
        }
    }
    else {
        cscov = ntohs(hdr->length);

        /* Make sure the checksum coverage is sane. */
        if(cscov && (cscov < 8 || cscov > size)) {
            ++udp_stats.pkt_recv_bad_chksum;
            return -1;
        }
        else if(!cscov) {
            cscov = size;
            partial = 0;
        }
        else if(cscov == size) {
            partial = 0;
        }

        cs = net_ipv6_checksum_pseudo(&ip->src_addr, &ip->dst_addr, size,
                                      IPPROTO_UDPLITE);

        /* If the checksum is right, we'll get zero back from the checksum
           function. */
        if(net_ipv4_checksum(data, cscov, cs)) {
            ++udp_stats.pkt_recv_bad_chksum;
            return -1;
        }
    }

    if(mutex_lock_irqsafe(&udp_mutex))
        /* If the mutex is locked, there isn't much that can be done. */
        return -1;

    LIST_FOREACH(sock, &net_udp_sockets, sock_list) {
        /* Don't even bother looking at IPv4 sockets */
        if(sock->domain == AF_INET)
            continue;

        /* If the ports don't match, don't look any further */
        if(sock->local_addr.sin6_port != hdr->dst_port)
            continue;

        /* If the socket has a remote port set and it isn't the one that this
           packet came from, bail */
        if(sock->remote_addr.sin6_port != 0 &&
           sock->remote_addr.sin6_port != hdr->src_port)
            continue;

        /* If we have a address specified and it is not the address this packet
           came from, bail out */
        if(!IN6_IS_ADDR_UNSPECIFIED(&sock->remote_addr.sin6_addr) &&
           memcmp(&sock->remote_addr.sin6_addr, &ip->src_addr,
                  sizeof(struct in6_addr)))
            continue;

        /* Make sure we have the right protocol */
        if(sock->proto != ip->next_header)
            continue;

        /* If this packet is UDP-Lite, make sure the checksum coverage is valid
           for the socket. We have to be careful here not to reject packets with
           full coverage that just happen to be smaller than the coverage set by
           the userspace program. Note that failing this check DOES NOT change
           any of the statistics counters at all, by design. */
        if((sock->int_flags & UDPSOCK_LITE_RCVCOV) && partial &&
           cscov < sock->udp_lite.recv_cscov) {
            /* Silently drop packets that fail the partial coverage check. */
            mutex_unlock(&udp_mutex);
            return 0;
        }

        if(!(pkt = (struct udp_pkt *)malloc(sizeof(struct udp_pkt)))) {
            mutex_unlock(&udp_mutex);
            return -1;
        }

        memset(pkt, 0, sizeof(struct udp_pkt));

        pkt->datasize = size - sizeof(udp_hdr_t);

        if(!(pkt->data = (uint8 *)malloc(pkt->datasize))) {
            free(pkt);
            mutex_unlock(&udp_mutex);
            return -1;
        }

        pkt->from.sin6_family = AF_INET6;
        pkt->from.sin6_addr = ip->src_addr;
        pkt->from.sin6_port = hdr->src_port;

        memcpy(pkt->data, data + sizeof(udp_hdr_t), pkt->datasize);

        TAILQ_INSERT_TAIL(&sock->packets, pkt, pkt_queue);

        ++udp_stats.pkt_recv;
        __poll_event_trigger(sock->sock, POLLRDNORM);
        genwait_wake_one(sock);
        mutex_unlock(&udp_mutex);

        return 0;
    }

    ++udp_stats.pkt_recv_no_sock;
    mutex_unlock(&udp_mutex);

    return -1;
}

static int net_udp_input(netif_t *src, int domain, const void *hdr,
                         const uint8 *data, size_t size) {
    switch(domain) {
        case AF_INET:
            return net_udp_input4(src, (const ip_hdr_t *)hdr, data, size);

        case AF_INET6:
            return net_udp_input6(src, (const ipv6_hdr_t *)hdr, data, size);
    }

    return -1;
}

/* XXX */
static int net_udp_send_raw(netif_t *net, const struct sockaddr_in6 *src,
                            const struct sockaddr_in6 *dst, const uint8 *data,
                            size_t size, uint32_t flags, int hops,
                            uint32_t iflags, int proto, uint16_t cscov) {
    uint8 buf[size + sizeof(udp_hdr_t)];
    udp_hdr_t *hdr = (udp_hdr_t *)buf;
    uint16 cs;
    int err;
    struct in6_addr srcaddr = src->sin6_addr;

    (void)flags;

    if(!net) {
        net = net_default_dev;

        if(!net) {
            errno = ENETDOWN;
            ++udp_stats.pkt_send_failed;
            return -1;
        }
    }

    if(IN6_IS_ADDR_UNSPECIFIED(&src->sin6_addr)) {
        if(IN6_IS_ADDR_V4MAPPED(&dst->sin6_addr)) {
            srcaddr.__s6_addr.__s6_addr16[5] = 0xFFFF;
            srcaddr.__s6_addr.__s6_addr32[3] =
                htonl(net_ipv4_address(net->ip_addr));
        }
        else {
            if(IN6_IS_ADDR_LOOPBACK(&dst->sin6_addr)) {
                srcaddr = in6addr_loopback;
            }
            else if(IN6_IS_ADDR_LINKLOCAL(&dst->sin6_addr) ||
                    IN6_IS_ADDR_MC_LINKLOCAL(&dst->sin6_addr)) {
                srcaddr = net->ip6_lladdr;
            }
            else if(net->ip6_addr_count) {
                /* Punt and pick the first non-link-local address */
                srcaddr = net->ip6_addrs[0];
            }
            else {
                errno = ENETDOWN;
                ++udp_stats.pkt_send_failed;
                return -1;
            }
        }
    }

    memcpy(buf + sizeof(udp_hdr_t), data, size);
    size += sizeof(udp_hdr_t);

    hdr->src_port = src->sin6_port;
    hdr->dst_port = dst->sin6_port;
    hdr->checksum = 0;

    /* Is this UDP or UDP-Lite? */
    if(proto == IPPROTO_UDP) {
        hdr->length = htons(size);

        if(!(iflags & UDPSOCK_NO_CHECKSUM)) {
            cs = net_ipv6_checksum_pseudo(&srcaddr, &dst->sin6_addr, size,
                                          proto);
            hdr->checksum = net_ipv4_checksum(buf, size, cs);
        }
    }
    else {
        if(cscov <= size) {
            hdr->length = htons(cscov);
        }
        else {
            hdr->length = 0;
            cscov = size;
        }

        cs = net_ipv6_checksum_pseudo(&srcaddr, &dst->sin6_addr, size, proto);
        hdr->checksum = net_ipv4_checksum(buf, size, cs);
    }

    /* Pass everything off to the network layer to do the rest. */
    err = net_ipv6_send(net, buf, size, hops, proto, &srcaddr,
                        &dst->sin6_addr);

    if(err < 0) {
        ++udp_stats.pkt_send_failed;
        return -1;
    }
    else {
        ++udp_stats.pkt_sent;
        return size - sizeof(udp_hdr_t);
    }
}

net_udp_stats_t net_udp_get_stats(void) {
    return udp_stats;
}

/* Protocol handler for fs_socket. */
static fs_socket_proto_t proto = {
    FS_SOCKET_PROTO_ENTRY,
    PF_INET6,                           /* domain */
    SOCK_DGRAM,                         /* type */
    IPPROTO_UDP,                        /* protocol */
    net_udp_socket,
    net_udp_close,
    net_udp_accept,
    net_udp_bind,
    net_udp_connect,
    net_udp_listen,
    net_udp_recvfrom,
    net_udp_sendto,
    net_udp_shutdownsock,
    net_udp_input,
    net_udp_getsockopt,
    net_udp_setsockopt,
    net_udp_getsockname,
    net_udp_getpeername,
    net_udp_fcntl,
    net_udp_poll
};

static fs_socket_proto_t proto_lite = {
    FS_SOCKET_PROTO_ENTRY,
    PF_INET6,                           /* domain */
    SOCK_DGRAM,                         /* type */
    IPPROTO_UDPLITE,                    /* protocol */
    net_udp_socket,
    net_udp_close,
    net_udp_accept,
    net_udp_bind,
    net_udp_connect,
    net_udp_listen,
    net_udp_recvfrom,
    net_udp_sendto,
    net_udp_shutdownsock,
    net_udp_input,
    net_udp_getsockopt,
    net_udp_setsockopt,
    net_udp_getsockname,
    net_udp_getpeername,
    net_udp_fcntl,
    net_udp_poll
};

int net_udp_init(void) {
    return fs_socket_proto_add(&proto) | fs_socket_proto_add(&proto_lite);
}

void net_udp_shutdown(void) {
    fs_socket_proto_remove(&proto);
    fs_socket_proto_remove(&proto_lite);
}

#if __GNUC__ >= 9
#pragma GCC diagnostic pop
#endif
