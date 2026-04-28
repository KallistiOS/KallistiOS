/* KallistiOS ##version##

   stack.c
   (c)2002 Megan Potter
*/

/* Functions to tinker with the stack, including obtaining a stack
   trace when frame pointers are enabled. If frame pointers are enabled,
   then you'll need to also define FRAME_POINTERS to get support for stack
   traces. */

#include <kos/dbgio.h>
#include <arch/arch.h>
#include <arch/stack.h>
#include <stdint.h>

static uintptr_t arch_stack_16m_dft = 0x8d000000;
static uintptr_t arch_stack_32m_dft = 0x8e000000;

extern uintptr_t arch_stack_16m __attribute__((weak,alias("arch_stack_16m_dft")));
extern uintptr_t arch_stack_32m __attribute__((weak,alias("arch_stack_32m_dft")));

/* This function is unnecessary and does nothing on Dreamcast */
void arch_stk_setup(kthread_t *nt) {
    (void)nt;
}

/* Do a stack trace from the current function; leave off the first n frames
   (i.e., in assert()). */
__noinline void arch_stk_trace(int n) {
    arch_stk_trace_at(arch_get_fptr(), n + 1);
}

/* Do a stack trace from the given frame pointer (useful for things like
   tracing from an ISR); leave off the first n frames. */
void arch_stk_trace_at(uint32_t fp, size_t n) {
    uint32_t ret_addr;
    dbgio_printf("-------- Stack Trace (innermost first) ---------\n");
    if(__is_defined(FRAME_POINTERS)) {
        while(fp != 0xffffffff) {
            /* Validate the function pointer (fp) */
            if((fp & 3) || (fp < 0x8c000000) || (fp > _arch_mem_top)) {
                dbgio_printf("   %08lx   (invalid frame pointer)\n", fp);
                break;
            }

            if(n == 0) {
                /* Get the return address from the function pointer */
                ret_addr = arch_fptr_ret_addr(fp);

                /* Validate the return address */
                if(!arch_valid_address(ret_addr)) {
                    dbgio_printf("   %08lx   (invalid return address)\n", ret_addr);
                    break;
                } else
                    dbgio_printf("   %08lx\n", ret_addr);
            }
            else n--;

            fp = arch_fptr_next(fp);
        }
    } else {
        extern const char etext[];
        uint32 sp;
        int depth = 0;
        dbgio_printf("Frame pointers disabled, using heuristics\n");
        __asm__ __volatile__(
            "mov    r15,%0\n"
            : "=r" (sp)
            :
            : );
        if(!(sp & 3) && sp > 0x8c000000 && sp < _arch_mem_top) {
            char** sp_ptr = (char**)sp;
            for (int so = 0; so < 16384; so++) {
                if ((uintptr_t)(&sp_ptr[so]) >= _arch_mem_top) {
                    dbgio_printf("(@@%08X) ", (uintptr_t)&sp_ptr[so]);
                    break;
                }
                if (sp_ptr[so] > (char*)0x8c000000 && sp_ptr[so] < etext) {
                    uintptr_t addr = (uintptr_t)(sp_ptr[so]);
                    // candidate return pointer
                    if (addr & 1) {
                        continue;
                    }

                    uint16_t* instrp = (uint16_t*)addr;

                    uint16_t instr = instrp[-2];
                    // BSR or BSRF or JSR @Rn ?
                    if (((instr & 0xf000) == 0xB000) || ((instr & 0xf0ff) == 0x0003) || ((instr & 0xf0ff) == 0x400B)) {
                        dbgio_printf("%08X ", (uintptr_t)instrp);
                        if (depth++ > 24) {
                            dbgio_printf("(@%08X) ", (uintptr_t)&sp_ptr[so]);
                            break;
                        }
                    }
                }
            }
            dbgio_printf("end\n");
        } else {
            dbgio_printf("(@%08X)\n", sp);
        }
    }
    dbgio_printf("-------------- End Stack Trace -----------------\n");
}

