/* KallistiOS ##version##

   include/kos/net.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013,
                 2016 Lawrence Sebald

*/

/** \file    kos/net.h
    \brief   Network support.
    \ingroup networking_drivers

    This file contains declarations related to networking support.

    \author Lawrence Sebald
    \author Megan Potter
*/

#ifndef __KOS_NET_H
#define __KOS_NET_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <sys/queue.h>
#include <netinet/in.h>

/* All functions in this header return < 0 on failure, and 0 on success. */

/** \defgroup networking Networking
    \brief    APIs and drivers for network and internet connectivity

    KOS' built-in network stack supports UDP over IPv4, and to some degree has
    some basic IPv6 support as well. This will change over time, hopefully
    leaving us with full TCP and UDP support both over IPv4 and IPv6.
*/

/** \defgroup networking_drivers    Drivers
    \brief                          Low-level Drivers for Network Devices
    \ingroup                        networking
*/

/** \brief   Structure describing one usable network device.
    \ingroup networking_drivers

    Each usable network device should have one of these describing it. These
    must be registered to the network layer before the device is usable.

    \headerfile kos/net.h
*/
typedef struct knetif {
    /** \brief  Device list handle (not a function!) */
    LIST_ENTRY(knetif)  if_list;

    /** \brief  Device name ("bba", "la", etc) */
    const char          *name;

    /** \brief  Long description of the device */
    const char          *descr;

    /** \brief  Unit index (starts at zero and counts upwards for multiple
                            network devices of the same type) */
    int                 index;

    /** \brief  Internal device ID (for whatever the driver wants) */
    uint32              dev_id;

    /** \brief  Interface flags */
    uint32              flags;

    /** \brief  The device's MAC address */
    uint8               mac_addr[6];

    /** \brief  The device's IP address (if any) */
    uint8               ip_addr[4];

    /** \brief  The device's netmask */
    uint8               netmask[4];

    /** \brief  The device's gateway's IP address */
    uint8               gateway[4];

    /** \brief  The device's broadcast address */
    uint8               broadcast[4];

    /** \brief  The device's DNS server address */
    uint8               dns[4];

    /** \brief  The device's MTU */
    int                 mtu;

    /** \brief  The device's Link-local IPv6 address */
    struct in6_addr     ip6_lladdr;

    /** \brief  Any further IPv6 addresses the device has.
        The first address in this list will always be used, unless otherwise
        specified. */
    struct in6_addr     *ip6_addrs;
    int                 ip6_addr_count;

    /** \brief  The device's gateway's IPv6 address */
    struct in6_addr     ip6_gateway;

    /** \brief  Default MTU over IPv6 */
    uint32              mtu6;

    /** \brief  Default hop limit over IPv6 */
    int                 hop_limit;

    /* All of the following callback functions should return a negative
       value on failure, and a zero or positive value on success. Some
       functions have special values, as noted. */

    /** \brief  Attempt to detect the device.
        \param  self        The network device in question.
        \return             0 on success, <0 on failure.
    */
    int (*if_detect)(struct knetif * self);

    /** \brief  Initialize the device.
        \param  self        The network device in question.
        \return             0 on success, <0 on failure.
    */
    int (*if_init)(struct knetif * self);

    /** \brief  Shutdown the device.
        \param  self        The network device in question.
        \return             0 on success, <0 on failure.
    */
    int (*if_shutdown)(struct knetif * self);

    /** \brief  Start the device (after init or stop).
        \param  self        The network device in question.
        \return             0 on success, <0 on failure.
    */
    int (*if_start)(struct knetif * self);

    /** \brief  Stop (hibernate) the device.
        \param  self        The network device in question.
        \return             0 on success, <0 on failure
    */
    int (*if_stop)(struct knetif * self);

    /** \brief  Queue a packet for transmission.
        \param  self        The network device in question.
        \param  data        The packet to transmit.
        \param  len         The length of the packet in bytes.
        \param  blocking    1 if we should block if needed, 0 otherwise.
        \retval NETIF_TX_OK     On success.
        \retval NETIF_TX_ERROR  On general failure.
        \retval NETIF_TX_AGAIN  If non-blocking and we must block to send.
    */
    int (*if_tx)(struct knetif * self, const uint8 * data, int len,
                 int blocking);

    /** \brief  Commit any queued output packets.
        \param  self        The network device in question.
        \return             0 on success, <0 on failure.
    */
    int (*if_tx_commit)(struct knetif * self);

    /** \brief  Poll for queued receive packets, if necessary.
        \param  self        The network device in question.
        \return             0 on success, <0 on failure.
    */
    int (*if_rx_poll)(struct knetif * self);

    /** \brief  Set flags; you should generally manipulate flags through here so
                that the driver gets a chance to act on the info.
        \param  self        The network device in question.
        \param  flags_and   Bitmask to and with the flags.
        \param  flags_or    Bitmask to or with the flags.
    */
    int (*if_set_flags)(struct knetif * self, uint32 flags_and, uint32 flags_or);

    /** \brief  Set the device's multicast list.
        \param  self        The network device in question.
        \param  list        The list of MAC addresses (6 * count bytes).
        \param  count       The number of addresses in list.
    */
    int (*if_set_mc)(struct knetif *self, const uint8 *list, int count);
} netif_t;

