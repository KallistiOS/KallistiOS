/* KallistiOS ##version##

   arch/xbox/kernel/pic.c
   Copyright (C) 2026 The KallistiOS Contributors
*/

#include <stdint.h>

#include "pic.h"

#define PIC_MASTER_COMMAND 0x20U
#define PIC_MASTER_DATA    0x21U
#define PIC_SLAVE_COMMAND  0xa0U
#define PIC_SLAVE_DATA     0xa1U

#define PIC_EOI            0x20U
#define PIC_ICW1_INIT      0x10U
#define PIC_ICW1_ICW4      0x01U
#define PIC_ICW4_8086      0x01U

static int pic_initialized;

static inline void pic_out8(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

/* Port 0x80 is implemented by the Xbox LPC bridge and provides the spacing
   required between consecutive legacy-controller writes. */
static inline void pic_io_wait(void) {
    pic_out8(0x80U, 0);
}

static void pic_remap(uint8_t master_vector, uint8_t slave_vector) {
    pic_out8(PIC_MASTER_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    pic_io_wait();
    pic_out8(PIC_SLAVE_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    pic_io_wait();
    pic_out8(PIC_MASTER_DATA, master_vector);
    pic_io_wait();
    pic_out8(PIC_SLAVE_DATA, slave_vector);
    pic_io_wait();
    pic_out8(PIC_MASTER_DATA, 1U << 2); /* Slave is connected to IRQ2. */
    pic_io_wait();
    pic_out8(PIC_SLAVE_DATA, 2U);       /* Slave cascade identity. */
    pic_io_wait();
    pic_out8(PIC_MASTER_DATA, PIC_ICW4_8086);
    pic_io_wait();
    pic_out8(PIC_SLAVE_DATA, PIC_ICW4_8086);
    pic_io_wait();
}

int xbox_pic_init(void) {
    if(pic_initialized)
        return -1;

    /* Mask everything before changing vector ownership. */
    pic_out8(PIC_MASTER_DATA, 0xffU);
    pic_out8(PIC_SLAVE_DATA, 0xffU);
    pic_remap(XBOX_PIC_MASTER_VECTOR, XBOX_PIC_SLAVE_VECTOR);

    /* During threading bring-up KOS owns only PIT IRQ0. All device sources
       remain masked until their individual KOS drivers take ownership. */
    pic_out8(PIC_MASTER_DATA, 0xfeU);
    pic_out8(PIC_SLAVE_DATA, 0xffU);
    pic_initialized = 1;
    return 0;
}

void xbox_pic_shutdown(void) {
    if(!pic_initialized)
        return;

    pic_out8(PIC_MASTER_DATA, 0xffU);
    pic_out8(PIC_SLAVE_DATA, 0xffU);
    /*
     * xbox-load-ip is a polling, bare-metal loader that keeps IF clear. Leave
     * every external source masked rather than inventing an Xbox-kernel PIC
     * mapping that KOS neither needs nor can discover from the 8259.
     */
    pic_initialized = 0;
}

void xbox_pic_eoi(uint32_t vector) {
    if(vector >= XBOX_PIC_SLAVE_VECTOR)
        pic_out8(PIC_SLAVE_COMMAND, PIC_EOI);
    pic_out8(PIC_MASTER_COMMAND, PIC_EOI);
}
