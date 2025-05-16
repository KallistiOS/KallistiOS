/* KallistiOS ##version##

   ping.c
   Copyright (C) 2009 Lawrence Sebald

   This example is a very basic "ping" program, much like you might find on
   any computer. However, this version lacks many of the niceties you might find
   on a real OS' ping program.

   This example also shows how to display things on the framebuffer with the
   "fb" device for dbgio.

*/

#include <arch/arch.h>
#include <kos/net.h>
#include <kos/thread.h>
#include <kos/dbgio.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

#define DATA_SIZE 56

int main(int argc, char *argv[]) {
    /* The address to ping... */
    uint8_t addr[4] = { 192, 168, 1, 1 };
    uint8_t data[DATA_SIZE];
    int i;

    dbgio_dev_select("fb");

    /* Fill in the data for the ping packet... this is pretty simple and doesn't
       really have any real meaning... */
    for(i = 0; i < DATA_SIZE; ++i) {
        data[i] = (uint8_t)i;
    }

    /* Send out 10 pings, waiting 250ms between attempts. */
    for(i = 0; i < 10; ++i) {
        net_icmp_send_echo(net_default_dev, addr, 0, i, data, DATA_SIZE);
        thd_sleep(250);
    }

    /* Wait 2 seconds for the user to see what's on the screen before we clear
       it during the exit back to the loader */
    thd_sleep(2 * 1000);

    return 0;
}
