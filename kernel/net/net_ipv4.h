/* KallistiOS ##version##

   kernel/net/net_ipv4.h
   Copyright (C) 2005, 2007, 2008, 2012, 2013 Lawrence Sebald

*/

#ifndef __LOCAL_NET_IPV4_H
#define __LOCAL_NET_IPV4_H

#include <kos/net.h>

/* These structs are from AndrewK's dcload-ip. */
typedef struct {
    uint8   dest[6];
    uint8   src[6];
    uint8   type[2];
} eth_hdr_t;

typedef struct {
    uint32 src_addr;
    uint32 dst_addr;
    uint8 zero;
    uint8 proto;
    uint16 length;
} ipv4_pseudo_hdr_t;

uint16 net_ipv4_checksum(const uint8 *data, size_t bytes, uint16 start);
int net_ipv4_send_packet(netif_t *net, ip_hdr_t *hdr, const uint8 *data,
                         size_t size);
int net_ipv4_send(netif_t *net, const uint8 *data, size_t size, int id, int ttl,
                  int proto, uint32 src, uint32 dst);
int net_ipv4_input(netif_t *src, const uint8 *pkt, size_t pktsize,
                   const eth_hdr_t *eth);
int net_ipv4_input_proto(netif_t *net, const ip_hdr_t *ip, const uint8 *data);

uint16 net_ipv4_checksum_pseudo(in_addr_t src, in_addr_t dst, uint8 proto,
                                uint16 len);

/* In net_ipv4_frag.c */
int net_ipv4_frag_send(netif_t *net, ip_hdr_t *hdr, const uint8 *data,
                       size_t size);
int net_ipv4_reassemble(netif_t *net, const ip_hdr_t *hdr, const uint8 *data,
                        size_t size);
int net_ipv4_frag_init(void);
void net_ipv4_frag_shutdown(void);

#endif /* __LOCAL_NET_IPV4_H */
