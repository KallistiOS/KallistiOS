/* Internal Xbox GDT ownership interface. */

#ifndef __KOS_XBOX_KERNEL_GDT_H
#define __KOS_XBOX_KERNEL_GDT_H

int xbox_gdt_init(void);
void xbox_gdt_shutdown(void);

#endif
