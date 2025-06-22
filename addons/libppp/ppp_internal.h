/* KallistiOS ##version##

   libppp/ppp_internal.h
   Copyright (C) 2007, 2014 Lawrence Sebald
*/

#ifndef __LOCAL_PPP_PPP_INTERNAL_H
#define __LOCAL_PPP_PPP_INTERNAL_H

#include <ppp/ppp.h>

#include <kos/thread.h>
#include <kos/net.h>

#ifdef PPP_DEBUG
#include <kos/dbglog.h>

#define DBG(...) dbglog(DBG_KDEBUG, __VA_ARGS__)
#else
#define DBG(...)
#endif

/* Internal state. */
typedef struct ppp_state {
    int initted;
    int state;
    int phase;

    uint32_t our_flags;
    uint32_t peer_flags;
    uint32_t our_magic;
    uint32_t peer_magic;

    uint32_t out_accm[8];
    uint32_t in_accm[8];

    uint16_t auth_proto;
    uint16_t peer_mru;
    uint8_t chap_type;

    char *username;
    char *passwd;

    ppp_device_t *device;
    kthread_t *thd;
    netif_t *netif;
} ppp_state_t;

/* PPP States - RFC 1661 Section 4.2 */
#define PPP_STATE_INITIAL       0x01
#define PPP_STATE_STARTING      0x02
#define PPP_STATE_CLOSED        0x03
#define PPP_STATE_STOPPED       0x04
#define PPP_STATE_CLOSING       0x05
#define PPP_STATE_STOPPING      0x06
#define PPP_STATE_REQUEST_SENT  0x07
#define PPP_STATE_ACK_RECEIVED  0x08
#define PPP_STATE_ACK_SENT      0x09
#define PPP_STATE_OPENED        0x0a

/* PPP Protocols we might care about. */
#define PPP_PROTOCOL_IPv4       0x0021
#define PPP_PROTOCOL_IPv6       0x0057

#define PPP_PROTOCOL_IPCP       0x8021    /* RFC 1332 */
#define PPP_PROTOCOL_IPV6CP     0x8057    /* RFC 2472 */

#define PPP_PROTOCOL_LCP        0xc021
#define PPP_PROTOCOL_PAP        0xc023    /* RFC 1334 */
#define PPP_PROTOCOL_CHAP       0xc223    /* RFC 1994 */

/* LCP packet structure - RFC 1661 Section 5 */
typedef struct lcp_packet {
    uint8_t code;
    uint8_t id;
    uint16_t len;
    uint8_t data[];
} __packed lcp_pkt_t;

typedef struct lcp_packet ipcp_pkt_t;

/* LCP Packet codes - RFC 1661 Section 5
   Note: These also apply (in part) to network control protocols like IPCP. */
#define LCP_CONFIGURE_REQUEST   1
#define LCP_CONFIGURE_ACK       2
#define LCP_CONFIGURE_NAK       3
#define LCP_CONFIGURE_REJECT    4
#define LCP_TERMINATE_REQUEST   5
#define LCP_TERMINATE_ACK       6
#define LCP_CODE_REJECT         7
#define LCP_PROTOCOL_REJECT     8
#define LCP_ECHO_REQUEST        9
#define LCP_ECHO_REPLY          10
#define LCP_DISCARD_REQUEST     11

/* From ppp.c */
int _ppp_enter_phase(int phase);

/* From lcp.c */
int _ppp_lcp_init(ppp_state_t *state);

/* From pap.c */
int _ppp_pap_init(ppp_state_t *state);

/* From ipcp.c */
int _ppp_ipcp_init(ppp_state_t *state);

#endif /* !__LOCAL_PPP_PPP_INTERNAL_H */
