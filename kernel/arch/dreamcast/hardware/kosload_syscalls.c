/* KallistiOS ##version##

   kosload_syscalls.c

   Copyright (C) 2026 Andy Barajas

*/

#include <kos/kosload.h>

#include <dc/fifo.h>
#include <dc/memory.h>

#include <kos/irq.h>

/* This is the address where the function pointer for the kosload syscall is fetched from */
#define VEC_KOSLOAD       (MEM_AREA_P1_BASE | 0x0C004008)

/*
    This is the single syscall kosload provides. It is then multiplexed out based on the `cmd`
    parameter.
*/

int kosload_syscall_native(kosload_cmd_t cmd, void *param1, void *param2, void *param3) {
    uintptr_t *syscall_ptr = (uintptr_t *)VEC_KOSLOAD;
    int (*syscall)() = (int (*)())(*syscall_ptr);

    /* Disable IRQs until the syscall returns */
    irq_disable_scoped();

    /* Ensure that the FIFO buffer is clear */
    /* XXX - Is this needed? It seems like something only for serial. */
    while(FIFO_STATUS & FIFO_SH4)            ;

    /* Make the call */
    return syscall(cmd, param1, param2, param3);
}
