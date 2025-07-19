/* KallistiOS ##version##

   kernel/net/net_arp.c

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2005, 2010, 2012, 2013, 2016 Lawrence Sebald
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <kos/dbglog.h>
#include <kos/net.h>
#include <kos/thread.h>
#include <arch/timer.h>

#include "net_ipv4.h"

/*

  ARP handling system

*/

/* ARP Packet Structure */
typedef struct {
    uint8_t hw_type[2];
    uint8_t pr_type[2];
    uint8_t hw_size;
    uint8_t pr_size;
    uint8_t opcode[2];
    uint8_t hw_send[6];
    uint8_t pr_send[4];
    uint8_t hw_recv[6];
    uint8_t pr_recv[6];
} __packed arp_pkt_t;

/* Structure describing an ARP entry; each entry contains a MAC address,
   an IP address, and a timestamp from 'jiffies'. The timestamp allows
   aging and eventual removal. */
typedef struct netarp {
    /* ARP cache list handle */
    LIST_ENTRY(netarp)  ac_list;

    /* Mac address */
    uint8_t             mac[6];

    /* Associated IP address */
    uint8_t             ip[4];

    /* Cache entry time; if zero, this entry won't expire */
    uint64_t            timestamp;

    /* Optional packet to send when the entry is filled in */
    ip_hdr_t            *pkt;

    /* Additional data for that packet, if any */
    uint8_t             *data;

    /* Size of the additional data for the packet */
    int                 data_size;
} netarp_t;

/* Define the list type */
LIST_HEAD(netarp_list, netarp);

/**************************************************************************/
/* Variables */

/* ARP cache */
struct netarp_list net_arp_cache = LIST_HEAD_INITIALIZER(0);

/**************************************************************************/
/* Cache management */

/* Garbage collect timed out entries */
static int net_arp_gc(netif_t *nif) {
    netarp_t *a1, *a2;
    uint64_t now = timer_ms_gettime64();

    a1 = LIST_FIRST(&net_arp_cache);

    while(a1 != NULL) {
        a2 = LIST_NEXT(a1, ac_list);

        if(a1->timestamp) {
            if(now >= (a1->timestamp + 120 * 1000)) {
                LIST_REMOVE(a1, ac_list);

                if(a1->pkt) {
                    free(a1->pkt);
                    free(a1->data);
                }

                free(a1);
                a1 = a2;
                continue;
            }

            /* If the entry is incomplete and it's been more than 5 seconds
               since we queried it, try again. */
            if(a1->mac[0] == 0 && a1->mac[1] == 0 && a1->mac[2] == 0 &&
               a1->mac[3] == 0 && a1->mac[4] == 0 && a1->mac[5] == 0 &&
               now > (a1->timestamp + 5 * 1000)) {
                net_arp_query(nif, a1->ip);
            }
        }

        a1 = a2;
    }

    return 0;
}

/* Add an entry to the ARP cache manually */
int net_arp_insert(netif_t *nif, const uint8_t mac[6], const uint8_t ip[4],
                   uint64_t timestamp) {
    netarp_t *cur;

    /* First make sure the entry isn't already there */
    LIST_FOREACH(cur, &net_arp_cache, ac_list) {
        if(!memcmp(ip, cur->ip, 4)) {
            memcpy(cur->mac, mac, 6);
            cur->timestamp = timestamp;

            /* Send our queued packet, if we have one */
            if(cur->pkt) {
                net_ipv4_send_packet(nif, cur->pkt, cur->data, cur->data_size);
                free(cur->pkt);
                free(cur->data);

                cur->pkt = NULL;
                cur->data = NULL;
                cur->data_size = 0;
            }

            return 0;
        }
    }

    /* It's not there, add an entry */
    cur = (netarp_t *)malloc(sizeof(netarp_t));

    if(cur == NULL)
        return -1;

    memcpy(cur->mac, mac, 6);
    memcpy(cur->ip, ip, 4);
    cur->timestamp = timestamp;
    cur->pkt = NULL;
    cur->data = NULL;
    cur->data_size = 0;
    LIST_INSERT_HEAD(&net_arp_cache, cur, ac_list);

    /* Garbage collect expired entries */
    net_arp_gc(nif);

    return 0;
}