/** \defgroup net_drivers_flags netif_t Flags
    \brief                      Network interface flags
    \ingroup                    networking_drivers
    @{
*/
#define NETIF_NO_FLAGS      0x00000000      /**< \brief No flags set */
#define NETIF_REGISTERED    0x00000001      /**< \brief Is it registered? */
#define NETIF_DETECTED      0x00000002      /**< \brief Is it detected? */
#define NETIF_INITIALIZED   0x00000004      /**< \brief Has it been initialized? */
#define NETIF_RUNNING       0x00000008      /**< \brief Has start() been called? */
#define NETIF_PROMISC       0x00010000      /**< \brief Promiscuous mode */
#define NETIF_NEEDSPOLL     0x01000000      /**< \brief Needs to be polled for input */
#define NETIF_NOETH         0x10000000      /**< \brief Does not use ethernet */
/** @} */

/** \defgroup net_drivers_returns    TX Return Values
    \brief                           Driver return values for TX network interfaces
    \ingroup                         networking_drivers
    @{
 */
#define NETIF_TX_OK     0   /**< \brief Tx success */
#define NETIF_TX_ERROR  -1  /**< \brief Tx general error */
#define NETIF_TX_AGAIN  -2  /**< \brief Retry Tx later */
/** @} */

/** \defgroup net_drivers_blocking  Blocking Types
    \brief                          Blocking type avlues for network interfaces
    \ingroup                        networking_drivers
    @{
 */
#define NETIF_NOBLOCK   0   /**< \brief Don't block for Tx */
#define NETIF_BLOCK     1   /**< \brief Blocking is OK for Tx */
/** @} */

/** \cond */
/* Define the list type */
LIST_HEAD(netif_list, knetif);
/** \endcond */

/** \defgroup networking_ip     IP
    \brief                      API for the Internet Protocol
    \ingroup                    networking
*/

/** \defgroup networking_ipv4   IPv4
    \brief                      IPv4 Network Stack
    \ingroup                    networking_ip
*/

/** \brief IPv4 Packet header.
    \headerfile kos/net.h
    \ingroup    networking_ipv4
*/
typedef struct ip_hdr_s {
    uint8   version_ihl;        /**< \brief IP version and header length */
    uint8   tos;                /**< \brief Type of Service */
    uint16  length;             /**< \brief Length */
    uint16  packet_id;          /**< \brief Packet ID */
    uint16  flags_frag_offs;    /**< \brief Flags and fragment offset */
    uint8   ttl;                /**< \brief Time to live */
    uint8   protocol;           /**< \brief IP protocol */
    uint16  checksum;           /**< \brief IP checksum */
    uint32  src;                /**< \brief Source IP address */
    uint32  dest;               /**< \brief Destination IP address */
} ip_hdr_t;

/** \defgroup networking_ipv6   IPv6
    \brief                      IPv6 Network Stack
    \ingroup                    networking_ip
*/

