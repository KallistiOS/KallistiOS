/* KallistiOS ##version##

   dmac.c

   Copyright (C) 2025 Paul Cercueil
*/
#include <arch/cache.h>
#include <arch/dmac.h>
#include <arch/irq.h>
#include <arch/memory.h>
#include <kos/dbglog.h>
#include <kos/genwait.h>
#include <kos/regfield.h>

#include <errno.h>

#define DMAC_BASE       0xffa00000

typedef enum dma_register {
    DMA_REG_SAR         = 0x00, /* Source Address Register */
    DMA_REG_DAR         = 0x04, /* Destination Address Register */
    DMA_REG_TCR         = 0x08, /* Transfer Count Register */
    DMA_REG_CHCR        = 0x0C, /* Channel Control Register */
    DMA_REG_DMAOR       = 0x40, /* DMA Operation Register */
} dma_register_t;

#define REG_CHCR_DST_ADDRMODE   GENMASK(15, 14)
#define REG_CHCR_SRC_ADDRMODE   GENMASK(13, 12)
#define REG_CHCR_REQUEST        GENMASK(11, 8)
#define REG_CHCR_TRANSMIT_MODE  BIT(7)
#define REG_CHCR_BUSWIDTH       GENMASK(6, 4)
#define REG_CHCR_INTERRUPT_EN   BIT(2)
#define REG_CHCR_TRANSFER_END   BIT(1)
#define REG_CHCR_DMAC_EN        BIT(0)

static const dma_config_t *channels_cfg[4];

static const irq_t channel_to_irq[] = {
    [0] = EXC_DMAC_DMTE0,
    [1] = EXC_DMAC_DMTE1,
    [2] = EXC_DMAC_DMTE2,
    [3] = EXC_DMAC_DMTE3,
};

static inline unsigned char irq_to_channel(irq_t irq) {
    return (irq - EXC_DMAC_DMTE0) >> 5;
}

static uint32_t dmac_read(dma_channel_t channel, dma_register_t reg) {
    return *(volatile uint32_t *)(DMAC_BASE + channel * 0x10 + reg);
}

static void dmac_write(dma_channel_t channel, dma_register_t reg, uint32_t val) {
    *(volatile uint32_t *)(DMAC_BASE + channel * 0x10 + reg) = val;
}

dma_addr_t hw_to_dma_addr(uintptr_t hw_addr) {
    return (dma_addr_t)(hw_addr & MEM_AREA_CACHE_MASK);
}

static dma_addr_t dma_map_src_dst(uintptr_t addr, size_t len, bool is_dst) {
    switch(addr & ~MEM_AREA_CACHE_MASK) {
    case MEM_AREA_P0_BASE:
    case MEM_AREA_P1_BASE:
    case MEM_AREA_P3_BASE:
        if(is_dst)
            dcache_inval_range(addr, len);
        else
            dcache_flush_range(addr, len);
        break;

    default:
        /* Nothing to do */
        break;
    }

    return hw_to_dma_addr(addr);
}

dma_addr_t dma_map_src(const void *ptr, size_t len) {
    return dma_map_src_dst((uintptr_t)ptr, len, false);
}

dma_addr_t dma_map_dst(void *ptr, size_t len) {
    return dma_map_src_dst((uintptr_t)ptr, len, true);
}

static const unsigned char dma_unit_size[] = {
    [DMA_UNITSIZE_64BIT] = 8,
    [DMA_UNITSIZE_8BIT] = 1,
    [DMA_UNITSIZE_16BIT] = 2,
    [DMA_UNITSIZE_32BIT] = 4,
    [DMA_UNITSIZE_32BYTE] = 32,
};

static void dma_irq_handler(irq_t code, irq_context_t *context, void *d) {
    unsigned char channel = irq_to_channel(code);

    (void)context;

    /* ACK the IRQ by clearing CHCR */
    dmac_write(channel, DMA_REG_CHCR, 0);

    genwait_wake_all((void *)&channels_cfg[channel]->callback);
    channels_cfg[channel]->callback(d);
}

static bool dma_is_running(dma_channel_t channel) {
    uint32_t chcr = dmac_read(channel, DMA_REG_CHCR);

    return (chcr & (REG_CHCR_TRANSFER_END | REG_CHCR_DMAC_EN)) == REG_CHCR_DMAC_EN;
}

void dma_wait_complete(dma_channel_t channel) {
    irq_disable_scoped();

    while(dma_is_running(channel)) {
        if(!irq_inside_int()) {
            if(channels_cfg[channel]->callback)
                genwait_wait(channels_cfg[channel]->callback, "DMA complete wait", 0, NULL);
            else
                thd_pass();
        }
    }
}

int dma_transfer(const dma_config_t *cfg, dma_addr_t dst, dma_addr_t src,
                 size_t len, void *cb_data) {
    unsigned int transfer_size = dma_unit_size[cfg->unit_size];
    uint32_t chcr;

    if((len | dst | src) & (transfer_size - 1)) {
        dbglog(DBG_ERROR, "dmac: src/dst/len not aligned to the bus width\n");
        errno = EFAULT;
        return -1;
    }

    irq_disable_scoped();

    dma_wait_complete(cfg->channel);
    channels_cfg[cfg->channel] = cfg;

    dmac_write(cfg->channel, DMA_REG_CHCR, 0);
    dmac_write(cfg->channel, DMA_REG_SAR, src);
    dmac_write(cfg->channel, DMA_REG_DAR, dst);
    dmac_write(cfg->channel, DMA_REG_TCR, len >> __builtin_ctz(transfer_size));

    irq_set_handler(channel_to_irq[cfg->channel], dma_irq_handler, cb_data);

    chcr = FIELD_PREP(REG_CHCR_DST_ADDRMODE, cfg->dst_mode)
        | FIELD_PREP(REG_CHCR_SRC_ADDRMODE, cfg->src_mode)
        | FIELD_PREP(REG_CHCR_REQUEST, cfg->request)
        | FIELD_PREP(REG_CHCR_TRANSMIT_MODE, cfg->transmit_mode)
        | FIELD_PREP(REG_CHCR_BUSWIDTH, cfg->unit_size)
        | FIELD_PREP(REG_CHCR_INTERRUPT_EN, !!cfg->callback)
        | FIELD_PREP(REG_CHCR_DMAC_EN, 1);

    dmac_write(cfg->channel, DMA_REG_CHCR, chcr);

    return 0;
}

size_t dma_transfer_get_remaining(dma_channel_t channel) {
    unsigned char unit_size = channels_cfg[channel]->unit_size;
    uint32_t tcr = dmac_read(channel, DMA_REG_TCR);

    return tcr * dma_unit_size[unit_size];
}
