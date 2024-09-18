/* KallistiOS ##version##

   sys/socket.h
   Copyright (C) 2006, 2010, 2012, 2017 Lawrence Sebald

*/

/** \file    sys/socket.h
    \brief   Main sockets header.
    \ingroup networking_sockets

    This file contains the standard definitions (as directed by the POSIX 2008
    spec) for socket-related functionality in the AF_INET and AF_INET6 address
    families. This does not include anything related to UNIX domain sockets
    and is not guaranteed to have everything that one might have in a
    fully-standards compliant implementation of the POSIX spec.

    \author Lawrence Sebald
*/

#ifndef __SYS_SOCKET_H
#define __SYS_SOCKET_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/uio.h>

__BEGIN_DECLS

/** \defgroup networking_sockets    Sockets
    \brief                          POSIX Sockets Interface for IPv4 and IPv6
                                    Address Families
    \ingroup                        networking

    @{
*/

/** \brief  Socket length type. */
typedef __uint32_t socklen_t;

/** \brief  Socket address family type. */
typedef __uint8_t sa_family_t;

/** \brief  Socket address structure.
    \headerfile sys/socket.h
*/
struct sockaddr {
    /** \brief  Address family. */
    sa_family_t sa_family;
    /** \brief  Address data. */
    char        sa_data[];
};

/** \brief  Size of the struct sockaddr_storage.
    The size here is chosen for compatibility with anything that may expect the
    struct sockaddr_storage to be of size 128. Technically, since there are no
    current plans to support AF_UNIX sockets, this could be smaller, but most
    implementations make it 128 bytes.
*/
#define _SS_MAXSIZE 128

/** \brief  Desired alignment of struct sockaddr_storage. */
#define _SS_ALIGNSIZE (sizeof(__uint64_t))

/** \brief  First padding size used within struct sockaddr_storage. */
#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof(sa_family_t))

/** \brief  Second padding size used within struct sockaddr_storage. */
#define _SS_PAD2SIZE (_SS_MAXSIZE - (sizeof(sa_family_t) + \
                                     _SS_PAD1SIZE + _SS_ALIGNSIZE))

/** \brief  Socket address structure of appropriate size to hold any supported
            socket type's addresses.
    \headerfile sys/socket.h
*/
struct sockaddr_storage {
    /** \brief  Address family. */
    sa_family_t  ss_family;

    /** \brief  First padding field. */
    char _ss_pad1[_SS_PAD1SIZE];

    /** \brief  Used to force alignment. */
    __uint64_t _ss_align;

    /** \brief  Second padding field to fill up the space required. */
    char _ss_pad2[_SS_PAD2SIZE];
};

/** \brief  Datagram socket type.

    This socket type specifies that the socket in question transmits datagrams
    that may or may not be reliably transmitted. With IP, this implies using UDP
    as the underlying protocol.
*/
#define SOCK_DGRAM  1

/** \brief  Stream socket type.

    This socket type specifies that the socket in question acts like a stream
    or pipe between the two endpoints. Sockets of this type can be assumed to be
    reliable -- unless an error is returned, all packets will be received at the
    other end in the order they are sent. With IP, this implies using TCP as the
    underlying protocol.
*/
#define SOCK_STREAM 2

/** \brief  Socket-level option setting.

    This constant should be used with the setsockopt() or getsockopt() function
    to represent that options should be accessed at the socket level, not the
    protocol level.
*/
#define SOL_SOCKET  1

/** @} */

