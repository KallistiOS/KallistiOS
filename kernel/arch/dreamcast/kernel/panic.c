/* KallistiOS ##version##

   panic.c
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2023 Falco Girgis
*/

#include <stdio.h>
#include <malloc.h>
#include <arch/arch.h>

/* If something goes badly wrong in the kernel and you don't think you
   can recover, call this. This is a pretty standard tactic from *nixy
   kernels which ought to be avoided if at all possible. */
void arch_panic(const char *msg) {
    fprintf(stderr, "kernel panic: %s\r\n", msg);
    arch_abort();
}
