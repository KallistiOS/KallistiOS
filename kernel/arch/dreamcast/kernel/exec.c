/* KallistiOS ##version##

   exec.c
   Copyright (C) 2002 Megan Potter
*/

#include <arch/arch.h>
#include <arch/exec.h>
#include <arch/irq.h>
#include <arch/cache.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

/* Pull the shutdown function in from main.c */
void arch_shutdown();

/* Pull these in from execasm.s */
extern uint32 _arch_exec_template[];
extern uint32 _arch_exec_template_values[];
extern uint32 _arch_exec_template_end[];

/* Pull this in from startup.s */
extern uint32 _arch_old_sr, _arch_old_vbr, _arch_old_stack, _arch_old_fpscr;

/* Replace the currently running image with whatever is at
   the pointer; note that this call will never return. */
void arch_exec_at(const void *image, uint32 length, uint32 address) {
    /* Find the start/end of the trampoline template and make a stack
       buffer of that size */
    uint32  tstart = (uint32)_arch_exec_template,
            tend = (uint32)_arch_exec_template_end;
    uint32  tcount = (tend - tstart) / 4;
    uint32  buffer[tcount];
    uint32  * values;
    uint32 i;

    assert((tend - tstart) % 4 == 0);

    /* Turn off interrupts */
    irq_disable();

    /* Flush the data cache for the source area */
    dcache_flush_range((uint32)image, length);

    /* Copy over the trampoline */
    for(i = 0; i < tcount; i++)
        buffer[i] = _arch_exec_template[i];

    /* Plug in values */
    values = buffer + (_arch_exec_template_values - _arch_exec_template);
    values[0] = (uint32)image;  /* Source */
    values[1] = address;        /* Destination */
    values[2] = length / 4;     /* Length in uint32's */
    values[3] = _arch_old_stack;    /* Patch in old R15 */

    /* Flush both caches for the trampoline area */
    dcache_flush_range((uint32)buffer, tcount * 4);
    icache_flush_range((uint32)buffer, tcount * 4);

    /* printf("Finished trampoline:\n");
    for (i=0; i<tcount; i++) {
        printf("%08x: %08x\n", (uint32)(buffer + i), buffer[i]);
    } */

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
        typedef void (*trampoline_func)() __noreturn;
        trampoline_func trampoline = (trampoline_func)buffer;

        trampoline();
    }
}

void arch_exec(const void *image, uint32 length) {
    arch_exec_at(image, length, 0xac010000);
}
