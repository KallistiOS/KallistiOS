/* KallistiOS ##version##

   kernel/net/net_tcp.c
   Copyright (C) 2012, 2013 Lawrence Sebald

*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include <kos/fs.h>
#include <kos/net.h>
#include <kos/cond.h>
#include <kos/mutex.h>
#include <kos/rwsem.h>
#include <kos/fs_socket.h>

#include <arch/timer.h>

#include <kos/dbglog.h>

#include "net_ipv4.h"
#include "net_ipv6.h"
#include "net_thd.h"

/* Since some of this is a bit odd in its implementation, here's a few notes on
   what my thinking was while writing all of this...

   On IRQs:
   All of the functions in here should be relatively IRQ-safe. That doesn't mean
   its a particularly good idea to use sockets all over the place in IRQs, but
   it is possible. I *strongly* recommend not closing sockets in an IRQ, since
   there are ways that can fail if it is done during an IRQ. I also recommend
   not calling socket(), listen(), or accept() in an IRQ. All of these functions
   allocate memory, which is not exactly the best idea in an IRQ for various
   reasons. Any functions called in an IRQ may return error and set errno to
   EWOULDBLOCK, even if they are not on non-blocking sockets. All socket calls
   in an IRQ context *MUST* be non-blocking, since we don't have any way to
   suspend an IRQ while its being processed.

   On locking:
   I took a two-leveled locking approach when writing all of this code. The
   first-level of locking is on the list of sockets itself. This is done with a
   reader/writer semaphore. If the function in question will ever modify the
   list of sockets in any way, it acquires the write lock. Otherwise, it will
   always acquire the read lock. The second level of locking is on the
   individual socket level. This is done with a standard mutex. When looking at
   an individual socket, grab that mutex in addition to the read or write lock,
   as is appropriate. The only function that is somewhat counter-intuitive in
   its locking is bind(). The bind() function, even though it does not modify
   the list itself, does grab the write lock. The reason for this is fairly
   simple. If bind() were to grab the read lock, then iterate through the list
   of sockets looking for duplicate binds, there is a possibility that a second
   call to bind() in another thread could cause a deadlock due to the locking
   and unlocking of the mutexes on the individual sockets. To prevent this
   problem, I have it grab the write lock instead. That way, no two bind() calls
   can be active at a time.

   On listening:
   When a connection comes in for a socket that is in the listening state, that
   connection will have some state saved about it and it will be added to the
   listening socket's queue for listening connections, assuming there is space
   for it in there. Otherwise, it'll simply be reset, as should be expected.
   The positive side effect of this is that all sockets should be created
   outside of IRQ context (unless you're crazy enough to make one in an IRQ
   handler in your own code), since incoming connections do not actually have a
   real socket created for them until they are accept()ed.

   On matching sockets:
   All new sockets (including those created by accept()) are added to the head
   of the list. Since we cannot bind a socket to an already used port for
   listening, that means that any fully-created sockets should appear in the
   list in front of those created for listening to a port (and thus that are
   only partially-created). This simplifies the procedure for finding matching
   sockets for incoming packets, since this makes sure that the fully-created
   socket will be found first if it exists when simply iterating through the
   list of sockets.

   On what's actually here:
   I didn't bother implementing any TCP extensions beyond RFC 793. That means
   that I just ignore things like the timestamp option and the selective
   acknowledgement option. That also means that the window size maxes out at
   65535. Some extensions may be implemented in the future, if I see fit to do
   so. That all said, everything in here works just fine over IPv4 or IPv6, and
   can be used just fine to communicate with "normal" TCP/IP implementations.
*/

typedef struct tcp_hdr {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint16_t off_flags;
    uint16_t wnd;
    uint16_t checksum;
    uint16_t urg;
    uint8_t options[];
} __packed tcp_hdr_t;

/* Listening socket. Each one of these is an incoming connection from a socket
   that is in the listen state */
struct lsock {
    netif_t *net;
    struct sockaddr_in6 local_addr;
    struct sockaddr_in6 remote_addr;
    uint32_t isn;
    uint32_t wnd;
    uint16_t mss;
};

/* Send/receive variables... */
struct sndrec {
    uint32_t una;
    uint32_t nxt;
    uint32_t wnd;
    uint32_t up;
    uint32_t wl1;
    uint32_t wl2;
    uint32_t iss;
    uint16_t mss;
};

struct rcvrec {
    uint32_t nxt;
    uint32_t wnd;
    uint32_t up;
    uint32_t irs;
};

struct tcp_sock {
    LIST_ENTRY(tcp_sock) sock_list;
    struct sockaddr_in6 local_addr;
    struct sockaddr_in6 remote_addr;

    uint32_t flags;
    uint32_t intflags;
    int domain;
    file_t sock;
    int state;
    mutex_t mutex;
    int hop_limit;
    uint32_t rcvbuf_sz;
    uint32_t sndbuf_sz;

    union {
        struct {
            int backlog;
            int head;
            int tail;
            int count;
            struct lsock *queue;
            condvar_t cv;
        } listen;
        struct {
            netif_t *net;
            struct sndrec snd;
            struct rcvrec rcv;
            uint8_t *rcvbuf;
            uint32_t rcvbuf_cur_sz;
            uint32_t rcvbuf_head;
            uint32_t rcvbuf_tail;
            uint8_t *sndbuf;
            uint32_t sndbuf_cur_sz;
            uint32_t sndbuf_head;
            uint32_t sndbuf_acked;
            uint32_t sndbuf_tail;
            uint64_t timer;
            condvar_t send_cv;
            condvar_t recv_cv;
        } data;
    };
};

LIST_HEAD(tcp_sock_list, tcp_sock);

static struct tcp_sock_list tcp_socks = LIST_HEAD_INITIALIZER(0);
static rw_semaphore_t tcp_sem = RWSEM_INITIALIZER;
static int thd_cb_id = 0;

/* Default starting window size for connections. This should be big enough as a
   starting point, in general. If you need to adjust it, you can do so... */
#define TCP_DEFAULT_WINDOW  8192

/* Default MSS */
#define TCP_DEFAULT_MSS     1460

/* Default Maximum Segment Lifetime (in milliseconds). I arbitrarily chose this
   to be 15 seconds, since that's what Mac OS X does. */
#define TCP_DEFAULT_MSL     15000

/* Default retransmission timeout (in milliseconds). */
#define TCP_DEFAULT_RTTO    2000

/* Default hop limit (or ttl for IPv4) for new sockets */
#define TCP_DEFAULT_HOPS    64

/* Flags that can be set in the off_flags field of the above struct */
#define TCP_FLAG_FIN    0x01
#define TCP_FLAG_SYN    0x02
#define TCP_FLAG_RST    0x04
#define TCP_FLAG_PSH    0x08
#define TCP_FLAG_ACK    0x10
#define TCP_FLAG_URG    0x20

#define TCP_GET_OFFSET(x)   (((x) & 0xF000) >> 10)
#define TCP_OFFSET(y)       (((y) & 0x0F) << 12)

#define TCP_STATE_CLOSED        0
#define TCP_STATE_LISTEN        1
#define TCP_STATE_SYN_SENT      2
#define TCP_STATE_SYN_RECEIVED  3
#define TCP_STATE_ESTABLISHED   4
#define TCP_STATE_FIN_WAIT_1    5
#define TCP_STATE_FIN_WAIT_2    6
#define TCP_STATE_CLOSE_WAIT    7
#define TCP_STATE_CLOSING       8
#define TCP_STATE_LAST_ACK      9
#define TCP_STATE_TIME_WAIT     10

#define TCP_STATE_RESET         0x80000000
#define TCP_STATE_ACCEPTING     0x40000000

/* Internal flags */
#define TCP_IFLAG_CANBEDEL      0x00000001
#define TCP_IFLAG_QUEUEDCLOSE   0x00000002
#define TCP_IFLAG_ACCEPTWAIT    0x00000004

#define TCP_OPT_EOL             0
#define TCP_OPT_NOP             1
#define TCP_OPT_MSS             2

/* A few macros for comparing sequence numbers */
#define SEQ_LT(x, y)    (((int32_t)((x) - (y))) < 0)
#define SEQ_LE(x, y)    (((int32_t)((x) - (y))) <= 0)
#define SEQ_GT(x, y)    (((int32_t)((x) - (y))) > 0)
#define SEQ_GE(x, y)    (((int32_t)((x) - (y))) >= 0)

#define MAX(x, y)       ((x) > (y) ? (x) : (y))

/* Forward declarations */
static fs_socket_proto_t proto;
static void tcp_rst(netif_t *net, const struct in6_addr *src,
                    const struct in6_addr *dst, uint16_t src_port,
                    uint16_t dst_port, uint16_t flags, uint32_t seq,
                    uint32_t ack);
static int tcp_send_syn(struct tcp_sock *sock, int ack);
static void tcp_send_ack(struct tcp_sock *sock);
static void tcp_send_data(struct tcp_sock *sock, int resend);
static void tcp_send_fin_ack(struct tcp_sock *sock);

/* Sockets interface... */
static int net_tcp_socket(net_socket_t *hnd, int domain, int type, int proto) {
    struct tcp_sock *sock;

    (void)type;
    (void)proto;

    if(!(sock = (struct tcp_sock *)malloc(sizeof(struct tcp_sock)))) {
        errno = ENOMEM;
        return -1;
    }

    memset(sock, 0, sizeof(struct tcp_sock));

    if(mutex_init(&sock->mutex, MUTEX_TYPE_NORMAL)) {
        errno = ENOMEM;
        free(sock);
        return -1;
    }

    sock->domain = domain;
    sock->sock = hnd->fd;
    sock->hop_limit = TCP_DEFAULT_HOPS;
    sock->rcvbuf_sz = TCP_DEFAULT_WINDOW;
    sock->sndbuf_sz = TCP_DEFAULT_WINDOW;

    if(rwsem_write_lock_irqsafe(&tcp_sem)) {
        free(sock);
        return -1;
    }

    hnd->data = sock;

    LIST_INSERT_HEAD(&tcp_socks, sock, sock_list);
    rwsem_write_unlock(&tcp_sem);

    return 0;
}

static struct tcp_sock *
net_tcp_write_lock_and_get_sock(net_socket_t *hnd, rw_semaphore_t *sem) {
    struct tcp_sock *sock;

    if(rwsem_write_lock_irqsafe(sem))
        return NULL;

    if(!(sock = (struct tcp_sock *)hnd->data)) {
        rwsem_write_unlock(sem);
        errno = EBADF;
        return NULL;
    }

    if(mutex_lock_irqsafe(&sock->mutex)) {
        rwsem_write_unlock(sem);
        return NULL;
    }

    return sock;
}

static struct tcp_sock *
net_tcp_read_lock_and_get_sock(net_socket_t *hnd, rw_semaphore_t *sem) {
    struct tcp_sock *sock;

    if(rwsem_read_lock_irqsafe(sem))
        return NULL;

    if(!(sock = (struct tcp_sock *)hnd->data)) {
        rwsem_read_unlock(sem);
        errno = EBADF;
        return NULL;
    }

    if(mutex_lock_irqsafe(&sock->mutex)) {
        rwsem_read_unlock(sem);
        return NULL;
    }

    return sock;
}