/** \brief  IPv6 Packet header.
    \headerfile kos/net.h
    \ingroup    networking_ipv6
*/
typedef struct ipv6_hdr_s {
    uint8           version_lclass; /**< \brief Version and low-order class
                                                byte */
    uint8           hclass_lflow;   /**< \brief High-order class byte, low-order
                                                flow byte */
    uint16          lclass;         /**< \brief Low-order class byte */
    uint16          length;         /**< \brief Length */
    uint8           next_header;    /**< \brief Next header type */
    uint8           hop_limit;      /**< \brief Hop limit */
    struct in6_addr src_addr;       /**< \brief Source IP address */
    struct in6_addr dst_addr;       /**< \brief Destination IP address */
} ipv6_hdr_t;

/***** net_arp.c **********************************************************/

/** \defgroup networking_arp    ARP
    \brief                      API for the Address Resolution Protocol
    \ingroup                    networking
*/

/** \brief   Init ARP.
    \ingroup networking_arp

    \retval  0               On success (no error conditions defined).
*/
int net_arp_init(void);

/** \brief   Shutdown ARP. 
    \ingroup networking_arp
 */
void net_arp_shutdown(void);

/** \brief   Add an entry to the ARP cache manually.
    \ingroup networking_arp

    \param  nif             The network device in use.
    \param  mac             The MAC address of the entry.
    \param  ip              The IPv4 address of the entry.
    \param  timestamp       The entry's timestamp. Set to 0 for a permanent
                            entry, otherwise set to the current number of
                            milliseconds since boot (i.e, timer_ms_gettime64()).
    
    \retval 0               On success.
    \retval -1              Error allocating memory.
*/
int net_arp_insert(netif_t *nif, const uint8 mac[6], const uint8 ip[4],
                   uint64 timestamp);

/** \brief   Look up an entry from the ARP cache.
    \ingroup networking_arp

    If no entry is found, then an ARP query will be sent and an error will be
    returned. If you specify a packet with the call, it will be sent when the
    reply comes in.

    \param  nif             The network device in use.
    \param  ip_in           The IP address to lookup.
    \param  mac_out         Storage for the MAC address, if found.
    \param  pkt             A simple IPv4 header, if you want to send one when a
                            response comes in (if not found immediately).
    \param  data            Packet data to go with the header.
    \param  data_size       The size of data.
    
    \retval 0               On success.
    \retval -1              A query is outstanding for that address.
    \retval -2              Address not found, query generated.
    \retval -3              Error allocating memory.
*/
int net_arp_lookup(netif_t *nif, const uint8 ip_in[4], uint8 mac_out[6],
                   const ip_hdr_t *pkt, const uint8 *data, int data_size);

/** \brief   Do a reverse ARP lookup.
    \ingroup networking_arp

    This function looks for an IP for a given mac address; note that if this
    fails, you have no recourse.

    \param  nif             The network device in use.
    \param  ip_out          Storage for the IPv4 address.
    \param  mac_in          The MAC address to look up.
    
    \retval 0               On success.
    \retval -1              On failure.
*/
int net_arp_revlookup(netif_t *nif, uint8 ip_out[4], const uint8 mac_in[6]);

/** \brief   Receive an ARP packet and process it (called by net_input).
    \ingroup networking_arp

    \param  nif             The network device in use.
    \param  pkt             The packet received.
    \param  len             The length of the packet.
    
    \retval 0               On success (no error conditions defined).
*/
int net_arp_input(netif_t *nif, const uint8 *pkt, int len);

/** \brief   Generate an ARP who-has query on the given device.
    \ingroup networking_arp

    \param  nif             The network device to use.
    \param  ip              The IP to query.
    
    \retval 0               On success (no error conditions defined).
*/
int net_arp_query(netif_t *nif, const uint8 ip[4]);


/***** net_input.c *********************************************************/

/** \brief   Network input callback type.
    \ingroup networking_drivers

    \param  nif             The network device in use.
    \param  pkt             The packet received.
    \param  len             The length of the packet, in bytes.
    
    \return                 0 on success, <0 on failure.
*/
typedef int (*net_input_func)(netif_t *nif, const uint8 *pkt, int len);

