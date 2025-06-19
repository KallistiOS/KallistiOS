/* KallistiOS ##version##

   g2dma.c
   Copyright (C) 2001, 2002, 2004 Megan Potter
   Copyright (C) 2023 Andy Barajas
   Copyright (C) 2024 Ruslan Rostovtsev
*/

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <dc/asic.h>
#include <dc/g2bus.h>
#include <kos/dbglog.h>
#include <kos/sem.h>
#include <kos/thread.h>

typedef struct {
    uint32_t      g2_addr;        /* G2 Bus start address */
    uint32_t      sh4_addr;       /* SH-4 start address */
    uint32_t      size;           /* Size in bytes; Must be 32-byte aligned */
    uint32_t      dir;            /* 0: sh4->g2bus; 1: g2bus->sh4 */
    uint32_t      trigger_select; /* DMA trigger select; 0-CPU, 1-HW, 2-I */
    uint32_t      enable;         /* DMA enable */
    uint32_t      start;          /* DMA start */
    uint32_t      suspend;        /* DMA suspend */
} g2_dma_ctrl_t;

typedef struct {
    g2_dma_ctrl_t dma[4];
    uint32_t      g2_id;         /* G2 ID Bus version (Read only) */
    uint32_t      u1[3];         /* Unused */
    uint32_t      ds_timeout;    /* G2 DS timeout in clocks (default: 0x3ff) */
    uint32_t      tr_timeout;    /* G2 TR timeout in clocks (default: 0x3ff) */
    uint32_t      modem_timeout; /* G2 Modem timeout in cycles */
    uint32_t      modem_wait;    /* G2 Modem wait time in cycles */
    uint32_t      u2[7];         /* Unused */
    uint32_t      protection;    /* System memory area protection range */
} g2_dma_reg_t;

/* Signaling semaphore */
static semaphore_t dma_done[4];
static int dma_progress[4];
static int dma_blocking[4];
static g2_dma_callback_t dma_callback[4];
static void *dma_cbdata[4];

static int dma_init;

/* G2 Bus DMA registers */
#define G2_DMA_REG_BASE 0xa05f7800
static volatile g2_dma_reg_t * const g2_dma = (g2_dma_reg_t *)G2_DMA_REG_BASE;

/* 
    List of possible initiation triggers values to assign to trigger_select:
        CPU_TRIGGER: Software-driven. (Setting enable and start to 1)
        HARDWARE_TRIGGER: Via AICA (DMA0) or expansion device.
        INTERRUPT_TRIGGER: Based on interrupt signals. 
*/
#define CPU_TRIGGER       0
#define HARDWARE_TRIGGER  1
#define INTERRUPT_TRIGGER 2

/*
    Controls whether the DMA suspend register of a channel is enabled:
        0x00000004: Enables the suspend register.
        0x00000000: Disables the suspend register.

    OR '|' this value with trigger when setting the trigger select of the
    DMA channel.
*/
#define DMA_SUSPEND_ENABLED    0x00000004
#define DMA_SUSPEND_DISABLED   0x00000000

/*  
    For sh4 and g2bus addresses, ensure bits 31-29 & 4-0 are '0' to avoid 
    illegal interrupts. Only bits 28-5 are used for valid addresses. 
*/
#define MASK_ADDRESS      0x1fffffe0

/*
    Controls DMA initiation behavior after a DMA transfer completes:
        0x00000000: Preserve the current DMA enable setting.
        0x80000000: Reset the DMA enable setting to "0" after transfer.

    OR '|' this value with length when setting the size of the DMA request.
*/
#define PRESERVE_ENABLED  0x00000000
#define RESET_ENABLED     0x80000000

/* 
    Specifies system memory address range for G2-DMA across channels 0-3.
    If a DMA transfer is generated outside of this range, an overrun error 
    occurs.

    Previous range (0x4659404f):
        0x0C400000 - 0x0C4F0000

    Current range (0x4659007F):
        0x0C000000 - 0x0CFFFFFF (Effectively disabling mem protection)

    How its calculated:

    xxxx xxxx xxxx xxxx ---- ---- ---- ---- : (0x4659)
    ---- ---- ---- ---- -xxx xxxx ---- ---- : Top range
    ---- ---- ---- ---- ---- ---- -xxx xxxx : Bottom range

    top_range = (value & 0x7f00) >> 8;
    bottom_range = (value & 0x7f);

    top_addr = (top_range << 20) | 0x08000000;
    bottom_addr = (bottom_range << 20) | 0x080fffff;
*/
#define SYS_MEM_SECURITY_CODE 0x4659
#define DISABLE_SYS_MEM_PROTECTION (SYS_MEM_SECURITY_CODE << 16 | 0x007F)
#define ENABLE_SYS_MEM_PROTECTION  (SYS_MEM_SECURITY_CODE << 16 | 0x7F00)

/*  
    Set the DS# (Data Strobe) timeout to 27 clock cycles for the external DMA. 
    If data isn't ready for latching by this time, an interrupt will be 
    triggered. 
    
    Not sure why its 27 but can be changed later. Default value 
    is 1023 cycles (0x3ff).
*/
#define DS_CYCLE_OVERRIDE  27

