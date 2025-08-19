/* KallistiOS ##version##

   netinet/udplite.h
   Copyright (C) 2014, 2023 Lawrence Sebald

*/

/** \file    netinet/udplite.h
    \brief   Definitions for UDP-Lite.
    \ingroup networking_udp

    This file contains definitions related to UDP-Lite, a version of UDP that
    allows for partial checksum coverage (rather than requiring that either all
    or none of the packet is covered as does UDP).

    UDP-Lite is described in RFC 3828.

    \author Lawrence Sebald
*/

#ifndef __NETINET_UDPLITE_H
#define __NETINET_UDPLITE_H

#include <kos/cdefs.h>

__BEGIN_DECLS

/** \defgroup udplite_opts              UDP-Lite Options
    \brief                              UDP-Lite protocol level options
    \ingroup                            networking_udp

    These are the various socket-level options that can be accessed with the
    setsockopt() and getsockopt() functions for the IPPROTO_UDPLITE level value.

    \see                so_opts
    \see                ipv4_opts
    \see                ipv6_opts
    \see                udp_opts
    \see                tcp_opts

    @{
*/

#define UDPLITE_SEND_CSCOV      26 /**< \brief Sending checksum coverage. */
#define UDPLITE_RECV_CSCOV      27 /**< \brief Receiving checksum coverage. */

/** @} */

__END_DECLS

#endif /* !__NETINET_UDPLITE_H */
