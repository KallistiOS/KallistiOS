/* KallistiOS ##version##

   cd-stream-test.c
   Copyright (C) 2025 Ruslan Rostovtsev

   This example program simply shows how CD streams works.
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/cdrom.h>

#include <arch/arch.h>
#include <arch/cache.h>

#include <kos/init.h>
#include <kos/dbgio.h>
#include <kos/dbglog.h>

#define BUFFER_SIZE (8 << 11)

static uint8_t dma_buf[BUFFER_SIZE] __attribute__((aligned(32)));
static uint8_t pio_buf[BUFFER_SIZE] __attribute__((aligned(2)));

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

static void cd_stream_callback(void *param) {
    (*(size_t *)param)++;
}

static int cd_stream_test(uint32_t lba, uint8_t *buffer, size_t size, int mode) {

    int rs;
    size_t cur_size = 0;
    size_t cb_count = 0;
    char *stream_name = (mode == CDROM_READ_PIO ? "PIO" : "DMA");

    dbglog(DBG_DEBUG, "Start %s stream.\n", stream_name);
    rs = cdrom_stream_start(lba, size / 2048, mode);

    if (rs != ERR_OK) {
        dbglog(DBG_ERROR, "Failed to start stream for %s.\n", stream_name);
        return -1;
    }

    cdrom_stream_set_callback(cd_stream_callback, (void *)&cb_count);
    rs = cdrom_stream_request(buffer, size / 2, 1);

    if (rs != ERR_OK) {
        dbglog(DBG_ERROR, "Failed to request %s transfer.\n", stream_name);
        return -1;
    }

    rs = cdrom_stream_progress(&cur_size);

    if (rs != 0 || cur_size != (size / 2)) {
        dbglog(DBG_ERROR, "Failed to check %s transfer: rs=%d sz=%d\n",
            stream_name, rs, cur_size);
        return -1;
    }

    rs = cdrom_stream_request(buffer + (size / 2), size / 2, 1);

    if (rs != ERR_OK) {
        dbglog(DBG_ERROR, "Failed to request %s transfer.\n", stream_name);
        return -1;
    }

    rs = cdrom_stream_progress(&cur_size);

    if (rs != 0 || cur_size != 0) {
        dbglog(DBG_ERROR, "Failed to check %s transfer: rs=%d sz=%d\n",
            stream_name, rs, cur_size);
        return -1;
    }

    rs = cdrom_stream_stop(false);

    if (rs != ERR_OK) {
        dbglog(DBG_ERROR, "Failed to stop %s stream.\n", stream_name);
        return -1;
    }

    if (cb_count != 2) {
        dbglog(DBG_ERROR, "%s transfer is done, but callback fails: %d\n",
            stream_name, cb_count);
        return -1;
    }

    dbglog(DBG_DEBUG, "%s transfer is done.\n", stream_name);
    return 0;
}

size_t print_diff(uint8_t *pio_buf, uint8_t *dma_buf, size_t size) {
    size_t i, j, rv = 0;

    for(i = 0; i < size; ++i) {
        if (dma_buf[i] != pio_buf[i]) {
            rv = i;
            if (i >= 8) {
                i -= 8;
            }
            break;
        }
    }
    dbglog(DBG_INFO, "DMA[%d]: ", i);

    for(j = 0; j < 16; ++j) {
        dbglog(DBG_INFO, "%02x", dma_buf[i + j]);
    }
    dbglog(DBG_INFO, "\nPIO[%d]: ", i);

    for(j = 0; j < 16; ++j) {
        dbglog(DBG_INFO, "%02x", pio_buf[i + j]);
    }
    dbglog(DBG_INFO, "\n\n");
    return rv;
}

int main(int argc, char *argv[]) {
    int rs;
    size_t i;
    uint32_t lba;
    CDROM_TOC toc;

    dbgio_dev_select("fb");
    dbglog(DBG_INFO, "CD-ROM stream test.\n\n");

    rs = cdrom_read_toc(&toc, 0);

    if (rs != ERR_OK) {
        dbglog(DBG_ERROR, "No disc present.\n");
        goto exit;
    }
    lba = cdrom_locate_data_track(&toc);

    if (lba == 0) {
        dbglog(DBG_ERROR, "No data track on disc.\n");
        goto exit;
    }

    memset(dma_buf, 0xff, BUFFER_SIZE);
    /* Inside the cdrom driver the cache will be invalidated,
       but we need to save what we wrote to it by memset.
       In normal cases you don't need to do this.
    */
    dcache_purge_range((uintptr_t)dma_buf, BUFFER_SIZE);

    rs = cd_stream_test(lba, dma_buf, BUFFER_SIZE, CDROM_READ_DMA);

    if (rs != ERR_OK) {
        dbglog(DBG_ERROR, "DMA stream test failed.\n");
        goto exit;
    }

    memset(pio_buf, 0xee, BUFFER_SIZE);
    rs = cd_stream_test(lba, pio_buf, BUFFER_SIZE, CDROM_READ_PIO);

    if (rs != ERR_OK) {
        dbglog(DBG_ERROR, "PIO stream test failed.\n");
        goto exit;
    }

    if (memcmp(dma_buf, pio_buf, BUFFER_SIZE) == 0) {
        dbglog(DBG_INFO, "Stream data matched.\n");
        goto exit;
    }

    dbglog(DBG_ERROR, "Stream data mismatch:\n");
    i = print_diff(pio_buf, dma_buf, BUFFER_SIZE);

    if (dma_buf[i] == 0xff) {
        dbglog(DBG_INFO, "Read DMA data.\n");
        memset(dma_buf, 0xff, BUFFER_SIZE);
        /* Inside the cdrom driver the cache will be invalidated,
            but we need to save what we wrote to it by memset.
            In normal cases you don't need to do this.
        */
        dcache_purge_range((uintptr_t)dma_buf, BUFFER_SIZE);

        rs = cdrom_read_sectors_ex(dma_buf, lba,
            BUFFER_SIZE >> 11, CDROM_READ_DMA);
    }
    else {
        dbglog(DBG_INFO, "Read PIO data.\n");
        memset(pio_buf, 0xee, BUFFER_SIZE);
        rs = cdrom_read_sectors_ex(pio_buf, lba,
            BUFFER_SIZE >> 11, CDROM_READ_PIO);
    }

    if (rs != ERR_OK) {
        dbglog(DBG_ERROR, "%s read sectors failed.\n",
            dma_buf[i] == 0xff ? "DMA" : "PIO");
    }
    else if (memcmp(dma_buf, pio_buf, BUFFER_SIZE)) {
        dbglog(DBG_ERROR, "Stream and read data mismatch:\n");
        print_diff(pio_buf, dma_buf, BUFFER_SIZE);
    }
    else {
        dbglog(DBG_INFO, "Stream and read data matched.\n");
    }

exit:
    dbglog(DBG_INFO, "\n");
    wait_exit();
    return 0;
}