/** \brief   Where will input packets be routed? 
    \ingroup networking_drivers
 */
extern net_input_func net_input_target;

/** \brief   Device drivers should call this function to submit packets received
             in the background.
    \ingroup networking_drivers

    This function may or may not return immediately but it won't take an
    infinitely long time (so it's safe to call inside interrupt handlers).

    \param  device          The network device submitting packets.
    \param  data            The packet to submit.
    \param  len             The length of the packet, in bytes.
    
    \return                 0 on success, <0 on failure.
*/
int net_input(netif_t *device, const uint8 *data, int len);

/** \brief   Setup a network input target.
    \ingroup networking_drivers

    \param  t               The new target callback.
    
    \return                 The old target.
*/
net_input_func net_input_set_target(net_input_func t);

/***** net_icmp.c *********************************************************/

/** \defgroup networking_icmp   ICMP
    \brief                      API for the Internet Control Message Protocol
    \ingroup                    networking
*/

/** \defgroup networking_icmpv4 ICMPv4
    \brief                      API for v4 of the Internet Control Message
                                Protocol
    \ingroup                    networking_icmp
*/

/** \brief   ICMPv4 echo reply callback type.
    \ingroup networking_icmpv4

    \param  ip              The IPv4 address the reply is from.
    \param  seq             The sequence number of the packet.
    \param  delta_us        The time difference, in microseconds.
    \param  ttl             The TTL value in the packet.
    \param  data            Any data in the packet.
    \param  len             The length of the data, in bytes.
*/
typedef void (*net_echo_cb)(const uint8 *ip, uint16 seq, uint64 delta_us,
                            uint8 ttl, const uint8 *data, size_t len);

/** \brief   Where will we handle possibly notifying the user of ping replies?
    \ingroup networking_icmp
 */
extern net_echo_cb net_icmp_echo_cb;

/** \brief   Send an ICMP Echo packet to the specified IP.
    \ingroup networking_icmpv4

    \param  net             The network device to use.
    \param  ipaddr          The IPv4 address to send to.
    \param  ident           A packet identifier.
    \param  seq             A packet sequence number.
    \param  data            Data to send with the packet.
    \param  size            The size of the data to send.
    
    \return                 0 on success, <0 on failure.
*/
int net_icmp_send_echo(netif_t *net, const uint8 ipaddr[4], uint16 ident,
                       uint16 seq, const uint8 *data, size_t size);

/** \defgroup networking_icmp_unreach Unreachable Values 
    \brief    Valid values for net_icmp_send_dest_unreach().
    \ingroup  networking_icmpv4
    @{
*/
#define ICMP_PROTOCOL_UNREACHABLE       2   /**< \brief Protocol unreachable */
#define ICMP_PORT_UNREACHABLE           3   /**< \brief Port unreachable */
/** @} */

/** \brief   Send an ICMP Destination Unreachable packet in reply to the given
             message.
    \ingroup networking_icmpv4

    \param  net             The network device to use.
    \param  code            The type of message this is.
    \param  msg             The message that caused this error.
    
    \return                 0 on success, <0 on failure.
*/
int net_icmp_send_dest_unreach(netif_t *net, uint8 code, const uint8 *msg);

/** \brief   Valid values for the code in the net_icmp_send_time_exceeded()
             function.
    \ingroup networking_icmpv4
 */
#define ICMP_REASSEMBLY_TIME_EXCEEDED   1

/** \brief   Send an ICMP Time Exceeded packet in reply to the given message.
    \ingroup networking_icmp
    
    \param  net             The network device to use.
    \param  code            The type of message this is.
    \param  msg             The message that caused this error.
    
    \return                 0 on success, <0 on failure.
*/
int net_icmp_send_time_exceeded(netif_t *net, uint8 code, const uint8 *msg);

/***** net_ipv4.c *********************************************************/

