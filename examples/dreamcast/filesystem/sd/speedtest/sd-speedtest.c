/* KallistiOS ##version##

   speedtest.c
   Copyright (C) 2023 Ruslan Rostovtsev (SWAT)

   This example program simply attempts to read some sectors from the first
   partition of an SD device attached to SCIF and then show the timing information.
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <dc/sd.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include <arch/arch.h>
#include <arch/timer.h>

#include <kos/init.h>
#include <kos/dbgio.h>
#include <kos/blockdev.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

static uint8_t tbuf[1024 * 512] __attribute__((aligned(32)));

static void __attribute__((__noreturn__)) wait_exit(void) {
    maple_device_t *dev;
    cont_state_t *state;

    printf("Press any button to exit.\n");

    for(;;) {
        dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(dev) {
            state = (cont_state_t *)maple_dev_status(dev);

            if(state)   {
                if(state->buttons)
                    arch_exit();
            }
        }
    }
}

int main(int argc, char *argv[]) {
    kos_blockdev_t sd_dev;
    uint64_t begin, end, timer, average;
    uint64_t sum = 0;
    uint8_t pt;
    int i;

    dbgio_dev_select("fb");
    dbglog(DBG_DEBUG, "Initializing SD card.\n");

    if(sd_init()) {
        dbglog(DBG_DEBUG, "Could not initialize the SD card. Please make sure that you "
               "have an SD card adapter plugged in and an SD card inserted.\n");
        wait_exit();
    }

    /* Grab the block device for the first partition on the SD card. Note that
       you must have the SD card formatted with an MBR partitioning scheme. */
    if(sd_blockdev_for_partition(0, &sd_dev, &pt)) {
        dbglog(DBG_DEBUG, "Could not find the first partition on the SD card!\n");
        wait_exit();
    }

    dbglog(DBG_DEBUG, "Calculating average speed for reading 1024 blocks.\n");

    for(i = 0; i < 10; i++) {
        begin = timer_ms_gettime64();

        if(sd_dev.read_blocks(&sd_dev, 0, 1024, tbuf)) {
            dbglog(DBG_DEBUG, "couldn't read block: %s\n", strerror(errno));
            return -1;
        }

        end = timer_ms_gettime64();
        timer = end - begin;
        sum += timer;
    }

    average = sum / 10;

    dbglog(DBG_DEBUG, "SD card read average took %llu ms (%.3f KB/sec)\n",
           average, (512 * 1024) / ((double)average));

    sd_shutdown();
    wait_exit();
    return 0;
}
