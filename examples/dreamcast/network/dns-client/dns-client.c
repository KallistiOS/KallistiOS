/* KallistiOS ##version##

   dns-client.c
   Copyright (C) 2014 Lawrence Sebald

   This example demonstrates how to use getaddrinfo() to look up the network
   address for a given hostname.

   This example also shows how to display things on the framebuffer with the
   "fb" device for dbgio.

*/

#include <stdio.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <kos/net.h>
#include <kos/dbgio.h>
#include <kos/thread.h>
#include <arch/arch.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

static void print_addrinfo(struct addrinfo *ai) {
    struct addrinfo *p;
    char str[INET6_ADDRSTRLEN];
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;
    void *addr;
    int i;

    /* Go through each result in the chain and print out the address that it
       contains. */
    for(p = ai, i = 0; p; p = p->ai_next, ++i) {
        if(p->ai_family == AF_INET) {
            addr4 = (struct sockaddr_in *)p->ai_addr;
            addr = &addr4->sin_addr;
        }
        else if(p->ai_family == AF_INET6) {
            addr6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &addr6->sin6_addr;
        }
        else {
            /* Shouldn't ever end up here... */
            continue;
        }

        /* Convert the raw address to a string. */
        inet_ntop(p->ai_family, addr, str, INET6_ADDRSTRLEN);
        printf("%d: %s\n", i, str);
    }
}


int main(int argc, char *argv[]) {
    struct addrinfo *ai;
    struct addrinfo hints;
    int err;

    /* Set the framebuffer as the output device for dbgio. */
    dbgio_dev_select("fb");

    /* Check if the default network device has a DNS server set, and set one if
       it does not for some reason. We'll set up 8.8.4.4 as the DNS server,
       which is one of Google's Public DNS servers. */
    if(!net_default_dev->dns[0]) {
        net_default_dev->dns[0] = 8;
        net_default_dev->dns[1] = 8;
        net_default_dev->dns[2] = 4;
        net_default_dev->dns[3] = 4;
    }

    /* First request just the IPv4 address of something. */
    printf("Looking up IPv4 address of sylverant.net\n");
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    if((err = getaddrinfo("sylverant.net", NULL, &hints, &ai))) {
        printf("Error looking up IPv4 address information: %d\n", err);
    }

    /* Print out each result, then clean it up. */
    print_addrinfo(ai);
    freeaddrinfo(ai);

    /* Now, request the IPv6 address of the same thing. Note, we could (and
       really should) have done both at the same time. But, this is an example,
       so I figured I should show both ways. */
    printf("Looking up IPv6 address of sylverant.net\n");
    hints.ai_family = AF_INET6;

    if((err = getaddrinfo("sylverant.net", NULL, &hints, &ai))) {
        printf("Error looking up IPv6 address information: %d\n", err);
    }

    /* Print out each result, then clean it up. */
    print_addrinfo(ai);
    freeaddrinfo(ai);

    /* If you want to look up both IPv4 and IPv6 at the same time, either set
       hints.ai_family to AF_UNSPEC, or (if you aren't using any of the other
       parts of the hints structure) pass NULL instead of a pointer to the hints
       structure. */

    /* Wait 10 seconds for the user to see what's on the screen before we clear
       it during the exit back to the loader */
    thd_sleep(10 * 1000);

    return 0;
}
