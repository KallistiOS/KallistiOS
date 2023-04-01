/* KallistiOS ##version##

   spudma.c
   Copyright (C)2001,2002,2004 Megan Potter
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <dc/spu.h>
#include <dc/asic.h>
#include <kos/sem.h>

/* testing */
#define ASIC_IRQ ASIC_IRQB

/* original value */
//#define ASIC_IRQ ASIC_IRQ_DEFAULT

/*

Handles the DMA part of the SPU functionality.

Thanks to Bitmaster for the info on SPU DMA, and Roger Cattermole who
got a well-functioning PVR DMA module (helped this a bit).

XXX: Right now this conflicts with PVR DMA but we ought to be able
to fix that by going to another channel.

XXX: This ought to be abstracted out to allow usage with the parallel
port as well.

VP : Generalized the code to allow configurable g2 dma channel and sh dma channel.
It is now possible to perform several g2 dma transfer at a time. Some things are still
a bit mysterious, like the mode parameter, which I set empirically depending
on the combination of dma channels.

Known working combination :

g2chn = 0, sh4chn = 3 --> mode = 5 (but many other values seem OK ?)
g2chn = 1, sh4chn = 1 --> mode = 0 (or 4 better ?)
g2chn = 1, sh4chn = 0 --> mode = 3

It seems that g2chn is not important when choosing mode, so this mode parameter is probably
how we actually connect the sh4chn to the g2chn.

Update : looks like there is a formula, mode = 3 + shchn

*/

typedef struct {
    uint32      ext_addr;       /* External address (SPU-RAM or parallel port) */
    uint32      sh4_addr;       /* SH-4 Address */
    uint32      size;           /* Size in bytes; all addresses and sizes must be 32-byte aligned */
    uint32      dir;            /* 0: cpu->ext; 1: ext->cpu */
    uint32      mode;           /* 5 for SPU transfer */
    uint32      ctrl1;          /* b0 */
    uint32      ctrl2;          /* b0 */
    uint32      u1;         /* ?? */
} g2_dma_ctrl_t;

typedef struct {
    uint32      ext_addr;
    uint32      sh4_addr;
    uint32      size;
    uint32      status;
} g2_dma_stat_t;

typedef struct {
    g2_dma_ctrl_t   dma[4];
    uint32      u1[4];          /* ?? */
    uint32      wait_state;
    uint32      u2[10];         /* ?? */
    uint32      magic;
    g2_dma_stat_t   dma_stat[4];
} g2_dma_reg_t;

/* DMA registers */
static vuint32  * const shdma = (vuint32 *)0xffa00000;
static volatile g2_dma_reg_t * const extdma = (g2_dma_reg_t *)0xa05f7800;

/* DMAC registers. */
/* VP : configurable channel number */
#define DMAC_SAR(chn)   ( (chn<<4) + 0x00)/4
#define DMAC_DAR(chn)   ( (chn<<4) + 0x04)/4
#define DMAC_DMATCR(chn) ( (chn<<4) + 0x08)/4
#define DMAC_CHCR(chn)  ( (chn<<4) + 0x0c)/4
#define DMAC_DMAOR  0x40/4


/* Signaling semaphore */
static semaphore_t dma_done[4];
static int dma_blocking[4];
static g2_dma_callback_t dma_callback[4];
static ptr_t dma_cbdata[4];
static int shchn[4]; /* Mapping g2chn --> shchn, -1 if channels are not connected */

static void dma_disable(int chn) {
    /* Disable the DMA */
    extdma->dma[chn].ctrl1 = 0;
    extdma->dma[chn].ctrl2 = 0;

    if(shchn[chn] >= 0) {
        shdma[DMAC_CHCR(shchn[chn])] &= ~1;
        shchn[chn] = -1;
    }
}

// VP
#define dbglog(...) (void) 0

static void g2_dma_irq(uint32 code) {
    int chn = code - ASIC_EVT_G2_DMA0;

    if(chn < 0 || chn > 3) {
        //panic("wrong channel received in g2_dma_irq");
        //return;
        //printf("wrong channel received in g2_dma_irq");
        chn = 1;
    }

    if(shdma[DMAC_DMATCR(shdma[chn])] != 0) {
        //dbglog(DBG_INFO, "g2_dma: DMA did not complete successfully\n");
        /*      return; */
    }

    dma_disable(chn);

    /* VP : changed the order of things so that we can chain dma calls */

    // Signal the calling thread to continue, if any.
    if(dma_blocking[chn]) {
        sem_signal(&dma_done[chn]);
        thd_schedule(1, 0);
        dma_blocking[chn] = 0;
    }

    // Call the callback, if any.
    if(dma_callback[chn]) {
        dma_callback[chn](dma_cbdata[chn]);
    }

}

