/* KallistiOS ##version##

   speedtest.c
   Copyright (C) 2025 Paul Cercueil
*/

#include <stdio.h>
#include <stdint.h>

#include <arch/dmac.h>
#include <arch/timer.h>
#include <arch/irq.h>
#include <dc/pvr.h>
#include <kos/genwait.h>

#define ARRAY_SIZE(x) (sizeof(x) ? sizeof(x) / sizeof((x)[0]) : 0)

#define BUF_SIZE (1024 * 1024)

static alignas(32) char buf1[BUF_SIZE];
static alignas(32) char buf2[BUF_SIZE];

static void dma_done(void *d)
{
	*(uint64_t *)d = timer_us_gettime64();
	genwait_wake_all(d);
}

static dma_config_t dma_cfg = {
	.channel = 1,
	.request = DMA_REQUEST_AUTO_MEM_TO_MEM,
	.unit_size = DMA_UNITSIZE_32BYTE,
	.src_mode = DMA_ADDRMODE_INCREMENT,
	.dst_mode = DMA_ADDRMODE_INCREMENT,
	.transmit_mode = DMA_TRANSMITMODE_BURST,
	.callback = dma_done,
};

static uint64_t
do_dma_transfer(unsigned int test, pvr_ptr_t vram1, pvr_ptr_t vram2)
{
	uint64_t before, after = 0;

	irq_disable_scoped();

	before = timer_us_gettime64();

	switch (test) {
	case 0:
		/* RAM to RAM */
		dma_transfer(&dma_cfg,
			     dma_map_dst(buf1, BUF_SIZE),
			     dma_map_src(buf2, BUF_SIZE),
			     BUF_SIZE, &after);
		break;
	case 1:
		/* RAM to VRAM */
		dma_transfer(&dma_cfg,
			     hw_to_dma_addr((uintptr_t)vram1),
			     dma_map_src(buf2, BUF_SIZE),
			     BUF_SIZE, &after);
		break;
	case 2:
		/* VRAM to RAM */
		dma_transfer(&dma_cfg,
			     dma_map_dst(buf1, BUF_SIZE),
			     hw_to_dma_addr((uintptr_t)vram1),
			     BUF_SIZE, &after);
		break;
	case 3:
		/* VRAM to VRAM */
		dma_transfer(&dma_cfg,
			     hw_to_dma_addr((uintptr_t)vram1),
			     hw_to_dma_addr((uintptr_t)vram2),
			     BUF_SIZE, &after);
		break;
	case 4:
		/* RAM to VRAM using SQs */
		pvr_txr_load(buf1, vram1, BUF_SIZE);

		return timer_us_gettime64() - before;
	case 5:
		/* RAM to 64-bit VRAM using PVR DMA */
		pvr_txr_load_dma(buf1, vram1, BUF_SIZE,
				 0, dma_done, &after);
		break;
	case 6:
		/* RAM to 32-bit VRAM using PVR DMA */
		pvr_dma_transfer(buf1, (uintptr_t)vram1, BUF_SIZE,
				 PVR_DMA_VRAM32, 0,
				 dma_done, &after);
		break;
	}

	while ((volatile uint64_t)after == 0)
		genwait_wait(&after, "IRQ wait", 0, NULL);

	return after - before;
}

static const char * const test_lbl[] = {
	"DMAC, RAM to RAM:   ",
	"DMAC, RAM to VRAM:  ",
	"DMAC, VRAM to RAM:  ",
	"DMAC, VRAM to VRAM: ",
	"PVR SQs:            ",
	"PVR DMA, 64-bit:    ",
	"PVR DMA, 32-bit:    ",
};

int main(int argc, char **argv)
{
	pvr_ptr_t vram, vram2;
	uint64_t time_us;
	unsigned int i;

	pvr_init_defaults();

	vram = pvr_mem_malloc(BUF_SIZE);
	vram2 = pvr_mem_malloc(BUF_SIZE);

	for (i = 0; i < ARRAY_SIZE(test_lbl); i++) {
		time_us = do_dma_transfer(i, vram, vram2);
		printf("%s%f MiB/s\n",
		       test_lbl[i], (float)BUF_SIZE / (float)time_us);

	}

	pvr_mem_free(vram);
	pvr_mem_free(vram2);

	return 0;
}
