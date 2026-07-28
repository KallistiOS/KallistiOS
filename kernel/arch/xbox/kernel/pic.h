/* KallistiOS ##version##

   arch/xbox/kernel/pic.h
   Copyright (C) 2026 The KallistiOS Contributors
*/

#ifndef __ARCH_XBOX_KERNEL_PIC_H
#define __ARCH_XBOX_KERNEL_PIC_H

#include <stdint.h>

#define XBOX_PIC_MASTER_VECTOR 0x20U
#define XBOX_PIC_SLAVE_VECTOR  0x28U
#define XBOX_PIC_IRQ_COUNT     16U

int xbox_pic_init(void);
void xbox_pic_shutdown(void);
void xbox_pic_eoi(uint32_t vector);

#endif /* __ARCH_XBOX_KERNEL_PIC_H */