/** \brief   IPv4 statistics structure.
    \ingroup networking_ipv4

    This structure holds some basic statistics about the IPv4 layer of the
    stack, and can be retrieved with the appropriate function.

    \headerfile kos/net.h
*/
typedef struct net_ipv4_stats {
    uint32  pkt_sent;               /** \brief Packets sent out successfully */
    uint32  pkt_send_failed;        /** \brief Packets that failed to send */
    uint32  pkt_recv;               /** \brief Packets received successfully */
    uint32  pkt_recv_bad_size;      /** \brief Packets of a bad size */
    uint32  pkt_recv_bad_chksum;    /** \brief Packets with a bad checksum */
    uint32  pkt_recv_bad_proto;     /** \brief Packets with an unknown proto */
} net_ipv4_stats_t;

/** \brief   Retrieve statistics from the IPv4 layer.
    \ingroup networking_ipv4

    \return                 The net_ipv4_stats_t structure.
*/
net_ipv4_stats_t net_ipv4_get_stats(void);

/** \brief   Create a 32-bit IP address, based on the individual numbers
             contained within the IP.
    \ingroup networking_ipv4

    \param  addr            Array of IP address octets.
    
    \return                 The address, in host byte order.
*/
uint32 net_ipv4_address(const uint8 addr[4]);

/** \brief   Parse an IP address that is packet into a uint32 into an array of
             the individual bytes.
    \ingroup networking_ipv4
    
    \param  addr            The full address, in host byte order.
    \param  out             The output buffer.
*/
void net_ipv4_parse_address(uint32 addr, uint8 out[4]);

/***** net_icmp6.c ********************************************************/

/** \defgroup networking_icmpv6     ICMPv6
    \brief                          API for v6 of the Internet Control Message
                                    Protocol
    \ingroup                        networking_icmp
*/

/** \brief   ICMPv6 echo reply callback type.
    \ingroup networking_icmpv6
    
    \param  ip              The IPv6 address the reply is from.
    \param  seq             The sequence number of the packet.
    \param  delta_us        The time difference, in microseconds.
    \param  hlim            The hop limit value in the packet.
    \param  data            Any data in the packet.
    \param  len             The length of the data, in bytes.
*/
typedef void (*net6_echo_cb)(const struct in6_addr *ip, uint16 seq,
                             uint64 delta_us, uint8 hlim, const uint8 *data,
                             size_t len);

/** \brief   Where will we handle possibly notifying the user of ping replies?
    \ingroup networking_icmpv6
 */
extern net6_echo_cb net_icmp6_echo_cb;

/** \brief   Send an ICMPv6 Echo (PING6) packet to the specified device.
    \ingroup networking_icmpv6
    
    \param  net             The network device to use.
    \param  dst             The address to send to.
    \param  ident           A packet identifier.
    \param  seq             A packet sequence number.
    \param  data            Data to send with the packet.
    \param  size            Length of the data, in bytes.
    
    \return                 0 on success, <0 on failure.
*/
int net_icmp6_send_echo(netif_t *net, const struct in6_addr *dst, uint16 ident,
                        uint16 seq, const uint8 *data, size_t size);

/** \brief   Send a Neighbor Solicitation packet on the specified device.
    \ingroup networking_icmpv6
    
    \param  net             The network device to use.
    \param  dst             The destination address.
    \param  target          The target address.
    \param  dupdet          1 if this is for duplicate detection.
    
    \return                 0 on success, <0 on failure.
*/
int net_icmp6_send_nsol(netif_t *net, const struct in6_addr *dst,
                        const struct in6_addr *target, int dupdet);

/** \brief   Send a Neighbor Advertisement packet on the specified device.
    \ingroup networking_icmpv6
    
    \param  net             The network device to use.
    \param  dst             The destination address.
    \param  target          The target address.
    \param  sol             1 if solicited, 0 otherwise.
    
    \return                 0 on success, <0 on failure.
*/
int net_icmp6_send_nadv(netif_t *net, const struct in6_addr *dst,
                        const struct in6_addr *target, int sol);

/** \brief   Send a Router Solicitation request on the specified interface.
    \ingroup networking_icmpv6
    
    \param  net             The network device to use.
    
    \return                 0 on success, <0 on failure.
*/
int net_icmp6_send_rsol(netif_t *net);

