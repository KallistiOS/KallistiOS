/* KallistiOS ##version##

   kernel/net/net_icmp.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2005, 2007, 2010, 2013 Lawrence Sebald

*/

#ifndef __LOCAL_NET_ICMP_H
#define __LOCAL_NET_ICMP_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/net.h>
#include "net_ipv4.h"

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    union {
        uint8_t m8[4];
        uint16_t m16[2];
        uint32_t m32;
    } misc;
} __packed icmp_hdr_t;

#define ICMP_MESSAGE_ECHO_REPLY         0
#define ICMP_MESSAGE_DEST_UNREACHABLE   3
#define ICMP_MESSAGE_ECHO               8
#define ICMP_MESSAGE_TIME_EXCEEDED      11

int net_icmp_input(netif_t *src, const ip_hdr_t *ih, const uint8_t *data,
                   size_t size);

__END_DECLS

#endif  /* __LOCAL_NET_ICMP_H */
