/* KallistiOS ##version##

   kernel/net/net_input.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2005, 2012 Lawrence Sebald
*/

#include <stdint.h>
#include <stdio.h>
#include <kos/net.h>
#include "net_ipv4.h"
#include "net_ipv6.h"

/*

  Main packet input system

*/

static int net_default_input(netif_t *nif, const uint8_t *data, int len) {
    uint16_t proto = (uint16_t)((data[12] << 8) | (data[13]));

    /* If this is bound for a multicast address, make sure we actually care
       about the one that it gets sent to. */
    if((data[0] & 0x01) &&
       (data[0] != 0xFF || data[1] != 0xFF || data[2] != 0xFF ||
        data[3] != 0xFF || data[4] != 0xFF || data[5] != 0xFF) &&
       !net_multicast_check(data)) {
        return 0;
    }

    switch(proto) {
        case 0x0800:
            return net_ipv4_input(nif, data + sizeof(eth_hdr_t),
                                  len - sizeof(eth_hdr_t),
                                  (const eth_hdr_t *)data);

        case 0x0806:
            return net_arp_input(nif, data, len);

        case 0x86DD:
            return net_ipv6_input(nif, data + sizeof(eth_hdr_t),
                                  len - sizeof(eth_hdr_t),
                                  (const eth_hdr_t *)data);

        default:
            return 0;
    }
}

/* Where will input packets be routed? */
net_input_func net_input_target = net_default_input;

/* Process an incoming packet */
int net_input(netif_t *device, const uint8_t *data, int len) {
    if(net_input_target != NULL)
        return net_input_target(device, data, len);
    else
        return 0;
}

/* Setup an input target; returns the old target */
net_input_func net_input_set_target(net_input_func t) {
    net_input_func old = net_input_target;
    net_input_target = t;
    return old;
}