/** \defgroup so_opts                   Options
    \brief                              Socket-level options
    \ingroup                            networking_sockets

    These are the various socket-level options that can be accessed with the
    setsockopt() and getsockopt() functions for the SOL_SOCKET level value.

    Not all of these are currently supported, but they are listed for
    completeness.

    \see                ipv6_opts
    \see                ipv4_opts
    \see                udp_opts

    @{
*/
#define SO_ACCEPTCONN   1   /**< \brief Socket is accepting connections (get) */
#define SO_BROADCAST    2   /**< \brief Support broadcasting (get/set) */
#define SO_DEBUG        3   /**< \brief Record debugging info (get/set) */
#define SO_DONTROUTE    4   /**< \brief Do not route packets (get/set) */
#define SO_ERROR        5   /**< \brief Retrieve error status (get) */
#define SO_KEEPALIVE    6   /**< \brief Send keepalive messages (get/set) */
#define SO_LINGER       7   /**< \brief Socket lingers on close (get/set) */
#define SO_OOBINLINE    8   /**< \brief OOB data is inline (get/set) */
#define SO_RCVBUF       9   /**< \brief Receive buffer size (get/set) */
#define SO_RCVLOWAT     10  /**< \brief Receive low-water mark (get/set) */
#define SO_RCVTIMEO     11  /**< \brief Receive timeout value (get/set) */
#define SO_REUSEADDR    12  /**< \brief Reuse local addresses (get/set) */
#define SO_SNDBUF       13  /**< \brief Send buffer size (get/set) */
#define SO_SNDLOWAT     14  /**< \brief Send low-water mark (get/set) */
#define SO_SNDTIMEO     15  /**< \brief Send timeout value (get/set) */
#define SO_TYPE         16  /**< \brief Socket type (get) */
/** @} */

/** \defgroup msg_flags                 Message Flags
    \brief                              Socket message flags
    \ingroup                            networking_sockets

    The following flags can be used with the recv(), recvfrom(), send(),
    and sendto() functions as the flags parameter.

    Note that not all of these are currently supported, but they are listed for
    completeness. Those that are unsupported have (U) at the end of their
    description. Also, for the time being, the supported flags are only
    supported for TCP.

    @{
*/
#define MSG_CTRUNC      0x01    /**< \brief Control data truncated (U) */
#define MSG_DONTROUTE   0x02    /**< \brief Send without routing (U) */
#define MSG_EOR         0x04    /**< \brief Terminate a record (U) */
#define MSG_OOB         0x08    /**< \brief Out-of-band data (U) */
#define MSG_PEEK        0x10    /**< \brief Leave received data in queue */
#define MSG_TRUNC       0x20    /**< \brief Normal data truncated (U) */
#define MSG_WAITALL     0x40    /**< \brief Attempt to fill read buffer */
#define MSG_DONTWAIT    0x80    /**< \brief Make this call non-blocking (non-standard) */
/** @} */

/** \addtogroup networking_sockets
    @{
*/

/** \brief  Unspecified address family. */
#define AF_UNSPEC   0

/** \brief  Internet domain sockets for use with IPv4 addresses. */
#define AF_INET     1

/** \brief  Internet domain sockets for use with IPv6 addresses. */
#define AF_INET6    2

/** \brief  Unspecified protocol family. */
#define PF_UNSPEC   AF_UNSPEC

/** \brief  Protocol family for Internet domain sockets (IPv4). */
#define PF_INET     AF_INET

/** \brief  Protocol family for Internet domain sockets (IPv6). */
#define PF_INET6    AF_INET6

/** \brief  Disable further receive operations. */
#define SHUT_RD   0x00000001

/** \brief  Disable further send operations. */
#define SHUT_WR   0x00000002

/** \brief  Disable further send and receive operations. */
#define SHUT_RDWR (SHUT_RD | SHUT_WR)

/** \brief  Maximum backlog for a listening socket. */
#define SOMAXCONN   32

/** \brief  Accept a new connection on a socket.

    This function extracts the first connection on the queue of connections of
    the specified socket, creating a new socket with the same protocol and
    address family as that socket for communication with the extracted
    connection.

    \param  socket      A socket created with socket() that has been bound to an
                        address with bind() and is listening for connections
                        after a call to listen().
    \param  address     A pointer to a sockaddr structure where the address of
                        the connecting socket will be returned (can be NULL).
    \param  address_len A pointer to a socklen_t which specifies the amount of
                        space in address on input, and the amount used of the
                        space on output.
    
    \return             On success, the non-negative file descriptor of the
                        new connection, otherwise -1 and errno will be set to
                        the appropriate error value.
*/
int accept(int socket, struct sockaddr *address, socklen_t *address_len);

