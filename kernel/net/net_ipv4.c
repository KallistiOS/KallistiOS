/* KallistiOS ##version##

   kernel/net/net_ipv4.c

   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013,
                 2016 Lawrence Sebald

   Portions adapted from KOS' old net_icmp.c file:
   Copyright (C) 2002 Megan Potter

*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <kos/net.h>
#include <kos/fs_socket.h>
#include <arch/timer.h>

#include "net_ipv4.h"
#include "net_icmp.h"

static net_ipv4_stats_t ipv4_stats = { 0 };

/* Perform an IP-style checksum on a block of data */
uint16_t __pure net_ipv4_checksum(const uint8_t *data, size_t bytes, uint16_t start) {
    uint32_t sum = start;
    size_t i = bytes;

    /* Make sure we don't do any unaligned memory accesses */
    if(((uint32_t)data) & 0x01) {
        const uint8_t *ptr = data;

        while(i > 1) {
            sum += *ptr | ((*ptr + 1) << 8);
            ptr += 2;
            i -= 2;

            while(sum >> 16)
                sum = (sum >> 16) + (sum & 0xFFFF);
        }
    }
    else {
        const uint16_t *ptr = (const uint16_t *)data;

        while(i > 1) {
            sum += *ptr++;
            i -= 2;

            while(sum >> 16)
                sum = (sum >> 16) + (sum & 0xFFFF);
        }
    }

    /* Handle the last byte, if we have an odd byte count */
    if(i)
        sum += data[bytes - 1];

    /* Take care of any carry bits */
    while(sum >> 16)
        sum = (sum >> 16) + (sum & 0xFFFF);

    return sum ^ 0xFFFF;
}

/* Determine if a given IP is in the current network */
static int __pure is_in_network(const uint8_t src[4], const uint8_t dest[4],
                         const uint8_t netmask[4]) {
    int i;

    for(i = 0; i < 4; i++) {
        if((dest[i] & netmask[i]) != (src[i] & netmask[i]))
            return 0;
    }

    return 1;
}

/* Determine if a given IP is the adapter's broadcast address. */
static int __pure is_broadcast(const uint8_t dest[4], const uint8_t bc[4]) {
    int i;

    for(i = 0; i < 4; ++i) {
        if(dest[i] != bc[i])
            return 0;
    }

    return 1;
}

/* Send a packet on the specified network adapter */
int net_ipv4_send_packet(netif_t *net, ip_hdr_t *hdr, const uint8_t *data,
                         size_t size) {
    uint8_t dest_ip[4];
    uint8_t dest_mac[6];
    uint8_t pkt[size + sizeof(ip_hdr_t) + sizeof(eth_hdr_t)];
    eth_hdr_t *ehdr;
    int err;

    if(net == NULL) {
        net = net_default_dev;

        if(!net) {
            errno = ENETDOWN;
            return -1;
        }
    }

    net_ipv4_parse_address(ntohl(hdr->dest), dest_ip);

    /* Is this a loopback address (127/8)? */
    if(dest_ip[0] == 0x7F) {
        /* Put the IP header / data into our packet */
        memcpy(pkt, hdr, 4 * (hdr->version_ihl & 0x0f));
        memcpy(pkt + 4 * (hdr->version_ihl & 0x0f), data, size);

        ++ipv4_stats.pkt_sent;

        /* Send it "away" */
        net_ipv4_input(NULL, pkt, 4 * (hdr->version_ihl & 0x0f) + size, NULL);

        return 0;
    }
    else if(net->flags & NETIF_NOETH) {
        /* Put the IP header / data into our packet */
        memcpy(pkt, hdr, 4 * (hdr->version_ihl & 0x0f));
        memcpy(pkt + 4 * (hdr->version_ihl & 0x0f), data, size);

        ++ipv4_stats.pkt_sent;

        /* Send it away */
        return net->if_tx(net, pkt, 4 * (hdr->version_ihl & 0x0f) + size,
                          NETIF_BLOCK);
    }

    /* Are we sending a broadcast packet? */
    if(hdr->dest == 0xFFFFFFFF || is_broadcast(dest_ip, net->broadcast)) {
        /* Set the destination to the datalink layer broadcast address. */
        memset(dest_mac, 0xFF, 6);
    }
    else {
        /* Is it in our network? */
        if(!is_in_network(net->ip_addr, dest_ip, net->netmask)) {
            memcpy(dest_ip, net->gateway, 4);
        }

        /* Get our destination's MAC address. If we do not have the MAC address
           cached, return a distinguished error to the upper-level protocol so
           that it can decide what to do. */
        err = net_arp_lookup(net, dest_ip, dest_mac, hdr, data, size);

        if(err == -1) {
            errno = ENETUNREACH;
            ++ipv4_stats.pkt_send_failed;
            return -1;
        }
        else if(err == -2) {
            /* It'll send when the ARP reply comes in (assuming one does), so
               return success. */
            return 0;
        }
    }

    /* Fill in the ethernet header */
    ehdr = (eth_hdr_t *)pkt;
    memcpy(ehdr->dest, dest_mac, 6);
    memcpy(ehdr->src, net->mac_addr, 6);
    ehdr->type[0] = 0x08;
    ehdr->type[1] = 0x00;

    /* Put the IP header / data into our ethernet packet */
    memcpy(pkt + sizeof(eth_hdr_t), hdr, 4 * (hdr->version_ihl & 0x0f));
    memcpy(pkt + sizeof(eth_hdr_t) + 4 * (hdr->version_ihl & 0x0f), data,
           size);

    ++ipv4_stats.pkt_sent;

    /* Send it away */
    net->if_tx(net, pkt, sizeof(ip_hdr_t) + size + sizeof(eth_hdr_t),
               NETIF_BLOCK);

    return 0;
}

