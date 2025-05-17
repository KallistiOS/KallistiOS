/* KallistiOS ##version##

   kernel/net/net_icmp.c

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2005, 2006, 2007, 2009, 2010, 2011, 2013 Lawrence Sebald

 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/queue.h>
#include <kos/dbglog.h>
#include <kos/net.h>
#include <kos/thread.h>
#include <arch/timer.h>
#include <arpa/inet.h>

#include "net_icmp.h"
#include "net_ipv4.h"

/*
This file implements RFC 792, the Internet Control Message Protocol.
Currently implemented message types are:
   0  - Echo Reply
   3  - Destination Unreachable
   8  - Echo
   11 - Time Exceeded

Message types that are not implemented yet (if ever):
   4  - Source Quench
   5  - Redirect
   6  - Alternate Host Address
   9  - Router Advertisement
   10 - Router Solicitation
   12 - Parameter Problem
   14 - Timestamp Reply
   13 - Timestamp
   15 - Information Request
   16 - Information Reply
   17 - Address Mask Request
   18 - Address Mask Reply
   30 - Traceroute
   31 - Datagram Conversion Error
   32 - Mobile Host Redirect
   33 - Where-Are-You
   34 - Here-I-Am
   35 - Mobile Registration Request
   36 - Mobile Registration Reply
   37 - Domain Name Request
   38 - Domain Name Reply
   Any other numbers not listed in the earlier list...
*/

static void icmp_default_echo_cb(const uint8 *ip, uint16 seq, uint64 delta_us,
                                 uint8 ttl, const uint8* data, size_t data_sz) {
    (void)data;

    if(delta_us != (uint64) - 1) {
        printf("%d bytes from %d.%d.%d.%d: icmp_seq=%d ttl=%d time=%.3f ms\n",
               data_sz, ip[0], ip[1], ip[2], ip[3], seq, ttl,
               delta_us / 1000.0);
    }
    else {
        printf("%d bytes from %d.%d.%d.%d: icmp_seq=%d ttl=%d\n", data_sz,
               ip[0], ip[1], ip[2], ip[3], seq, ttl);
    }
}

/* The default echo (ping) callback */
net_echo_cb net_icmp_echo_cb = icmp_default_echo_cb;

/* Handle Echo Reply (ICMP type 0) packets */
static void net_icmp_input_0(netif_t *src, const ip_hdr_t *ip, icmp_hdr_t *icmp,
                             const uint8 *d, size_t s) {
    uint64 tmr, otmr;
    uint16 seq;

    (void)src;
    (void)icmp;

    tmr = timer_us_gettime64();
    seq = (d[7] | (d[6] << 8));

    /* Read back the time if we have it */
    if(s >= sizeof(icmp_hdr_t) + 8) {
        otmr = ((uint64)d[8] << 56) | ((uint64)d[9] << 48) |
               ((uint64)d[10] << 40) | ((uint64)d[11] << 32) |
               (d[12] << 24) | (d[13] << 16) | (d[14] << 8) | (d[15]);
        net_icmp_echo_cb((uint8 *)&ip->src, seq, tmr - otmr, ip->ttl, d, s);
    }
    else {
        net_icmp_echo_cb((uint8 *)&ip->src, seq, -1, ip->ttl, d, s);
    }
}

/* Handle Echo (ICMP type 8) packets */
static void net_icmp_input_8(netif_t *src, const ip_hdr_t *ip, icmp_hdr_t *icmp,
                             const uint8 *d, size_t s) {
    /* Set type to echo reply */
    icmp->type = ICMP_MESSAGE_ECHO_REPLY;

    /* Recompute the ICMP header checksum */
    icmp->checksum = 0;
    icmp->checksum = net_ipv4_checksum((uint8 *)icmp, ntohs(ip->length) -
                                       4 * (ip->version_ihl & 0x0f), 0);

    /* Set the destination to the original source, and substitute in our IP
       for the src (done this way so that pings that are broadcasted get an
       appropriate reply), and send it away. */
    net_ipv4_send(src, d, s, ip->packet_id, 255, 1, ip->dest, ip->src);
}

int net_icmp_input(netif_t *src, const ip_hdr_t *ip, const uint8 *d, size_t s) {
    icmp_hdr_t *icmp;
    int i;

    /* Find ICMP header */
    icmp = (icmp_hdr_t*)d;

    /* Check the ICMP checksum */
    i = net_ipv4_checksum(d, s, 0);

    if(i) {
        dbglog(DBG_KDEBUG, "net_icmp: icmp with invalid checksum\n");
        return -1;
    }

    switch(icmp->type) {
        case ICMP_MESSAGE_ECHO_REPLY:
            net_icmp_input_0(src, ip, icmp, d, s);
            break;

        case ICMP_MESSAGE_DEST_UNREACHABLE:
            dbglog(DBG_WARNING, "net_icmp: Destination unreachable,"
                   " code %d\n", icmp->code);
            break;

        case ICMP_MESSAGE_ECHO:
            net_icmp_input_8(src, ip, icmp, d, s);
            break;

        case ICMP_MESSAGE_TIME_EXCEEDED:
            dbglog(DBG_WARNING, "net_icmp: Time exceeded, code %d\n",
                   icmp->code);
            break;

        default:
            dbglog(DBG_KDEBUG, "net_icmp: unknown icmp type: %d\n",
                   icmp->type);
    }

    return 0;
}

