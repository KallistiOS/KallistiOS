/* KallistiOS ##version##
 *
 *   ntp.c
 *   Copyright (C) 2023 Eric Fradella
 *
 *   This example demonstrates how to set the Dreamcast's
 *   real-time clock to current UTC time via an NTP server.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>

#include <arch/arch.h>
#include <arch/rtc.h>
#include <kos/dbgio.h>
#include <kos/net.h>
#include <kos/thread.h>

#define NTP_PORT    "123"
#define NTP_SERVER  "us.pool.ntp.org"
#define NTP_DELTA   2208988800ULL

#define ERR_EXIT() { thd_sleep(2000); exit(EXIT_FAILURE); }

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

/* Structure for 48-byte NTP packet */
typedef struct ntp_packet {
    uint8_t leap_ver_mode;    /* First 2 bits are leap indicator, next
                                 three bits NTP version, last 3 bits mode */
    uint8_t stratum;
    uint8_t poll_interval;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_time_s;
    uint32_t ref_time_f;
    uint32_t orig_time_s;
    uint32_t orig_time_f;
    uint32_t rcv_time_s;
    uint32_t rcv_time_f;
    uint32_t trns_time_s;
    uint32_t trns_time_f;
} ntp_packet_t;

int main(int argc, char **argv) {
    ntp_packet_t packet;
    int sockfd;
    struct addrinfo *ai;
    time_t ntp_time, dc_time;

    /* Set the framebuffer as the output device for dbgio. */
    dbgio_dev_select("fb");

    /* Create NTP packet and clear it */
    memset(&packet, 0, sizeof(ntp_packet_t));

    /* Leave leap indicator blank, set version number to 4,and
       set client mode to 3. 0x23 = 00 100 011 = 0, 4, 3 */
    packet.leap_ver_mode = 0x23;

    /* Create a new UDP socket */
    if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        printf("Error opening socket!\n");
        ERR_EXIT();
    }

    /* Retrieve IP address for our specified hostname */
    if(getaddrinfo(NTP_SERVER, NTP_PORT, NULL, &ai)) {
        printf("Error resolving host!\n");
        ERR_EXIT();
    }

    if(connect(sockfd, ai->ai_addr, ai->ai_addrlen) < 0) {
        printf("Error connecting to server!\n");
        ERR_EXIT();
    }

    freeaddrinfo(ai);

    /* Send the NTP packet we constructed */
    if(write(sockfd, &packet, sizeof(ntp_packet_t)) < 0) {
        printf("Error writing to socket!\n");
        ERR_EXIT();
    }

    /* Receive the packet back from the server,
       now filled out with the current time */
    if(read(sockfd, &packet, sizeof(ntp_packet_t)) < 0) {
        printf("Error reading response from socket!\n");
        ERR_EXIT();
    }

    /* Grab time from the structure, and subtract 70 years to convert
       from NTP's 1900 epoch to Unix time's 1970 epoch */
    ntp_time = (ntohl(packet.trns_time_s) - NTP_DELTA);
    printf("The current NTP time is...\n %s\n", ctime(&ntp_time));

    /* Print the current system time */
    dc_time = rtc_unix_secs();
    printf("Dreamcast system time is...\n %s\n", ctime(&dc_time));

    /* Set the system time to the NTP time and read it back */
    printf("Setting Dreamcast clock's time to NTP time...\n\n");
    rtc_set_unix_secs(ntp_time);
    dc_time = rtc_unix_secs();
    printf("Dreamcast system time is now...\n %s\n", ctime(&dc_time));

    /* Wait 10 seconds for the user to see what's on the screen before we clear
       it during the exit back to the loader */
    thd_sleep(10 * 1000);

    return 0;
}

