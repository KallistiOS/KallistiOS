/* KallistiOS ##version##

   basic.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2009 Lawrence Sebald
*/

#include <kos.h>

/*

This is a simple network test example. All it does is bring up the
networking system and wait for a bit to let you have a chance to
ping it, etc. Note that this program is totally portable as long
as your KOS platform has networking capabilities, but I'm leaving
it in "dreamcast" for now.

Now with statistics printing!

*/

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

int main(int argc, char **argv) {
    net_ipv4_stats_t ip;
    net_udp_stats_t udp;

    /* Wait for a bit so the user can ping, etc */
    thd_sleep(10 * 1000);

    /* Print out some statistics about the connection. */
    ip = net_ipv4_get_stats();
    udp = net_udp_get_stats();

    printf("IPv4 Stats:\n"
           "Packets sent successfully:       %6ld\n"
           "Packets that failed to send:     %6ld\n"
           "Packets received successfully:   %6ld\n"
           "Packets rejected (bad size):     %6ld\n"
           "                 (bad checksum): %6ld\n"
           "                 (bad protocol): %6ld\n\n",
           ip.pkt_sent, ip.pkt_send_failed, ip.pkt_recv, 
           ip.pkt_recv_bad_size, ip.pkt_recv_bad_chksum, 
           ip.pkt_recv_bad_proto);

    printf("UDP Stats:\n"
           "Packets sent successfully:       %6ld\n"
           "Packets that failed to send:     %6ld\n"
           "Packets received successfully:   %6ld\n"
           "Packets rejected (bad size):     %6ld\n"
           "                 (bad checksum): %6ld\n"
           "                 (no socket):    %6ld\n\n",
           udp.pkt_sent, udp.pkt_send_failed, udp.pkt_recv,
           udp.pkt_recv_bad_size, udp.pkt_recv_bad_chksum,
           udp.pkt_recv_no_sock);

    return 0;
}


