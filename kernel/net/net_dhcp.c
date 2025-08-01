/* KallistiOS ##version##

   kernel/net/net_dhcp.c
   Copyright (C) 2008, 2009, 2013 Lawrence Sebald

*/

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <kos/net.h>
#include <kos/genwait.h>
#include <kos/mutex.h>
#include <kos/fs_socket.h>

#include <arch/timer.h>

#include "net_dhcp.h"
#include "net_thd.h"

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

#define DHCP_MIN_OPTIONS_SIZE 64


static int dhcp_sock = -1;
struct sockaddr_in srv_addr;

struct dhcp_pkt_out {
    STAILQ_ENTRY(dhcp_pkt_out) pkt_queue;
    uint8_t *buf;
    int size;
    int pkt_type;
    int next_delay;
    uint64_t next_send;
};

STAILQ_HEAD(dhcp_pkt_queue, dhcp_pkt_out);

static struct dhcp_pkt_queue dhcp_pkts = STAILQ_HEAD_INITIALIZER(dhcp_pkts);
static mutex_t dhcp_lock = RECURSIVE_MUTEX_INITIALIZER;
static int dhcp_cbid = -1;
static uint64_t renew_time = 0xFFFFFFFFFFFFFFFFULL;
static uint64_t rebind_time = 0xFFFFFFFFFFFFFFFFULL;
static uint64_t lease_expires = 0xFFFFFFFFFFFFFFFFULL;
static int state = DHCP_STATE_INIT;

static int net_dhcp_fill_options(netif_t *net, dhcp_pkt_t *req, uint8_t msgtype,
                                 uint32_t serverid, uint32_t reqip) {
    int pos = 0;

    /* DHCP Magic Cookie */
    req->options[pos++] = 0x63;
    req->options[pos++] = 0x82;
    req->options[pos++] = 0x53;
    req->options[pos++] = 0x63;

    /* Message Type: DHCPDISCOVER */
    req->options[pos++] = DHCP_OPTION_MESSAGE_TYPE;
    req->options[pos++] = 1; /* Length = 1 */
    req->options[pos++] = msgtype;

    /* Max Message Length: 1500 octets */
    req->options[pos++] = DHCP_OPTION_MAX_MESSAGE;
    req->options[pos++] = 2; /* Length = 2 */
    req->options[pos++] = (net->mtu >> 8) & 0xFF;
    req->options[pos++] = (net->mtu >> 0) & 0xFF;

    /* Host Name: Dreamcast */
    const char* host_name = "KallistiOS";
    const uint8 size = strlen(host_name);
    req->options[pos++] = DHCP_OPTION_HOST_NAME;
    req->options[pos++] = size;
    memcpy(req->options + pos, host_name, size);
    pos += size;

    /* Client Identifier: The network adapter's MAC address */
    req->options[pos++] = DHCP_OPTION_CLIENT_ID;
    req->options[pos++] = 1 + DHCP_HLEN_ETHERNET; /* Length = 7 */
    req->options[pos++] = DHCP_HTYPE_10MB_ETHERNET;
    memcpy(req->options + pos, net->mac_addr, DHCP_HLEN_ETHERNET);
    pos += DHCP_HLEN_ETHERNET;

    /* Parameters requested: Subnet, Router, DNS, Broadcast, MTU */
    req->options[pos++] = DHCP_OPTION_PARAMETER_REQUEST;
    req->options[pos++] = 5; /* Length = 5 */
    req->options[pos++] = DHCP_OPTION_SUBNET_MASK;
    req->options[pos++] = DHCP_OPTION_ROUTER;
    req->options[pos++] = DHCP_OPTION_DOMAIN_NAME_SERVER;
    req->options[pos++] = DHCP_OPTION_BROADCAST_ADDR;
    req->options[pos++] = DHCP_OPTION_INTERFACE_MTU;

    if(serverid) {
        /* Add the Server identifier option */
        req->options[pos++] = DHCP_OPTION_SERVER_ID;
        req->options[pos++] = 4; /* Length = 4 */
        req->options[pos++] = (serverid >> 24) & 0xFF;
        req->options[pos++] = (serverid >> 16) & 0xFF;
        req->options[pos++] = (serverid >>  8) & 0xFF;
        req->options[pos++] = (serverid >>  0) & 0xFF;
    }

    if(reqip) {
        /* Add the requested IP address option */
        req->options[pos++] = DHCP_OPTION_REQ_IP_ADDR;
        req->options[pos++] = 4; /* Length = 4 */
        req->options[pos++] = (reqip >> 24) & 0xFF;
        req->options[pos++] = (reqip >> 16) & 0xFF;
        req->options[pos++] = (reqip >>  8) & 0xFF;
        req->options[pos++] = (reqip >>  0) & 0xFF;
    }

    /* The End */
    req->options[pos++] = DHCP_OPTION_END;

    /* DHCP is an extension of the BOOTP RFC which specifies that the
     * vendor specific area (which became 'options' in DHCP) is 64 bytes,
     * some routers reject DHCP packets if the options area is less than this */
    return (pos < DHCP_MIN_OPTIONS_SIZE) ? DHCP_MIN_OPTIONS_SIZE : pos;
}

