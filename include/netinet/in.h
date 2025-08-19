/* KallistiOS ##version##

   netinet/in.h
   Copyright (C) 2006, 2007, 2010 Lawrence Sebald

*/

/** \file    netinet/in.h
    \brief   Definitions for the Internet address family.
    \ingroup networking_ip

    This file contains the standard definitions (as directed by the POSIX 2008
    standard) for internet-related functionality in the AF_INET address family.
    This does is not guaranteed to have everything that one might have in a
    fully-standard compliant implementation of the POSIX standard.

    \author Lawrence Sebald
*/

#ifndef __NETINET_IN_H
#define __NETINET_IN_H

#include <kos/cdefs.h>

__BEGIN_DECLS

/* Bring in <inttypes.h> to grab the uint16_t and uint32_t types (for in_port_t
   and in_addr_t below), along with uint8_t. Bring in <sys/socket.h> for the
   sa_family_t type, as required. IEEE Std 1003.1-2008 specifically states that
   <netinet/in.h> can make visible all symbols from both <inttypes.h> and
   <sys/socket.h>. */
#include <inttypes.h>
#include <sys/socket.h>

/** \brief   16-bit type used to store a value for an internet port. 
    \ingroup networking_ip
*/
typedef uint16_t in_port_t;

/** \brief   32-bit value used to store an IPv4 address. 
    \ingroup networking_ipv4
*/
typedef uint32_t in_addr_t;

/** \brief  Structure used to store an IPv4 address.
    \headerfile netinet/in.h
    \ingroup networking_ipv4
*/
struct in_addr {
    in_addr_t s_addr;
};

/** \brief  Structure used to store an IPv6 address.
    \headerfile netinet/in.h
    \ingroup networking_ipv6
*/
struct in6_addr {
    union {
        uint8_t  __s6_addr8[16];
        uint16_t __s6_addr16[8];
        uint32_t __s6_addr32[4];
        uint64_t __s6_addr64[2];
    } __s6_addr;
#define s6_addr __s6_addr.__s6_addr8
};

/* Bring in <arpa/inet.h> to make ntohl/ntohs/htonl/htons visible, as per IEEE
   Std 1003.1-2008 (the standard specifically states that <netinet/in.h> may
   make all symbols from <arpa/inet.h> visible. The <arpa/inet.h> header
   actually needs the stuff above, so that's why we include it here. */
#include <arpa/inet.h>

/** \brief   Structure used to store an IPv4 address for a socket.
    \ingroup networking_ipv4

    This structure is the standard way to set up addresses for sockets in the
    AF_INET address family. Generally you will not send one of these directly
    to a function, but rather will cast it to a struct sockaddr. Also, this
    structure contains the old sin_zero member which is no longer required by
    the standard (for compatibility with applications that expect it).

    \headerfile netinet/in.h
*/
struct sockaddr_in {
    /** \brief  Family for the socket. Must be AF_INET. */
    sa_family_t    sin_family;

    /** \brief  Port for the socket. Must be in network byte order. */
    in_port_t      sin_port;

    /** \brief  Address for the socket. Must be in network byte order. */
    struct in_addr sin_addr;

    /** \brief  Empty space, ignored for all intents and purposes. */
    unsigned char  sin_zero[8];
};

/** \brief   Structure used to store an IPv6 address for a socket.
    \ingroup networking_ipv6

    This structure is the standard way to set up addresses for sockets in the
    AF_INET6 address family. Generally you will not send one of these directly
    to a function, but rather will cast it to a struct sockaddr.

    \headerfile netinet/in.h
 */
struct sockaddr_in6 {
    /** \brief  Family for the socket. Must be AF_INET6. */
    sa_family_t     sin6_family;

    /** \brief  Port for the socket. Must be in network byte order. */
    in_port_t       sin6_port;

    /** \brief  Traffic class and flow information. */
    uint32_t        sin6_flowinfo;

    /** \brief  Address for the socket. Must be in network byte order. */
    struct in6_addr sin6_addr;

    /** \brief  Set of interfaces for a scope. */
    uint32_t        sin6_scope_id;
};

/** \brief   Local IPv4 host address.
    \ingroup networking_ipv4

    This address can be used by many things if you prefer to not specify the
    local address, and would rather it be detected automatically.
*/
#define INADDR_ANY       0x00000000

/** \brief   IPv4 broadcast address.
    \ingroup networking_ipv4

    This address is the normal IPv4 broadcast address (255.255.255.255).
*/
#define INADDR_BROADCAST 0xFFFFFFFF

/** \brief   IPv4 error address.
    \ingroup networking_ipv4

    This address is non-standard, but is available on many systems. It is used
    to detect failure from some functions that normally return addresses (such
    as the inet_addr function).
*/
#define INADDR_NONE      0xFFFFFFFF

/** \brief   Initialize an IPv6 local host address.
    \ingroup networking_ipv6

    This macro can be used to initialize a struct in6_addr to any local address.
    It functions similarly to INADDR_ANY for IPv4.
*/
#define IN6ADDR_ANY_INIT {{{ 0, 0, 0, 0, 0, 0, 0, 0, \
                             0, 0, 0, 0, 0, 0, 0, 0 }}}

