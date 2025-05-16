/* KallistiOS ##version##

   echo.c
   Copyright (C) 2012 Lawrence Sebald

   This example is a very simple UDP/IPv6 server that simply echos whatever is
   sent to it to the screen. Nothing really fancy here at all, but it does
   provide a nice test program for making sure that sockets work right.

   This example also shows how to display things on the framebuffer with the
   "fb" device for dbgio.

   To interact with this, you'll probably want to use the "nc" or "netcat"
   program on your PC. On Mac OS X, I did something like this to test it out:
       nc -6 -u [Dreamcast's IPv6 address] 1337
   Then, whatever you type should echo over on the screen hooked up to the DC.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <arch/arch.h>
#include <kos/net.h>
#include <kos/thread.h>
#include <kos/dbgio.h>
#include <dc/maple/controller.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

#define DATA_SIZE 1280
#define ECHO_PORT 1337

#define ERR_EXIT() { thd_sleep(2000); exit(EXIT_FAILURE); }

int main(int argc, char *argv[]) {
    /* The address to ping... */
    uint8_t data[DATA_SIZE];
    char str1[INET6_ADDRSTRLEN];
    int i, sock;
    struct sockaddr_in6 addr;
    ssize_t sz;
    socklen_t alen;

    /* Set a callback to exit when start is pressed */
    cont_btn_callback(0, CONT_START, (cont_btn_callback_t)arch_exit);

    dbgio_dev_select("fb");

    /* Send out a router solicitation so that we get a global prefix. */
    printf("Attempting to solicit a router\n");
    net_icmp6_send_rsol(net_default_dev);
    thd_sleep(10000);

    if(!net_default_dev->ip6_addr_count) {
        printf("Couldn't get a global prefix!\n");
    }
    else {
        inet_ntop(AF_INET6, &net_default_dev->ip6_addrs[0], str1,
                  INET6_ADDRSTRLEN);
        printf("Global address: %s\n", str1);
    }

    inet_ntop(AF_INET6, &net_default_dev->ip6_lladdr, str1, INET6_ADDRSTRLEN);
    printf("Link-local address: %s\n", str1);

    printf("IPv4 address: %d.%d.%d.%d\n", net_default_dev->ip_addr[0],
           net_default_dev->ip_addr[1], net_default_dev->ip_addr[2],
           net_default_dev->ip_addr[3]);

    sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(sock < 0) {
        perror("socket");
        ERR_EXIT();
    }

    memset(&addr, 0, sizeof(struct sockaddr_in6));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(ECHO_PORT);

    if(bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in6))) {
        perror("bind");
        ERR_EXIT();
    }

    printf("Bound to IPv6 port %d\n", ECHO_PORT);

    for(;;) {
        alen = sizeof(struct sockaddr_in6);

        if((sz = recvfrom(sock, data, DATA_SIZE, 0, (struct sockaddr *)&addr,
                          &alen)) < 0) {
            perror("recvfrom");
            ERR_EXIT();
        }

        inet_ntop(AF_INET6, &addr.sin6_addr, str1, INET6_ADDRSTRLEN);
        printf("Data from %s:\n", str1);

        for(i = 0; i < sz; ++i) {
            printf("%c", data[i]);
        }

        printf("\n\n");

        /* If they send the magic word, exit */
        if(!memcmp(data, "QUIT", 4)) {
            break;
        }
    }

    close(sock);

    return 0;
}
