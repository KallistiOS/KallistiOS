/* KallistiOS ##version##

   netinet/tcp.h
   Copyright (C) 2023 Lawrence Sebald

*/

/** \file    netinet/tcp.h
    \brief   Definitions for the Transmission Control Protocol.
    \ingroup networking_tcp

    This file contains the standard definitions (as directed by the Open Group
    Base Specifications Issue 7, 2018 Edition aka POSIX 2017) for  functionality
    of the Transmission Control Protocol.
    This does is not guaranteed to have everything that one might have in a
    fully-standard compliant implementation of the POSIX standard.

    \author Lawrence Sebald
*/

#ifndef __NETINET_TCP_H
#define __NETINET_TCP_H

#include <kos/cdefs.h>

__BEGIN_DECLS

/** \defgroup tcp_opts                  Options
    \brief                              TCP protocol level options
    \ingroup                            networking_tcp

    These are the various socket-level options that can be accessed with the
    setsockopt() and getsockopt() functions for the IPPROTO_TCP level value.

    All options listed here are at least guaranteed to be accepted by
    setsockopt() and getsockopt() for IPPROTO_TCP, however they are not
    guaranteed to be implemented in any meaningful way.

    \see                so_opts
    \see                ipv4_opts
    \see                ipv6_opts
    \see                udp_opts
    \see                udplite_opts

    @{
*/

#define TCP_NODELAY             1 /**< \brief Don't delay to coalesce. */

/** @} */

__END_DECLS

#endif /* !__NETINET_TCP_H */
