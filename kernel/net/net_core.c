/* KallistiOS ##version##

   kernel/net/net_core.c

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2005, 2013 Lawrence Sebald
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <kos/net.h>
#include <kos/fs_socket.h>
#include <kos/dbglog.h>

#include "net_dhcp.h"
#include "net_thd.h"
#include "net_ipv4.h"
#include "net_ipv6.h"

/*

This module, and others in this tree, handle the architecture-independent
part of the networking system of KOS. They serve the following purposes:

- Specific network card drivers may register themselves using the functions
  here, if their hardware is present
- Link-level messages will be handled here, such as ARP
- The whole networking system can be enabled or disabled from here

*/

/**************************************************************************/
/* Variables */

/* Active network devices list */
struct netif_list net_if_list = LIST_HEAD_INITIALIZER(0);

/* Are we already initialized? */
static int net_initted = 0;

/* Default net device */
netif_t *net_default_dev = NULL;

/**************************************************************************/
/* Driver list management
   Note that this stuff might be used before net_core is actually
   itself initialized. This lets us let the drivers build up the available
   driver list, and then we'll come back later and look through them all. */

/* Register a network device */
int net_reg_device(netif_t *device) {
    /* Make sure it's not already registered */
    if(device->flags & NETIF_REGISTERED) {
        dbglog(DBG_WARNING, "net_reg_device: '%s' is already registered\n",
               device->name);
        return -1;
    }

    /* Insert it into the interface list */
    LIST_INSERT_HEAD(&net_if_list, device, if_list);

    /* Mark it as registered */
    device->flags |= NETIF_REGISTERED;

    /* We need to do more processing in here eventually like looking for
       duplicate device IDs and assigning new indices, but that can
       wait until we're actually supporting a box with the possibility
       of more than one network interface... which probably won't happen
       any time soon... */

    /* Success */
    return 0;
}

/* Unregister a network device */
int net_unreg_device(netif_t *device) {
    /* Make sure it really was registered */
    if(!(device->flags & NETIF_REGISTERED)) {
        dbglog(DBG_WARNING, "net_unreg_device: '%s' isn't registered\n",
               device->name);
        return -1;
    }

    /* Remove it from the list */
    LIST_REMOVE(device, if_list);

    /* Mark it as unregistered */
    device->flags &= ~NETIF_REGISTERED;

    /* Success */
    return 0;
}

struct netif_list * net_get_if_list(void) {
    return &net_if_list;
}

/*****************************************************************************/
/* Init/shutdown */

/* Set default */
netif_t *net_set_default(netif_t *n) {
    netif_t *olddev = net_default_dev;

    net_default_dev = n;

    return olddev;
}

/* Device detect / init */
int net_dev_init(void) {
    int detected = 0;
    netif_t *cur;

    LIST_FOREACH(cur, &net_if_list, if_list) {
        /* Make sure we have one */
        if(cur->if_detect(cur) < 0)
            continue;

        /* Ok, we do -- initialize it */
        if(cur->if_init(cur) < 0)
            continue;

        /* It's initialized, so now enable it */
        if(cur->if_start(cur) < 0) {
            cur->if_shutdown(cur);
            continue;
        }

        /* Set the first detected device to be the default */
        if(net_default_dev == NULL)
            net_set_default(cur);

        detected++;
    }

    dbglog(DBG_DEBUG, "net_dev_init: detected %d usable network device(s)\n", detected);

    if(detected)
        return 0;
    else
        return -1;
}

/* Init */
int net_init(uint32 ip) {
    int rv = 0;

    /* Make sure we haven't already done this */
    if(net_initted)
        return 0;

    /* Detect and potentially initialize devices */
    if(net_dev_init() < 0)
        return -1;

    /* Initialize the network thread. */
    net_thd_init();

    /* Initialize the ARP cache */
    net_arp_init();

    /* Initialize the NDP cache */
    net_ndp_init();

    /* Initialize IPv4 fragmentation support */
    net_ipv4_frag_init();

    /* Initialize multicast support */
    net_multicast_init();

    /* Initialize IPv6 support */
    net_ipv6_init();

    /* Initialize the sockets-like interface */
    fs_socket_init();

    /* Initialize the UDP/TCP systems */
    net_udp_init();
    net_tcp_init();

    /* Initialize the DHCP system */
    net_dhcp_init();

    if(net_default_dev) {
        /* Did we get a requested IP address? this normally happens over dcload-ip. */
        if(ip) {
            rv = net_dhcp_request(ip);
            if(rv < 0) {
                dbglog(DBG_DEBUG, "Failed to acquire the specified IP with DHCP\n");

                /* If that fails, set the address manually. Although gateway
                   and dns etc. will also manually need setting */
                net_ipv4_parse_address(ip, net_default_dev->ip_addr);
                rv = 0;
            }
        }

        /* We didn't get a requested IP address, if we don't already have one
           set, then do so via DHCP. */
        else if(!net_default_dev->ip_addr[0])
            rv = net_dhcp_request(0);
    }

    net_initted = 1;

    return rv;
}

/* Shutdown */
void net_shutdown(void) {
    netif_t *cur;

    /* Make sure we actually initialized the net stuff to begin with */
    if(!net_initted)
        return;

    /* Stop the network thread, since otherwise we might have problems down the
       road here... This simplifies things greatly in shutting everything else
       down in here. */
    net_thd_kill();

    /* Shut down DHCP */
    net_dhcp_shutdown();

    /* Shut down the UDP/TCP systems */
    net_tcp_shutdown();
    net_udp_shutdown();

    /* Shut down the sockets-like interface */
    fs_socket_shutdown();

    /* Shut down IPv6 support */
    net_ipv6_shutdown();

    /* Shut down multicast support */
    net_multicast_shutdown();

    /* Shut down IPv4 fragmentation support */
    net_ipv4_frag_shutdown();

    /* Shut down the NDP cache */
    net_ndp_shutdown();

    /* Shut down the ARP cache */
    net_arp_shutdown();

    /* Shut down the network thread */
    net_thd_shutdown();

    /* Shut down all activated network devices */
    LIST_FOREACH(cur, &net_if_list, if_list) {
        if(cur->flags & NETIF_RUNNING && cur->if_stop)
            cur->if_stop(cur);

        if(cur->flags & NETIF_INITIALIZED && cur->if_shutdown)
            cur->if_shutdown(cur);

        cur->flags &= ~NETIF_REGISTERED;
    }

    /* Blank out the list */
    LIST_INIT(&net_if_list);

    net_initted = 0;
}
