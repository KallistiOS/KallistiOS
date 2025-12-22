/* KallistiOS ##version##

   exec.c
   Copyright (C) 2002 Megan Potter
*/

#include <assert.h>

#include <arch/arch.h>
#include <arch/cache.h>
#include <arch/exec.h>
#include <arch/irq.h>
#include <dc/memory.h>

/* Pull the shutdown function in from init.c */
void arch_shutdown();

/* Pull these in from execasm.s */
extern uint32_t _arch_exec_template[];
extern uint32_t _arch_exec_template_values[];
extern uint32_t _arch_exec_template_end[];

/* Pull this in from startup.s */
extern uint32_t _arch_old_sr, _arch_old_vbr, _arch_old_stack, _arch_old_fpscr;

/* Replace the currently running image with whatever is at
   the pointer; note that this call will never return. */
void arch_exec_at(const void *image, uint32_t length, uint32_t address) {
    /* Find the start/end of the trampoline template and make a stack
       buffer of that size */
    uintptr_t   tstart = (uintptr_t)_arch_exec_template,
                tend   = (uintptr_t)_arch_exec_template_end,
                tvals  = (uintptr_t)_arch_exec_template_values;
    size_t      tcount = (tend - tstart) / 4;
    uint32_t    buffer[tcount];
    uint32_t    *values = &buffer[(tvals - tstart) / 4];
    size_t      i;

    assert((tend - tstart) % 4 == 0);

    /* Turn off interrupts */
    irq_disable();

    /* Flush the data cache for the source area */
    dcache_flush_range((uintptr_t)image, length);

    /* Copy over the trampoline */
    for(i = 0; i < tcount; i++)
        buffer[i] = _arch_exec_template[i];

    /* Plug in values */
    values[0] = (uintptr_t)image;   /* Source */
    values[1] = address;            /* Destination */
    values[2] = length / 4;         /* Length in uint32's */
    values[3] = _arch_old_stack;    /* Patch in old R15 */

    /* Flush both caches for the trampoline area */
    dcache_flush_range((uintptr_t)buffer, tcount * 4);
    icache_flush_range((uintptr_t)buffer, tcount * 4);

    /* Shut us down */
    arch_shutdown();

    /* Reset our old SR, VBR, and FPSCR */
    __asm__ __volatile__("ldc	%0,sr\n"
                         : /* no outputs */
                         : "z"(_arch_old_sr)
                         : "memory");
    __asm__ __volatile__("ldc	%0,vbr\n"
                         : /* no outputs */
                         : "z"(_arch_old_vbr)
                         : "memory");
    __asm__ __volatile__("lds	%0,fpscr\n"
                         : /* no outputs */
                         : "z"(_arch_old_fpscr)
                         : "memory");

    /* Jump to the trampoline */
    {
        typedef void (*trampoline_func)(void) __noreturn;
        trampoline_func trampoline = (trampoline_func)buffer;

        trampoline();
    }
}

void arch_exec(const void *image, uint32_t length) {
    arch_exec_at(image, length, MEM_AREA_P2_BASE | 0x0c010000);
}