static int __pure net_dhcp_get_message_type(dhcp_pkt_t *pkt, int len) {
    int i;

    len -= sizeof(dhcp_pkt_t);

    /* Read each byte of the options field looking for the message type option.
       when we find it, return the message type. */
    for(i = 4; i < len;) {
        if(pkt->options[i] == DHCP_OPTION_MESSAGE_TYPE) {
            return pkt->options[i + 2];
        }
        else if(pkt->options[i] == DHCP_OPTION_PAD ||
                pkt->options[i] == DHCP_OPTION_END) {
            ++i;
        }
        else {
            i += pkt->options[i + 1] + 2;
        }
    }

    return -1;
}

static uint32_t __pure net_dhcp_get_32bit(dhcp_pkt_t *pkt, uint8_t opt, int len) {
    int i;

    len -= sizeof(dhcp_pkt_t);

    /* Read each byte of the options field looking for the specified option,
       return it when found. */
    for(i = 4; i < len;) {
        if(pkt->options[i] == opt) {
            return (pkt->options[i + 2] << 24) | (pkt->options[i + 3] << 16) |
                   (pkt->options[i + 4] << 8) | (pkt->options[i + 5]);
        }
        else if(pkt->options[i] == DHCP_OPTION_PAD) {
            ++i;
        }
        else if(pkt->options[i] == DHCP_OPTION_END) {
            break;
        }
        else {
            i += pkt->options[i + 1] + 2;
        }
    }

    return 0;
}

static uint16_t __pure net_dhcp_get_16bit(dhcp_pkt_t *pkt, uint8_t opt, int len) {
    int i;

    len -= sizeof(dhcp_pkt_t);

    /* Read each byte of the options field looking for the specified option,
       return it when found. */
    for(i = 4; i < len;) {
        if(pkt->options[i] == opt) {
            return (pkt->options[i + 2] << 8) | (pkt->options[i + 3]);
        }
        else if(pkt->options[i] == DHCP_OPTION_PAD) {
            ++i;
        }
        else if(pkt->options[i] == DHCP_OPTION_END) {
            break;
        }
        else {
            i += pkt->options[i + 1] + 2;
        }
    }

    return 0;
}