/** \defgroup networking_icmpv6_unreachable Destination Unreachable Codes
    \brief                                  Destination unreachable packet types
    \ingroup                                networking_icmpv6

    Only port unreachable really makes sense

    @{
*/
#define ICMP6_DEST_UNREACH_NO_ROUTE     0   /**< \brief No route available */
#define ICMP6_DEST_UNREACH_PROHIBITED   1   /**< \brief Access prohibited */
#define ICMP6_DEST_UNREACH_BEYOND_SCOPE 2   /**< \brief Gone beyond scope */
#define ICMP6_DEST_UNREACH_ADDR_UNREACH 3   /**< \brief Address unreachable */
#define ICMP6_DEST_UNREACH_PORT_UNREACH 4   /**< \brief Port unreachable */
#define ICMP6_DEST_UNREACH_FAIL_EGRESS  5   /**< \brief Egress failure */
#define ICMP6_DEST_UNREACH_BAD_ROUTE    6   /**< \brief Bad route specified */
/** @} */

/** \brief   Send a destination unreachable packet on the specified interface.
    \ingroup networking_icmpv6

    \param  net             The network device to use.
    \param  code            The type of message this is.
    \param  ppkt            The message that caused this error.
    \param  psz             Size of the original message.
    
    \return                 0 on success, <0 on failure.
*/
int net_icmp6_send_dest_unreach(netif_t *net, uint8 code, const uint8 *ppkt,
                                size_t psz);

/** \defgroup networking_icmpv6_time_exceeded Time Exceeded Codes
    \brief                                    Time exceeded codes for ICMPv6
    \ingroup                                  networking_icmpv6

    Only fragment reassembly time exceeded makes sense 

    @{
*/
#define ICMP6_TIME_EXCEEDED_HOPS_EXC    0   /**< \brief Hops exceeded */
#define ICMP6_TIME_EXCEEDED_FRAGMENT    1   /**< \brief Reassembly time gone */
/** @} */

/** \brief   Send a time exceeded message on the specified interface.
    \ingroup networking_icmpv6
    
    \param  net             The network device to use.
    \param  code            The error code.
    \param  ppkt            The message that caused this error.
    \param  psz             Size of the original packet.
    
    \return                 0 on success, <0 on failure.
*/
int net_icmp6_send_time_exceeded(netif_t *net, uint8 code, const uint8 *ppkt,
                                 size_t psz);

/** \defgroup networking_icmpv6_param_problem   Parameter Problem Codes
    \brief                                      Codes for ICMPv6 parameter problem packets
    \ingroup                                    networking_icmpv6
    @{
 */
#define ICMP6_PARAM_PROB_BAD_HEADER     0   /**< \brief Malformed header */
#define ICMP6_PARAM_PROB_UNK_HEADER     1   /**< \brief Unknown header */
#define ICMP6_PARAM_PROB_UNK_OPTION     2   /**< \brief Unknown header option */
/** @} */

/** \brief   Send an ICMPv6 Parameter Problem about the given packet.
    \ingroup networking_icmpv6
    
    \param  net             The network device to use.
    \param  code            The error code.
    \param  ptr             Where in the packet is the error?
    \param  ppkt            The message that caused the error.
    \param  psz             Size of the original packet.
    
    \return                 0 on success, <0 on failure.
*/
int net_icmp6_send_param_prob(netif_t *net, uint8 code, uint32 ptr,
                              const uint8 *ppkt, size_t psz);

/***** net_ipv6.c *********************************************************/

/** \brief   IPv6 statistics structure.
    \ingroup networking_ipv6

    This structure holds some basic statistics about the IPv6 layer of the
    stack, and can be retrieved with the appropriate function.

    \headerfile kos/net.h
*/
typedef struct net_ipv6_stats {
    uint32  pkt_sent;               /**< \brief Packets sent out successfully */
    uint32  pkt_send_failed;        /**< \brief Packets that failed to send */
    uint32  pkt_recv;               /**< \brief Packets received successfully */
    uint32  pkt_recv_bad_size;      /**< \brief Packets of a bad size */
    uint32  pkt_recv_bad_proto;     /**< \brief Packets with an unknown proto */
    uint32  pkt_recv_bad_ext;       /**< \brief Packets with an unknown hdr */
} net_ipv6_stats_t;