/* Send an ICMP Echo (PING) packet to the specified device */
int net_icmp_send_echo(netif_t *net, const uint8 ipaddr[4], uint16 ident,
                       uint16 seq, const uint8 *data, size_t size) {
    icmp_hdr_t *icmp;
    int r = -1;
    uint16 sz = sizeof(icmp_hdr_t) + size + 8;
    uint8 databuf[sz];
    uint32 src;
    uint64 t;

    icmp = (icmp_hdr_t *)databuf;

    /* Fill in the ICMP Header */
    icmp->type = ICMP_MESSAGE_ECHO;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->misc.m16[0] = htons(ident);
    icmp->misc.m16[1] = htons(seq);
    memcpy(databuf + sizeof(icmp_hdr_t) + 8, data, size);

    /* Put the time in now, at the latest possible time (since we have to
       calculate the checksum over it) */
    t = timer_us_gettime64();
    databuf[sizeof(icmp_hdr_t) + 0] = t >> 56;
    databuf[sizeof(icmp_hdr_t) + 1] = t >> 48;
    databuf[sizeof(icmp_hdr_t) + 2] = t >> 40;
    databuf[sizeof(icmp_hdr_t) + 3] = t >> 32;
    databuf[sizeof(icmp_hdr_t) + 4] = t >> 24;
    databuf[sizeof(icmp_hdr_t) + 5] = t >> 16;
    databuf[sizeof(icmp_hdr_t) + 6] = t >>  8;
    databuf[sizeof(icmp_hdr_t) + 7] = t >>  0;

    /* Compute the ICMP Checksum */
    icmp->checksum = net_ipv4_checksum(databuf, sz, 0);

    /* If we're sending to the loopback, set that as our source too. */
    if(ipaddr[0] == 127) {
        src = net_ipv4_address(ipaddr);
    }
    else {
        src = net_ipv4_address(net->ip_addr);
    }

    r = net_ipv4_send(net, databuf, sz, seq, 255, 1, htonl(src),
                      htonl(net_ipv4_address(ipaddr)));

    return r;
}

/* Send an ICMP Destination Unreachable packet in reply to the given message */
int net_icmp_send_dest_unreach(netif_t *net, uint8 code, const uint8 *msg) {
    icmp_hdr_t *icmp;
    const ip_hdr_t *orig_msg = (ip_hdr_t *)msg;
    int hdrsz = (orig_msg->version_ihl & 0x0F) << 2;
    int sz = ((hdrsz + 8) > orig_msg->length) ? orig_msg->length : (hdrsz + 8);
    uint8 databuf[sizeof(icmp_hdr_t) + sz];

    icmp = (icmp_hdr_t *)databuf;

    /* Fill in the ICMP Header */
    icmp->type = ICMP_MESSAGE_DEST_UNREACHABLE;
    icmp->code = code;
    icmp->checksum = 0;
    icmp->misc.m32 = 0;
    memcpy(databuf + sizeof(icmp_hdr_t), orig_msg, sz);

    /* Compute the ICMP Checksum */
    icmp->checksum = net_ipv4_checksum(databuf, sizeof(icmp_hdr_t) + sz, 0);

    /* Send the packet away */
    return net_ipv4_send(net, databuf, sizeof(icmp_hdr_t) + sz, 0, 255, 1,
                         orig_msg->dest, orig_msg->src);
}

/* Send an ICMP Time Exceeded packet in reply to the given message */
int net_icmp_send_time_exceeded(netif_t *net, uint8 code, const uint8 *msg) {
    icmp_hdr_t *icmp;
    const ip_hdr_t *orig_msg = (ip_hdr_t *)msg;
    int hdrsz = (orig_msg->version_ihl & 0x0F) << 2;
    int sz = ((hdrsz + 8) > orig_msg->length) ? orig_msg->length : (hdrsz + 8);
    uint8 databuf[sizeof(icmp_hdr_t) + sz];

    icmp = (icmp_hdr_t *)databuf;

    /* Fill in the ICMP Header */
    icmp->type = ICMP_MESSAGE_TIME_EXCEEDED;
    icmp->code = code;
    icmp->checksum = 0;
    icmp->misc.m32 = 0;
    memcpy(databuf + sizeof(icmp_hdr_t), orig_msg, sz);

    /* Compute the ICMP Checksum */
    icmp->checksum = net_ipv4_checksum(databuf, sizeof(icmp_hdr_t) + sz, 0);

    /* Send the packet away */
    return net_ipv4_send(net, databuf, sizeof(icmp_hdr_t) + sz, 0, 255, 1,
                         orig_msg->dest, orig_msg->src);
}
