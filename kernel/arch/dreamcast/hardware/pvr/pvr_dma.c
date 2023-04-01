/* KallistiOS ##version##

   pvr_dma.c
   Copyright (C)2002 Roger Cattermole
   Copyright (C)2004 Megan Potter
   http://www.boob.co.uk
 */

#include <stdio.h>
#include <errno.h>
#include <dc/pvr.h>
#include <dc/asic.h>
#include <kos/thread.h>
#include <kos/sem.h>

#include "pvr_internal.h"

/* Modified for inclusion into KOS by Megan Potter */

/* Signaling semaphore */
static semaphore_t dma_done;
static int dma_blocking;
static pvr_dma_callback_t dma_callback;
static ptr_t dma_cbdata;

/* DMA registers */
static vuint32  * const pvrdma = (vuint32 *)0xa05f6800;
static vuint32  * const shdma  = (vuint32 *)0xffa00000;

/* DMAC registers */
#define DMAC_SAR2   0x20/4
#define DMAC_DMATCR2    0x28/4
#define DMAC_CHCR2  0x2c/4
#define DMAC_DMAOR  0x40/4

/* PVR Dma registers - Offset by 0xA05F6800 */
#define PVR_STATE   0x00
#define PVR_LEN     0x04/4
#define PVR_DST     0x08/4
#define PVR_LMMODE0 0x84/4
#define PVR_LMMODE1 0x88/4

static void pvr_dma_irq_hnd(uint32 code) {
    (void)code;

    if(shdma[DMAC_DMATCR2] != 0)
        dbglog(DBG_INFO, "pvr_dma: The dma did not complete successfully\n");

    // DBG(("pvr_dma_irq_hnd\n"));

    // Call the callback, if any.
    if(dma_callback) {
        // This song and dance is necessary because the handler
        // could chain to itself.
        pvr_dma_callback_t cb = dma_callback;
        ptr_t d = dma_cbdata;

        dma_callback = NULL;
        dma_cbdata = 0;

        cb(d);
    }

    // Signal the calling thread to continue, if any.
    if(dma_blocking) {
        sem_signal(&dma_done);
        thd_schedule(1, 0);
        dma_blocking = 0;
    }
}

int pvr_dma_transfer(void * src, uint32 dest, uint32 count, int type,
                     int block, pvr_dma_callback_t callback, ptr_t cbdata) {
    uint32 val;
    uint32 src_addr = ((uint32)src);
    uint32 dest_addr;

    // Send the data to the right place
    if(type == PVR_DMA_TA)
        dest_addr = (((unsigned long)dest) & 0xFFFFFF) | 0x10000000;
    else
        dest_addr = (((unsigned long)dest) & 0xFFFFFF) | 0x11000000;

    // Make sure we're not already DMA'ing
    if(pvrdma[PVR_DST] != 0) {
        dbglog(DBG_ERROR, "pvr_dma: PVR_DST != 0\n");
        errno = EINPROGRESS;
        return -1;
    }

    val = shdma[DMAC_CHCR2];

    if(val & 0x1)  /* DE bit set so we must clear it */
        shdma[DMAC_CHCR2] = val | 0x1;

    if(val & 0x2)  /* TE bit set so we must clear it */
        shdma[DMAC_CHCR2] = val | 0x2;

    /* Check for 32-byte alignment */
    if(src_addr & 0x1F)
        dbglog(DBG_WARNING, "pvr_dma: src is not 32-byte aligned\n");

    src_addr &= 0x0FFFFFE0;

    if(src_addr < 0x0c000000) {
        dbglog(DBG_ERROR, "pvr_dma: src address < 0x0c000000\n");
        errno = EFAULT;
        return -1;
    }

    shdma[DMAC_SAR2] = src_addr;
    shdma[DMAC_DMATCR2] = count / 32;
    shdma[DMAC_CHCR2] = 0x12c1;

    val = shdma[DMAC_DMAOR];

    if((val & 0x8007) != 0x8001) {
        dbglog(DBG_ERROR, "pvr_dma: Failed DMAOR check\n");
        errno = EIO;
        return -1;
    }

    dma_blocking = block;
    dma_callback = callback;
    dma_cbdata = cbdata;

    pvrdma[PVR_LMMODE0] = type == PVR_DMA_VRAM64 ? 0 : 1;
    pvrdma[PVR_STATE] = dest_addr;
    pvrdma[PVR_LEN] = count;
    pvrdma[PVR_DST] = 0x1;

    /* Wait for us to be signaled */
    if(block)
        sem_wait(&dma_done);

    return 0;
}

/* Count is in bytes. */
int pvr_txr_load_dma(void * src, pvr_ptr_t dest, uint32 count, int block,
                     pvr_dma_callback_t callback, ptr_t cbdata) {
    return pvr_dma_transfer(src, (uint32)dest, count, PVR_DMA_VRAM64, block, callback, cbdata);
}

int pvr_dma_load_ta(void * src, uint32 count, int block, pvr_dma_callback_t callback, ptr_t cbdata) {
    return pvr_dma_transfer(src, 0, count, PVR_DMA_TA, block, callback, cbdata);
}

int pvr_dma_ready() {
    return pvrdma[PVR_DST] == 0;
}

void pvr_dma_init() {
    /* Create an initially blocked semaphore */
    sem_init(&dma_done, 0);
    dma_blocking = 0;
    dma_callback = NULL;
    dma_cbdata = 0;

    /* Hook the neccessary interrupts */
    asic_evt_set_handler(ASIC_EVT_PVR_DMA, pvr_dma_irq_hnd);
    asic_evt_enable(ASIC_EVT_PVR_DMA, ASIC_IRQ_DEFAULT);
}

void pvr_dma_shutdown() {
    /* XXX Need to ensure that no DMA is in progress, does this work?? */
    if(!pvr_dma_ready()) {
        pvrdma[PVR_DST] = 0;
    }

    /* Clean up */
    asic_evt_disable(ASIC_EVT_PVR_DMA, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_DMA, NULL);
    sem_destroy(&dma_done);
}