/** \brief  Bind a name to a socket.

    This function assigns the socket to a unique name (address).

    \param  socket      A socket that is to be bound.
    \param  address     A pointer to a sockaddr structure where the name to be
                        assigned to the socket resides.
    \param  address_len The length of the address structure.
    
    \retval 0           On success.
    \retval -1          On error, sets errno as appropriate.
*/
int bind(int socket, const struct sockaddr *address, socklen_t address_len);

/** \brief  Connect a socket.

    This function attempts to make a connection to a resource on a connection-
    mode socket, or sets/resets the peer address on a connectionless one.

    \param  socket      A socket that is to be connected.
    \param  address     A pointer to a sockaddr structure where the name of the
                        peer resides.
    \param  address_len The length of the address structure.
    
    \retval 0           On success.
    \retval -1          On error, sets errno as appropriate.
*/
int connect(int socket, const struct sockaddr *address, socklen_t address_len);

/** \brief  Listen for socket connections and set the queue length.

    This function marks a connection-mode socket for incoming connections.

    \param  socket      A connection-mode socket to listen on.
    \param  backlog     The number of queue entries.
    
    \retval 0           On success.
    \retval -1          On error, sets errno as appropriate.
*/
int listen(int socket, int backlog);

/** \brief  Receive a message on a connected socket.

    This function receives messages from the peer on a connected socket.

    \param  socket      The socket to receive on.
    \param  buffer      A pointer to a buffer to store the message in.
    \param  length      The length of the buffer.
    \param  flags       The type of message reception. Set to 0 for now.
    
    \return             On success, the length of the message in bytes. If no
                        messages are available, and the socket has been shut
                        down, 0. On error, -1, and sets errno as appropriate.
*/
ssize_t recv(int socket, void *buffer, size_t length, int flags);

/** \brief  Receive a message on a socket.

    This function receives messages from a peer on a (usually connectionless)
    socket.

    \param  socket      The socket to receive on.
    \param  buffer      A pointer to a buffer to store the message in.
    \param  length      The length of the buffer.
    \param  flags       The type of message reception. Set to 0 for now.
    \param  address     A pointer to a sockaddr structure to store the peer's
                        name in.
    \param  address_len A pointer to the length of the address structure on
                        input, the number of bytes used on output.
    
    \return             On success, the length of the message in bytes. If no
                        messages are available, and the socket has been shut
                        down, 0. On error, -1, and sets errno as appropriate.
*/
ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
                 struct sockaddr *address, socklen_t *address_len);

/** \brief  Send a message on a connected socket.

    This function sends messages to the peer on a connected socket.

    \param  socket      The socket to send on.
    \param  message     A pointer to a buffer with the message to send.
    \param  length      The length of the message.
    \param  flags       The type of message transmission. Set to 0 for now.
    
    \return             On success, the number of bytes sent. On error, -1,
                        and sets errno as appropriate.
*/
ssize_t send(int socket, const void *message, size_t length, int flags);

/** \brief  Send a message on a socket.

    This function sends messages to the peer on a (usually connectionless)
    socket. If used on a connection-mode socket, this function may change the
    peer that the socket is connected to, or it may simply return error.

    \param  socket      The socket to send on.
    \param  message     A pointer to a buffer with the message to send.
    \param  length      The length of the message.
    \param  flags       The type of message transmission. Set to 0 for now.
    \param  dest_addr   A pointer to a sockaddr structure with the peer's name.
    \param  dest_len    The length of dest_addr, in bytes.
    
    \return             On success, the number of bytes sent. On error, -1,
                        and sets errno as appropriate.
*/
ssize_t sendto(int socket, const void *message, size_t length, int flags,
               const struct sockaddr *dest_addr, socklen_t dest_len);