int net_ipv4_send(netif_t *net, const uint8_t *data, size_t size, int id, int ttl,
                  int proto, uint32_t src, uint32_t dst) {
    ip_hdr_t hdr;

    /* If the ID is -1, generate a random ID value that can be used in case the
       packet gets fragmented. */
    if(id == -1) {
        id = rand() & 0xFFFF;
    }

    /* Fill in the IPv4 Header */
    hdr.version_ihl = 0x45;
    hdr.tos = 0;
    hdr.length = htons(size + 20);
    hdr.packet_id = id;
    hdr.flags_frag_offs = 0;
    hdr.ttl = ttl;
    hdr.protocol = proto;
    hdr.checksum = 0;
    hdr.src = src;
    hdr.dest = dst;

    hdr.checksum = net_ipv4_checksum((uint8_t *)&hdr, sizeof(ip_hdr_t), 0);

    return net_ipv4_frag_send(net, &hdr, data, size);
}

int net_ipv4_input(netif_t *src, const uint8_t *pkt, size_t pktsize,
                   const eth_hdr_t *eth) {
    const ip_hdr_t *ip;
    const uint8_t *data;
    size_t hdrlen;
    uint8_t ipa[4];

    if(pktsize < sizeof(ip_hdr_t)) {
        /* This is obviously a bad packet, drop it */
        ++ipv4_stats.pkt_recv_bad_size;
        return -1;
    }

    ip = (const ip_hdr_t *)pkt;
    hdrlen = (ip->version_ihl & 0x0F) << 2;

    if(pktsize < hdrlen) {
        /* The packet is smaller than the listed header length, bail */
        ++ipv4_stats.pkt_recv_bad_size;
        return -1;
    }

    /* Check ip header checksum */
    if(net_ipv4_checksum((uint8_t *)ip, hdrlen, 0)) {
        /* The checksums don't match, bail */
        ++ipv4_stats.pkt_recv_bad_chksum;
        return -1;
    }

    data = (const uint8_t *)(pkt + hdrlen);

    /* Add the sender to the ARP cache, if they're not already there. */
    if(eth) {
        net_ipv4_parse_address(ntohl(ip->src), ipa);
        net_arp_insert(src, eth->src, ipa, timer_ms_gettime64());
    }

    /* Submit the packet for possible reassembly. */
    return net_ipv4_reassemble(src, ip, data, ntohs(ip->length) - hdrlen);
}

int net_ipv4_input_proto(netif_t *src, const ip_hdr_t *ip, const uint8_t *data) {
    size_t hdrlen = (ip->version_ihl & 0x0F) << 2;
    size_t datalen = ntohs(ip->length) - hdrlen;
    int rv;

    /* Send the packet along to the appropriate protocol. */
    switch(ip->protocol) {
        case IPPROTO_ICMP:
            ++ipv4_stats.pkt_recv;
            return net_icmp_input(src, ip, data, datalen);

        default:
            rv = fs_socket_input(src, AF_INET, ip->protocol, ip, data, datalen);

            if(rv > -2) {
                ++ipv4_stats.pkt_recv;
                return rv;
            }
    }

    /* There's no handler for this packet type, send an ICMP Destination
       Unreachable, and log the unknown protocol. */
    ++ipv4_stats.pkt_recv_bad_proto;
    net_icmp_send_dest_unreach(src, ICMP_PROTOCOL_UNREACHABLE, (uint8_t *)ip);

    return -1;
}

uint32_t __pure net_ipv4_address(const uint8_t addr[4]) {
    return (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | (addr[3]);
}

void net_ipv4_parse_address(uint32_t addr, uint8_t out[4]) {
    out[0] = (uint8_t)((addr >> 24) & 0xFF);
    out[1] = (uint8_t)((addr >> 16) & 0xFF);
    out[2] = (uint8_t)((addr >> 8) & 0xFF);
    out[3] = (uint8_t)(addr & 0xFF);
}

uint16_t __pure net_ipv4_checksum_pseudo(in_addr_t src, in_addr_t dst, uint8_t proto,
                                uint16_t len) {
    ipv4_pseudo_hdr_t ps = { src, dst, 0, proto, htons(len) };

    return ~net_ipv4_checksum((uint8_t *)&ps, sizeof(ipv4_pseudo_hdr_t), 0);
}

net_ipv4_stats_t net_ipv4_get_stats(void) {
    return ipv4_stats;
}