static void net_tcp_close(net_socket_t *hnd) {
    struct tcp_sock *sock;
    struct lsock *ls;
    int i;

retry:
    if(!(sock = net_tcp_write_lock_and_get_sock(hnd, &tcp_sem)))
        return;

    /* This is to work-around the ugly issue discussed in the accept() function.
       This is... well, bad... Granted, you really shouldn't ever have this
       happening if you're sane... */
    if(sock->state == (TCP_STATE_LISTEN | TCP_STATE_ACCEPTING)) {
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);

        if(irq_inside_int()) {
            errno = EWOULDBLOCK;
            return;
        }

        thd_pass();
        goto retry;
    }

    /* Deal with queued data and/or connections and sending the closing messages
       as appropriate. */
    switch(sock->state) {
        case TCP_STATE_LISTEN:

            for(i = sock->listen.head; i < sock->listen.tail; ++i) {
                ls = sock->listen.queue + i;

                /* Reset the connection */
                tcp_rst(ls->net, &ls->local_addr.sin6_addr,
                        &ls->remote_addr.sin6_addr, ls->local_addr.sin6_port,
                        ls->remote_addr.sin6_port, TCP_FLAG_ACK | TCP_FLAG_RST,
                        0, ls->isn + 1);
            }

            /* If we were waiting on an accept call, then we have to let it
               handle tearing down the connection... */
            if(sock->intflags & TCP_IFLAG_ACCEPTWAIT) {
                sock->state = TCP_STATE_CLOSED;
                goto ret_no_remove;
            }

            free(sock->listen.queue);
            cond_destroy(&sock->listen.cv);
            goto ret_remove;

        case TCP_STATE_SYN_SENT:
            free(sock->data.rcvbuf);
            free(sock->data.sndbuf);
            cond_destroy(&sock->data.send_cv);
            cond_destroy(&sock->data.recv_cv);
            goto ret_remove;

        case TCP_STATE_ESTABLISHED:

            /* See if all sends have finished... */
            if(sock->data.sndbuf_cur_sz) {
                goto ret_no_remove;
            }

            /* Fall through... */
            __fallthrough;

        case TCP_STATE_SYN_RECEIVED:
            /* Don't have to worry about queued packets, since we don't allow
               any queueing until after the connection is established. */
            tcp_send_fin_ack(sock);
            ++sock->data.snd.nxt;
            sock->state = TCP_STATE_FIN_WAIT_1;
            goto ret_no_remove;

        case TCP_STATE_CLOSE_WAIT:

            /* See if all sends have finished... */
            if(sock->data.sndbuf_cur_sz) {
                goto ret_no_remove;
            }

            tcp_send_fin_ack(sock);
            ++sock->data.snd.nxt;
            sock->state = TCP_STATE_CLOSING;
            goto ret_no_remove;

        case TCP_STATE_CLOSED | TCP_STATE_RESET:
            goto ret_no_remove;

        case TCP_STATE_FIN_WAIT_1:
        case TCP_STATE_FIN_WAIT_2:
            goto ret_no_remove;

        case TCP_STATE_CLOSING:
        case TCP_STATE_LAST_ACK:
        case TCP_STATE_TIME_WAIT:
            /* Uh... shouldn't get here... */
            dbglog(DBG_KDEBUG, "close() on TCP socket in invalid state!\n");
            goto ret_no_remove;
    }

ret_remove:
    LIST_REMOVE(sock, sock_list);
    mutex_unlock(&sock->mutex);
    mutex_destroy(&sock->mutex);
    free(sock);

    rwsem_write_unlock(&tcp_sem);
    return;

ret_no_remove:
    if(sock->state != TCP_STATE_LISTEN)
        sock->intflags = TCP_IFLAG_CANBEDEL;

    if(sock->state == TCP_STATE_ESTABLISHED ||
            sock->state == TCP_STATE_CLOSE_WAIT)
        sock->intflags |= TCP_IFLAG_QUEUEDCLOSE;

    sock->sock = -1;

    /* Don't free anything here, it will be dealt with later on in the
       net_thd callback. */
    mutex_unlock(&sock->mutex);
    rwsem_write_unlock(&tcp_sem);
    return;
}

