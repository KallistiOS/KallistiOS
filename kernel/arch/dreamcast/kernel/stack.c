/* KallistiOS ##version##

   stack.c
   (c)2002 Megan Potter
*/

/* Functions to tinker with the stack, includeing obtaining a stack
   trace when frame pointers are enabled. If frame pointers are enabled,
   then you'll need to also define FRAME_POINTERS to get support for stack
   traces.

   We could probably technically move this into arch indep with a bit more
   work... */

#include <kos/dbgio.h>
#include <arch/arch.h>
#include <arch/stack.h>

/* Do a stack trace from the current function; leave off the first n frames
   (i.e., in assert()). */
void arch_stk_trace(int n) {
    arch_stk_trace_at(arch_get_fptr(), n + 1);
}

/* Do a stack trace from the given frame pointer (useful for things like
   tracing from an ISR); leave off the first n frames. */
void arch_stk_trace_at(uint32 fp, int n) {
#ifdef FRAME_POINTERS
    dbgio_printf("-------- Stack Trace (innermost first) ---------\n");

    while(fp != 0xffffffff) {
        if((fp & 3) || (fp < 0x8c000000) || (fp > _arch_mem_top)) {
            dbgio_printf("   (invalid frame pointer)\n");
            break;
        }

        if(n <= 0) {
            dbgio_printf("   %08lx\n", arch_fptr_ret_addr(fp));
        }
        else n--;

        fp = arch_fptr_next(fp);
    }

    dbgio_printf("-------------- End Stack Trace -----------------\n");
#else
    (void)fp;
    (void)n;
    dbgio_printf("Stack Trace: frame pointers not enabled!\n");
#endif
}