int net_dhcp_request(uint32_t required_address) {
    uint8_t pkt[1500];
    uint16_t pkt_len;
    dhcp_pkt_t *req = (dhcp_pkt_t *)pkt;
    int optlen;
    struct dhcp_pkt_out *qpkt;
    int rv = 0;

    if(dhcp_sock == -1) {
        return -1;
    }

    if(mutex_lock_irqsafe(&dhcp_lock))
        return -1;

    /* Fill in the initial DHCPDISCOVER packet */
    req->op = DHCP_OP_BOOTREQUEST;
    req->htype = DHCP_HTYPE_10MB_ETHERNET;
    req->hlen = DHCP_HLEN_ETHERNET;
    req->hops = 0;
    req->xid = htonl(time(NULL) ^ 0xDEADBEEF);
    req->secs = 0;
    req->flags = 0;
    req->ciaddr = 0;
    req->yiaddr = 0;
    req->siaddr = 0;
    req->giaddr = 0;
    memcpy(req->chaddr, net_default_dev->mac_addr, DHCP_HLEN_ETHERNET);
    memset(req->chaddr + DHCP_HLEN_ETHERNET, 0, sizeof(req->chaddr) -
           DHCP_HLEN_ETHERNET);
    memset(req->sname, 0, sizeof(req->sname));
    memset(req->file, 0, sizeof(req->file));

    /* Fill in options */
    optlen = net_dhcp_fill_options(net_default_dev, req, DHCP_MSG_DHCPDISCOVER,
                                   0, required_address);

    /* Add to our packet queue */
    qpkt = (struct dhcp_pkt_out *)malloc(sizeof(struct dhcp_pkt_out));

    if(!qpkt) {
        mutex_unlock(&dhcp_lock);
        return -1;
    }

    pkt_len = sizeof(dhcp_pkt_t) + optlen;

    qpkt->buf = (uint8_t *)malloc(pkt_len);

    if(!qpkt->buf) {
        free(qpkt);
        mutex_unlock(&dhcp_lock);
        return -1;
    }

    qpkt->size = pkt_len;
    memcpy(qpkt->buf, pkt, pkt_len);
    qpkt->pkt_type = DHCP_MSG_DHCPDISCOVER;
    qpkt->next_send = 0;
    qpkt->next_delay = 4000;

    STAILQ_INSERT_TAIL(&dhcp_pkts, qpkt, pkt_queue);

    state = DHCP_STATE_SELECTING;
    mutex_unlock(&dhcp_lock);

    /* We need to wait til we're either bound to an IP address, or until we give
       up all hope of doing so (give us 60 seconds). */
    if(!net_thd_is_current()) {
        rv = genwait_wait(&dhcp_sock, "net_dhcp_request", 60 * 1000, NULL);
    }

    return rv;
}

static void net_dhcp_send_request(dhcp_pkt_t *pkt, int pktlen, dhcp_pkt_t *pkt2,
                                  int pkt2len) {
    uint8_t buf[1500];
    dhcp_pkt_t *req = (dhcp_pkt_t *)buf;
    int optlen;
    struct dhcp_pkt_out *qpkt;
    uint32_t serverid = net_dhcp_get_32bit(pkt, DHCP_OPTION_SERVER_ID, pktlen);

    (void)pkt2;
    (void)pkt2len;

    if(serverid == 0)
        return;

    /* Fill in the DHCP request */
    req->op = DHCP_OP_BOOTREQUEST;
    req->htype = DHCP_HTYPE_10MB_ETHERNET;
    req->hlen = DHCP_HLEN_ETHERNET;
    req->hops = 0;
    req->xid = pkt->xid;
    req->secs = 0;
    req->flags = 0;
    req->ciaddr = 0;
    req->yiaddr = 0;
    req->siaddr = 0;
    req->giaddr = 0;
    memcpy(req->chaddr, net_default_dev->mac_addr, DHCP_HLEN_ETHERNET);
    memset(req->chaddr + DHCP_HLEN_ETHERNET, 0, sizeof(req->chaddr) -
           DHCP_HLEN_ETHERNET);
    memset(req->sname, 0, sizeof(req->sname));
    memset(req->file, 0, sizeof(req->file));

    /* Fill in options */
    optlen = net_dhcp_fill_options(net_default_dev, req, DHCP_MSG_DHCPREQUEST,
                                   serverid, ntohl(pkt->yiaddr));

    /* Add to our packet queue */
    qpkt = (struct dhcp_pkt_out *)malloc(sizeof(struct dhcp_pkt_out));

    if(!qpkt) {
        return;
    }

    qpkt->buf = (uint8_t *)malloc(sizeof(dhcp_pkt_t) + optlen);

    if(!qpkt->buf) {
        free(qpkt);
        return;
    }

    qpkt->size = sizeof(dhcp_pkt_t) + optlen;
    memcpy(qpkt->buf, buf, sizeof(dhcp_pkt_t) + optlen);
    qpkt->pkt_type = DHCP_MSG_DHCPREQUEST;
    qpkt->next_send = 0;
    qpkt->next_delay = 4000;

    STAILQ_INSERT_TAIL(&dhcp_pkts, qpkt, pkt_queue);

    state = DHCP_STATE_REQUESTING;
}

