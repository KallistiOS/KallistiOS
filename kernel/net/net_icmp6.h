/* KallistiOS ##version##

   kernel/net/net_icmp6.h
   Copyright (C) 2010, 2013 Lawrence Sebald

*/

#ifndef __LOCAL_NET_ICMP6_H
#define __LOCAL_NET_ICMP6_H

#include <kos/net.h>
#include "net_ipv6.h"

typedef struct icmp6_hdr_s {
    uint8   type;
    uint8   code;
    uint16  checksum;
} icmp6_hdr_t;

/* Header for Destination Unreachable packets (type 1) */
typedef struct icmp6_dest_unreach_s {
    uint8   type;
    uint8   code;
    uint16  checksum;
    uint32  unused;
} icmp6_dest_unreach_t;

/* Header for Packet Too Big packets (type 2) */
typedef struct icmp6_pkt_too_big_s {
    uint8   type;
    uint8   code;
    uint16  checksum;
    uint32  mtu;
} icmp6_pkt_too_big_t;

/* Header for Time Exceeded packets (type 3) */
typedef struct icmp6_time_exceeded_s {
    uint8   type;
    uint8   code;
    uint16  checksum;
    uint32  unused;
} icmp6_time_exceeded_t;

/* Header for Parameter Problem packets (type 4) */
typedef struct icmp6_param_problem_s {
    uint8   type;
    uint8   code;
    uint16  checksum;
    uint32  ptr;
} icmp6_param_problem_t;

/* Header for Echo/Echo Reply packets (types 128/129) */
typedef struct icmp6_echo_hdr_s {
    uint8   type;
    uint8   code;
    uint16  checksum;
    uint16  ident;
    uint16  seq;
} icmp6_echo_hdr_t;

/* Format for Router Solicitation packets (type 133) - RFC 4861 */
typedef struct icmp6_router_sol_s {
    uint8   type;
    uint8   code;
    uint16  checksum;
    uint32  reserved;
    uint8   options[];
} icmp6_router_sol_t;

/* Format for Router Advertisement packets (type 134) - RFC 4861 */
typedef struct icmp6_router_adv_s {
    uint8   type;
    uint8   code;
    uint16  checksum;
    uint8   cur_hop_limit;
    uint8   flags;
    uint16  router_lifetime;
    uint32  reachable_time;
    uint32  retrans_timer;
    uint8   options[];
} icmp6_router_adv_t;

/* Format for Neighbor Solicitation packets (type 135) - RFC 4861 */
typedef struct icmp6_neighbor_sol_s {
    uint8           type;
    uint8           code;
    uint16          checksum;
    uint32          reserved;
    struct in6_addr target;
    uint8           options[];
} icmp6_neighbor_sol_t;

/* Format for Neighbor Advertisement packets (type 136) - RFC 4861 */
typedef struct icmp6_neighbor_adv_s {
    uint8           type;
    uint8           code;
    uint16          checksum;
    uint8           flags;
    uint8           reserved[3];
    struct in6_addr target;
    uint8           options[];
} icmp6_neighbor_adv_t;

/* Link-layer address option for neighbor advertisement/solictation packets for
   ethernet. */
typedef struct icmp6_nsol_lladdr_s {
    uint8           type;
    uint8           length;
    uint8           mac[6];
} icmp6_nsol_lladdr_t;

/* Redirect packet (type 137) - RFC 4861 */
typedef struct icmp6_redirect_s {
    uint8           type;
    uint8           code;
    uint16          checksum;
    uint32          reserved;
    struct in6_addr target;
    struct in6_addr dest;
    uint8           options[];
} icmp6_redirect_t;

/* Prefix information for router advertisement packets */
typedef struct icmp6_ndp_prefix_s {
    uint8           type;
    uint8           length;
    uint8           prefix_length;
    uint8           flags;
    uint32          valid_time;
    uint32          preferred_time;
    uint32          reserved;
    struct in6_addr prefix;
} icmp6_ndp_prefix_t;

/* ICMPv6 Message types */
/* Error messages (type < 127) */
#define ICMP6_MESSAGE_DEST_UNREACHABLE  1
#define ICMP6_MESSAGE_PKT_TOO_BIG       2
#define ICMP6_MESSAGE_TIME_EXCEEDED     3
#define ICMP6_MESSAGE_PARAM_PROBLEM     4

/* Informational messages (127 < type < 255) */
#define ICMP6_MESSAGE_ECHO              128
#define ICMP6_MESSAGE_ECHO_REPLY        129

/* Neighbor Discovery Protocol (RFC 4861) */
#define ICMP6_ROUTER_SOLICITATION       133
#define ICMP6_ROUTER_ADVERTISEMENT      134
#define ICMP6_NEIGHBOR_SOLICITATION     135
#define ICMP6_NEIGHBOR_ADVERTISEMENT    136
#define ICMP6_REDIRECT                  137     /* Not supported */

#define NDP_OPT_SOURCE_LINK_ADDR        1
#define NDP_OPT_TARGET_LINK_ADDR        2
#define NDP_OPT_PREFIX_INFO             3
#define NDP_OPT_REDIRECTED_HDR          4
#define NDP_OPT_MTU                     5

int net_icmp6_input(netif_t *src, ipv6_hdr_t *ih, const uint8 *data,
                    size_t size);

#endif /* !__LOCAL_NET_ICMP6_H */