/** \brief   Retrieve statistics from the IPv6 layer.
    \ingroup networking_ipv6

    \return                 The global IPv6 stats structure.
*/
net_ipv6_stats_t net_ipv6_get_stats(void);

/***** net_ndp.c **********************************************************/

/** \defgroup networking_ndp    NDP
    \brief                      API for the Neighbor Discovery Protocol
    \ingroup                    networking
    @{
*/

/** \brief  Init NDP.
    \retval 0               On success (no error conditions defined).
*/
int net_ndp_init(void);

/** \brief  Shutdown NDP. */
void net_ndp_shutdown(void);

/** \brief  Garbage collect timed out NDP entries.
    This will be called periodically as NDP queries come in.
*/
void net_ndp_gc(void);

/** \brief  Add an entry to the NDP cache.
    \param  nif             The network device in question.
    \param  mac             The MAC address for the entry.
    \param  ip              The IPv6 address for the entry.
    \param  unsol           Was this unsolicited?
    \return                 0 on success, <0 on failure.
*/
int net_ndp_insert(netif_t *nif, const uint8 mac[6], const struct in6_addr *ip,
                   int unsol);

/** \brief  Look up an entry from the NDP cache.

    If no entry is found, then an NDP query will be sent and an error will be
    returned. If you specify a packet with the call, it will be sent when the
    reply comes in.

    \param  net             The network device to use.
    \param  ip              The IPv6 address to query.
    \param  mac_out         Storage for the MAC address on success.
    \param  pkt             A simple IPv6 header, if you want to send a packet
                            when a reply comes in.
    \param  data            Anything that comes after the header.
    \param  data_size       The size of data.
    \return                 0 on success, <0 on failure.
*/
int net_ndp_lookup(netif_t *net, const struct in6_addr *ip, uint8 mac_out[6],
                   const ipv6_hdr_t *pkt, const uint8 *data, int data_size);

/** @} */

/***** net_udp.c **********************************************************/

/** \defgroup networking_udp    UDP
    \brief                      API for the User Datagram Protocol
    \ingroup                    networking
    @{
*/

/** \brief  UDP statistics structure.

    This structure holds some basic statistics about the UDP layer of the stack,
    and can be retrieved with the appropriate function.

    \headerfile kos/net.h
*/
typedef struct net_udp_stats {
    uint32  pkt_sent;               /**< \brief Packets sent out successfully */
    uint32  pkt_send_failed;        /**< \brief Packets that failed to send */
    uint32  pkt_recv;               /**< \brief Packets received successfully */
    uint32  pkt_recv_bad_size;      /**< \brief Packets of a bad size */
    uint32  pkt_recv_bad_chksum;    /**< \brief Packets with a bad checksum */
    uint32  pkt_recv_no_sock;       /**< \brief Packets with to a closed port */
} net_udp_stats_t;

/** \brief  Retrieve statistics from the UDP layer.
    
    \return                 The global UDP stats struct.
*/
net_udp_stats_t net_udp_get_stats(void);

/** \brief  Init UDP.
    
    \retval 0               On success (no error conditions defined).
*/
int net_udp_init(void);

/** \brief  Shutdown UDP. */
void net_udp_shutdown(void);

/** @} */

/***** net_tcp.c **********************************************************/

/** \defgroup networking_tcp TCP
    \brief                   API for the Transmission Control Protocol
    \ingroup                 networking
    @{
*/

/** \brief  Init TCP.
    \retval 0               On success (no error conditions defined).
*/
int net_tcp_init(void);

/** \brief  Shutdown TCP. */
void net_tcp_shutdown(void);

/** @} */

/***** net_crc.c **********************************************************/

/** \defgroup networking_crc    CRC
    \brief                      API for Cyclic Redundancy Checking
    \ingroup                    networking
    @{
*/