/** \brief   Initialize an IPv6 loopback address.
    \ingroup networking_ipv6

    This macro can be used to initialize a struct in6_addr to the loopback
    address.
*/
#define IN6ADDR_LOOPBACK_INIT {{{ 0, 0, 0, 0, 0, 0, 0, 0, \
                                  0, 0, 0, 0, 0, 0, 0, 1 }}}

/** \brief   IPv6 local host address.
    \ingroup networking_ipv6

    This constant variable contains the IPv6 local host address.
*/
extern const struct in6_addr in6addr_any;

/** \brief   IPv6 loopback address.
    \ingroup networking_ipv6

    This constant variable contains the IPv6 loopback address.
*/
extern const struct in6_addr in6addr_loopback;

/** \brief   Length of a string form of a maximal IPv4 address. 
    \ingroup networking_ipv4
*/
#define INET_ADDRSTRLEN 16

/** \brief   Length of a string form of a maximal IPv6 address. 
    \ingroup networking_ipv6
*/
#define INET6_ADDRSTRLEN 46

/** \brief   Internet Protocol Version 4. 
    \ingroup networking_ip
*/
#define IPPROTO_IP      0

/** \brief   Internet Control Message Protocol.
    \ingroup networking_ip
*/
#define IPPROTO_ICMP    1

/** \brief   Transmission Control Protocol. 
    \ingroup networking_ip
*/
#define IPPROTO_TCP     6

/** \brief   User Datagram Protocol. 
    \ingroup networking_ip
*/
#define IPPROTO_UDP     17

/** \brief   Internet Protocol Version 6. 
    \ingroup networking_ip
*/
#define IPPROTO_IPV6    41

/** \brief   Lightweight User Datagram Protocol. 
    \ingroup networking_ip
*/
#define IPPROTO_UDPLITE 136

/** \defgroup ipv4_opts                 Options
    \brief                              Options for v4 of the Internet Protocol
    \ingroup                            networking_ipv4

    These are the various socket-level options that can be accessed with the
    setsockopt() and getsockopt() functions for the IPPROTO_IP level value.

    As there isn't really a full standard list of these defined in the SUS
    (apparently), only ones that we support are listed here.

    \see                so_opts
    \see                ipv6_opts
    \see                udp_opts
    \see                udplite_opts
    \see                tcp_opts

    @{
*/

#define IP_TTL              24  /**< \brief TTL for unicast (get/set) */

/** @} */

/** \defgroup ipv6_opts                 Options
    \brief                              Options for v6 of the Internet Protocol
    \ingroup                            networking_ipv6

    These are the various socket-level options that can be accessed with the
    setsockopt() and getsockopt() functions for the IPPROTO_IPV6 level value.

    Not all of these are currently supported, but they are listed for
    completeness.

    \see                so_opts
    \see                ipv4_opts
    \see                udp_opts
    \see                udplite_opts
    \see                tcp_opts

    @{
*/

#define IPV6_JOIN_GROUP     17  /**< \brief Join a multicast group (set) */
#define IPV6_LEAVE_GROUP    18  /**< \brief Leave a multicast group (set) */
#define IPV6_MULTICAST_HOPS 19  /**< \brief Hop limit for multicast (get/set) */
#define IPV6_MULTICAST_IF   20  /**< \brief Multicast interface (get/set) */
#define IPV6_MULTICAST_LOOP 21  /**< \brief Multicasts loopback (get/set) */
#define IPV6_UNICAST_HOPS   22  /**< \brief Hop limit for unicast (get/set) */
#define IPV6_V6ONLY         23  /**< \brief IPv6 only -- no IPv4 (get/set) */

/** @} */

/** \brief   Test if an IPv6 Address is unspecified.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is an
    unspecified address.

    \param  a           The address to test (struct in6_addr *)
    
    \return             Nonzero if the address is unspecified, 0 otherwise.
*/
#define IN6_IS_ADDR_UNSPECIFIED(a)  \
    ((a)->__s6_addr.__s6_addr32[0] == 0 && \
     (a)->__s6_addr.__s6_addr32[1] == 0 && \
     (a)->__s6_addr.__s6_addr32[2] == 0 && \
     (a)->__s6_addr.__s6_addr32[3] == 0)

/** \brief   Test if an IPv6 Address is a loopback address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is a
    loopback address.

    \param  a           The address to test (struct in6_addr *)
    
    \return             Nonzero if the address is a loopback, 0 otherwise.
*/
#define IN6_IS_ADDR_LOOPBACK(a)  \
    ((a)->__s6_addr.__s6_addr32[0] == 0 && \
     (a)->__s6_addr.__s6_addr32[1] == 0 && \
     (a)->__s6_addr.__s6_addr32[2] == 0 && \
     (a)->__s6_addr.__s6_addr16[6] == 0 && \
     (a)->__s6_addr.__s6_addr8[14] == 0 && \
     (a)->__s6_addr.__s6_addr8[15] == 1)