/* Look up an entry from the ARP cache; if no entry is found, then an ARP
   query will be sent and an error will be returned. Thus your packet send
   should also fail. Later when the transmit retries, hopefully the answer
   will have arrived. */
int net_arp_lookup(netif_t *nif, const uint8_t ip_in[4], uint8_t mac_out[6],
                   const ip_hdr_t *pkt, const uint8_t *data, int data_size) {
    netarp_t *cur;

    /* Garbage collect expired entries */
    net_arp_gc(nif);

    /* Look for the entry */
    LIST_FOREACH(cur, &net_arp_cache, ac_list) {
        if(!memcmp(ip_in, cur->ip, 4)) {
            if(cur->mac[0] == 0 && cur->mac[1] == 0 &&
               cur->mac[2] == 0 && cur->mac[3] == 0 &&
               cur->mac[4] == 0 && cur->mac[5] == 0) {
                return -1;
            }

            memcpy(mac_out, cur->mac, 6);

            if(cur->timestamp != 0)
                cur->timestamp = timer_ms_gettime64();

            return 0;
        }
    }

    /* It's not there... Add an incomplete ARP entry */
    cur = (netarp_t *)malloc(sizeof(netarp_t));

    if(cur == NULL)
        return -3;

    memset(cur, 0, sizeof(netarp_t));
    memcpy(cur->ip, ip_in, 4);
    cur->timestamp = timer_ms_gettime64();

    /* Copy our packet if we have one to copy. */
    if(pkt && data && data_size) {
        cur->data = (uint8_t *)malloc(data_size);

        if(cur->data) {
            cur->pkt = (ip_hdr_t *)malloc(sizeof(ip_hdr_t));

            if(!cur->pkt) {
                free(cur->data);
                cur->data = NULL;
            }
            else {
                memcpy(cur->pkt, pkt, sizeof(ip_hdr_t));
                memcpy(cur->data, data, data_size);
                cur->data_size = data_size;
            }
        }
    }

    LIST_INSERT_HEAD(&net_arp_cache, cur, ac_list);

    /* Generate an ARP who-has packet */
    net_arp_query(nif, ip_in);

    /* Return failure */
    memset(mac_out, 0, 6);
    return -2;
}

/* Do a reverse ARP lookup: look for an IP for a given mac address; note
   that if this fails, you have no recourse. */
int net_arp_revlookup(netif_t *nif, uint8_t ip_out[4], const uint8_t mac_in[6]) {
    netarp_t *cur;

    (void)nif;

    /* Look for the entry */
    LIST_FOREACH(cur, &net_arp_cache, ac_list) {
        if(!memcmp(mac_in, cur->mac, 6)) {
            memcpy(ip_out, cur->ip, 4);

            if(cur->timestamp != 0)
                cur->timestamp = timer_ms_gettime64();

            return 0;
        }
    }

    return -1;
}

/* Send an ARP reply packet on the specified network adapter */
static int net_arp_send(netif_t *nif, arp_pkt_t *pkt) {
    arp_pkt_t pkt_out;
    eth_hdr_t eth_hdr;
    uint8_t buf[sizeof(arp_pkt_t) + sizeof(eth_hdr_t)];

    /* First, fill in the ARP packet. */
    pkt_out.hw_type[0] = 0;
    pkt_out.hw_type[1] = 1;
    pkt_out.pr_type[0] = 0x08;
    pkt_out.pr_type[1] = 0x00;
    pkt_out.hw_size = 6;
    pkt_out.pr_size = 4;
    pkt_out.opcode[0] = 0;
    pkt_out.opcode[1] = 2;
    memcpy(pkt_out.hw_send, nif->mac_addr, 6);
    memcpy(pkt_out.pr_send, nif->ip_addr, 4);
    memcpy(pkt_out.pr_recv, pkt->pr_send, 4);
    memcpy(pkt_out.hw_recv, pkt->hw_send, 6);

    /* Fill in the ethernet header. */
    memcpy(eth_hdr.src, nif->mac_addr, 6);
    memcpy(eth_hdr.dest, pkt->hw_send, 6);
    eth_hdr.type[0] = 0x08; /* ARP */
    eth_hdr.type[1] = 0x06;

    memcpy(buf, &eth_hdr, sizeof(eth_hdr_t));
    memcpy(buf + sizeof(eth_hdr_t), &pkt_out, sizeof(arp_pkt_t));

    /* Send it away */
    nif->if_tx(nif, buf, sizeof(eth_hdr_t) + sizeof(arp_pkt_t), NETIF_BLOCK);

    return 0;
}

