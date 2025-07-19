/* KallistiOS ##version##

   kernel/net/net_ipv6.c
   Copyright (C) 2010, 2012, 2013 Lawrence Sebald

*/

#include <stdint.h>
#include <string.h>
#include <netinet/in.h>
#include <kos/net.h>
#include <kos/fs_socket.h>
#include <errno.h>

#include "net_ipv6.h"
#include "net_icmp6.h"
#include "net_ipv4.h"

#if __GNUC__ >= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#endif

static net_ipv6_stats_t ipv6_stats = { 0 };
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;

/* These few aren't in the IEEE 1003.1-2008 spec, but do appear on (at least)
   Mac OS X (in not strictly compliant mode) and are useful for us... */
const struct in6_addr in6addr_linklocal_allnodes = {
    {   {
            0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
        }
    }
};
const struct in6_addr in6addr_linklocal_allrouters = {
    {   {
            0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
        }
    }
};

static int is_in_network(netif_t *net, const struct in6_addr *ip) {
    int i;

    /* Make sure its not trivially link-local */
    if(IN6_IS_ADDR_LINKLOCAL(ip)) {
        return 1;
    }

    /* Go through each prefix, and see if any match */
    for(i = 0; i < net->ip6_addr_count; ++i) {
        if(!memcmp(ip, &net->ip6_addrs[i], 8)) {
            return 1;
        }
    }

    return 0;
}

/* Send a packet on the specified network adapter */
int net_ipv6_send_packet(netif_t *net, ipv6_hdr_t *hdr, const uint8_t *data,
                         size_t data_size) {
    uint8_t pkt[data_size + sizeof(ipv6_hdr_t) + sizeof(eth_hdr_t)];
    uint8_t dst_mac[6];
    int err;
    struct in6_addr dst = hdr->dst_addr;
    eth_hdr_t *ehdr;

    if(!net) {
        net = net_default_dev;

        if(!net) {
            errno = ENETDOWN;
            return -1;
        }
    }

    /* Are we sending a packet to loopback? */
    if(IN6_IS_ADDR_LOOPBACK(&hdr->dst_addr)) {
        memcpy(pkt, hdr, sizeof(ipv6_hdr_t));
        memcpy(pkt + sizeof(ipv6_hdr_t), data, data_size);

        ++ipv6_stats.pkt_sent;

        /* Send the packet "away" */
        net_ipv6_input(NULL, pkt, sizeof(ipv6_hdr_t) + data_size, NULL);
        return 0;
    }
    else if(net->flags & NETIF_NOETH) {
        memcpy(pkt, hdr, sizeof(ipv6_hdr_t));
        memcpy(pkt + sizeof(ipv6_hdr_t), data, data_size);

        ++ipv6_stats.pkt_sent;

        /* Send the packet away */
        return net->if_tx(net, pkt, sizeof(ipv6_hdr_t) + data_size,
                          NETIF_BLOCK);
    }
    else if(IN6_IS_ADDR_MULTICAST(&hdr->dst_addr)) {
        dst_mac[0] = dst_mac[1] = 0x33;
        dst_mac[2] = hdr->dst_addr.__s6_addr.__s6_addr8[12];
        dst_mac[3] = hdr->dst_addr.__s6_addr.__s6_addr8[13];
        dst_mac[4] = hdr->dst_addr.__s6_addr.__s6_addr8[14];
        dst_mac[5] = hdr->dst_addr.__s6_addr.__s6_addr8[15];
    }
    else {
        if(!is_in_network(net, &dst)) {
            dst = net->ip6_gateway;
        }

        err = net_ndp_lookup(net, &dst, dst_mac, hdr, data, data_size);

        if(err == -1) {
            errno = ENETUNREACH;
            ++ipv6_stats.pkt_send_failed;
            return err;
        }
        else if(err == -2) {
            return 0;
        }
    }

    /* Fill in the ethernet header */
    ehdr = (eth_hdr_t *)pkt;
    memcpy(ehdr->dest, dst_mac, 6);
    memcpy(ehdr->src, net->mac_addr, 6);
    ehdr->type[0] = 0x86;
    ehdr->type[1] = 0xDD;

    /* Put the IP header / data into our ethernet packet */
    memcpy(pkt + sizeof(eth_hdr_t), hdr, sizeof(ipv6_hdr_t));
    memcpy(pkt + sizeof(eth_hdr_t) + sizeof(ipv6_hdr_t), data, data_size);

    ++ipv6_stats.pkt_sent;

    /* Send it away */
    net->if_tx(net, pkt, sizeof(ipv6_hdr_t) + data_size + sizeof(eth_hdr_t),
               NETIF_BLOCK);

    return 0;
}

int net_ipv6_send(netif_t *net, const uint8_t *data, size_t data_size,
                  int hop_limit, int proto, const struct in6_addr *src,
                  const struct in6_addr *dst) {
    ipv6_hdr_t hdr;

    if(!net) {
        net = net_default_dev;

        if(!net) {
            errno = ENETDOWN;
            return -1;
        }
    }

    /* Set up the hop limit. We need to do this here, in case we end up passing
       this off to the IPv4 code, otherwise we could end up with a 0 down there
       for the ttl, which would be bad. */
    if(!hop_limit) {
        if(net->hop_limit)
            hop_limit = net->hop_limit;
        else
            hop_limit = 255;
    }

    /* If this is actually going both to and from an IPv4 address, use the IPv4
       send function to do the rest. Note that only V4-mapped addresses are
       supported here (::ffff:x.y.z.w) */
    if(IN6_IS_ADDR_V4MAPPED(src) && IN6_IS_ADDR_V4MAPPED(dst)) {
        return net_ipv4_send(net, data, data_size, -1, hop_limit, proto,
                             src->__s6_addr.__s6_addr32[3],
                             dst->__s6_addr.__s6_addr32[3]);
    }
    else if(IN6_IS_ADDR_V4MAPPED(src) || IN6_IS_ADDR_V4MAPPED(dst) ||
            IN6_IS_ADDR_V4COMPAT(src) || IN6_IS_ADDR_V4COMPAT(dst)) {
        return -1;
    }

    hdr.version_lclass = 0x60;
    hdr.hclass_lflow = 0;
    hdr.lclass = 0;
    hdr.length = ntohs(data_size);
    hdr.next_header = proto;
    hdr.hop_limit = hop_limit;
    hdr.src_addr = *src;
    hdr.dst_addr = *dst;

    /* XXXX: Handle fragmentation... */
    return net_ipv6_send_packet(net, &hdr, data, data_size);
}

