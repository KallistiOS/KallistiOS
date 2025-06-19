/* KallistiOS ##version##

   sd-speedtest.c
   Copyright (C) 2023, 2025 Ruslan Rostovtsev

   This example program performs speed tests for reading sectors from the first
   partition of an SD device using both SCI-SPI and SCIF-SPI interfaces with
   CRC checking enabled and disabled, and then shows the timing information.
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <dc/sd.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include <arch/arch.h>
#include <arch/timer.h>

#include <kos/init.h>
#include <kos/dbgio.h>
#include <kos/dbglog.h>
#include <kos/blockdev.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

#define TEST_BLOCK_COUNT 1024

static uint8_t tbuf[TEST_BLOCK_COUNT * 512] __attribute__((aligned(32)));

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

static int run_speed_test(sd_interface_t interface, bool check_crc) {
    sd_init_params_t params = {
        .interface = interface,
        .check_crc = check_crc
    };
    kos_blockdev_t sd_dev;
    uint64_t begin, end, timer, average;
    uint64_t sum = 0;
    uint8_t pt;
    int i;

    const char *interface_name = (interface == SD_IF_SCI) ? "SCI-SPI" : "SCIF-SPI";

    while(sd_init_ex(&params)) {
        dbglog(DBG_DEBUG, "Could not initialize the SD card on %s interface.\n", interface_name);
        return -1;
    }

    /* Grab the block device for the first partition on the SD card. Note that
       you must have the SD card formatted with an MBR partitioning scheme. */
    if(sd_blockdev_for_partition(0, &sd_dev, &pt)) {
        dbglog(DBG_DEBUG, "Could not find the first partition on the SD card!\n");
        sd_shutdown();
        return -1;
    }

    for(i = 0; i < 10; i++) {
        begin = timer_ms_gettime64();

        if(sd_dev.read_blocks(&sd_dev, 0, TEST_BLOCK_COUNT, tbuf)) {
            dbglog(DBG_DEBUG, "couldn't read block: %s\n", strerror(errno));
            sd_shutdown();
            return -1;
        }

        end = timer_ms_gettime64();
        timer = end - begin;
        sum += timer;
    }

    average = sum / 10;

    dbglog(DBG_DEBUG, "%s: read average took %llu ms (%.3f KB/sec)\n",
           interface_name, average, (512 * TEST_BLOCK_COUNT) / ((double)average));

    sd_shutdown();
    return 0;
}

int main(int argc, char *argv[]) {
    // dbgio_dev_select("fb");

    dbglog(DBG_DEBUG, "Starting SD card speed tests\n");

    dbglog(DBG_DEBUG, "Testing SCI-SPI interface with CRC disabled\n");
    if (run_speed_test(SD_IF_SCI, false) == 0) {
        dbglog(DBG_DEBUG, "Testing SCI-SPI interface with CRC enabled\n");
        run_speed_test(SD_IF_SCI, true);
    }
    else {
        dbglog(DBG_DEBUG, "Skipping SCI-SPI interface with CRC enabled\n");
    }

    dbglog(DBG_DEBUG, "Testing SCIF-SPI interface with CRC disabled\n");
    if (run_speed_test(SD_IF_SCIF, false) == 0) {
        dbglog(DBG_DEBUG, "Testing SCIF-SPI interface with CRC enabled\n");
        run_speed_test(SD_IF_SCIF, true);
    }
    else {
        dbglog(DBG_DEBUG, "Skipping SCIF-SPI interface with CRC enabled\n");
    }

    dbglog(DBG_DEBUG, "All tests completed\n");

    wait_exit();
    return 0;
}
