/* KallistiOS ##version##

   fs_socket.c
   Copyright (C) 2006, 2009, 2012, 2013, 2016 Lawrence Sebald

*/

#include <kos/mutex.h>
#include <kos/fs.h>
#include <kos/fs_socket.h>
#include <kos/net.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/queue.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Define the protocol list type */
TAILQ_HEAD(proto_list, fs_socket_proto);

/* Define the socket list type */
LIST_HEAD(socket_list, net_socket);

static struct proto_list protocols;
static struct socket_list sockets;
static mutex_t proto_rlock = RECURSIVE_MUTEX_INITIALIZER;
static mutex_t list_rlock = RECURSIVE_MUTEX_INITIALIZER;

static int fs_socket_close(void *hnd) {
    net_socket_t *sock = (net_socket_t *)hnd;

    if(mutex_lock_irqsafe(&list_rlock))
        return -1;

    LIST_REMOVE(sock, sock_list);
    mutex_unlock(&list_rlock);

    /* Protect against botched socket() calls */
    if(sock->protocol)
        sock->protocol->close(sock);

    free(sock);
    return 0;
}

static ssize_t fs_socket_read(void *hnd, void *buffer, size_t cnt) {
    net_socket_t *sock = (net_socket_t *)hnd;

    return sock->protocol->recvfrom(sock, buffer, cnt, 0, NULL, NULL);
}

static off_t fs_socket_seek(void *hnd, off_t offset, int whence) {
    (void)hnd;
    (void)offset;
    (void)whence;
    errno = ESPIPE;
    return (off_t) - 1;
}

static off_t fs_socket_tell(void *hnd) {
    (void)hnd;
    errno = ESPIPE;
    return (off_t) - 1;
}

static ssize_t fs_socket_write(void *hnd, const void *buffer, size_t cnt) {
    net_socket_t *sock = (net_socket_t *)hnd;

    return sock->protocol->sendto(sock, buffer, cnt, 0, NULL, 0);
}

static int fs_socket_fcntl(void *hnd, int cmd, va_list ap) {
    net_socket_t *sock = (net_socket_t *)hnd;
    return sock->protocol->fcntl(sock, cmd, ap);
}

static short fs_socket_poll(void *hnd, short events) {
    net_socket_t *sock = (net_socket_t *)hnd;
    return sock->protocol->poll(sock, events);
}

static int fs_socket_fstat(void *hnd, struct stat *st) {
    (void)hnd;

    /* Not very useful, but whatever... */
    memset(st, 0, sizeof(struct stat));
    st->st_mode = S_IFSOCK;
    return 0;
}

/* VFS handler */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/sock",        /* Name */
        0,              /* tbfi */
        0x00010000,     /* Version 1.0 */
        0,              /* Flags */
        NMMGR_TYPE_VFS,
        NMMGR_LIST_INIT,
    },

    0, NULL,        /* No cache, privdata */

    NULL,            /* open */
    fs_socket_close, /* close */
    fs_socket_read,  /* read */
    fs_socket_write, /* write */
    fs_socket_seek,  /* seek */
    fs_socket_tell,  /* tell */
    NULL,            /* total */
    NULL,            /* readdir */
    NULL,            /* ioctl */
    NULL,            /* rename */
    NULL,            /* unlink */
    NULL,            /* mmap */
    NULL,            /* complete */
    NULL,            /* stat */
    NULL,            /* mkdir */
    NULL,            /* rmdir */
    fs_socket_fcntl, /* fcntl */
    fs_socket_poll,  /* poll */
    NULL,            /* link */
    NULL,            /* symlink */
    NULL,            /* seek64 */
    NULL,            /* tell64 */
    NULL,            /* total64 */
    NULL,            /* readlink */
    NULL,            /* rewinddir */
    fs_socket_fstat  /* fstat */
};

/* Have we been initialized? */
static int initted = 0;

int fs_socket_init(void) {
    if(initted == 1)
        return 0;

    TAILQ_INIT(&protocols);
    LIST_INIT(&sockets);

    if(nmmgr_handler_add(&vh.nmmgr) < 0)
        return -1;

    initted = 1;

    return 0;
}

int fs_socket_shutdown(void) {
    net_socket_t *c, *n;
    fs_socket_proto_t *i, *j;

    if(initted == 0)
        return 0;

    c = LIST_FIRST(&sockets);

    while(c != NULL) {
        n = LIST_NEXT(c, sock_list);
        fs_close(c->fd);
        c = n;
    }

    if(nmmgr_handler_remove(&vh.nmmgr) < 0)
        return -1;

    i = TAILQ_FIRST(&protocols);

    while(i != NULL) {
        j = TAILQ_NEXT(i, entry);
        free(i);
        i = j;
    }

    TAILQ_INIT(&protocols);
    LIST_INIT(&sockets);
    initted = 0;

    return 0;
}

int fs_socket_input(netif_t *src, int domain, int protocol, const void *hdr,
                    const uint8_t *data, size_t size) {
    fs_socket_proto_t *i;
    int rv = -2;

    if(!initted)
        return -1;

    /* Find the protocol handler and call its input function... */
    if(mutex_lock_irqsafe(&proto_rlock))
        return -1;

    TAILQ_FOREACH(i, &protocols, entry) {
        if(i->protocol == protocol) {
            rv = i->input(src, domain, hdr, data, size);
            break;
        }
    }

    mutex_unlock(&proto_rlock);

    return rv;
}

int fs_socket_proto_add(fs_socket_proto_t *proto) {
    if(!initted)
        return -1;

    if(mutex_lock_irqsafe(&proto_rlock))
        return -1;

    TAILQ_INSERT_TAIL(&protocols, proto, entry);
    mutex_unlock(&proto_rlock);

    return 0;
}

