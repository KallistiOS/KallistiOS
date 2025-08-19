/* KallistiOS ##version##

   kernel/net/net_ndp.c
   Copyright (C) 2010, 2013 Lawrence Sebald

*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/queue.h>
#include <kos/net.h>
#include <arch/timer.h>

#include "net_ipv6.h"
#include "net_icmp6.h"

/* This file implements the Neighbor Discovery Protocol for IPv6. Basically, NDP
   acts much like ARP does for IPv4. It is responsible for keeping track of the
   low-level addresses of other hosts on the network. Everything it does is
   through ICMPv6 packets. NDP is specified in RFC 4861. Note however, that, for
   the time being at least, this isn't fully compliant with that spec. */

/* Structure describing a NDP entry. Analogous to the netarp_t for ARP. */
typedef struct ndp_entry {
    LIST_ENTRY(ndp_entry)   entry;
    struct in6_addr         ip;
    uint64_t                last_reachable;
    int                     state;
    uint8_t                 mac[6];
    ipv6_hdr_t              *pkt;
    uint8_t                 *data;
    int                     data_size;
} ndp_entry_t;

LIST_HEAD(ndp_list, ndp_entry);
static struct ndp_list ndp_cache = LIST_HEAD_INITIALIZER(0);

/* List of states for the ndp entry */
#define NDP_STATE_INCOMPLETE    0
#define NDP_STATE_REACHABLE     1
#define NDP_STATE_STALE         2
#define NDP_STATE_DELAY         3
#define NDP_STATE_PROBE         4

void net_ndp_gc(void) {
    ndp_entry_t *i, *tmp;
    uint64_t now = timer_ms_gettime64();

    i = LIST_FIRST(&ndp_cache);

    while(i) {
        tmp = LIST_NEXT(i, entry);

        /* If we haven't gotten a reachable confirmation within 10 minutes, its
           pretty safe to remove it. Also, remove any incomplete entries that
           are still incomplete after a few seconds have passed. */
        if(i->last_reachable + 600000 < now ||
                (i->state == NDP_STATE_INCOMPLETE &&
                 i->last_reachable + 2000 < now)) {
            LIST_REMOVE(i, entry);

            if(i->pkt) {
                free(i->pkt);
                free(i->data);
            }

            free(i);
        }

        i = tmp;
    }
}

int net_ndp_insert(netif_t *net, const uint8_t mac[6], const struct in6_addr *ip,
                   int unsol) {
    ndp_entry_t *i;
    uint64_t now = timer_ms_gettime64();

    /* Don't allow any multicast or unspecified addresses to end up in the NDP
       cache... */
    if(ip->s6_addr[0] == 0xFF || ip->s6_addr[0] == 0x00) {
        return -1;
    }

    /* Look through the list first to see if its there */
    LIST_FOREACH(i, &ndp_cache, entry) {
        if(!memcmp(ip, &i->ip, sizeof(struct in6_addr))) {
            /* We found it, update everything */
            if(unsol && memcmp(i->mac, mac, 6)) {
                i->state = NDP_STATE_STALE;
            }
            else {
                i->state = NDP_STATE_REACHABLE;
            }

            memcpy(i->mac, mac, 6);
            i->last_reachable = now;

            /* Send our queued packet, if we have one */
            if(i->pkt) {
                net_ipv6_send_packet(net, i->pkt, i->data, i->data_size);
                free(i->pkt);
                free(i->data);

                i->pkt = NULL;
                i->data = NULL;
                i->data_size = 0;
            }

            return 0;
        }
    }

    /* No entry exists yet, so create one */
    if(!(i = (ndp_entry_t *)malloc(sizeof(ndp_entry_t)))) {
        return -1;
    }

    memset(i, 0, sizeof(ndp_entry_t));
    memcpy(&i->ip, ip, sizeof(struct in6_addr));
    memcpy(i->mac, mac, 6);
    i->last_reachable = now;

    if(unsol) {
        i->state = NDP_STATE_STALE;
    }
    else {
        i->state = NDP_STATE_REACHABLE;
    }

    LIST_INSERT_HEAD(&ndp_cache, i, entry);

    /* Garbage collect! */
    net_ndp_gc();

    return 0;
}

/* Set up and send a neighbor solicitation about the specified address */
static void net_ndp_send_sol(netif_t *net, const struct in6_addr *ip) {
    struct in6_addr dst = *ip;

    /* Send to the solicited nodes multicast group for the specified addr */
    dst.s6_addr[0] = 0xFF;
    dst.s6_addr[1] = 0x02;
    dst.__s6_addr.__s6_addr16[1] = 0x0000;
    dst.__s6_addr.__s6_addr16[2] = 0x0000;
    dst.__s6_addr.__s6_addr16[3] = 0x0000;
    dst.__s6_addr.__s6_addr16[4] = 0x0000;
    dst.s6_addr[10] = 0x00;
    dst.s6_addr[11] = 0x01;
    dst.s6_addr[12] = 0xFF;

    net_icmp6_send_nsol(net, &dst, ip, 0);
}

int net_ndp_lookup(netif_t *net, const struct in6_addr *ip, uint8_t mac_out[6],
                   const ipv6_hdr_t *pkt, const uint8_t *data, int data_size) {
    ndp_entry_t *i;
    uint64_t now = timer_ms_gettime64();

    /* Garbage collect, so we don't end up returning really stale entries */
    net_ndp_gc();

    /* Look for the entry */
    LIST_FOREACH(i, &ndp_cache, entry) {
        if(!memcmp(ip, &i->ip, sizeof(struct in6_addr))) {
            if(i->state == NDP_STATE_INCOMPLETE) {
                memset(mac_out, 0, 6);
                return -1;
            }
            else if(i->state == NDP_STATE_STALE) {
                net_ndp_send_sol(net, ip);
            }

            memcpy(mac_out, i->mac, 6);
            return 0;
        }
    }

    /* Its not there, add an incomplete entry and solicit the info */
    if(!(i = (ndp_entry_t *)malloc(sizeof(ndp_entry_t)))) {
        return -1;
    }

    memset(i, 0, sizeof(ndp_entry_t));
    memcpy(&i->ip, ip, sizeof(struct in6_addr));
    i->last_reachable = now;
    i->state = NDP_STATE_INCOMPLETE;

    /* Copy our packet if we have one to copy. */
    if(pkt && data && data_size) {
        i->data = (uint8_t *)malloc(data_size);

        if(i->data) {
            i->pkt = (ipv6_hdr_t *)malloc(sizeof(ipv6_hdr_t));

            if(!i->pkt) {
                free(i->data);
                i->data = NULL;
            }
            else {
                memcpy(i->pkt, pkt, sizeof(ipv6_hdr_t));
                memcpy(i->data, data, data_size);
                i->data_size = data_size;
            }
        }
    }

    LIST_INSERT_HEAD(&ndp_cache, i, entry);

    net_ndp_send_sol(net, ip);

    memset(mac_out, 0, 6);
    return -2;
}

int net_ndp_init(void) {
    return 0;
}

void net_ndp_shutdown(void) {
    /* Free all entries */
    ndp_entry_t *i, *tmp;

    i = LIST_FIRST(&ndp_cache);

    while(i) {
        tmp = LIST_NEXT(i, entry);

        if(i->pkt) {
            free(i->pkt);
            free(i->data);
        }

        free(i);
        i = tmp;
    }

    /* Reinit the list to the clean state, in case we call net_ndp_init later */
    LIST_INIT(&ndp_cache);
}
