/* KallistiOS ##version##

   stack.c
   (c)2002 Megan Potter
*/

/* Functions to tinker with the stack, including obtaining a stack
   trace when frame pointers are enabled. If frame pointers are enabled,
   then you'll need to also define FRAME_POINTERS to get support for stack
   traces.

   We could probably technically move this into arch indep with a bit more
   work... */

#include <kos/dbgio.h>
#include <arch/arch.h>
#include <arch/stack.h>
#include <stdint.h>

static uintptr_t arch_stack_16m_dft = 0x8d000000;
static uintptr_t arch_stack_32m_dft = 0x8e000000;

extern uintptr_t arch_stack_16m __attribute__((weak,alias("arch_stack_16m_dft")));
extern uintptr_t arch_stack_32m __attribute__((weak,alias("arch_stack_32m_dft")));

/* Do a stack trace from the current function; leave off the first n frames
   (i.e., in assert()). */
void arch_stk_trace(int n) {
    arch_stk_trace_at(arch_get_fptr(), n + 1);
}

/* Do a stack trace from the given frame pointer (useful for things like
   tracing from an ISR); leave off the first n frames. */
void arch_stk_trace_at(uint32_t fp, size_t n) {
    if(!__is_defined(FRAME_POINTERS)) {
        dbgio_printf("Stack Trace: frame pointers not enabled!\n");
        return;
    }

    dbgio_printf("-------- Stack Trace (innermost first) ---------\n");

    while(fp != 0xffffffff) {
        /* Validate the function pointer (fp) */
        if((fp & 3) || (fp < 0x8c000000) || (fp > _arch_mem_top)) {
            dbgio_printf("   (invalid frame pointer)\n");
            break;
        }
        
        if(n == 0) {
            /* Get the return address from the function pointer */
            fp = arch_fptr_ret_addr(fp);

            /* Validate the return address */
            if(!arch_valid_address(fp)) {
                dbgio_printf("   %08lx   (invalid return address)\n", fp);
                break;
            } else
                dbgio_printf("   %08lx\n", fp);
        }
        else n--;

        fp = arch_fptr_next(fp);
    }

    dbgio_printf("-------------- End Stack Trace -----------------\n");
}