static void net_dhcp_renew(void) {
    uint8_t buf[1500];
    dhcp_pkt_t *req = (dhcp_pkt_t *)buf;
    int optlen;
    struct dhcp_pkt_out *qpkt;

    /* Fill in the DHCP request */
    req->op = DHCP_OP_BOOTREQUEST;
    req->htype = DHCP_HTYPE_10MB_ETHERNET;
    req->hlen = DHCP_HLEN_ETHERNET;
    req->hops = 0;
    req->xid = time(NULL) ^ 0xDEADBEEF;
    req->secs = 0;
    req->flags = 0;
    req->ciaddr = htonl(net_ipv4_address(net_default_dev->ip_addr));
    req->yiaddr = 0;
    req->siaddr = 0;
    req->giaddr = 0;
    memcpy(req->chaddr, net_default_dev->mac_addr, DHCP_HLEN_ETHERNET);
    memset(req->chaddr + DHCP_HLEN_ETHERNET, 0, sizeof(req->chaddr) -
           DHCP_HLEN_ETHERNET);
    memset(req->sname, 0, sizeof(req->sname));
    memset(req->file, 0, sizeof(req->file));

    /* Fill in options */
    optlen = net_dhcp_fill_options(net_default_dev, req, DHCP_MSG_DHCPREQUEST,
                                   0, ntohl(req->ciaddr));

    /* Add to our packet queue */
    qpkt = (struct dhcp_pkt_out *)malloc(sizeof(struct dhcp_pkt_out));

    if(!qpkt) {
        return;
    }

    qpkt->buf = (uint8_t *)malloc(sizeof(dhcp_pkt_t) + optlen);

    if(!qpkt->buf) {
        free(qpkt);
        return;
    }

    qpkt->size = sizeof(dhcp_pkt_t) + optlen;
    memcpy(qpkt->buf, buf, sizeof(dhcp_pkt_t) + optlen);
    qpkt->pkt_type = DHCP_MSG_DHCPREQUEST;
    qpkt->next_send = 0;
    qpkt->next_delay = 60000;

    STAILQ_INSERT_TAIL(&dhcp_pkts, qpkt, pkt_queue);
}