int fs_socket_proto_remove(fs_socket_proto_t *proto) {
    fs_socket_proto_t *i;
    int rv = -1;

    if(!initted)
        return -1;

    /* Make sure its registered. */
    if(mutex_lock_irqsafe(&proto_rlock))
        return -1;

    TAILQ_FOREACH(i, &protocols, entry) {
        if(i == proto) {
            /* We've got it, remove it. */
            TAILQ_REMOVE(&protocols, proto, entry);
            rv = 0;
            break;
        }
    }

    mutex_unlock(&proto_rlock);

    return rv;
}

int socket(int domain, int type, int protocol) {
    net_socket_t *sock;
    fs_socket_proto_t *i;

    /* We only support IPv4 and IPv6 sockets for now. */
    if(domain != PF_INET && domain != PF_INET6) {
        errno = EAFNOSUPPORT;
        return -1;
    }

    if(mutex_lock_irqsafe(&proto_rlock))
        return -1;

    /* Look for a matching protocol entry. */
    TAILQ_FOREACH(i, &protocols, entry) {
        if(type == i->type && (protocol == i->protocol || protocol == 0)) {
            break;
        }
    }

    /* If i is NULL, we got through the whole list without finding anything. */
    if(!i) {
        errno = EPROTONOSUPPORT;
        mutex_unlock(&proto_rlock);
        return -1;
    }

    /* Allocate the socket structure, if we have the space */
    sock = (net_socket_t *)malloc(sizeof(net_socket_t));

    if(!sock) {
        errno = ENOMEM;
        mutex_unlock(&proto_rlock);
        return -1;
    }

    /* Attempt to get a handle for this socket */
    sock->fd = fs_open_handle(&vh, sock);

    if(sock->fd < 0) {
        free(sock);
        mutex_unlock(&proto_rlock);
        return -1;
    }

    /* Initialize protocol-specific data */
    if(i->socket(sock, domain, type, protocol) == -1) {
        fs_close(sock->fd);
        mutex_unlock(&proto_rlock);
        return -1;
    }

    /* Finish initialization */
    sock->protocol = i;
    mutex_unlock(&proto_rlock);

    /* Add this socket into the list of sockets, and return */
    if(mutex_lock_irqsafe(&list_rlock)) {
        free(sock);
        return -1;
    }

    LIST_INSERT_HEAD(&sockets, sock, sock_list);
    mutex_unlock(&list_rlock);

    return sock->fd;
}

net_socket_t *fs_socket_open_sock(fs_socket_proto_t *proto) {
    net_socket_t *sock;

    /* Allocate the socket structure, if we have the space */
    sock = (net_socket_t *)malloc(sizeof(net_socket_t));

    if(!sock) {
        errno = ENOMEM;
        return NULL;
    }

    /* Attempt to get a handle for this socket */
    sock->fd = fs_open_handle(&vh, sock);

    if(sock->fd < 0) {
        free(sock);
        return NULL;
    }

    /* Initialize as much as we can */
    sock->protocol = proto;

    /* Add this socket into the list of sockets, and return */
    if(mutex_lock_irqsafe(&list_rlock)) {
        free(sock);
        return NULL;
    }

    LIST_INSERT_HEAD(&sockets, sock, sock_list);
    mutex_unlock(&list_rlock);

    return sock;
}

int accept(int sock, struct sockaddr *address, socklen_t *address_len) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->accept(hnd, address, address_len);
}

int bind(int sock, const struct sockaddr *address, socklen_t address_len) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->bind(hnd, address, address_len);
}

int connect(int sock, const struct sockaddr *address, socklen_t address_len) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->connect(hnd, address, address_len);
}

int listen(int sock, int backlog) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->listen(hnd, backlog);
}

ssize_t recv(int sock, void *buffer, size_t length, int flags) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->recvfrom(hnd, buffer, length, flags, NULL, NULL);
}

ssize_t recvfrom(int sock, void *buffer, size_t length, int flags,
                 struct sockaddr *address, socklen_t *address_len) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->recvfrom(hnd, buffer, length, flags, address,
                                   address_len);
}

ssize_t send(int sock, const void *message, size_t length, int flags) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->sendto(hnd, message, length, flags, NULL, 0);
}

ssize_t sendto(int sock, const void *message, size_t length, int flags,
               const struct sockaddr *dest_addr, socklen_t dest_len) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->sendto(hnd, message, length, flags, dest_addr,
                                 dest_len);
}

int shutdown(int sock, int how) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->shutdownsock(hnd, how);
}

int getsockname(int sock, struct sockaddr *name, socklen_t *name_len) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->getsockname(hnd, name, name_len);
}

int getpeername(int sock, struct sockaddr *__RESTRICT name, socklen_t *__RESTRICT name_len) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    if(!hnd->protocol->getpeername) {
        errno = EOPNOTSUPP;
        return -1;
    }

    return hnd->protocol->getpeername(hnd, name, name_len);
}

int getsockopt(int sock, int level, int option_name, void *option_value,
               socklen_t *option_len) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->getsockopt(hnd, level, option_name, option_value,
                                     option_len);
}

int setsockopt(int sock, int level, int option_name, const void *option_value,
               socklen_t option_len) {
    net_socket_t *hnd;

    hnd = (net_socket_t *)fs_get_handle(sock);

    if(hnd == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Make sure this is actually a socket. */
    if(fs_get_handler(sock) != &vh) {
        errno = ENOTSOCK;
        return -1;
    }

    return hnd->protocol->setsockopt(hnd, level, option_name, option_value,
                                     option_len);
}