/** \brief  Shutdown socket send and receive operations.

    This function closes a specific socket for the set of specified operations.

    \param  socket      The socket to shutdown.
    \param  how         The type of shutdown.
    
    \retval 0           On success.
    \retval -1          On error, sets errno as appropriate.
    
    \see                SHUT_RD
    \see                SHUT_WR
    \see                SHUT_RDWR
*/
int shutdown(int socket, int how);

/** \brief  Create an endpoint for communications.

    This function creates an unbound socket for communications with the
    specified parameters.

    \param  domain      The domain to create the socket in (i.e, AF_INET).
    \param  type        The type of socket to be created (i.e, SOCK_DGRAM).
    \param  protocol    The protocol to use with the socket. May be 0 to allow
                        a default to be used.
    
    \return             A non-negative file descriptor on success. -1 on error,
                        and sets errno as appropriate.
*/
int socket(int domain, int type, int protocol);

/** \brief  Get socket name.

    This function returns the locally bound address information for a specified
    socket.

    \param  socket          The socket to get the name of.
    \param  name            Pointer to a sockaddr structure which will hold the
                            resulting address information.
    \param  name_len        The amount of space pointed to by name, in bytes.
                            On return, this is set to the actual size of the
                            returned address information.
    
    \retval -1              On error, sets errno as appropriate.
    \retval 0               On success.
*/
int getsockname(int socket, struct sockaddr *name, socklen_t *name_len);

/**
    \brief  Get the name of the connected peer socket.

    This function retrieves the address of the peer connected to the socket
    specified by the socket descriptor. The address is returned in the buffer
    pointed to by the name parameter, and the actual length of the address is
    returned in the name_len parameter.

    \param  socket          A socket that is already connected to a peer.
    \param  name            A pointer to a sockaddr structure where the peer 
                            address will be stored.
    \param  name_len        A pointer to a socklen_t variable that specifies the 
                            length of the address structure.
                            On return, it will contain the actual size of the 
                            address returned.

    \retval 0               On success.
    \retval -1              On error, sets errno as appropriate, such as:
                            - EBADF: The socket argument is not a valid file 
                                     descriptor.
                            - EFAULT: The addr argument points to memory not in a 
                                      valid part of the process address space.
                            - ENOBUFS: Insufficient resources were available in the 
                                       system to perform the operation.
                            - ENOTCONN: The socket is not connected.
                            - ENOTSOCK: The socket argument does not refer to a 
                                        socket.
                            - EOPNOTSUPP: The socket does not support getpeername.
*/
int getpeername(int socket, struct sockaddr *__RESTRICT name, 
                socklen_t *__RESTRICT name_len);

/** \brief  Get socket options.

    This function retrieves options associated with a socket. This function
    shall attempt to retrieve the specified option (at the specified level) from
    the given socket.

    \param  socket          The socket to get options for.
    \param  level           The protocol level to get options at.
    \param  option_name     The option to look up.
    \param  option_value    Storage for the value of the option.
    \param  option_len      The length of option_value on call, and the real
                            option length (if less than the original value)
                            on return.
    
    \return                 Zero on success. -1 on error, and sets errno as
                            appropriate.

    \see                    so_opts
    \see                    ipv4_opts
    \see                    ipv6_opts
    \see                    udp_opts
*/
int getsockopt(int socket, int level, int option_name, void *option_value,
               socklen_t *option_len);

/** \brief  Set socket options.

    This function sets options associated with a socket. This function shall
    attempt to set the specified option (at the specified level) from the given
    socket.

    \param  socket          The socket to set options for.
    \param  level           The protocol level to set options at.
    \param  option_name     The option to set.
    \param  option_value    The value to set for the option.
    \param  option_len      The length of option_value in bytes.
    
    \return                 Zero on success. -1 on error, and sets errno as
                            appropriate.

    \see                    so_opts
    \see                    ipv4_opts
    \see                    ipv6_opts
    \see                    udp_opts
*/
int setsockopt(int socket, int level, int option_name, const void *option_value,
               socklen_t option_len);

/** @} */

__END_DECLS

#endif /* __SYS_SOCKET_H */
