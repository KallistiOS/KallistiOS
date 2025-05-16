/* KallistiOS ##version##

   atatest.c
   Copyright (C) 2014 Lawrence Sebald

   This example program simply attempts to read some sectors from the first
   partition of an ATA device attached to G1, both over PIO and DMA and then
   compares the timing information from both. PIO reads seem to run at about
   3.5 MB/sec, whereas DMA gets around 12.5 MB/sec (quite the improvement).
*/

#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <dc/g1ata.h>

#include <arch/timer.h>
#include <arch/types.h>

#include <kos/dbglog.h>
#include <kos/blockdev.h>

static unsigned char dmabuf[1024 * 512] __attribute__((aligned(32)));
static unsigned char piobuf[1024 * 512] __attribute__((aligned(32)));
static unsigned char tmp[512] __attribute__((aligned(32)));

int main(int argc, char *argv[]) {
    kos_blockdev_t bd_pio, bd_dma;
    uint64_t spio, epio, sdma, edma, timer;
    uint8_t pt;

    dbglog(DBG_DEBUG, "Starting G1 ATA test program...\n");
    g1_ata_init();

    /* Grab the blockdevs that we'll use to access the partitions. */
    dbglog(DBG_DEBUG, "Looking for first partition...\n");
    if(g1_ata_blockdev_for_partition(0, 0, &bd_pio, &pt)) {
        dbglog(DBG_DEBUG, "Couldn't get PIO blockdev for partition!\n");
        return -1;
    }

    if(g1_ata_blockdev_for_partition(0, 1, &bd_dma, &pt)) {
        dbglog(DBG_DEBUG, "Couldn't get DMA blockdev for partition!\n");
        return -1;
    }

    /* For some reason, the first DMA read takes a while... So, read one sector
       and discard it so to not mess up the timing stuff below. */
    if(bd_dma.read_blocks(&bd_dma, 1024, 1, tmp)) {
        dbglog(DBG_DEBUG, "Couldn't read block 1024 by dma: %s\n",
               strerror(errno));
        return -1;
    }

    /* Read blocks 0 - 1023 by DMA and print out timing information. */
    dbglog(DBG_DEBUG, "Reading 1024 blocks by DMA!\n");

    sdma = timer_ms_gettime64();
    if(bd_dma.read_blocks(&bd_dma, 0, 1024, dmabuf)) {
        dbglog(DBG_DEBUG, "couldn't read block by DMA: %s\n", strerror(errno));
        return -1;
    }
    edma = timer_ms_gettime64();
    timer = edma - sdma;

    dbglog(DBG_DEBUG, "DMA read took %llu ms (%f MB/sec)\n", timer,
           (512 * 1024) / ((double)timer) / 1000.0);

    /* Read blocks 0 - 1023 by PIO and print out timing information. */
    dbglog(DBG_DEBUG, "Reading 1024 blocks by PIO!\n");

    spio = timer_ms_gettime64();
    if(bd_pio.read_blocks(&bd_pio, 0, 1024, piobuf)) {
        dbglog(DBG_DEBUG, "couldn't read block by PIO: %s\n", strerror(errno));
        return -1;
    }
    epio = timer_ms_gettime64();
    timer = epio - spio;

    dbglog(DBG_DEBUG, "PIO read took %llu ms (%f MB/sec)\n", timer,
           (512 * 1024) / ((double)timer) / 1000.0);

    /* Check the buffers for consistency... */
    if(memcmp(piobuf, dmabuf, 1024 * 512)) {
        dbglog(DBG_DEBUG, "Buffers do not match?!\n");
    }
    else {
        dbglog(DBG_DEBUG, "Both buffers matched!\n");
    }

    /* Clean up... */
    g1_ata_shutdown();

    return 0;
}
