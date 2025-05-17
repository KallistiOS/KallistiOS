/* KallistiOS ##version##

   ping6.c
   Copyright (C) 2010 Lawrence Sebald

   This example is a very basic "ping6" program, much like you might find on
   any computer. However, this version lacks many of the niceties you might find
   on a real OS' ping program.

   This example also shows how to display things on the framebuffer with the
   "fb" device for dbgio.

*/

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <arch/arch.h>
#include <kos/net.h>
#include <kos/thread.h>
#include <kos/dbgio.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

#define DATA_SIZE 56

int main(int argc, char *argv[]) {
    /* The address to ping... */
    struct in6_addr addr;
    uint8_t data[DATA_SIZE];
    char str1[INET6_ADDRSTRLEN], str2[INET6_ADDRSTRLEN];
    int i;

    dbgio_dev_select("fb");

    /* Set the address to ping. This corresponds to sylverant.net */
    inet_pton(AF_INET6, "2001:470:8:68d::1", &addr);

    /* Fill in the data for the ping packet... this is pretty simple and doesn't
       really have any real meaning... */
    for(i = 0; i < DATA_SIZE; ++i) {
        data[i] = (uint8_t)i;
    }

    /* Send out a router solicitation so that we get a global prefix. */
    printf("Attempting to solicit a router\n");
    net_icmp6_send_rsol(net_default_dev);
    thd_sleep(10000);

    if(!net_default_dev->ip6_addr_count) {
        printf("Couldn't get a global prefix!\n");
        thd_sleep(2000);
        return 0;
    }

    inet_ntop(AF_INET6, &net_default_dev->ip6_addrs[0], str1, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, &addr, str2, INET6_ADDRSTRLEN);

    printf("PING6 %s --> %s\n", str1, str2);

    /* Send out 10 pings, waiting 250ms between attempts. */
    for(i = 0; i < 10; ++i) {
        net_icmp6_send_echo(net_default_dev, &addr, 0, i, data, DATA_SIZE);
        thd_sleep(250);
    }

    /* Wait 2 seconds for the user to see what's on the screen before we clear
       it during the exit back to the loader */
    thd_sleep(10 * 1000);

    return 0;
}
