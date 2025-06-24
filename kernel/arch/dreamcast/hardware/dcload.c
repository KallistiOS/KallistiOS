/* KallistiOS ##version##

   dcload.c

   Copyright (C) 2025 Donald Haase

*/

#include <stdio.h>

#include <dc/fifo.h>
#include <dc/dcload.h>
#include <arch/irq.h>
#include <arch/memory.h>

/* This is the address where the function pointer for the dcload syscall is fetched from */
#define VEC_DCLOAD        (MEM_AREA_P1_BASE | 0x0C004008)

/*
    This is the single syscall dcload provides. It is then multiplexed out based on the `cmd`
    parameter.
*/

int dcload_syscall(dcload_cmd_t cmd, void *param1, void *param2, void *param3) {
    uintptr_t *syscall_ptr = (uintptr_t *)VEC_DCLOAD;
    int (*syscall)() = (int (*)())(*syscall_ptr);

    /* Disable IRQs until the syscall returns */
    irq_disable_scoped();

    /* Ensure that the FIFO buffer is clear */
    /* XXX - Is this needed? It seems like something only for serial. */
    while(FIFO_STATUS & FIFO_SH4)            ;

    /* Make the call */
    return syscall(cmd, param1, param2, param3);
}

size_t dcload_gdbpacket(const char* in_buf, size_t in_size, char* out_buf, size_t out_size) {
    /* we have to pack the sizes together because the dcloadsyscall handler
       can only take 4 parameters */
    return dcload_syscall(DCLOAD_GDBPACKET, (void *)in_buf, (void *)((in_size << 16) | (out_size & 0xffff)), (void *)out_buf);
}