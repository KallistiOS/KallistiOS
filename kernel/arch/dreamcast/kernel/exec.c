/* KallistiOS ##version##

   exec.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

#include <arch/arch.h>
#include <arch/exec.h>
#include <arch/irq.h>
#include <arch/cache.h>
#include <arch/memory.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Pull the shutdown function in from main.c */
void arch_shutdown(void);

/* Pull these in from execasm.s */
extern uint32_t _arch_exec_template[];
extern uint32_t _arch_exec_template_values[];
extern uint32_t _arch_exec_template_end[];

/* Pull this in from startup.s */
extern uint32_t _arch_old_sr, _arch_old_vbr, _arch_old_stack, _arch_old_fpscr;
extern uint32_t _executable_start;
extern int _argc;
extern char **_argv;

/* Replace the currently running image with whatever is at
   the pointer; note that this call will never return. */
__noreturn
static void arch_exec_at_with_argsv(const void *image, size_t length, 
                                    uintptr_t address, va_list *var_args) {
    /* Find the start/end of the trampoline template and make a stack
       buffer of that size */
    uint32_t tstart = (uint32_t)_arch_exec_template,
             tend   = (uint32_t)_arch_exec_template_end;
    uint32_t tcount = (tend - tstart) / 4;
    uint32_t buffer[tcount];
    uint32_t *values;
    uint32_t i;
    va_list var_args2;
    const char* arg;

    assert((tend - tstart) % 4 == 0);

    /* Turn off interrupts */
    irq_disable();

    _argc = 0;
    _argv = NULL;

    va_copy(var_args2, *var_args);
    while((arg = va_arg(*var_args, const char*)))
        ++_argc;
    va_end(*var_args);

    _argv = alloca(sizeof(const char *) * _argc);
    i = 0;
    while((arg = va_arg(var_args2, const char*))) {
        _argv[i] = alloca(strlen(arg) + 1);
        strcpy(_argv[i], arg);
        ++i;
    }
    va_end(var_args2);

    /* Flush the data cache for the source area */
    dcache_flush_range((uintptr_t)image, length);

    /* Copy over the trampoline */
    memcpy(buffer, _arch_exec_template, tcount * sizeof(uint32_t));

    /* Plug in values */
    values = buffer + (_arch_exec_template_values - _arch_exec_template);
    values[0] = (uint32_t)image;  /* Source */
    values[1] = address;        /* Destination */
    values[2] = length / 4;     /* Length in uint32_t's */
    values[3] = _arch_old_stack;    /* Patch in old R15 */

    /* Flush both caches for the trampoline area */
    dcache_flush_range((uintptr_t)buffer, tcount * 4);
    icache_flush_range((uintptr_t)buffer, tcount * 4);

    /* printf("Finished trampoline:\n");
    for(i=0; i<tcount; i++) {
        printf("%08x: %08x\n", (uint32_t)(buffer + i), buffer[i]);
    } */

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

void arch_exec_at_with_args(const void *image, size_t length, uintptr_t address, ...) {
    va_list var_args;
    va_start(var_args, address);
    arch_exec_at_with_argsv(image, length, address, &var_args);
}

void arch_exec_at(const void *image, size_t length, uintptr_t address) {
    arch_exec_at_with_args(image, length, address, NULL);
}

void arch_exec_with_args(const void *image, size_t length, ...) {
    va_list var_args;
    va_start(var_args, length);
    arch_exec_at_with_argsv(image, length, arch_exec_address(), &var_args);
}

void arch_exec(const void *image, size_t length) {
    arch_exec_at(image, length, arch_exec_address());
}

uintptr_t arch_exec_address(void) {
    return (uintptr_t)&_executable_start | MEM_AREA_P2_BASE;
}