/* Receive an ARP packet and process it (called by net_input) */
int net_arp_input(netif_t *nif, const uint8_t *pkt_in, int len) {
    //eth_hdr_t *eth_hdr = (eth_hdr_t *)pkt_in;
    arp_pkt_t *pkt;

    (void)len;

    pkt = (arp_pkt_t *)(pkt_in + sizeof(eth_hdr_t));

    switch(pkt->opcode[1]) {
        case 1: /* ARP Request */
            /* Send reply if we are the intended recipient */
            if(!memcmp(nif->ip_addr, pkt->pr_recv, 4))
                net_arp_send(nif, pkt);
            __fallthrough; /* Yes, this really should fall through here. */

        case 2: /* ARP Reply */
            /* Insert into ARP cache */
            net_arp_insert(nif, pkt->hw_send, pkt->pr_send,
                           timer_ms_gettime64());
            break;
        default:
            dbglog(DBG_KDEBUG, "net_arp: Unknown ARP Opcode: %d\n",
                   pkt->opcode[1]);
    }

    return 0;
}

/* Generate an ARP who-has query on the given device */
int net_arp_query(netif_t *nif, const uint8_t ip[4]) {
    arp_pkt_t pkt_out;
    eth_hdr_t eth_hdr;
    uint8_t buf[sizeof(arp_pkt_t) + sizeof(eth_hdr_t)];

    /* First, fill in the ARP packet. */
    pkt_out.hw_type[0] = 0;
    pkt_out.hw_type[1] = 1;
    pkt_out.pr_type[0] = 0x08;
    pkt_out.pr_type[1] = 0x00;
    pkt_out.hw_size = 6;
    pkt_out.pr_size = 4;
    pkt_out.opcode[0] = 0;
    pkt_out.opcode[1] = 1;
    memcpy(pkt_out.hw_send, nif->mac_addr, 6);
    memcpy(pkt_out.pr_send, nif->ip_addr, 4);
    memcpy(pkt_out.pr_recv, ip, 4);
    pkt_out.hw_recv[0] = 0xFF; /* Ethernet broadcast address */
    pkt_out.hw_recv[1] = 0xFF;
    pkt_out.hw_recv[2] = 0xFF;
    pkt_out.hw_recv[3] = 0xFF;
    pkt_out.hw_recv[4] = 0xFF;
    pkt_out.hw_recv[5] = 0xFF;

    /* Fill in the ethernet header. */
    memcpy(eth_hdr.src, nif->mac_addr, 6);
    eth_hdr.dest[0] = 0xFF; /* Ethernet broadcast address */
    eth_hdr.dest[1] = 0xFF;
    eth_hdr.dest[2] = 0xFF;
    eth_hdr.dest[3] = 0xFF;
    eth_hdr.dest[4] = 0xFF;
    eth_hdr.dest[5] = 0xFF;
    eth_hdr.type[0] = 0x08; /* ARP */
    eth_hdr.type[1] = 0x06;

    memcpy(buf, &eth_hdr, sizeof(eth_hdr_t));
    memcpy(buf + sizeof(eth_hdr_t), &pkt_out, sizeof(arp_pkt_t));

    /* Send it away */
    nif->if_tx(nif, buf, sizeof(eth_hdr_t) + sizeof(arp_pkt_t), NETIF_BLOCK);

    return 0;
}

/*****************************************************************************/
/* Init/shutdown */

/* Init */
int net_arp_init(void) {
    /* Initialize the ARP cache */
    LIST_INIT(&net_arp_cache);

    return 0;
}

/* Shutdown */
void net_arp_shutdown(void) {
    /* Free all ARP entries */
    netarp_t *a1, *a2;

    a1 = LIST_FIRST(&net_arp_cache);

    while(a1 != NULL) {
        a2 = LIST_NEXT(a1, ac_list);

        if(a1->pkt) {
            free(a1->pkt);
            free(a1->data);
        }

        free(a1);
        a1 = a2;
    }

    LIST_INIT(&net_arp_cache);
}