int net_ipv6_input(netif_t *src, const uint8_t *pkt, size_t pktsize,
                   const eth_hdr_t *eth) {
    ipv6_hdr_t *ip;
    uint8_t next_hdr;
    //int pos;
    size_t len;
    int rv;

    if(pktsize < sizeof(ipv6_hdr_t)) {
        /* This is obviously a bad packet, drop it */
        ++ipv6_stats.pkt_recv_bad_size;
        return -1;
    }

    ip = (ipv6_hdr_t *)pkt;
    len = ntohs(ip->length);

    if(pktsize < len + sizeof(ipv6_hdr_t)) {
        /* The packet is of size less than the payload length + the size of a
           minimal IPv6 header; it must be bad, drop it */
        ++ipv6_stats.pkt_recv_bad_size;
        return -1;
    }

    /* Parse the header to find the payload */
    //pos = sizeof(ipv6_hdr_t); // Currently unused, but will be needed later.
    next_hdr = ip->next_header;

    if(eth)
        net_ndp_insert(src, eth->src, &ip->src_addr, 1);

    /* XXXX: Parse options and deal with fragmentation */
    switch(next_hdr) {
        case IPV6_HDR_ICMP:
            return net_icmp6_input(src, ip, pkt + sizeof(ipv6_hdr_t), len);

        default:
            rv = fs_socket_input(src, AF_INET6, next_hdr, pkt,
                                 pkt + sizeof(ipv6_hdr_t), len);

            if(rv == -2) {
                /* We don't know what to do with this packet, so send an ICMPv6
                   message indicating that. */
                ++ipv6_stats.pkt_recv_bad_proto;
                return net_icmp6_send_param_prob(src,
                                                 ICMP6_PARAM_PROB_UNK_HEADER, 6,
                                                 pkt, pktsize);
            }

            ++ipv6_stats.pkt_recv;
            return rv;
    }

    return 0;
}

net_ipv6_stats_t net_ipv6_get_stats(void) {
    return ipv6_stats;
}

uint16_t net_ipv6_checksum_pseudo(const struct in6_addr *src,
                                const struct in6_addr *dst,
                                uint32_t upper_len, uint8_t next_hdr) {
    ipv6_pseudo_hdr_t ps;

    /* Since the src and dst addresses aren't necessarily aligned when we send
       them in from header processing, do this the hard way. */
    memcpy(&ps.src_addr, src, sizeof(struct in6_addr));
    memcpy(&ps.dst_addr, dst, sizeof(struct in6_addr));

    /* If this is actually an IPv4 packet, do the calculation there instead. */
    if(IN6_IS_ADDR_V4MAPPED(&ps.src_addr) &&
            IN6_IS_ADDR_V4MAPPED(&ps.dst_addr)) {
        return net_ipv4_checksum_pseudo(ps.src_addr.__s6_addr.__s6_addr32[3],
                                        ps.dst_addr.__s6_addr.__s6_addr32[3],
                                        next_hdr, (uint16_t)upper_len);
    }

    ps.upper_layer_len = htonl(upper_len);
    ps.next_header = next_hdr;
    ps.zero[0] = ps.zero[1] = ps.zero[2] = 0;

    return ~net_ipv4_checksum((uint8_t *)&ps, sizeof(ipv6_pseudo_hdr_t), 0);
}

int net_ipv6_init(void) {
    /* Make sure we're registered to get "All nodes" multicasts from the
       ethernet layer. */
    uint8_t mac[6] = { 0x33, 0x33, 0x00, 0x00, 0x00, 0x01 };
    net_multicast_add(mac);

    /* Also register for the one for our link-local address' solicited nodes
       group (which will do the same for all our other addresses too). */
    mac[2] = 0xFF;
    mac[3] = net_default_dev->ip6_lladdr.s6_addr[13];
    mac[4] = net_default_dev->ip6_lladdr.s6_addr[14];
    mac[5] = net_default_dev->ip6_lladdr.s6_addr[15];
    net_multicast_add(mac);

    return 0;
}

void net_ipv6_shutdown(void) {
    uint8_t mac[6] = { 0x33, 0x33, 0x00, 0x00, 0x00, 0x01 };

    /* Remove from the all nodes multicast group */
    net_multicast_del(mac);

    /* ... and our solicited nodes multicast group */
    mac[2] = 0xFF;
    mac[3] = net_default_dev->ip6_lladdr.s6_addr[13];
    mac[4] = net_default_dev->ip6_lladdr.s6_addr[14];
    mac[5] = net_default_dev->ip6_lladdr.s6_addr[15];
    net_multicast_del(mac);
}

#if __GNUC__ >= 9
#pragma GCC diagnostic pop
#endif
