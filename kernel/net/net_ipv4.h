/* KallistiOS ##version##

   kernel/net/net_ipv4.h
   Copyright (C) 2005, 2007, 2008, 2012, 2013 Lawrence Sebald

*/

#ifndef __LOCAL_NET_IPV4_H
#define __LOCAL_NET_IPV4_H

#include <kos/net.h>

/* These structs are from AndrewK's dcload-ip. */
typedef struct {
    uint8_t   dest[6];
    uint8_t   src[6];
    uint8_t   type[2];
} __packed eth_hdr_t;

typedef struct {
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t zero;
    uint8_t proto;
    uint16_t length;
} __packed ipv4_pseudo_hdr_t;

uint16_t net_ipv4_checksum(const uint8_t *data, size_t bytes, uint16_t start);
int net_ipv4_send_packet(netif_t *net, ip_hdr_t *hdr, const uint8_t *data,
                         size_t size);
int net_ipv4_send(netif_t *net, const uint8_t *data, size_t size, int id, int ttl,
                  int proto, uint32_t src, uint32_t dst);
int net_ipv4_input(netif_t *src, const uint8_t *pkt, size_t pktsize,
                   const eth_hdr_t *eth);
int net_ipv4_input_proto(netif_t *net, const ip_hdr_t *ip, const uint8_t *data);

uint16_t net_ipv4_checksum_pseudo(in_addr_t src, in_addr_t dst, uint8_t proto,
                                uint16_t len);

/* In net_ipv4_frag.c */
int net_ipv4_frag_send(netif_t *net, ip_hdr_t *hdr, const uint8_t *data,
                       size_t size);
int net_ipv4_reassemble(netif_t *net, const ip_hdr_t *hdr, const uint8_t *data,
                        size_t size);
int net_ipv4_frag_init(void);
void net_ipv4_frag_shutdown(void);

#endif /* __LOCAL_NET_IPV4_H */