/** \brief  Calculate a "little-endian" CRC-32 over a block of data.
    
    \param  data            The data to calculate over.
    \param  size            The size of the data, in bytes.
    
    \return                 The calculated CRC-32.
*/
uint32 net_crc32le(const uint8 *data, int size);

/** \brief  Calculate a "big-endian" CRC-32 over a block of data.
    
    \param  data            The data to calculate over.
    \param  size            The size of the data, in bytes.
    
    \return                 The calculated CRC-32.
*/
uint32 net_crc32be(const uint8 *data, int size);

/** \brief  Calculate a CRC16-CCITT over a block of data.
    
    \note                   Based on code found online at
                            http://www.ccsinfo.com/forum/viewtopic.php?t=24977

    \param  data            The data to calculate over.
    \param  size            The size of the data, in bytes.
    \param  start           The value to start with. This could be a previous
                            return value from this function (if continuing a
                            previous calculation) or some initial seed value
                            (typically 0xFFFF or 0x0000).
    
    \return                 The calculated CRC16-CCITT.
*/
uint16 net_crc16ccitt(const uint8 *data, int size, uint16 start);

/** @} */

/***** net_multicast.c ****************************************************/

/** \defgroup networking_multicast  Multicast
    \brief                          API for Managing the Multicast List
    \ingroup                        networking
    @{
*/

/** \brief  Add a entry to our multicast list.

    This function will auto-commit the multicast list to the network interface
    in the process.

    \param  mac             The MAC address to add.
    \return                 0 on success, <0 on failure.
*/
int net_multicast_add(const uint8 mac[6]);

/** \brief  Delete a entry from our multicast list.

    This function will auto-commit the multicast list to the network interface
    in the process.

    \param  mac             The MAC address to add.
    \return                 0 on success, <0 on failure.
*/
int net_multicast_del(const uint8 mac[6]);

/** \brief  Check if an address is on the multicast list.
    \param  mac             The MAC address to check for.
    \retval 0               The address is not in the list.
    \retval 1               The address is in the list.
    \retval -1              On error.
*/
int net_multicast_check(const uint8 mac[6]);

/** \brief  Init multicast support.
    \return                 0 on success, !0 on error.
*/
int net_multicast_init(void);

/** \brief  Shutdown multicast support. */
void net_multicast_shutdown(void);

/** @} */

/***** net_core.c *********************************************************/

/** \brief   Interface list; note: do not manipulate directly! 
    \ingroup networking_drivers
 */
extern struct netif_list net_if_list;

/** \brief   Function to retrieve the interface list.
    \ingroup networking_drivers
    
    \warning
    Do not manipulate what this returns to you!

    \return                 The network interface list.
*/
struct netif_list * net_get_if_list(void);

/** \brief   The default network device, used with sockets (read-only). 
    \ingroup networking_drivers    
*/
extern netif_t *net_default_dev;

/** \brief   Set our default device to an arbitrary device.
    \ingroup networking_drivers

    \param  n               The device to set as default.
    
    \return                 The old default device.
*/
netif_t *net_set_default(netif_t *n);

/** \brief   Register a network device.
    \ingroup networking_drivers

    \param  device          The device to register.
    \
    \return                 0 on success, <0 on failure.
*/
int net_reg_device(netif_t *device);

/** \brief   Unregister a network device.
    \ingroup networking_drivers

    \param  device          The device to unregister.
    
    \return                 0 on success, <0 on failure.
*/
int net_unreg_device(netif_t *device);

/** \brief   Init network support.
    \ingroup networking_drivers
    
    \note                   To auto-detect the IP address to assign to the
                            default device (i.e, over DHCP or from the flashrom
                            on the Dreamcast), pass 0 as the IP parameter.

    \param  ip              The IPv4 address to set on the default device, in
                            host byte order.

    \return                 0 on success, <0 on failure.
*/
int net_init(uint32 ip);

/** \brief   Shutdown network support. 
    \ingroup networking_drivers
*/
void net_shutdown(void);

__END_DECLS

#endif  /* __KOS_NET_H */