// XXX: Write (and check here) spu_dma_ready()
// VP : extended for any g2 dma, configurable sh dma channel and g2 channel
/* The mode parameter is quite mysterious. A value of 5 or 0 or 1 or 6 but not 2 or 10 or 18,
   are working with g2chn==0 and sh4chn==3 (used for spu, but could as well be used for the bba).
   On the other hand , if g2chn is 1 and sh4chn is 1 (that's what I'm using for the BBA),
   then it's better to set mode to 0, and not 1 or 2 and probably many other values but
   I got tired of rebooting my console ;)
*/
int g2_dma_transfer(void *from, void * dest, uint32 length, int block,
                    g2_dma_callback_t callback, ptr_t cbdata,
                    uint32 dir, uint32 mode, uint32 g2chn, uint32 sh4chn) {
    uint32 val;

    if(g2chn > 3 || sh4chn > 3) {
        errno = EINVAL;
        return -1;
    }

    /* Check alignments */
    if(((uint32)from) & 31) {
        dbglog(DBG_ERROR, "g2_dma: unaligned source DMA %p\n", from);
        errno = EFAULT;
        return -1;
    }

    if(((uint32)dest) & 31) {
        dbglog(DBG_ERROR, "g2_dma: unaligned dest DMA %p\n", dest);
        errno = EFAULT;
        return -1;
    }

    length = (length + 0x1f) & ~0x1f;

    shchn[g2chn] = sh4chn;
    val = shdma[DMAC_CHCR(sh4chn)];

    // DE bit set so we must clear it?
    if(val & 1)
        shdma[DMAC_CHCR(sh4chn)] = val | 1;

    // TE bit set so we must clear it?
    if(val & 2)
        shdma[DMAC_CHCR(sh4chn)] = val | 2;

    /* Setup the SH-4 channel */
    shdma[DMAC_SAR(sh4chn)] = 0;        /* SAR = 0 */
    shdma[DMAC_DMATCR(sh4chn)] = 0;         /* DMATCR = 0 */
    shdma[DMAC_CHCR(sh4chn)] = 0x12c1;  /* CHCR = 0x12c0; 32-byte block transfer,
                           burst mode, external request, single address mode,
                           source address incremented; */

    val = shdma[DMAC_DMAOR];

    if((val & 0x8007) != 0x8001) {
        dbglog(DBG_ERROR, "g2_dma: failed DMAOR check\n");
        errno = EIO;
        return -1;
    }

    dma_blocking[g2chn] = block;
    dma_callback[g2chn] = callback;
    dma_cbdata[g2chn] = cbdata;

    /* Start the DMA transfer */
    extdma->dma[g2chn].ctrl1 = 0;
    extdma->dma[g2chn].ctrl2 = 0;
    extdma->dma[g2chn].ext_addr = ((uint32)dest) & 0x1fffffe0;
    extdma->dma[g2chn].sh4_addr = ((uint32)from) & 0x1fffffe0;
    extdma->dma[g2chn].size = (length & ~31) | 0x80000000;
    extdma->dma[g2chn].dir = dir;
    extdma->dma[g2chn].mode = mode;
    extdma->dma[g2chn].ctrl1 = 1;
    extdma->dma[g2chn].ctrl2 = 1;

    /* Wait for us to be signaled */
    if(block)
        sem_wait(&dma_done[g2chn]);

    return 0;
}

int spu_dma_transfer(void *from, uint32 dest, uint32 length, int block,
                     g2_dma_callback_t callback, ptr_t cbdata) {
    /* Adjust destination to SPU RAM */
    dest += 0x00800000;

    return g2_dma_transfer(from, (void *) dest, length, block, callback, cbdata, 0,
                           SPU_DMA_MODE, SPU_DMA_G2CHN, SPU_DMA_SHCHN);
}

static int init;
int spu_dma_init() {
    int i;

    if(init)
        return 0;

    init = 1;

    for(i = 0; i < 4; i++) {
        /* Create an initially blocked semaphore */
        sem_init(&dma_done[i], 0);
        dma_blocking[i] = 0;
        dma_callback[i] = NULL;
        dma_cbdata[i] = 0;

        /* reset mapping g2chn --> shchn */
        shchn[i] = -1;

        //Hook the interrupt
        asic_evt_set_handler(ASIC_EVT_SPU_DMA + i, g2_dma_irq);
        asic_evt_enable(ASIC_EVT_SPU_DMA + i, ASIC_IRQ);
    }

    /* Setup the DMA transfer on the external side */
    extdma->wait_state = 27;
    extdma->magic = 0x4659404f;

    return 0;
}

void spu_dma_shutdown() {
    int i;

    if(!init)
        return;

    init = 0;

    for(i = 0; i < 4; i++) {
        /* Unhook the G2 interrupt */
        asic_evt_disable(ASIC_EVT_SPU_DMA + i, ASIC_IRQ);
        asic_evt_set_handler(ASIC_EVT_SPU_DMA + i, NULL);

        /* Destroy the semaphore */
        sem_destroy(&dma_done[i]);

        /* Turn off any remaining DMA */
        dma_disable(i);
    }
}

