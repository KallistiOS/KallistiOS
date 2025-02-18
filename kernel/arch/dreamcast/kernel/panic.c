/* KallistiOS ##version##

   panic.c
   (c)2001 Megan Potter
*/

#include <stdio.h>
#include <arch/arch.h>

/* If something goes badly wrong in the kernel and you don't think you
   can recover, call this. This is a pretty standard tactic from *nixy
   kernels which ought to be avoided if at all possible. */
void arch_panic(const char *msg) {
    printf("\nkernel panic: %s\r\n", msg);
    arch_abort();
}
