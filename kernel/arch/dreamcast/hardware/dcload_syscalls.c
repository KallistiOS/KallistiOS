/* KallistiOS ##version##

   dcload_syscalls.c

   Copyright (C) 2026 Andy Barajas

*/

#include <dc/dcload.h>

#include <dc/fifo.h>
#include <dc/memory.h>

#include <kos/irq.h>

/* This is the address where the function pointer for the dcload syscall is fetched from */
#define VEC_DCLOAD        (MEM_AREA_P1_BASE | 0x0C004008)

/*
    This is the single syscall dcload provides. It is then multiplexed out based on the `cmd`
    parameter.
*/

int dcload_syscall_native(dcload_cmd_t cmd, void *param1, void *param2, void *param3) {
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
