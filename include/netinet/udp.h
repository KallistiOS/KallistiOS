/* KallistiOS ##version##

   netinet/udp.h
   Copyright (C) 2014, 2023 Lawrence Sebald

*/

/** \file    netinet/udp.h
    \brief   Definitions for the User Datagram Protocol.
    \ingroup networking_udp

    This file contains definitions related to the User Datagram Protocol (UDP).
    UDP is a connectionless datagram delivery protocol, which provides optional
    datagram integrity validation.

    UDP is described in RFC 768.

    \author Lawrence Sebald
*/

#ifndef __NETINET_UDP_H
#define __NETINET_UDP_H

#include <kos/cdefs.h>

__BEGIN_DECLS

/** \defgroup udp_opts                  UDP Options
    \brief                              UDP protocol level options
    \ingroup                            networking_udp

    These are the various socket-level options that can be accessed with the
    setsockopt() and getsockopt() functions for the IPPROTO_UDP level value.

    \see                so_opts
    \see                ipv4_opts
    \see                ipv6_opts
    \see                udplite_opts
    \see                tcp_opts

    @{
*/

#define UDP_NOCHECKSUM          25  /**< \brief Don't calculate UDP checksums */

/** @} */

__END_DECLS

#endif /* !__NETINET_UDP_H */