static void net_dhcp_bind(dhcp_pkt_t *pkt, int len) {
    uint32_t tmp = ntohl(pkt->yiaddr);

    irq_disable_scoped();

    /* Bind the IP address first */
    net_default_dev->ip_addr[0] = (tmp >> 24) & 0xFF;
    net_default_dev->ip_addr[1] = (tmp >> 16) & 0xFF;
    net_default_dev->ip_addr[2] = (tmp >>  8) & 0xFF;
    net_default_dev->ip_addr[3] = (tmp >>  0) & 0xFF;

    /* Grab the netmask if it was returned to us */
    tmp = net_dhcp_get_32bit(pkt, DHCP_OPTION_SUBNET_MASK, len);

    if(tmp != 0) {
        net_default_dev->netmask[0] = (tmp >> 24) & 0xFF;
        net_default_dev->netmask[1] = (tmp >> 16) & 0xFF;
        net_default_dev->netmask[2] = (tmp >>  8) & 0xFF;
        net_default_dev->netmask[3] = (tmp >>  0) & 0xFF;
    }

    /* Grab the router address, if it was returned to us */
    tmp = net_dhcp_get_32bit(pkt, DHCP_OPTION_ROUTER, len);

    if(tmp != 0) {
        net_default_dev->gateway[0] = (tmp >> 24) & 0xFF;
        net_default_dev->gateway[1] = (tmp >> 16) & 0xFF;
        net_default_dev->gateway[2] = (tmp >>  8) & 0xFF;
        net_default_dev->gateway[3] = (tmp >>  0) & 0xFF;
    }

    /* Grab the DNS address if it was returned to us */
    tmp = net_dhcp_get_32bit(pkt, DHCP_OPTION_DOMAIN_NAME_SERVER, len);

    if(tmp != 0) {
        net_default_dev->dns[0] = (tmp >> 24) & 0xFF;
        net_default_dev->dns[1] = (tmp >> 16) & 0xFF;
        net_default_dev->dns[2] = (tmp >>  8) & 0xFF;
        net_default_dev->dns[3] = (tmp >>  0) & 0xFF;
    }

    /* Grab the broadcast address if it was sent to us, otherwise infer it from
       the netmask and IP address. */
    tmp = net_dhcp_get_32bit(pkt, DHCP_OPTION_BROADCAST_ADDR, len);

    if(tmp != 0) {
        net_default_dev->broadcast[0] = (tmp >> 24) & 0xFF;
        net_default_dev->broadcast[1] = (tmp >> 16) & 0xFF;
        net_default_dev->broadcast[2] = (tmp >>  8) & 0xFF;
        net_default_dev->broadcast[3] = (tmp >>  0) & 0xFF;
    }
    else {
        net_default_dev->broadcast[0] =
            net_default_dev->ip_addr[0] | (~net_default_dev->netmask[0]);
        net_default_dev->broadcast[1] =
            net_default_dev->ip_addr[1] | (~net_default_dev->netmask[1]);
        net_default_dev->broadcast[2] =
            net_default_dev->ip_addr[2] | (~net_default_dev->netmask[2]);
        net_default_dev->broadcast[3] =
            net_default_dev->ip_addr[3] | (~net_default_dev->netmask[3]);
    }

    /* Grab the Lease expiry time */
    tmp = net_dhcp_get_32bit(pkt, DHCP_OPTION_IP_LEASE_TIME, len);

    if(tmp != 0 && tmp != 0xFFFFFFFF) {
        /* Set our renewal timer to half the lease time and the rebinding timer
           to .875 * lease time. */
        uint64_t now = timer_ms_gettime64();
        int expiry = ntohl(tmp) * 1000;

        renew_time = now + (expiry >> 1);
        rebind_time = now + (uint64)(expiry * 0.875);
        lease_expires = now + expiry;
    }
    else if(tmp == 0xFFFFFFFF) {
        renew_time = rebind_time = lease_expires = 0xFFFFFFFFFFFFFFFFULL;
    }

    /* Grab the interface MTU, if we got it */
    tmp = net_dhcp_get_16bit(pkt, DHCP_OPTION_INTERFACE_MTU, len);

    if(tmp != 0) {
        net_default_dev->mtu = (int)((uint16_t)tmp);
    }

    state = DHCP_STATE_BOUND;
}