static int net_tcp_accept(net_socket_t *hnd, struct sockaddr *addr,
                          socklen_t *addr_len) {
    struct tcp_sock *sock, *sock2;
    net_socket_t *newhnd;
    struct lsock lsock;
    int canblock = 1;
    int fd;

    if(addr != NULL && addr_len == NULL) {
        errno = EFAULT;
        return -1;
    }

    if(!(sock = net_tcp_read_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

    canblock = !irq_inside_int()
        && !(sock->flags & FS_SOCKET_NONBLOCK);

    rwsem_read_unlock(&tcp_sem);

    /* Make sure the socket is listening... */
    if(sock->state != TCP_STATE_LISTEN) {
        errno = EINVAL;
        mutex_unlock(&sock->mutex);
        return -1;
    }

    /* See if there are any waiting connections... */
    while(!sock->listen.count) {
        if(!canblock) {
            errno = EWOULDBLOCK;
            mutex_unlock(&sock->mutex);
            return -1;
        }

        /* There are no waiting connections and we can block, so block while we
           wait for an incoming connection. */
        sock->intflags |= TCP_IFLAG_ACCEPTWAIT;
        cond_wait(&sock->listen.cv, &sock->mutex);

        /* If we come out of the wait in the closed state, that means that the
           user has run a close() on the socket in another thread. Bail out in a
           graceful fashion. */
        if(sock->state == TCP_STATE_CLOSED) {
            mutex_unlock(&sock->mutex);
            rwsem_write_lock(&tcp_sem);
            mutex_lock(&sock->mutex);
            free(sock->listen.queue);
            cond_destroy(&sock->listen.cv);
            LIST_REMOVE(sock, sock_list);
            mutex_unlock(&sock->mutex);
            mutex_destroy(&sock->mutex);
            free(sock);

            rwsem_write_unlock(&tcp_sem);

            errno = EINTR;              /* Close enough, I suppose. */
            return -1;
        }
    }

    /* We now have a connection to use, so, lets grab it and release the lock on
       the old socket. */
    lsock = sock->listen.queue[sock->listen.head++];
    --sock->listen.count;

    if(sock->listen.head == sock->listen.backlog)
        sock->listen.head = 0;

    /* Allocate the memory we will need... */
    if(!(sock2 = (struct tcp_sock *)malloc(sizeof(struct tcp_sock)))) {
        mutex_unlock(&sock->mutex);
        errno = ENOMEM;
        return -1;
    }

    memset(sock2, 0, sizeof(struct tcp_sock));

    if(mutex_init(&sock2->mutex, MUTEX_TYPE_NORMAL)) {
        mutex_unlock(&sock->mutex);
        errno = ENOMEM;
        free(sock2);
        return -1;
    }

    if(!(sock2->data.rcvbuf = (uint8_t *)malloc(sock->rcvbuf_sz))) {
        errno = ENOMEM;
        mutex_unlock(&sock->mutex);
        mutex_destroy(&sock2->mutex);
        free(sock2);
        return -1;
    }

    if(!(sock2->data.sndbuf = (uint8_t *)malloc(sock->sndbuf_sz))) {
        errno = ENOMEM;
        mutex_unlock(&sock->mutex);
        free(sock2->data.rcvbuf);
        mutex_destroy(&sock2->mutex);
        free(sock2);
        return -1;
    }

    if(cond_init(&sock2->data.send_cv)) {
        errno = ENOMEM;
        mutex_unlock(&sock->mutex);
        free(sock2->data.sndbuf);
        free(sock2->data.rcvbuf);
        mutex_destroy(&sock2->mutex);
        free(sock2);
        return -1;
    }

    if(cond_init(&sock2->data.recv_cv)) {
        errno = ENOMEM;
        mutex_unlock(&sock->mutex);
        cond_destroy(&sock2->data.send_cv);
        free(sock2->data.sndbuf);
        free(sock2->data.rcvbuf);
        mutex_destroy(&sock2->mutex);
        free(sock2);
        return -1;
    }

    /* Create a partial socket */
    if(!(newhnd = fs_socket_open_sock(&proto))) {
        mutex_unlock(&sock->mutex);
        cond_destroy(&sock2->data.recv_cv);
        cond_destroy(&sock2->data.send_cv);
        free(sock2->data.sndbuf);
        free(sock2->data.rcvbuf);
        mutex_destroy(&sock2->mutex);
        free(sock2);
        return -1;
    }

    /* Fill in the important parts */
    sock2->domain = sock->domain;
    sock2->sock = newhnd->fd;
    sock2->state = TCP_STATE_SYN_RECEIVED;
    sock2->local_addr = lsock.local_addr;
    sock2->remote_addr = lsock.remote_addr;
    sock2->hop_limit = sock->hop_limit;
    sock2->rcvbuf_sz = sock->rcvbuf_sz;
    sock2->sndbuf_sz = sock->sndbuf_sz;
    sock2->data.rcv.wnd = sock->rcvbuf_sz;

    /* Fill in the address, if they asked for it. */
    if(addr != NULL) {
        if(sock2->domain == AF_INET) {
            struct sockaddr_in realaddr;

            memset(&realaddr, 0, sizeof(struct sockaddr_in));
            realaddr.sin_family = AF_INET;
            realaddr.sin_addr.s_addr =
                sock2->remote_addr.sin6_addr.__s6_addr.__s6_addr32[3];
            realaddr.sin_port = sock2->remote_addr.sin6_port;

            if(*addr_len < sizeof(struct sockaddr_in)) {
                memcpy(addr, &realaddr, *addr_len);
            }
            else {
                memcpy(addr, &realaddr, sizeof(struct sockaddr_in));
                *addr_len = sizeof(struct sockaddr_in);
            }
        }
        else if(sock2->domain == AF_INET6) {
            struct sockaddr_in6 realaddr6;

            memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
            realaddr6.sin6_family = AF_INET6;
            realaddr6.sin6_addr = sock2->remote_addr.sin6_addr;
            realaddr6.sin6_port = sock2->remote_addr.sin6_port;

            if(*addr_len < sizeof(struct sockaddr_in6)) {
                memcpy(addr, &realaddr6, *addr_len);
            }
            else {
                memcpy(addr, &realaddr6, sizeof(struct sockaddr_in6));
                *addr_len = sizeof(struct sockaddr_in6);
            }
        }
    }

    if(irq_inside_int()) {
        if(rwsem_write_trylock(&tcp_sem)) {
            /* Kabuki dance to clean things up... */
            mutex_unlock(&sock->mutex);

            newhnd->protocol = NULL;
            fs_close(sock2->sock);
            cond_destroy(&sock2->data.recv_cv);
            cond_destroy(&sock2->data.send_cv);
            free(sock2->data.sndbuf);
            free(sock2->data.rcvbuf);
            mutex_destroy(&sock2->mutex);
            free(sock2);
            errno = EWOULDBLOCK;
            return -1;
        }
    }
    /* Ugh... I really need to find a better way to deal with this... There is
       a possibility (however slight) that between here and when the write lock
       is locked that we might get a SYN packet again for this connection that
       might end up screwing something up later. Honestly, I can't think of any
       non-hackish way of dealing with this that would not result in a possible
       deadlock.

       Anyone have any ideas? The only two possibilities that I can think of are
       either doing the actual work of building sockets in the IRQ when the
       listening socket gets the incoming SYN or holding the write lock through
       this whole function. Both open up their own cans of worms, so to speak...
       The first one means we need to be able to do socket() calls reliably in
       an IRQ, which I really don't want to touch. The second would essentially
       mean that during an accept() where we end up blocking waiting for an
       incoming connection, no other socket write operations can take place.
       That is absolutely unacceptable. Locking the write lock right here,
       before releasing the socket lock would create the possibility of a
       deadlock in bind() or close().

       The hacked-up solution here basically means that the socket can't accept
       any more SYNs for queueing until the socket actually gets added to the
       list of sockets... Not really a very good solution, but it should do the
       trick until I can come up with something else to do. Note that this also
       means that calling accept() with the same socket in two threads is
       unacceptable (and the second one will probably end up with an EINVAL
       error because of this). */
    else {
        sock->state |= TCP_STATE_ACCEPTING;
        mutex_unlock(&sock->mutex);
        rwsem_write_lock(&tcp_sem);
        mutex_lock(&sock->mutex);
    }

    newhnd->data = sock2;

    /* Bad way of generating an initial sequence number, but technically correct
       by the wording of the RFC... */
    sock2->data.snd.iss = (uint32_t)(timer_us_gettime64() >> 2);
    sock2->data.snd.nxt = sock2->data.snd.iss + 1;
    sock2->data.snd.una = sock2->data.snd.iss;
    sock2->data.snd.wnd = lsock.wnd;
    sock2->data.snd.wl1 = sock2->data.snd.iss;
    sock2->data.snd.mss = lsock.mss;
    sock2->data.rcv.nxt = lsock.isn + 1;
    sock2->data.rcv.irs = lsock.isn;

    /* Since nothing else has a pointer to this socket, this will not fail. */
    mutex_trylock(&sock2->mutex);

    /* Send the <SYN,ACK> packet now, add it to the list, and clean up. */
    tcp_send_syn(sock2, 1);
    sock2->data.timer = timer_ms_gettime64();
    fd = sock2->sock;
    LIST_INSERT_HEAD(&tcp_socks, sock2, sock_list);
    mutex_unlock(&sock2->mutex);

    sock->state &= ~TCP_STATE_ACCEPTING;
    mutex_unlock(&sock->mutex);
    rwsem_write_unlock(&tcp_sem);

    return fd;
}

static int net_tcp_bind(net_socket_t *hnd, const struct sockaddr *addr,
                        socklen_t addr_len) {
    struct tcp_sock *sock, *iter;
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

    if(!(sock = net_tcp_write_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

    /* Make sure the socket is still in the closed state and hasn't already been
       bound. */
    if(sock->state == TCP_STATE_LISTEN) {
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        errno = EINVAL;
        return -1;
    }
    else if(sock->state != TCP_STATE_CLOSED) {
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        errno = EISCONN;
        return -1;
    }
    else if(sock->local_addr.sin6_port) {
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        errno = EINVAL;
        return -1;
    }

    /* Make sure the address family we're binding to matches that which is set
       on the socket itself */
    if(addr->sa_family != sock->domain) {
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        errno = EINVAL;
        return -1;
    }

    /* See if we requested a specific port or not */
    if(realaddr6.sin6_port != 0) {
        /* Make sure we don't already have a socket bound to the port
           specified */
        LIST_FOREACH(iter, &tcp_socks, sock_list) {
            if(iter == sock)
                continue;

            if(mutex_lock_irqsafe(&iter->mutex)) {
                mutex_unlock(&sock->mutex);
                rwsem_write_unlock(&tcp_sem);
                return -1;
            }

            if(iter->local_addr.sin6_port == realaddr6.sin6_port) {
                mutex_unlock(&iter->mutex);
                mutex_unlock(&sock->mutex);
                rwsem_write_unlock(&tcp_sem);
                errno = EADDRINUSE;
                return -1;
            }

            mutex_unlock(&iter->mutex);
        }

        sock->local_addr = realaddr6;
    }
    else {
        uint16_t port = 1024, tmp = 0;

        /* Grab the first unused port >= 1024. This is, unfortunately, O(n^2) */
        while(tmp != port) {
            tmp = port;

            LIST_FOREACH(iter, &tcp_socks, sock_list) {
                if(iter == sock)
                    continue;

                if(mutex_lock_irqsafe(&iter->mutex)) {
                    mutex_unlock(&sock->mutex);
                    rwsem_write_unlock(&tcp_sem);
                    return -1;
                }

                if(iter->local_addr.sin6_port == port) {
                    ++port;
                    break;
                }

                mutex_unlock(&iter->mutex);
            }
        }

        sock->local_addr = realaddr6;
        sock->local_addr.sin6_port = htons(port);
    }

    /* Release the locks, we're done */
    mutex_unlock(&sock->mutex);
    rwsem_write_unlock(&tcp_sem);

    return 0;
}

static int net_tcp_connect(net_socket_t *hnd, const struct sockaddr *addr,
                           socklen_t addr_len) {
    struct tcp_sock *sock, *iter;
    struct sockaddr_in *realaddr4;
    struct sockaddr_in6 realaddr6;

    if(addr == NULL) {
        errno = EDESTADDRREQ;
        return -1;
    }

    if(!net_default_dev) {
        errno = ENETDOWN;
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

    if(!(sock = net_tcp_write_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

    /* Make sure the socket is still in the CLOSED state */
    if(sock->state != TCP_STATE_CLOSED) {
        if(sock->state == TCP_STATE_LISTEN) {
            errno = EOPNOTSUPP;
        }
        else if(sock->state == TCP_STATE_SYN_SENT) {
            errno = EALREADY;
        }
        else {
            errno = EISCONN;
        }

        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        return -1;
    }

    /* Make sure the address family we're binding to matches that which is set
       on the socket itself */
    if(addr->sa_family != sock->domain) {
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        errno = EINVAL;
        return -1;
    }

    /* Make sure we have a valid address to connect to */
    if(IN6_IS_ADDR_UNSPECIFIED(&realaddr6.sin6_addr) ||
            realaddr6.sin6_port == 0) {
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        errno = EADDRNOTAVAIL;
        return -1;
    }

    /* See if the socket is already bound to a local port */
    if(!sock->local_addr.sin6_port) {
        uint16_t port = 1024, tmp = 0;

        /* Grab the first unused port >= 1024. This is, unfortunately, O(n^2) */
        while(tmp != port) {
            tmp = port;

            LIST_FOREACH(iter, &tcp_socks, sock_list) {
                if(iter == sock)
                    continue;

                if(mutex_lock_irqsafe(&iter->mutex)) {
                    mutex_unlock(&sock->mutex);
                    rwsem_write_unlock(&tcp_sem);
                    return -1;
                }

                if(iter->local_addr.sin6_port == port) {
                    ++port;
                    break;
                }

                mutex_unlock(&iter->mutex);
            }
        }

        sock->local_addr.sin6_port = htons(port);

        if(addr->sa_family == AF_INET) {
            sock->local_addr.sin6_addr.__s6_addr.__s6_addr16[5] = 0xFFFF;
            sock->local_addr.sin6_addr.__s6_addr.__s6_addr32[3] =
                htonl(net_ipv4_address(net_default_dev->ip_addr));
        }
    }

    /* Set the remote address on the socket and go to the SYN-SENT state (this
       includes setting up all the data we need for that). */
    sock->remote_addr = realaddr6;

    if(!(sock->data.rcvbuf = (uint8_t *)malloc(sock->rcvbuf_sz))) {
        errno = ENOBUFS;
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        return -1;
    }

    if(!(sock->data.sndbuf = (uint8_t *)malloc(sock->sndbuf_sz))) {
        errno = ENOBUFS;
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        free(sock->data.rcvbuf);
        return -1;
    }

    if(cond_init(&sock->data.send_cv)) {
        errno = ENOBUFS;
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        free(sock->data.sndbuf);
        free(sock->data.rcvbuf);
        return -1;
    }

    if(cond_init(&sock->data.recv_cv)) {
        errno = ENOBUFS;
        mutex_unlock(&sock->mutex);
        rwsem_write_unlock(&tcp_sem);
        cond_destroy(&sock->data.send_cv);
        free(sock->data.sndbuf);
        free(sock->data.rcvbuf);
        return -1;
    }

    sock->data.rcv.wnd = sock->rcvbuf_sz;
    sock->data.rcvbuf_head = sock->data.rcvbuf_tail = 0;
    sock->data.net = net_default_dev;
    sock->data.snd.iss = timer_us_gettime64() >> 2;
    sock->data.snd.una = sock->data.snd.iss;
    sock->data.snd.nxt = sock->data.snd.iss + 1;
    sock->state = TCP_STATE_SYN_SENT;

    /* Send a <SYN> packet */
    if(tcp_send_syn(sock, 0) == -1) {
        rwsem_write_unlock(&tcp_sem);
        mutex_unlock(&sock->mutex);
        return -1;
    }

    /* Release the write lock... */
    rwsem_write_unlock(&tcp_sem);

    /* Now, lets see if this is socket is non-blocking... */
    if(sock->flags & FS_SOCKET_NONBLOCK || irq_inside_int()) {
        /* We can't wait for the connection, so let them know it is in
           progress... */
        mutex_unlock(&sock->mutex);
        errno = EINPROGRESS;
        return -1;
    }

    /* Block until the connection can be established... */
    if(cond_wait_timed(&sock->data.send_cv, &sock->mutex,
                       2 * TCP_DEFAULT_MSL)) {
        errno = ETIMEDOUT;
        sock->state = TCP_STATE_CLOSED;
        mutex_unlock(&sock->mutex);
        return -1;
    }

    if(sock->state & TCP_STATE_RESET) {
        errno = ECONNREFUSED;
        mutex_unlock(&sock->mutex);
        return -1;
    }

    mutex_unlock(&sock->mutex);
    return 0;
}

static int net_tcp_listen(net_socket_t *hnd, int backlog) {
    struct tcp_sock *sock;

    /* Clamp the backlog between some sane values */
    if(backlog > SOMAXCONN)
        backlog = SOMAXCONN;
    else if(backlog <= 0)
        backlog = 1;

    /* Lock the socket's mutex, since we're going to be manipulating its state
       in here... */
    if(!(sock = net_tcp_read_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

    /* Make sure the socket is still in the closed state, otherwise we can't
       actually move it to the listening state */
    if(sock->state != TCP_STATE_CLOSED) {
        mutex_unlock(&sock->mutex);
        rwsem_read_unlock(&tcp_sem);
        errno = EINVAL;
        return -1;
    }

    /* Make sure the socket has been bound */
    if(!sock->local_addr.sin6_port) {
        mutex_unlock(&sock->mutex);
        rwsem_read_unlock(&tcp_sem);
        errno = EDESTADDRREQ;
        return -1;
    }

    /* Allocate the queue and set up everything */
    sock->listen.queue = (struct lsock *)malloc(sizeof(struct lsock) * backlog);

    if(!sock->listen.queue) {
        mutex_unlock(&sock->mutex);
        rwsem_read_unlock(&tcp_sem);
        errno = ENOBUFS;
        return -1;
    }

    if(cond_init(&sock->listen.cv)) {
        free(sock->listen.queue);
        sock->listen.queue = NULL;
        mutex_unlock(&sock->mutex);
        rwsem_read_unlock(&tcp_sem);
        errno = ENOBUFS;
        return -1;
    }

    sock->listen.backlog = backlog;
    sock->listen.head = sock->listen.tail = 0;
    sock->state = TCP_STATE_LISTEN;

    /* We're done now, clean up the locks */
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);

    return 0;
}

static ssize_t net_tcp_recvfrom(net_socket_t *hnd, void *buffer, size_t length,
                                int flags, struct sockaddr *addr,
                                socklen_t *addr_len) {
    struct tcp_sock *sock;
    ssize_t size = 0;
    uint8_t *buf = (uint8_t *)buffer;
    uint8_t *rb;
    int tmp;

    /* Check the parameters first */
    if(buffer == NULL || (addr != NULL && addr_len == NULL)) {
        errno = EFAULT;
        return -1;
    }

    /* Lock the socket's mutex, since we're going to be manipulating its state
       in here... */
    if(!(sock = net_tcp_read_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

    rwsem_read_unlock(&tcp_sem);

    /* Make sure they haven't shut down the socket... */
    if(sock->flags & (SHUT_RD << 24)) {
        goto out;
    }

    /* Make sure its not reset already */
    if(sock->state & TCP_STATE_RESET) {
        errno = ECONNRESET;
        size = -1;
        goto out;
    }

    /* See if we have any data */
    if(!sock->data.rcvbuf_cur_sz) {
        /* Check if we're in a state where there's not going to be any more
           messages coming in. */
        if(sock->state == TCP_STATE_CLOSED ||
           sock->state == TCP_STATE_CLOSE_WAIT ||
           sock->state == TCP_STATE_CLOSING ||
           sock->state == TCP_STATE_LAST_ACK ||
           sock->state == TCP_STATE_TIME_WAIT) {
            goto out;
        }

        if((sock->flags & FS_SOCKET_NONBLOCK) || (flags & MSG_DONTWAIT) ||
           irq_inside_int()) {
            errno = EWOULDBLOCK;
            size = -1;
            goto out;
        }

        cond_wait(&sock->data.recv_cv, &sock->mutex);
    }

    /* Once we get here, we should have data, unless the other side has closed
       the connection... */
    if(!sock->data.rcvbuf_cur_sz) {
        if(sock->state & TCP_STATE_RESET) {
            errno = ECONNRESET;
            size = -1;
        }

        goto out;
    }

    /* If we're supposed to fill the whole buffer, see if we have enough. */
    if((flags & MSG_WAITALL) && sock->data.rcvbuf_cur_sz < length) {
        /* Check if we're in a state where there's not going to be any more
           messages coming in. */
        if(sock->state != TCP_STATE_CLOSED &&
           sock->state != TCP_STATE_CLOSE_WAIT &&
           sock->state != TCP_STATE_CLOSING &&
           sock->state != TCP_STATE_LAST_ACK &&
           sock->state != TCP_STATE_TIME_WAIT) {
            /* If we can't block, return an error. */
            if((sock->flags & FS_SOCKET_NONBLOCK) || (flags & MSG_DONTWAIT) ||
               irq_inside_int()) {
                errno = EWOULDBLOCK;
                size = -1;
                goto out;
            }

            /* Wait around until we have enough data in the queue to satisfy the
               request... */
            while(sock->data.rcvbuf_cur_sz < length) {
                cond_wait(&sock->data.recv_cv, &sock->mutex);
            }
        }
    }

    /* Figure out how much we're going to give the user. */
    if(length > sock->data.rcvbuf_cur_sz)
        size = sock->data.rcvbuf_cur_sz;
    else
        size = length;

    rb = sock->data.rcvbuf + sock->data.rcvbuf_head;

    /* Advance the window if we're pulling data out of the queue. */
    if(!(flags & MSG_PEEK)) {
        sock->data.rcv.wnd += size;
        sock->data.rcvbuf_cur_sz -= size;
    }

    if(sock->data.rcvbuf_head + size <= sock->rcvbuf_sz) {
        memcpy(buf, rb, size);

        if(!(flags & MSG_PEEK)) {
            sock->data.rcvbuf_head += size;

            if(sock->data.rcvbuf_head == sock->rcvbuf_sz)
                sock->data.rcvbuf_head = 0;
        }
    }
    else {
        tmp = sock->rcvbuf_sz - sock->data.rcvbuf_head;
        memcpy(buf, rb, tmp);
        memcpy(buf + tmp, sock->data.rcvbuf, size - tmp);

        if(!(flags & MSG_PEEK))
            sock->data.rcvbuf_head = size - tmp;
    }

    /* If we've got nothing left, move the pointers back to the beginning */
    if(!sock->data.rcvbuf_cur_sz) {
        sock->data.rcvbuf_head = sock->data.rcvbuf_tail = 0;
    }

    if(addr != NULL) {
        if(sock->domain == AF_INET) {
            struct sockaddr_in realaddr;

            memset(&realaddr, 0, sizeof(struct sockaddr_in));
            realaddr.sin_family = AF_INET;
            realaddr.sin_addr.s_addr =
                sock->remote_addr.sin6_addr.__s6_addr.__s6_addr32[3];
            realaddr.sin_port = sock->remote_addr.sin6_port;

            if(*addr_len < sizeof(struct sockaddr_in)) {
                memcpy(addr, &realaddr, *addr_len);
            }
            else {
                memcpy(addr, &realaddr, sizeof(struct sockaddr_in));
                *addr_len = sizeof(struct sockaddr_in);
            }
        }
        else if(sock->domain == AF_INET6) {
            struct sockaddr_in6 realaddr6;

            memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
            realaddr6.sin6_family = AF_INET6;
            realaddr6.sin6_addr = sock->remote_addr.sin6_addr;
            realaddr6.sin6_port = sock->remote_addr.sin6_port;

            if(*addr_len < sizeof(struct sockaddr_in6)) {
                memcpy(addr, &realaddr6, *addr_len);
            }
            else {
                memcpy(addr, &realaddr6, sizeof(struct sockaddr_in6));
                *addr_len = sizeof(struct sockaddr_in6);
            }
        }
    }

out:
    mutex_unlock(&sock->mutex);
    return size;
}

static ssize_t net_tcp_sendto(net_socket_t *hnd, const void *message,
                              size_t length, int flags,
                              const struct sockaddr *addr, socklen_t addr_len) {
    struct tcp_sock *sock;
    ssize_t size;
    uint32_t bsz, tmp;
    uint8_t *sb, *buf = (uint8_t *)message;

    /* Check the parameters first */
    if(message == NULL || (addr != NULL && addr_len == 0)) {
        errno = EFAULT;
        return -1;
    }

    /* Lock the socket's mutex, since we're going to be manipulating its state
       in here... */
    if(!(sock = net_tcp_read_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

    rwsem_read_unlock(&tcp_sem);

    /* Check if the socket has been shut down for writing. */
    if(sock->flags & (SHUT_WR << 24)) {
        errno = EPIPE;
        size = -1;
        goto out;
    }

    /* Check to make sure the socket is connected. */
    switch(sock->state) {
        case TCP_STATE_CLOSED | TCP_STATE_RESET:
            errno = ECONNRESET;
            size = -1;
            goto out;

        case TCP_STATE_CLOSED:
        case TCP_STATE_LISTEN:
        case TCP_STATE_SYN_SENT:
            errno = ENOTCONN;
            size = -1;
            goto out;

        case TCP_STATE_FIN_WAIT_1:
        case TCP_STATE_FIN_WAIT_2:
        case TCP_STATE_CLOSING:
        case TCP_STATE_LAST_ACK:
        case TCP_STATE_TIME_WAIT:
            errno = EPIPE;
            size = -1;
            goto out;
    }

    /* Check if there was an address specified, if so, return error. */
    if(addr) {
        errno = EISCONN;
        size = -1;
        goto out;
    }

    /* See if we have space to buffer at least some of the data... */
    if(sock->data.sndbuf_cur_sz == sock->sndbuf_sz) {
        /* Can we block? */
        if((sock->flags & FS_SOCKET_NONBLOCK) || (flags & MSG_DONTWAIT) ||
           irq_inside_int()) {
            errno = EWOULDBLOCK;
            size = -1;
            goto out;
        }

        cond_wait(&sock->data.send_cv, &sock->mutex);

        /* If we still don't have any buffer space, its because the connection
           has either been closed or reset by the other side... */
        if(sock->data.sndbuf_cur_sz == sock->sndbuf_sz) {
            if(sock->state & TCP_STATE_RESET) {
                errno = ECONNRESET;
                size = -1;
                goto out;
            }
            else {
                errno = ENOTCONN;
                size = -1;
                goto out;
            }
        }
    }

    /* Reset the pointers if there's nothing in the buffer */
    if(sock->data.sndbuf_cur_sz == 0)
        sock->data.sndbuf_head = sock->data.sndbuf_acked =
                                     sock->data.sndbuf_tail = 0;

    /* Figure out how much we can copy in */
    bsz = sock->sndbuf_sz - sock->data.sndbuf_cur_sz;

    if(length > bsz)
        size = bsz;
    else
        size = length;

    sb = sock->data.sndbuf + sock->data.sndbuf_tail;
    sock->data.sndbuf_cur_sz += size;

    if(sock->data.sndbuf_tail + size <= sock->sndbuf_sz) {
        memcpy(sb, buf, size);
        sock->data.sndbuf_tail += size;

        if(sock->data.sndbuf_tail == sock->sndbuf_sz)
            sock->data.sndbuf_tail = 0;
    }
    else {
        tmp = sock->sndbuf_sz - sock->data.sndbuf_tail;
        memcpy(sb, buf, tmp);
        memcpy(sock->data.sndbuf, buf + tmp, size - tmp);
        sock->data.sndbuf_tail = size - tmp;
    }

    /* Send some data! */
    tcp_send_data(sock, 0);

out:
    mutex_unlock(&sock->mutex);
    return size;
}

static int net_tcp_shutdownsock(net_socket_t *hnd, int how) {
    struct tcp_sock *sock;

    if(!(sock = net_tcp_read_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

    if(how & 0xFFFFFFFC) {
        mutex_unlock(&sock->mutex);
        rwsem_read_unlock(&tcp_sem);
        errno = EINVAL;
        return -1;
    }

    sock->flags |= (how << 24);

    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);

    return 0;
}

static int net_tcp_getsockopt(net_socket_t *hnd, int level, int option_name,
                              void *option_value, socklen_t *option_len) {
    int tmp;
    struct tcp_sock *sock;

    if(!option_value || !option_len) {
        errno = EFAULT;
        return -1;
    }

    if(!(sock = net_tcp_read_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

    switch(level) {
        case SOL_SOCKET:

            switch(option_name) {
                case SO_ACCEPTCONN:
                    tmp = sock->state == TCP_STATE_LISTEN;
                    goto copy_int;

                case SO_ERROR:
                    /* Checking/resetting errors not implemented */
                    goto simply_return;

                case SO_RCVBUF:
                    tmp = sock->rcvbuf_sz;
                    goto copy_int;

                case SO_SNDBUF:
                    tmp = sock->sndbuf_sz;
                    goto copy_int;

                case SO_TYPE:
                    tmp = SOCK_STREAM;
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

        case IPPROTO_TCP:
            switch(option_name) {
                case TCP_NODELAY:
                    tmp = 1;
                    goto copy_int;
            }

            break;
    }

    /* If it wasn't handled, return that error. */
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    errno = ENOPROTOOPT;
    return -1;

ret_inval:
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
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

simply_return:
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    return 0;
}

static int net_tcp_setsockopt(net_socket_t *hnd, int level, int option_name,
                              const void *option_value, socklen_t option_len) {
    struct tcp_sock *sock;
    int tmp;
    uint8_t *new_ptr;

    if(!option_value || !option_len) {
        errno = EFAULT;
        return -1;
    }

    if(!(sock = net_tcp_read_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

    switch(level) {
        case SOL_SOCKET:
            switch(option_name) {
                case SO_ACCEPTCONN:
                case SO_ERROR:
                case SO_TYPE:
                    goto ret_inval;

                case SO_RCVBUF:
                    if(option_len != sizeof(uint32_t))
                        goto ret_inval;

                    tmp = *(uint32_t *)option_value;
                    /* Receive buffer size must be in the range 256 - 65535 */
                    if(tmp < 256)
                        tmp = 256;
                    else if(tmp > 65535)
                        tmp = 65535;

                    new_ptr = realloc(sock->data.rcvbuf, tmp);
                    if(!new_ptr)
                        goto ret_nomem;

                    sock->data.rcvbuf = new_ptr;
                    sock->rcvbuf_sz = tmp;
                    goto ret_success;

                case SO_SNDBUF:
                    if(option_len != sizeof(uint32_t))
                        goto ret_inval;

                    tmp = *(uint32_t *)option_value;
                    /* Send buffer size must be in the range 2048 - 65535 */
                    if(tmp < 2048)
                        tmp = 2048;
                    else if(tmp > 65535)
                        tmp = 65535;

                    new_ptr = realloc(sock->data.sndbuf, tmp);
                    if(!new_ptr) {
                        goto ret_nomem;
                    }

                    sock->data.sndbuf = new_ptr;
                    sock->sndbuf_sz = tmp;
                    goto ret_success;
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
                        sock->hop_limit = TCP_DEFAULT_HOPS;
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
                        sock->hop_limit = TCP_DEFAULT_HOPS;
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

        case IPPROTO_TCP:
            switch(option_name) {
                case TCP_NODELAY:
                    if(option_len != sizeof(int))
                        goto ret_inval;

                    tmp = *((int *)option_value);

                    if(tmp == 0)
                        goto ret_inval;

                    goto ret_success;
            }

            break;
    }

    /* If it wasn't handled, return that error. */
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    errno = ENOPROTOOPT;
    return -1;

ret_inval:
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    errno = EINVAL;
    return -1;

ret_nomem:
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    errno = ENOMEM;
    return -1;

ret_success:
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    return 0;
}

static int net_tcp_getsockname(net_socket_t *hnd, struct sockaddr *name,
                               socklen_t *name_len) {
    struct tcp_sock *sock;
    struct sockaddr_in realaddr;
    struct sockaddr_in6 realaddr6;

    if(!name || !name_len) {
        errno = EFAULT;
        return -1;
    }

    if(!(sock = net_tcp_read_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

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

    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    errno = ENOTSOCK;
    return -1;

ret_success:
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    return 0;
}

static int net_tcp_getpeername(net_socket_t *hnd, struct sockaddr *name, socklen_t *name_len) {
    struct tcp_sock *sock;
    struct sockaddr_in realaddr;
    struct sockaddr_in6 realaddr6;

    if(!name || !name_len) {
        errno = EFAULT;
        return -1;
    }

    if(irq_inside_int()) {
        if(rwsem_read_trylock(&tcp_sem)) {
            errno = EWOULDBLOCK;
            return -1;
        }
    } else {
        rwsem_read_lock(&tcp_sem);
    }

    if(!(sock = (struct tcp_sock *)hnd->data)) {
        rwsem_read_unlock(&tcp_sem);
        errno = EBADF;
        return -1;
    }

    if(irq_inside_int()) {
        if(mutex_trylock(&sock->mutex)) {
            rwsem_read_unlock(&tcp_sem);
            errno = EWOULDBLOCK;
            return -1;
        }
    } else {
        mutex_lock(&sock->mutex);
    }

    if(sock->state == TCP_STATE_CLOSED) {
        mutex_unlock(&sock->mutex);
        rwsem_read_unlock(&tcp_sem);
        errno = ENOTCONN;
        return -1;
    }

    if(sock->domain == AF_INET) {
        memset(&realaddr, 0, sizeof(struct sockaddr_in));
        realaddr.sin_family = AF_INET;
        realaddr.sin_addr.s_addr =
            sock->remote_addr.sin6_addr.__s6_addr.__s6_addr32[3];
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

    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    errno = ENOTSOCK;
    return -1;

ret_success:
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    return 0;
}

static int net_tcp_fcntl(net_socket_t *hnd, int cmd, va_list ap) {

    struct tcp_sock *sock;
    int rv = -1;
    long val;

    if(!(sock = net_tcp_read_lock_and_get_sock(hnd, &tcp_sem)))
        return -1;

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
    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    return rv;
}

static short net_tcp_poll(net_socket_t *hnd, short events) {
    struct tcp_sock *sock;
    short rv = 0;

    if(rwsem_read_lock_irqsafe(&tcp_sem))
        return 0;

    /* Lock the mutex on the socket itself first. We need to pull some data from
       it that doesn't affect the rest of the list, so let's start there... */
    if(!(sock = (struct tcp_sock *)hnd->data)) {
        rwsem_read_unlock(&tcp_sem);
        return POLLNVAL;
    }

    if(mutex_lock_irqsafe(&sock->mutex)) {
        rwsem_read_unlock(&tcp_sem);
        return 0;
    }

    switch(sock->state) {
        case TCP_STATE_LISTEN:
            if(sock->listen.count)
                rv = POLLRDNORM;
            break;

        case TCP_STATE_CLOSED | TCP_STATE_RESET:
            rv = POLLHUP;

            if(sock->data.rcvbuf_cur_sz)
                rv |= POLLRDNORM;
            break;

        case TCP_STATE_CLOSED:
        case TCP_STATE_SYN_SENT:
            break;

        case TCP_STATE_CLOSE_WAIT:
            if(sock->data.sndbuf_cur_sz < sock->sndbuf_sz)
                rv |= POLLWRNORM | POLLWRBAND;
            /* Fall through... */
            __fallthrough;

        case TCP_STATE_TIME_WAIT:
        case TCP_STATE_CLOSING:
            rv |= POLLRDNORM;
            break;

        case TCP_STATE_SYN_RECEIVED:
        case TCP_STATE_ESTABLISHED:
            if(sock->data.sndbuf_cur_sz < sock->sndbuf_sz)
                rv |= POLLWRNORM | POLLWRBAND;
            /* Fall through... */
            __fallthrough;

        case TCP_STATE_FIN_WAIT_1:
        case TCP_STATE_FIN_WAIT_2:
        case TCP_STATE_LAST_ACK:
            if(sock->data.rcvbuf_cur_sz)
                rv |= POLLRDNORM;
            break;
    }

    mutex_unlock(&sock->mutex);
    rwsem_read_unlock(&tcp_sem);
    return rv & (events | POLLHUP | POLLERR);
}

static void tcp_rst(netif_t *net, const struct in6_addr *src,
                    const struct in6_addr *dst, uint16_t src_port,
                    uint16_t dst_port, uint16_t flags, uint32_t seq,
                    uint32_t ack) {
    tcp_hdr_t pkt;
    uint16_t c;

    /* Fill in the packet */
    pkt.src_port = src_port;
    pkt.dst_port = dst_port;
    pkt.seq = htonl(seq);
    pkt.ack = htonl(ack);
    pkt.off_flags = htons(flags | TCP_OFFSET(5));
    pkt.wnd = 0;
    pkt.checksum = 0;
    pkt.urg = 0;

    c = net_ipv6_checksum_pseudo(src, dst, sizeof(tcp_hdr_t), IPPROTO_TCP);
    pkt.checksum = net_ipv4_checksum((const uint8_t *)&pkt, sizeof(tcp_hdr_t), c);

    net_ipv6_send(net, (const uint8_t *)&pkt, sizeof(tcp_hdr_t), 0, IPPROTO_TCP,
                  src, dst);
}

static void tcp_bpkt_rst(netif_t *net, const struct in6_addr *src,
                         const struct in6_addr *dst, const tcp_hdr_t *ohdr,
                         int size) {
    tcp_hdr_t pkt;
    uint16_t cs;
    uint16_t flags = ntohs(ohdr->off_flags);

    /* Fill in the packet */
    pkt.src_port = ohdr->dst_port;
    pkt.dst_port = ohdr->src_port;

    if(flags & TCP_FLAG_SYN)
        size += 1;

    if(flags & TCP_FLAG_FIN)
        size += 1;

    if(flags & TCP_FLAG_ACK) {
        pkt.seq = ohdr->ack;
        pkt.ack = 0;
        pkt.off_flags = TCP_FLAG_RST;
    }
    else {
        pkt.seq = 0;
        pkt.ack = htonl(ntohl(ohdr->seq) + size);
        pkt.off_flags = TCP_FLAG_ACK | TCP_FLAG_RST;
    }

    pkt.off_flags = htons(pkt.off_flags | TCP_OFFSET(5));
    pkt.wnd = 0;
    pkt.checksum = 0;
    pkt.urg = 0;

    cs = net_ipv6_checksum_pseudo(dst, src, sizeof(tcp_hdr_t), IPPROTO_TCP);
    pkt.checksum = net_ipv4_checksum((const uint8_t *)&pkt, sizeof(tcp_hdr_t),
                                     cs);

    net_ipv6_send(net, (const uint8_t *)&pkt, sizeof(tcp_hdr_t), 0, IPPROTO_TCP,
                  dst, src);
}

static int tcp_send_syn(struct tcp_sock *sock, int ack) {
    uint8_t rawpkt[sizeof(tcp_hdr_t) + 4];
    tcp_hdr_t *hdr = (tcp_hdr_t *)rawpkt;
    uint16_t cs;

    /* Fill in the base packet */
    hdr->src_port = sock->local_addr.sin6_port;
    hdr->dst_port = sock->remote_addr.sin6_port;
    hdr->seq = htonl(sock->data.snd.iss);
    hdr->ack = htonl(sock->data.rcv.nxt);

    if(ack) {
        hdr->off_flags = htons(TCP_FLAG_SYN | TCP_FLAG_ACK | TCP_OFFSET(6));
    }
    else {
        hdr->off_flags = htons(TCP_FLAG_SYN | TCP_OFFSET(6));
    }

    hdr->wnd = htons(sock->data.rcv.wnd);
    hdr->checksum = 0;
    hdr->urg = 0;

    /* Fill in our SYN options. The only one we worry about right now is MSS. */
    hdr->options[0] = TCP_OPT_MSS;
    hdr->options[1] = 4;
    hdr->options[2] = (TCP_DEFAULT_MSS >> 8) & 0xFF;
    hdr->options[3] = TCP_DEFAULT_MSS & 0xFF;

    /* Calculate the real checksum */
    cs = net_ipv6_checksum_pseudo(&sock->local_addr.sin6_addr,
                                  &sock->remote_addr.sin6_addr,
                                  sizeof(tcp_hdr_t) + 4, IPPROTO_TCP);
    hdr->checksum = net_ipv4_checksum(rawpkt, sizeof(tcp_hdr_t) + 4, cs);

    return net_ipv6_send(sock->data.net, rawpkt, sizeof(tcp_hdr_t) + 4,
                         sock->hop_limit, IPPROTO_TCP,
                         &sock->local_addr.sin6_addr,
                         &sock->remote_addr.sin6_addr);
}

static void tcp_send_fin_ack(struct tcp_sock *sock) {
    uint8_t rawpkt[sizeof(tcp_hdr_t)];
    tcp_hdr_t *hdr = (tcp_hdr_t *)rawpkt;
    uint16_t cs;

    /* Fill in the base packet */
    hdr->src_port = sock->local_addr.sin6_port;
    hdr->dst_port = sock->remote_addr.sin6_port;
    hdr->seq = htonl(sock->data.snd.nxt);
    hdr->ack = htonl(sock->data.rcv.nxt);
    hdr->off_flags = htons(TCP_FLAG_FIN | TCP_FLAG_ACK | TCP_OFFSET(5));
    hdr->wnd = htons(sock->data.rcv.wnd);
    hdr->checksum = 0;
    hdr->urg = 0;

    /* Calculate the real checksum */
    cs = net_ipv6_checksum_pseudo(&sock->local_addr.sin6_addr,
                                  &sock->remote_addr.sin6_addr,
                                  sizeof(tcp_hdr_t), IPPROTO_TCP);
    hdr->checksum = net_ipv4_checksum(rawpkt, sizeof(tcp_hdr_t), cs);

    net_ipv6_send(sock->data.net, rawpkt, sizeof(tcp_hdr_t), sock->hop_limit,
                  IPPROTO_TCP, &sock->local_addr.sin6_addr,
                  &sock->remote_addr.sin6_addr);
}

static void tcp_send_ack(struct tcp_sock *sock) {
    tcp_hdr_t hdr;
    uint16_t c;

    /* Fill in the base packet */
    hdr.src_port = sock->local_addr.sin6_port;
    hdr.dst_port = sock->remote_addr.sin6_port;
    hdr.seq = htonl(sock->data.snd.nxt);
    hdr.ack = htonl(sock->data.rcv.nxt);
    hdr.off_flags = htons(TCP_FLAG_ACK | TCP_OFFSET(5));
    hdr.wnd = htons(sock->data.rcv.wnd);
    hdr.checksum = 0;
    hdr.urg = 0;

    /* Calculate the real checksum */
    c = net_ipv6_checksum_pseudo(&sock->local_addr.sin6_addr,
                                 &sock->remote_addr.sin6_addr,
                                 sizeof(tcp_hdr_t), IPPROTO_TCP);
    hdr.checksum = net_ipv4_checksum((const uint8_t *)&hdr, sizeof(tcp_hdr_t), c);

    net_ipv6_send(sock->data.net, (const uint8_t *)&hdr, sizeof(tcp_hdr_t),
                  sock->hop_limit, IPPROTO_TCP, &sock->local_addr.sin6_addr,
                  &sock->remote_addr.sin6_addr);
}

static void tcp_send_data(struct tcp_sock *sock, int resend) {
    uint32_t wnd = sock->data.snd.wnd, snd;
    int sz = sizeof(tcp_hdr_t);
    uint8_t rawpkt[1500];
    tcp_hdr_t *hdr = (tcp_hdr_t *)rawpkt;
    uint16_t cs;
    uint8_t *sb, *buf;
    uint32_t seq, unacked, head;

    if(!resend) {
        seq = sock->data.snd.nxt;
        unacked = sock->data.snd.nxt - sock->data.snd.una;
        wnd -= unacked;
        head = sock->data.sndbuf_head;
    }
    else {
        seq = sock->data.snd.una;
        unacked = 0;
        head = sock->data.sndbuf_acked;
    }

    if(!wnd)
        wnd = 1;

    /* Fill in the base packet */
    hdr->src_port = sock->local_addr.sin6_port;
    hdr->dst_port = sock->remote_addr.sin6_port;
    hdr->ack = htonl(sock->data.rcv.nxt);
    hdr->off_flags = htons(TCP_FLAG_ACK | TCP_OFFSET(5));
    hdr->wnd = htons(sock->data.rcv.wnd);
    hdr->urg = 0;

    /* Put on some data if we should do so */
    while(sock->data.sndbuf_cur_sz - unacked && wnd) {
        hdr->seq = htonl(seq);
        hdr->checksum = 0;
        buf = rawpkt + sizeof(tcp_hdr_t);
        sb = sock->data.sndbuf + head;
        snd = wnd;

        if(snd > sock->data.snd.mss - sizeof(tcp_hdr_t))
            snd = sock->data.snd.mss - sizeof(tcp_hdr_t);

        if(snd > sock->data.sndbuf_cur_sz - unacked)
            snd = sock->data.sndbuf_cur_sz - unacked;

        /* Copy in the data */
        if(head + snd <= sock->sndbuf_sz) {
            memcpy(buf, sb, snd);
            head += snd;

            if(head == sock->sndbuf_sz)
                head = 0;
        }
        else {
            sz = sock->sndbuf_sz - head;
            memcpy(buf, sb, sz);
            memcpy(buf + sz, sock->data.sndbuf, snd - sz);
            head = snd - sz;
        }

        sz = snd + sizeof(tcp_hdr_t);
        wnd -= snd;
        seq += snd;
        unacked += snd;

        /* Calculate the checksum */
        cs = net_ipv6_checksum_pseudo(&sock->local_addr.sin6_addr,
                                      &sock->remote_addr.sin6_addr, sz,
                                      IPPROTO_TCP);
        hdr->checksum = net_ipv4_checksum(rawpkt, sz, cs);

        net_ipv6_send(sock->data.net, rawpkt, sz, sock->hop_limit, IPPROTO_TCP,
                      &sock->local_addr.sin6_addr,
                      &sock->remote_addr.sin6_addr);
    }

    sock->data.timer = timer_ms_gettime64();
    sock->data.sndbuf_head = head;
    sock->data.snd.nxt = seq;
}

#define ADDR_EQUAL(a1, a2) \
    (((a1).__s6_addr.__s6_addr32[0] == (a2).__s6_addr.__s6_addr32[0]) && \
     ((a1).__s6_addr.__s6_addr32[1] == (a2).__s6_addr.__s6_addr32[1]) && \
     ((a1).__s6_addr.__s6_addr32[2] == (a2).__s6_addr.__s6_addr32[2]) && \
     ((a1).__s6_addr.__s6_addr32[3] == (a2).__s6_addr.__s6_addr32[3]))

/* Match a socket to an incoming packet. If an actual socket is returned, it is
   the caller's responsibility  to release the socket's mutex when they're done
   with it. */
static struct tcp_sock *find_sock(const struct in6_addr *src,
                                  const struct in6_addr *dst,
                                  uint16_t sport, uint16_t dport, int domain) {
    struct tcp_sock *i;

    LIST_FOREACH(i, &tcp_socks, sock_list) {
        /* Ignore any closed sockets */
        if(i->state == TCP_STATE_CLOSED)
            continue;

        /* Ignore any sockets that are IPv6 only when we have an incoming IPv4
           packet, or any that are IPv4 only when we have an incoming IPv6
           packet. */
        if((domain == AF_INET && (i->flags & FS_SOCKET_V6ONLY)) ||
                (domain == AF_INET6 && i->domain == AF_INET))
            continue;

        /* See if the remote end matches what's in the socket */
        if(!IN6_IS_ADDR_UNSPECIFIED(&i->remote_addr.sin6_addr) &&
                (!ADDR_EQUAL(i->remote_addr.sin6_addr, *src) ||
                 i->remote_addr.sin6_port != sport))
            continue;

        /* See if it matches the local end */
        if((!IN6_IS_ADDR_UNSPECIFIED(&i->local_addr.sin6_addr) &&
                !ADDR_EQUAL(i->local_addr.sin6_addr, *dst)) ||
                i->local_addr.sin6_port != dport)
            continue;

        if(mutex_lock_irqsafe(&i->mutex))
            return (struct tcp_sock *) -1;

        /* Because we always add new sockets to the head of the list, this
           should be sufficient to match the socket. See the comment at the top
           of the file for more discussion of this, if you're interested. */
        return i;
    }

    return NULL;
}

extern void __poll_event_trigger(int fd, short event);

/* This function is basically a direct implementation of the first two and a
   half steps of the SEGMENT ARRIVES event processing defined in RFC 793 on
   pages 65 and 66. There are a few parts that are omitted and some are put off
   until actually accepting the connection. */
static int listen_pkt(netif_t *src, const struct in6_addr *srca,
                      const struct in6_addr *dsta, const tcp_hdr_t *tcp,
                      struct tcp_sock *s, uint16_t flags, int size) {
    int j = 0;
    int end_of_opts;
    uint16_t mss = 576;

    (void)size;

    /* Incoming segments with a RST should be ignored */
    if(flags & TCP_FLAG_RST)
        return 0;

    /* Incoming segments with an ACK cause a RST to be generated */
    if(flags & TCP_FLAG_ACK)
        return -1;

    /* Parse options now, in case we need to update the max segment size. */
    end_of_opts = TCP_GET_OFFSET(flags) - 20;

    while(j < end_of_opts) {
        switch(tcp->options[j]) {
            case TCP_OPT_EOL:
                j = end_of_opts;
                break;

            case TCP_OPT_NOP:
                ++j;
                break;

            case TCP_OPT_MSS:
                if(j + 4 > end_of_opts || tcp->options[j + 1] != 4)
                    return -1;

                mss = (tcp->options[j + 2] << 8) | tcp->options[j + 3];
                j += 4;
                break;

            default:

                /* Skip unknown options */
                if(j + 1 > end_of_opts || j + tcp->options[j + 1] > end_of_opts)
                    return -1;

                j += tcp->options[j + 1];
        }
    }

    /* Silently cap the MSS... */
    if(mss > 1460)
        mss = 1460;

    /* If the SYN bit is set, we should check the security/compartment. We just
       silently ignore them for now. We also ignore the precedence... Thus, the
       next thing is to make sure that we don't already have this connection in
       the queue... */
    for(j = s->listen.head; j < s->listen.tail; ++j) {
        if(ADDR_EQUAL(s->listen.queue[j].remote_addr.sin6_addr, *srca) &&
                ADDR_EQUAL(s->listen.queue[j].local_addr.sin6_addr, *dsta) &&
                s->listen.queue[j].remote_addr.sin6_port == tcp->src_port) {
            s->listen.queue[j].isn = ntohl(tcp->seq);
            s->listen.queue[j].mss = mss;
            return 0;
        }
    }

    /* Next, see if we have space for this one in the queue... */
    if(s->listen.count == s->listen.backlog)
        return -1;

    /* The rest of the processing is put off until the program does an accept().
       Save the connection in the list of incoming sockets. */
    s->listen.queue[s->listen.tail].net = src;
    s->listen.queue[s->listen.tail].remote_addr.sin6_addr = *srca;
    s->listen.queue[s->listen.tail].remote_addr.sin6_port = tcp->src_port;
    s->listen.queue[s->listen.tail].local_addr.sin6_addr = *dsta;
    s->listen.queue[s->listen.tail].local_addr.sin6_port = tcp->dst_port;
    s->listen.queue[s->listen.tail].isn = ntohl(tcp->seq);
    s->listen.queue[s->listen.tail].mss = mss;
    s->listen.queue[s->listen.tail].wnd = ntohs(tcp->wnd);
    ++s->listen.count;
    ++s->listen.tail;

    if(s->listen.tail == s->listen.backlog)
        s->listen.tail = 0;

    /* Signal the condvar, in case anyone's waiting */
    __poll_event_trigger(s->sock, POLLRDNORM);
    cond_signal(&s->listen.cv);

    /* We're done, return success. */
    return 0;
}

/* This implements the processing described for the SYN-SENT state, as described
   in pages 66-68 of the RFC. */
static int synsent_pkt(netif_t *src, const struct in6_addr *srca,
                       const struct in6_addr *dsta, const tcp_hdr_t *tcp,
                       struct tcp_sock *s, uint16_t flags, int size) {
    uint32_t ack, seq;
    int sz = size - TCP_GET_OFFSET(flags), gotack = 0;
    int j = 0, end_of_opts, mss = 536;

    (void)src;

    /* Grab the ack and seq numbers from the packet */
    ack = ntohl(tcp->ack);
    seq = ntohl(tcp->seq);

    /* First, we need to check the ACK bit */
    if(flags & TCP_FLAG_ACK) {
        gotack = 1;

        if(SEQ_LE(ack, s->data.snd.iss) || SEQ_GT(ack, s->data.snd.nxt)) {
            tcp_bpkt_rst(s->data.net, srca, dsta, tcp, sz);
            return 0;
        }
    }

    /* Next, we check the RST bit */
    if(flags & TCP_FLAG_RST) {
        if(gotack) {
            s->state = TCP_STATE_CLOSED | TCP_STATE_RESET;
            __poll_event_trigger(s->sock, POLLHUP);
            cond_signal(&s->data.recv_cv);
            cond_signal(&s->data.send_cv);
            return 0;
        }
    }

    /* We should check security and precedence here... */

    /* Next, we check the SYN bit */
    if(flags & TCP_FLAG_SYN) {
        s->data.rcv.nxt = seq + 1;
        s->data.rcv.irs = seq;

        end_of_opts = TCP_GET_OFFSET(flags) - 20;

        while(j < end_of_opts) {
            switch(tcp->options[j]) {
                case TCP_OPT_EOL:
                    j = end_of_opts;
                    break;

                case TCP_OPT_NOP:
                    ++j;
                    break;

                case TCP_OPT_MSS:

                    if(j + 4 > end_of_opts || tcp->options[j + 1] != 4)
                        return -1;

                    mss = (tcp->options[j + 2] << 8) | tcp->options[j + 3];
                    j += 4;
                    break;

                default:

                    /* Skip unknown options */
                    if(j + 1 > end_of_opts ||
                            j + tcp->options[j + 1] > end_of_opts)
                        return -1;

                    j += tcp->options[j + 1];
            }
        }

        s->data.snd.mss = mss > 1460 ? 1460 : mss;
        s->data.snd.wnd = htons(tcp->wnd);

        if(gotack) {
            s->data.snd.una = ack;

            /* If the ack covers our iss, then we've established the connection.
               Update the state and ack it. */
            if(SEQ_GT(ack, s->data.snd.iss)) {
                s->state = TCP_STATE_ESTABLISHED;
                tcp_send_ack(s);
                __poll_event_trigger(s->sock, POLLWRNORM | POLLWRBAND);
                cond_signal(&s->data.send_cv);
            }
        }
        else {
            s->state = TCP_STATE_SYN_RECEIVED;
            tcp_send_syn(s, 1);
            __poll_event_trigger(s->sock, POLLWRNORM | POLLWRBAND);
            cond_signal(&s->data.send_cv);
        }
    }

    return 0;
}

/* This implements the processing described for the synchronized states, as
   described in pages 69-76 of the RFC. */
static int process_pkt(netif_t *src, const struct in6_addr *srca,
                       const struct in6_addr *dsta, const tcp_hdr_t *tcp,
                       struct tcp_sock *s, uint16_t flags, size_t size) {
    uint32_t seq, ack, up;
    size_t sz;
    int bad_pkt = 0, tmp, acksyn = 0;
    const uint8_t *buf = (const uint8_t *)tcp;
    uint8_t *rb;

    (void)src;

    /* Grab the seq and ack values from the header. */
    seq = ntohl(tcp->seq);
    ack = ntohl(tcp->ack);

    /* Check the validity of the incoming segment's sequence number */
    sz = size - TCP_GET_OFFSET(flags);
    buf += TCP_GET_OFFSET(flags);

    if(s->data.rcv.wnd == 0) {
        if(sz || seq != s->data.rcv.nxt)
            bad_pkt = 1;
    }
    else {
        if(!sz) {
            if(!(SEQ_GE(seq, s->data.rcv.nxt) &&
                    SEQ_LT(seq, s->data.rcv.nxt + s->data.rcv.wnd)))
                bad_pkt = 1;
        }
        else {
            if(!(SEQ_GE(seq, s->data.rcv.nxt) &&
                    SEQ_LT(seq, s->data.rcv.nxt + s->data.rcv.wnd)))
                bad_pkt = 1;
        }
    }

    /* If the sequence number isn't valid, check the RST bit. If its not set,
       send the appropriate ACK. */
    if(bad_pkt) {
        if(flags & TCP_FLAG_RST) {
            return 0;
        }

        /* Send the ACK */
        tcp_send_ack(s);
        return 0;
    }

    /* See if we have a reset, and process it */
    if(flags & TCP_FLAG_RST) {
        if(s->state == TCP_STATE_SYN_SENT) {
            /* The only acceptable RST is one that has an ack field matching our
               SND.NXT (i.e, the ISS + 1). */
            if(ack != s->data.snd.nxt)
                bad_pkt = 1;
        }

        /* Drop a bad RST segment */
        if(bad_pkt) {
            return 0;
        }
        else {
            s->state = TCP_STATE_RESET | TCP_STATE_CLOSED;
            __poll_event_trigger(s->sock, POLLHUP);
            cond_signal(&s->data.recv_cv);
            cond_signal(&s->data.send_cv);
            return 0;
        }
    }

    /* We should check security/precedence here, but we'll ignore it for now. */

    /* Next, check for a SYN. Any SYN will be in the window, otherwise it would
       have been handled in the sequence check above. Any SYN in the window will
       cause the connection to be reset. */
    if(flags & TCP_FLAG_SYN) {
        tcp_bpkt_rst(s->data.net, srca, dsta, tcp, sz);
        return 0;
    }

    /* Any packet without an ACK should be dropped */
    if(!(flags & TCP_FLAG_ACK)) {
        return 0;
    }

    /* The state changes how we handle the rest... */
    if(s->state == TCP_STATE_SYN_RECEIVED) {
        if(SEQ_LE(s->data.snd.una, ack) && SEQ_LE(ack, s->data.snd.nxt)) {
            s->state = TCP_STATE_ESTABLISHED;
            acksyn = 1;
        }
        else {
            tcp_bpkt_rst(s->data.net, srca, dsta, tcp, sz);
            return 0;
        }
    }

    /* Check the ack number for validity */
    if(SEQ_LT(s->data.snd.una, ack) && SEQ_LE(ack, s->data.snd.nxt)) {
        s->data.sndbuf_acked += (int32_t)(ack - s->data.snd.una - acksyn);
        s->data.sndbuf_cur_sz -= (int32_t)(ack - s->data.snd.una - acksyn);
        s->data.snd.una = ack;
        __poll_event_trigger(s->sock, POLLWRNORM | POLLWRBAND);
        cond_signal(&s->data.send_cv);

        if(s->data.sndbuf_acked >= s->sndbuf_sz)
            s->data.sndbuf_acked -= s->sndbuf_sz;

        if(SEQ_LT(s->data.snd.wl1, seq) ||
                (s->data.snd.wl1 == seq && SEQ_LE(s->data.snd.wl2, ack))) {
            s->data.snd.wnd = ntohs(tcp->wnd);
            s->data.snd.wl1 = seq;
            s->data.snd.wl2 = ack;
        }
    }
    else if(SEQ_GT(ack, s->data.snd.nxt)) {
        /* This ACKs something we haven't sent, so try to correct the other side
           and return */
        tcp_send_ack(s);
        return 0;
    }

    /* We need to do a bit more processing in certain states... */
    switch(s->state) {
        case TCP_STATE_FIN_WAIT_1:

            /* If the FIN has been acked, go to the FIN-WAIT-2 state. */
            if(ack == s->data.snd.nxt) {
                s->state = TCP_STATE_FIN_WAIT_2;
            }

            break;

        case TCP_STATE_CLOSING:

            /* If the FIN has been acked, go to TIME-WAIT */
            if(ack == s->data.snd.nxt) {
                s->state = TCP_STATE_TIME_WAIT;
                s->data.timer = timer_ms_gettime64();
                break;
            }
            else {
                return 0;
            }

        case TCP_STATE_LAST_ACK:

            /* If the FIN has been acked, go to CLOSED */
            if(ack == s->data.snd.nxt) {
                s->state = TCP_STATE_CLOSED;
                return 0;
            }

            break;

        case TCP_STATE_TIME_WAIT:
            /* ACK the FIN again, and restart the timer */
            s->data.timer = timer_ms_gettime64();
            tcp_send_ack(s);
            break;
    }

    /* Next, we handle the URG bit */
    if(flags & TCP_FLAG_URG) {
        if(s->state == TCP_STATE_ESTABLISHED ||
                s->state == TCP_STATE_FIN_WAIT_1 ||
                s->state == TCP_STATE_FIN_WAIT_2) {
            up = ntohl(tcp->urg);
            s->data.rcv.up = MAX(s->data.rcv.up, up);
        }
    }

    if(s->state == TCP_STATE_ESTABLISHED || s->state == TCP_STATE_FIN_WAIT_1 ||
            s->state == TCP_STATE_FIN_WAIT_2) {
        /* Next, check the data size versus our window. If its more than the
           window, truncate the data and copy out what we can. */
        if(sz > s->data.rcv.wnd) {
            sz = s->data.rcv.wnd;
            bad_pkt = 1;
        }

        /* Copy the data out */
        if(sz) {
            rb = s->data.rcvbuf + s->data.rcvbuf_tail;
            s->data.rcv.nxt += sz;
            s->data.rcv.wnd -= sz;
            s->data.rcvbuf_cur_sz += sz;

            if(s->data.rcvbuf_tail + sz <= s->rcvbuf_sz) {
                memcpy(rb, buf, sz);
                s->data.rcvbuf_tail += sz;
            }
            else {
                tmp = s->rcvbuf_sz - s->data.rcvbuf_tail;
                memcpy(rb, buf, tmp);
                sz -= tmp;
                buf += tmp;
                memcpy(s->data.rcvbuf, buf, sz);
                s->data.rcvbuf_tail = sz;
            }

            /* Signal any waiting thread and send an ack for what we read */
            __poll_event_trigger(s->sock, POLLRDNORM);
            cond_signal(&s->data.recv_cv);
            tcp_send_ack(s);
        }
    }
    else if(sz) {
        /* If we get any segment text in here, there's a problem with the other
           side... Ignore it. */
        bad_pkt = 1;
    }

    /* Finally, check the FIN bit. We don't try to ack it if the packet had too
       much data. */
    if(!bad_pkt && (flags & TCP_FLAG_FIN)) {
        /* ACK the FIN */
        ++s->data.rcv.nxt;
        tcp_send_ack(s);
        __poll_event_trigger(s->sock, POLLRDNORM);
        cond_signal(&s->data.recv_cv);

        /* Do the various processing that needs to be done based on our state */
        switch(s->state) {
            case TCP_STATE_SYN_RECEIVED:
            case TCP_STATE_ESTABLISHED:
                s->state = TCP_STATE_CLOSE_WAIT;
                break;

            case TCP_STATE_FIN_WAIT_1:

                if(ack < s->data.snd.nxt) {
                    s->state = TCP_STATE_CLOSING;
                }

                break;

            case TCP_STATE_FIN_WAIT_2:
                s->state = TCP_STATE_TIME_WAIT;
                s->data.timer = timer_ms_gettime64();
                break;

            case TCP_STATE_TIME_WAIT:
                s->data.timer = timer_ms_gettime64();
                break;
        }
    }

    /* And... We're done, finally. */
    return 0;
}

static int net_tcp_input(netif_t *src, int domain, const void *hdr,
                         const uint8 *data, size_t size) {
    struct in6_addr srca, dsta;
    const ip_hdr_t *ip4;
    const ipv6_hdr_t *ip6;
    const tcp_hdr_t *tcp;
    uint16_t flags;
    struct tcp_sock *s;
    int rv = -1;
    uint16_t c;

    switch(domain) {
        case AF_INET:
            ip4 = (const ip_hdr_t *)hdr;
            srca.__s6_addr.__s6_addr32[0] = srca.__s6_addr.__s6_addr32[1] = 0;
            srca.__s6_addr.__s6_addr16[4] = 0;
            srca.__s6_addr.__s6_addr16[5] = 0xFFFF;
            srca.__s6_addr.__s6_addr32[3] = ip4->src;
            dsta.__s6_addr.__s6_addr32[0] = dsta.__s6_addr.__s6_addr32[1] = 0;
            dsta.__s6_addr.__s6_addr16[4] = 0;
            dsta.__s6_addr.__s6_addr16[5] = 0xFFFF;
            dsta.__s6_addr.__s6_addr32[3] = ip4->dest;
            break;

        case AF_INET6:
            ip6 = (const ipv6_hdr_t *)hdr;
            srca = ip6->src_addr;
            dsta = ip6->dst_addr;
            break;

        default:
            /* Shouldn't get here... */
            return -1;
    }

    tcp = (const tcp_hdr_t *)data;

    /* Check the TCP checksum */
    c = net_ipv6_checksum_pseudo(&srca, &dsta, size, IPPROTO_TCP);
    c = net_ipv4_checksum(data, size, c);

    if(c) {
        /* The checksum should be 0 on success, so discard the packet if it does
           not match that expectation. */
        return 0;
    }

    flags = ntohs(tcp->off_flags);

    if(rwsem_read_lock_irqsafe(&tcp_sem))
        return -1;

    /* Find a matching socket */
    if((s = find_sock(&srca, &dsta, tcp->src_port, tcp->dst_port, domain))) {
        /* Make sure we take care of busy sockets... */
        if(s == (struct tcp_sock *) - 1) {
            rwsem_read_unlock(&tcp_sem);
            return 0;
        }

        /* We have to do different things for different states, so figure out
           what this socket is doing. */
        switch(s->state) {
            case TCP_STATE_LISTEN:
                rv = listen_pkt(src, &srca, &dsta, tcp, s, flags, size);
                break;

            case TCP_STATE_LISTEN | TCP_STATE_ACCEPTING:
                /* Ugly hack... */
                rv = 0;
                break;

            case TCP_STATE_SYN_SENT:
                rv = synsent_pkt(src, &srca, &dsta, tcp, s, flags, size);
                break;

            case TCP_STATE_SYN_RECEIVED:
            case TCP_STATE_ESTABLISHED:
            case TCP_STATE_FIN_WAIT_1:
            case TCP_STATE_FIN_WAIT_2:
            case TCP_STATE_CLOSE_WAIT:
            case TCP_STATE_CLOSING:
            case TCP_STATE_LAST_ACK:
            case TCP_STATE_TIME_WAIT:
                rv = process_pkt(src, &srca, &dsta, tcp, s, flags, size);
                break;
        }

        mutex_unlock(&s->mutex);
    }

    rwsem_read_unlock(&tcp_sem);

    /* If we get in here, something went wrong... Send a RST. */
    if(rv && !(flags & TCP_FLAG_RST)) {
        tcp_bpkt_rst(src, &srca, &dsta, tcp, size - TCP_GET_OFFSET(flags));
    }

    return 0;
}

static void tcp_thd_cb(void *arg) {
    struct tcp_sock *i, *tmp;
    uint64_t timer;

    (void)arg;

    rwsem_read_lock(&tcp_sem);

    LIST_FOREACH(i, &tcp_socks, sock_list) {
        mutex_lock_scoped(&i->mutex);
        timer = timer_ms_gettime64();

        switch(i->state) {
            case TCP_STATE_LISTEN:
                break;

            case TCP_STATE_SYN_SENT:

                /* If our last <SYN> was sent more than one  retransmission
                   timeout period ago and we are still in the SYN-SENT state,
                   send another one. */
                if(i->data.timer + TCP_DEFAULT_RTTO <= timer) {
                    tcp_send_syn(i, 0);
                    i->data.timer = timer;
                }

                break;

            case TCP_STATE_SYN_RECEIVED:

                /* If our last <SYN,ACK> was sent more than one  retransmission
                   timeout period ago and we are still in the SYN-RECEIVED
                   state, send another one. */
                if(i->data.timer + TCP_DEFAULT_RTTO <= timer) {
                    tcp_send_syn(i, 1);
                    i->data.timer = timer;
                }

                break;

            case TCP_STATE_TIME_WAIT:

                /* If the TIME-WAIT timer has expired, then clean up the rest of
                   the connection (the fd was already taken care of by a close()
                   call earlier that ended up putting us in this state). */
                if(i->data.timer + 2 * TCP_DEFAULT_MSL <= timer)
                    i->state = TCP_STATE_CLOSED;

                break;

            case TCP_STATE_ESTABLISHED:
            case TCP_STATE_CLOSE_WAIT:

                if(i->data.sndbuf_cur_sz &&
                        i->data.timer + TCP_DEFAULT_RTTO <= timer) {
                    tcp_send_data(i, 1);
                }
                else if(!i->data.sndbuf_cur_sz &&
                        (i->intflags & TCP_IFLAG_QUEUEDCLOSE)) {
                    if(i->state == TCP_STATE_ESTABLISHED) {
                        i->state = TCP_STATE_FIN_WAIT_1;
                    }
                    else {
                        i->state = TCP_STATE_CLOSING;
                    }

                    tcp_send_fin_ack(i);
                    ++i->data.snd.nxt;
                }

                break;
        }
    }

    rwsem_read_unlock(&tcp_sem);

    /* Go through and clean up any sockets that need to be destroyed. */
    rwsem_write_lock(&tcp_sem);

    i = LIST_FIRST(&tcp_socks);

    while(i) {
        tmp = LIST_NEXT(i, sock_list);

        if((i->intflags & TCP_IFLAG_CANBEDEL) &&
                (i->state & 0x0F) == TCP_STATE_CLOSED) {
            LIST_REMOVE(i, sock_list);
            cond_destroy(&i->data.send_cv);
            cond_destroy(&i->data.recv_cv);
            mutex_destroy(&i->mutex);
            free(i->data.sndbuf);
            free(i->data.rcvbuf);
            free(i);
        }

        i = tmp;
    }

    rwsem_write_unlock(&tcp_sem);
}

/* Protocol handler for fs_socket. */
static fs_socket_proto_t proto = {
    FS_SOCKET_PROTO_ENTRY,
    PF_INET6,                           /* domain */
    SOCK_STREAM,                        /* type */
    IPPROTO_TCP,                        /* protocol */
    net_tcp_socket,                     /* socket */
    net_tcp_close,                      /* close */
    net_tcp_accept,                     /* accept */
    net_tcp_bind,                       /* bind */
    net_tcp_connect,                    /* connect */
    net_tcp_listen,                     /* listen */
    net_tcp_recvfrom,                   /* recvfrom */
    net_tcp_sendto,                     /* sendto */
    net_tcp_shutdownsock,               /* shutdown */
    net_tcp_input,                      /* input */
    net_tcp_getsockopt,                 /* getsockopt */
    net_tcp_setsockopt,                 /* setsockopt */
    net_tcp_getsockname,                /* getsockname */
    net_tcp_getpeername,                /* getpeername */
    net_tcp_fcntl,                      /* fcntl */
    net_tcp_poll                        /* poll */
};

int net_tcp_init(void) {
    if((thd_cb_id = net_thd_add_callback(tcp_thd_cb, NULL, 50)) < 0)
        return -1;

    return fs_socket_proto_add(&proto);
}

void net_tcp_shutdown(void) {
    struct tcp_sock *i, *tmp;

    /* Kill the thread and make sure we can grab the lock */
    if(thd_cb_id >= 0)
        net_thd_del_callback(thd_cb_id);

    /* Disable IRQs so we can kill the sockets in peace... */
    irq_disable_scoped();

    /* Clean up existing sockets */
    i = LIST_FIRST(&tcp_socks);

    while(i) {
        tmp = LIST_NEXT(i, sock_list);

        if(i->sock != -1) {
            close(i->sock);
        }
        else {
            LIST_REMOVE(i, sock_list);
            cond_destroy(&i->data.send_cv);
            cond_destroy(&i->data.recv_cv);
            mutex_destroy(&i->mutex);
            free(i->data.sndbuf);
            free(i->data.rcvbuf);
            free(i);
        }

        i = tmp;
    }

    LIST_INIT(&tcp_socks);

    /* Remove us from fs_socket and clean up the semaphore */
    fs_socket_proto_remove(&proto);
}
