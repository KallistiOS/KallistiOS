/* KallistiOS ##version##

   kernel/net/net_dhcp.h
   Copyright (C) 2008, 2013 Lawrence Sebald

*/

#ifndef __LOCAL_NET_DHCP_H
#define __LOCAL_NET_DHCP_H

#include <kos/cdefs.h>

__BEGIN_DECLS

/* Available values for the op fields of dhcp_pkt_t */
#define DHCP_OP_BOOTREQUEST             1
#define DHCP_OP_BOOTREPLY               2

/* The defined htype for Ethernet */
#define DHCP_HTYPE_10MB_ETHERNET        1

/* The length of an Ethernet hardware address */
#define DHCP_HLEN_ETHERNET              6

/* DHCP Option types, as defined in RFC 2132.
   Note that most of these aren't actually supported/used, but they're here
   for completeness. */
#define DHCP_OPTION_PAD                 0
#define DHCP_OPTION_SUBNET_MASK         1
#define DHCP_OPTION_TIME_OFFSET         2
#define DHCP_OPTION_ROUTER              3
#define DHCP_OPTION_TIME_SERVER         4
#define DHCP_OPTION_NAME_SERVER         5
#define DHCP_OPTION_DOMAIN_NAME_SERVER  6
#define DHCP_OPTION_LOG_SERVER          7
#define DHCP_OPTION_COOKIE_SERVER       8
#define DHCP_OPTION_LPR_SERVER          9
#define DHCP_OPTION_IMPRESS_SERVER      10
#define DHCP_OPTION_RESOURCE_LOC_SERVER 11
#define DHCP_OPTION_HOST_NAME           12
#define DHCP_OPTION_BOOT_FILE_SIZE      13
#define DHCP_OPTION_MERIT_DUMP_FILE     14
#define DHCP_OPTION_DOMAIN_NAME         15
#define DHCP_OPTION_SWAP                16
#define DHCP_OPTION_ROOT_PATH           17
#define DHCP_OPTION_EXTENSIONS_PATH     18
#define DHCP_OPTION_IP_FORWARDING       19
#define DHCP_OPTION_NON_LOCAL_SRC_ROUTE 20
#define DHCP_OPTION_POLICY_FILTER       21
#define DHCP_OPTION_MAX_REASSEMBLY      22
#define DHCP_OPTION_DEFAULT_TTL         23
#define DHCP_OPTION_PATH_MTU_AGING_TIME 24
#define DHCP_OPTION_PATH_MTU_PLATEAU    25
#define DHCP_OPTION_INTERFACE_MTU       26
#define DHCP_OPTION_ALL_SUBNETS_LOCAL   27
#define DHCP_OPTION_BROADCAST_ADDR      28
#define DHCP_OPTION_PERFORM_MASK_DISC   29
#define DHCP_OPTION_MASK_SUPPLIER       30
#define DHCP_OPTION_PERFORM_ROUTER_DISC 31
#define DHCP_OPTION_ROUTER_SOLICT_ADDR  32
#define DHCP_OPTION_STATIC_ROUTE        33
#define DHCP_OPTION_TRAILER_ENCAPS      34
#define DHCP_OPTION_ARP_CACHE_TIMEOUT   35
#define DHCP_OPTION_ETHERNET_ENCAPS     36
#define DHCP_OPTION_TCP_TTL             37
#define DHCP_OPTION_TCP_KEEPALIVE_INT   38
#define DHCP_OPTION_TCP_KEEPALIVE_GARB  39
#define DHCP_OPTION_NIS_DOMAIN          40
#define DHCP_OPTION_NIS_SERVER          41
#define DHCP_OPTION_NTP_SERVER          42
#define DHCP_OPTION_VENDOR              43
#define DHCP_OPTION_NBNS_NAME_SERVER    44
#define DHCP_OPTION_NBDD_SERVER         45
#define DHCP_OPTION_NB_NODE_TYPE        46
#define DHCP_OPTION_NB_SCOPE            47
#define DHCP_OPTION_X_FONT_SERVER       48
#define DHCP_OPTION_X_DISPLAY_MGR       49
#define DHCP_OPTION_REQ_IP_ADDR         50
#define DHCP_OPTION_IP_LEASE_TIME       51
#define DHCP_OPTION_OVERLOAD            52
#define DHCP_OPTION_MESSAGE_TYPE        53
#define DHCP_OPTION_SERVER_ID           54
#define DHCP_OPTION_PARAMETER_REQUEST   55
#define DHCP_OPTION_MESSAGE             56
#define DHCP_OPTION_MAX_MESSAGE         57
#define DHCP_OPTION_RENEWAL_TIME        58
#define DHCP_OPTION_REBINDING_TIME      59
#define DHCP_OPTION_VENDOR_CLASS_ID     60
#define DHCP_OPTION_CLIENT_ID           61
/* 62 and 63 undefined by RFC 2132 */
#define DHCP_OPTION_NISPLUS_DOMAIN      64
#define DHCP_OPTION_NISPLUS_SERVER      65
#define DHCP_OPTION_TFTP_SERVER         66
#define DHCP_OPTION_BOOTFILE_NAME       67
#define DHCP_OPTION_MIP_HOME_AGENT      68
#define DHCP_OPTION_SMTP_SERVER         69
#define DHCP_OPTION_POP3_SERVER         70
#define DHCP_OPTION_NNTP_SERVER         71
#define DHCP_OPTION_WWW_SERVER          72
#define DHCP_OPTION_FINGER_SERVER       73
#define DHCP_OPTION_IRC_SERVER          74
#define DHCP_OPTION_STREETTALK_SERVER   75
#define DHCP_OPTION_STDA_SERVER         76
#define DHCP_OPTION_END                 255

/* DHCP Message Types, as defined in RFC 2132. */
#define DHCP_MSG_DHCPDISCOVER   1
#define DHCP_MSG_DHCPOFFER      2
#define DHCP_MSG_DHCPREQUEST    3
#define DHCP_MSG_DHCPDECLINE    4
#define DHCP_MSG_DHCPACK        5
#define DHCP_MSG_DHCPNAK        6
#define DHCP_MSG_DHCPRELEASE    7
#define DHCP_MSG_DHCPINFORM     8

/* DHCP Client States */
#define DHCP_STATE_INIT         0
#define DHCP_STATE_SELECTING    1
#define DHCP_STATE_REQUESTING   2
#define DHCP_STATE_BOUND        3
#define DHCP_STATE_RENEWING     4
#define DHCP_STATE_REBINDING    5
#define DHCP_STATE_INIT_REBOOT  6
#define DHCP_STATE_REBOOTING    7

typedef struct dhcp_pkt {
    uint8_t   op;
    uint8_t   htype;
    uint8_t   hlen;
    uint8_t   hops;
    uint32_t  xid;
    uint16_t  secs;
    uint16_t  flags;
    uint32_t  ciaddr;
    uint32_t  yiaddr;
    uint32_t  siaddr;
    uint32_t  giaddr;
    uint8_t   chaddr[16];
    char      sname[64];
    char      file[128];
    uint8_t   options[];
} __packed dhcp_pkt_t;

int net_dhcp_init(void);

void net_dhcp_shutdown(void);

int net_dhcp_request(uint32_t required_address);

__END_DECLS

#endif /* !__LOCAL_NET_DHCP_H */
