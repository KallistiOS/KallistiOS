/* KallistiOS ##version##

   kernel/net/net_icmp.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2005, 2007, 2010, 2013 Lawrence Sebald

*/

#ifndef __LOCAL_NET_ICMP_H
#define __LOCAL_NET_ICMP_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/net.h>
#include "net_ipv4.h"

typedef struct {
    uint8 type;
    uint8 code;
    uint16 checksum;
    union {
        uint8 m8[4];
        uint16 m16[2];
        uint32 m32;
    } misc;
} icmp_hdr_t;

#define ICMP_MESSAGE_ECHO_REPLY         0
#define ICMP_MESSAGE_DEST_UNREACHABLE   3
#define ICMP_MESSAGE_ECHO               8
#define ICMP_MESSAGE_TIME_EXCEEDED      11

int net_icmp_input(netif_t *src, const ip_hdr_t *ih, const uint8 *data,
                   size_t size);

__END_DECLS

#endif  /* __LOCAL_NET_ICMP_H */