/** \brief   Test if an IPv6 Address is an IPv4 mapped address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is an IPv4
    mapped address.

    \param  a           The address to test (struct in6_addr *)
    
    \return             Nonzero if the address is IPv4 mapped, 0 otherwise.
*/
#define IN6_IS_ADDR_V4MAPPED(a)  \
    ((a)->__s6_addr.__s6_addr32[0] == 0 &&      \
     (a)->__s6_addr.__s6_addr32[1] == 0 &&      \
     (a)->__s6_addr.__s6_addr16[4] == 0 &&      \
     (a)->__s6_addr.__s6_addr16[5] == 0xFFFF)

/** \brief   Test if an IPv6 Address is an IPv4 compatibility address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is an IPv4
    compatibility address.

    \param  a           The address to test (struct in6_addr *)
    
    \return             Nonzero if the address is IPv4 compat, 0 otherwise.
*/
#define IN6_IS_ADDR_V4COMPAT(a)  \
    ((a)->__s6_addr.__s6_addr32[0] == 0 && \
     (a)->__s6_addr.__s6_addr32[1] == 0 && \
     (a)->__s6_addr.__s6_addr32[2] == 0 && \
     (a)->__s6_addr.__s6_addr32[3] != 0 && \
     (a)->__s6_addr.__s6_addr8[15] != 1)

/** \brief   Test if an IPv6 Address is a link-local address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is a link-local
    address.

    \param  a           The address to test (struct in6_addr *)
    
    \return             Nonzero if the address is link-local, 0 otherwise.
*/
#define IN6_IS_ADDR_LINKLOCAL(a)  \
    (((a)->__s6_addr.__s6_addr8[0] == 0xFE) && \
     (((a)->__s6_addr.__s6_addr8[1] & 0xC0) == 0x80))

/** \brief   Test if an IPv6 Address is a site-local address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is a site-local
    address.

    \param  a           The address to test (struct in6_addr *)
    
    \return             Nonzero if the address is site-local, 0 otherwise.
*/
#define IN6_IS_ADDR_SITELOCAL(a)  \
    (((a)->__s6_addr.__s6_addr8[0] == 0xFE) && \
     (((a)->__s6_addr.__s6_addr8[1] & 0xC0) == 0xC0))

/** \brief   Test if an IPv6 Address is a multicast address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is a multicast
    address.

    \param  a           The address to test (struct in6_addr *)
    
    \return             Nonzero if the address is multicast, 0 otherwise.
*/
#define IN6_IS_ADDR_MULTICAST(a)  \
    ((a)->__s6_addr.__s6_addr8[0] == 0xFF)

/** \brief   Test if an IPv6 Address is a node-local multicast address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is a node-local
    multicast address.

    \param  a           The address to test (struct in6_addr *)
    
    \return             Nonzero if the address is a node-local multicast
                        address, 0 otherwise.
*/
#define IN6_IS_ADDR_MC_NODELOCAL(a)  \
    (IN6_IS_ADDR_MULTICAST(a) && \
     (((a)->__s6_addr.__s6_addr8[1] & 0x0F) == 0x01))

/** \brief   Test if an IPv6 Address is a link-local multicast address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is a link-local
    multicast address.

    \param  a           The address to test (struct in6_addr *)
    
    \return             Nonzero if the address is a link-local multicast
                        address, 0 otherwise.
*/
#define IN6_IS_ADDR_MC_LINKLOCAL(a)  \
    (IN6_IS_ADDR_MULTICAST(a) && \
     (((a)->__s6_addr.__s6_addr8[1] & 0x0F) == 0x02))

/** \brief   Test if an IPv6 Address is a site-local multicast address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is a site-local
    multicast address.

    \param  a           The address to test (struct in6_addr *)
    
    \return             Nonzero if the address is a site-local multicast
                        address, 0 otherwise.
*/
#define IN6_IS_ADDR_MC_SITELOCAL(a)  \
    (IN6_IS_ADDR_MULTICAST(a) && \
     (((a)->__s6_addr.__s6_addr8[1] & 0x0F) == 0x05))

/** \brief   Test if an IPv6 Address is an organization-local multicast address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is an
    organization-local multicast address.

    \param  a           The address to test (struct in6_addr *)

    \return             Nonzero if the address is an organization-local
                        multicast address, 0 otherwise.
*/
#define IN6_IS_ADDR_MC_ORGLOCAL(a)  \
    (IN6_IS_ADDR_MULTICAST(a) && \
     (((a)->__s6_addr.__s6_addr8[1] & 0x0F) == 0x08))

/** \brief   Test if an IPv6 Address is a global multicast address.
    \ingroup networking_ipv6

    This macro tests whether an IPv6 address (struct in6_addr *) is a global
    multicast address.

    \param  a           The address to test (struct in6_addr *)

    \return             Nonzero if the address is a global multicast address,
                        0 otherwise.
*/
#define IN6_IS_ADDR_MC_GLOBAL(a)  \
    (IN6_IS_ADDR_MULTICAST(a) && \
     (((a)->__s6_addr.__s6_addr8[1] & 0x0F) == 0x0E))

__END_DECLS

#endif /* __NETINET_IN_H */