/* Disable the DMA */
inline static void dma_disable(uint32_t chn) {
    g2_dma->dma[chn].enable = 0;
    g2_dma->dma[chn].start = 0;
}

static void g2_dma_irq_hnd(uint32_t code, void *data) {
    int chn = code - ASIC_EVT_G2_DMA0;

    (void)data;

    if(chn < G2_DMA_CHAN_SPU || chn > G2_DMA_CHAN_CH3) {
        dbglog(DBG_ERROR, "g2_dma: Wrong channel received in g2_dma_irq_hnd");
        return;
    }

    /* VP : changed the order of things so that we can chain dma calls */

    if(dma_progress[chn]) {
        dma_progress[chn] = 0;

        /* Signal the calling thread to continue, if any. */
        if(dma_blocking[chn]) {
            dma_blocking[chn] = 0;
            sem_signal(&dma_done[chn]);
            thd_schedule(1, 0);
        }

        /* Call the callback, if any. */
        if(dma_callback[chn]) {
            dma_callback[chn](dma_cbdata[chn]);
        }
    }
}

int g2_dma_transfer(void *sh4, void *g2bus, size_t length, uint32_t block,
                    g2_dma_callback_t callback, void *cbdata,
                    uint32_t dir, uint32_t mode, uint32_t g2chn, uint32_t sh4chn) {
    /* No longer used but we keep then around for compatibility */
    (void)mode;
    (void)sh4chn;

    if(g2chn > G2_DMA_CHAN_CH3) {
        errno = EINVAL;
        return -1;
    }

    /* Check alignments */
    if(((uint32_t)sh4) & 31) {
        dbglog(DBG_ERROR, "g2_dma: Unaligned sh4 DMA %p\n", sh4);
        errno = EFAULT;
        return -1;
    }

    if(((uint32_t)g2bus) & 31) {
        dbglog(DBG_ERROR, "g2_dma: Unaligned g2bus DMA %p\n", g2bus);
        errno = EFAULT;
        return -1;
    }

    /* Make sure we're not already DMA'ing */
    if(dma_progress[g2chn] != 0) {
        errno = EINPROGRESS;
        return -1;
    }
    dma_progress[g2chn] = 1;

    /* Make sure length is a multiple of 32 */
    length = (length + 0x1f) & ~0x1f;

    dma_blocking[g2chn] = block;
    dma_callback[g2chn] = callback;
    dma_cbdata[g2chn] = cbdata;

    /* Set needed registers */
    g2_dma->dma[g2chn].g2_addr = ((uint32_t)g2bus) & MASK_ADDRESS;
    g2_dma->dma[g2chn].sh4_addr = ((uint32_t)sh4) & MASK_ADDRESS;
    g2_dma->dma[g2chn].size = length | RESET_ENABLED;
    g2_dma->dma[g2chn].dir = dir;

    if(g2chn == G2_DMA_CHAN_SPU) {
        /* Wait until fifo is empty and start. */
        g2_dma->dma[g2chn].trigger_select = HARDWARE_TRIGGER | DMA_SUSPEND_ENABLED;
    }
    else {
        g2_dma->dma[g2chn].trigger_select = CPU_TRIGGER | DMA_SUSPEND_ENABLED;
    }

    /* Start the DMA transfer */
    g2_dma->dma[g2chn].enable = 1;
    g2_dma->dma[g2chn].start = 1;

    /* Wait for us to be signaled */
    if(block)
        sem_wait(&dma_done[g2chn]);

    return 0;
}

int g2_dma_init(void) {
    int i;

    if(dma_init)
        return 0;

    dma_init = 1;

    for(i = 0; i < 4; i++) {
        /* Create an initially blocked semaphore */
        sem_init(&dma_done[i], 0);
        dma_progress[i] = 0;
        dma_blocking[i] = 0;
        dma_callback[i] = NULL;
        dma_cbdata[i] = 0;

        /* Hook the interrupt */
        asic_evt_set_handler(ASIC_EVT_G2_DMA0 + i, g2_dma_irq_hnd, NULL);
        asic_evt_enable(ASIC_EVT_G2_DMA0 + i, ASIC_IRQB);
    }

    /* Setup the DMA transfer on the external side */
    g2_dma->ds_timeout = DS_CYCLE_OVERRIDE;
    g2_dma->protection = DISABLE_SYS_MEM_PROTECTION;

    return 0;
}

void g2_dma_shutdown(void) {
    int i;

    if(!dma_init)
        return;

    dma_init = 0;

    for(i = 0; i < 4; i++) {
        /* Unhook the G2 interrupt */
        asic_evt_disable(ASIC_EVT_G2_DMA0 + i, ASIC_IRQB);
        asic_evt_remove_handler(ASIC_EVT_G2_DMA0 + i);

        /* Destroy the semaphore */
        sem_destroy(&dma_done[i]);

        /* Turn off any remaining DMA */
        dma_disable(i);
    }

    g2_dma->protection = ENABLE_SYS_MEM_PROTECTION;
}