static void net_dhcp_thd(void *obj) {
    struct dhcp_pkt_out *qpkt, *q_tmp;
    uint64_t now;
    struct sockaddr_in addr;
    uint8_t buf[1500];
    ssize_t len = 0;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    dhcp_pkt_t *pkt = (dhcp_pkt_t *)buf, *pkt2;
    int found;

    (void)obj;

    now = timer_ms_gettime64();
    len = 0;

    mutex_lock_scoped(&dhcp_lock);

    /* Make sure we don't need to renew our lease */
    if(lease_expires <= now && (state == DHCP_STATE_BOUND ||
                                state == DHCP_STATE_RENEWING || state == DHCP_STATE_REBINDING)) {
        STAILQ_FOREACH_SAFE(qpkt, &dhcp_pkts, pkt_queue, q_tmp) {
            STAILQ_REMOVE(&dhcp_pkts, qpkt, dhcp_pkt_out, pkt_queue);
            free(qpkt->buf);
            free(qpkt);
        }

        state = DHCP_STATE_INIT;
        srv_addr.sin_addr.s_addr = INADDR_BROADCAST;
        memset(net_default_dev->ip_addr, 0, 4);
        net_dhcp_request(0);
    }
    else if(rebind_time <= now &&
            (state == DHCP_STATE_BOUND || state == DHCP_STATE_RENEWING)) {
        /* Clear out any existing packets. */
        STAILQ_FOREACH_SAFE(qpkt, &dhcp_pkts, pkt_queue, q_tmp) {
            STAILQ_REMOVE(&dhcp_pkts, qpkt, dhcp_pkt_out, pkt_queue);
            free(qpkt->buf);
            free(qpkt);
        }

        state = DHCP_STATE_REBINDING;
        srv_addr.sin_addr.s_addr = INADDR_BROADCAST;
        net_dhcp_renew();
    }
    else if(renew_time <= now && state == DHCP_STATE_BOUND) {
        state = DHCP_STATE_RENEWING;
        net_dhcp_renew();
    }

    /* Check if we have any packets waiting to come in. */
    while((len = recvfrom(dhcp_sock, buf, 1024, 0,
                          (struct sockaddr *)&addr, &addr_len)) != -1) {
        /* Ignore any boot request packets -- they shouldn't be sent to
           the port we're monitoring anyway. */
        if(pkt->op != DHCP_OP_BOOTREPLY) {
            continue;
        }

        /* Check the magic cookie to make sure we've actually got a DHCP
           packet coming in. */
        if(pkt->options[0] != 0x63 || pkt->options[1] != 0x82 ||
                pkt->options[2] != 0x53 || pkt->options[3] != 0x63) {
            continue;
        }

        found = 0;

        /* Check the xid field of the new packet versus what we're still
           waiting on responses for. */
        STAILQ_FOREACH(qpkt, &dhcp_pkts, pkt_queue) {
            pkt2 = (dhcp_pkt_t *)qpkt->buf;

            if(pkt2->xid == pkt->xid) {
                found = 1;
                break;
            }
        }

        /* If we've found a pending request, act on the message received. */
        if(found) {
            switch(net_dhcp_get_message_type(pkt2, qpkt->size)) {
                case DHCP_MSG_DHCPDISCOVER:

                    if(net_dhcp_get_message_type(pkt, len) !=
                            DHCP_MSG_DHCPOFFER) {
                        break;
                    }

                    /* Send our DHCPREQUEST packet */
                    net_dhcp_send_request(pkt, len, pkt2, qpkt->size);

                    /* Remove the old packet from our queue */
                    STAILQ_REMOVE(&dhcp_pkts, qpkt, dhcp_pkt_out,
                                  pkt_queue);
                    free(qpkt->buf);
                    free(qpkt);
                    break;

                case DHCP_MSG_DHCPREQUEST:
                    found = net_dhcp_get_message_type(pkt, len);

                    if(found == DHCP_MSG_DHCPACK) {
                        srv_addr.sin_addr.s_addr = addr.sin_addr.s_addr;

                        /* Bind to the specified IP address */
                        net_dhcp_bind(pkt, len);
                        genwait_wake_all(&dhcp_sock);
                    }
                    else if(found == DHCP_MSG_DHCPNAK) {
                        /* We got a NAK, try to discover again. */
                        state = DHCP_STATE_INIT;
                        net_dhcp_request(0);
                    }

                    /* Remove the old packet from our queue */
                    STAILQ_REMOVE(&dhcp_pkts, qpkt, dhcp_pkt_out,
                                  pkt_queue);
                    free(qpkt->buf);
                    free(qpkt);
                    break;

                    /* Currently, these are the only two DHCP packets the code
                       here sends out, so other packet types are omitted for
                       the time being. */
            }
        }
    }

    /* Send any packets that need to be sent. */
    STAILQ_FOREACH(qpkt, &dhcp_pkts, pkt_queue) {
        if(qpkt->next_send <= now) {
            sendto(dhcp_sock, qpkt->buf, qpkt->size, 0,
                   (struct sockaddr *)&srv_addr, sizeof(srv_addr));
            qpkt->next_send = now + qpkt->next_delay;
            qpkt->next_delay <<= 1;
        }
    }
}

int net_dhcp_init(void) {
    struct sockaddr_in addr;

    /* Create the DHCP socket */
    dhcp_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(dhcp_sock == -1) {
        return -1;
    }

    /* Bind the socket to the DHCP "Client" port */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DHCP_CLIENT_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(dhcp_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        return -2;
    }

    /* Set up the server address */
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(DHCP_SERVER_PORT);
    srv_addr.sin_addr.s_addr = INADDR_BROADCAST;

    /* Make the socket non-blocking */
    fs_fcntl(dhcp_sock, F_SETFL, O_NONBLOCK);

    /* Create the callback for processing DHCP packets */
    dhcp_cbid = net_thd_add_callback(&net_dhcp_thd, NULL, 50);

    return 0;
}

void net_dhcp_shutdown(void) {
    if(dhcp_cbid != -1) {
        net_thd_del_callback(dhcp_cbid);
        dhcp_cbid = -1;
    }

    if(dhcp_sock != -1) {
        close(dhcp_sock);
        dhcp_sock = -1;
    }
}
