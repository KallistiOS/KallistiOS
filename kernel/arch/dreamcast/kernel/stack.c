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
#include <stdbool.h>

static uintptr_t arch_stack_16m_dft = 0x8d000000;
static uintptr_t arch_stack_32m_dft = 0x8e000000;

extern uintptr_t arch_stack_16m __attribute__((weak,alias("arch_stack_16m_dft")));
extern uintptr_t arch_stack_32m __attribute__((weak,alias("arch_stack_32m_dft")));

/* Resolve the scan end for a stack walk.

   When SP falls within the current thread's stack, use the actual end of that
   stack so repeated unwind steps remain within one fixed stack window.
   Otherwise, fall back to a conservative kernel-stack-sized scan. */
static uintptr_t arch_stk_scan_end(uintptr_t sp) {
    uintptr_t scan_end = sp + THD_KERNEL_STACK_SIZE;

    if(thd_current && thd_current->stack && thd_current->stack_size) {
        uintptr_t stack_base = (uintptr_t)thd_current->stack;
        uintptr_t stack_end = stack_base + thd_current->stack_size;

        if(sp >= stack_base && sp < stack_end)
            scan_end = stack_end;
    }

    if(scan_end > _arch_mem_top)
        scan_end = _arch_mem_top;

    return scan_end;
}

/* Check if an address looks like a SH4 return address from a call.

   Validates that the instruction at (ret_addr - 4) is a BSR, BSRF, or JSR.
   This filters out false positives when scanning for saved PR values on
   the stack. */
static bool arch_is_call_return(uintptr_t ret_addr) {
    uint16_t insn;
    uintptr_t text_start = (uintptr_t)&_executable_start;
    uintptr_t text_end = (uintptr_t)&_etext;

    /* Must be aligned, within .text, and far enough in to read the
       preceding call instruction at ret_addr - 4. */
    if((ret_addr & 1) || ret_addr < text_start + 4 || ret_addr >= text_end)
        return false;

    /* Grab call instruction */
    insn = *(const uint16_t *)(ret_addr - 4);

    return ((insn & 0xf000) == 0xb000)      /* BSR */
        || ((insn & 0xf0ff) == 0x0003)      /* BSRF */
        || ((insn & 0xf0ff) == 0x400b);     /* JSR */
}

/* Find the next return address by scanning the stack upward from sp.

   Searches for saved PR values validated by arch_is_call_return().
   On success, sets *ret_addr_out to the found return address and
   *next_sp_out to the stack position to continue scanning from. */
static bool arch_stk_unwind_step_bounded(uintptr_t sp, uintptr_t scan_end,
                                         uintptr_t *ret_addr_out,
                                         uintptr_t *next_sp_out) {
    uintptr_t addr;

    if(!ret_addr_out || !next_sp_out)
        return false;

    /* Initialize outputs to safe defaults. */
    *ret_addr_out = 0;
    *next_sp_out = 0xffffffff;

    /* 0xffffffff is the sentinel for end-of-chain. */
    if(sp == 0xffffffff)
        return false;

    /* SP must be word-aligned and within usable Dreamcast RAM. */
    if((sp & 3) || sp < page_phys_base || sp > _arch_mem_top)
        return false;

    if(scan_end > _arch_mem_top)
        scan_end = _arch_mem_top;

    if(sp >= scan_end)
        return false;

    /* Walk each word on the stack looking for a saved PR value that
       points to just after a call instruction. */
    for(addr = sp; addr < scan_end; addr += 4) {
        uintptr_t candidate = *(uintptr_t *)addr;

        if(!arch_is_call_return(candidate))
            continue;

        /* Found a valid return address; report it and tell the caller
           where to resume scanning. */
        *ret_addr_out = candidate;
        *next_sp_out = addr + 4;
        return true;
    }

    /* Exhausted the scan range without finding a return address. */
    return false;
}

bool arch_stk_unwind_step(uintptr_t sp, uintptr_t *ret_addr_out,
                      uintptr_t *next_sp_out) {
    return arch_stk_unwind_step_bounded(sp, arch_stk_scan_end(sp),
                                        ret_addr_out, next_sp_out);
}

/* This function is unnecessary and does nothing on Dreamcast */
void arch_stk_setup(kthread_t *nt) {
    (void)nt;
}

/* Do a stack trace from the current function; leave off the first n frames
   (i.e., in assert()). */
__noinline void arch_stk_trace(int n) {
    register uintptr_t sp asm("r15");

    /* Keep previous behaviour: arch_stk_trace skipped one extra frame
       (the tracer itself), so pass n+1 to arch_stk_trace_at. */
    arch_stk_trace_at(sp, n + 1);
}

/* Do a stack trace from the given stack pointer. Leave off the first
   n frames. */
void arch_stk_trace_at(uintptr_t sp, size_t n) {
    const uint32_t max_frames = 25;
    uintptr_t ret_addr;
    uintptr_t next_sp;
    uintptr_t scan_end = arch_stk_scan_end(sp);
    uint32_t printed = 0;

    dbgio_printf("-------- Stack Trace (innermost first) ---------\n");

    while(arch_stk_unwind_step_bounded(sp, scan_end, &ret_addr, &next_sp)) {
        if(n) {
            --n;
            sp = next_sp;
            continue;
        }

        dbgio_printf("   %08lx\n", (unsigned long)ret_addr);
        ++printed;

        if(printed >= max_frames) {
            dbgio_printf("   ... (stack scan truncated)\n");
            break;
        }

        sp = next_sp;
    }

    if(printed == 0)
        dbgio_printf("   (no frames found)\n");

    dbgio_printf("-------------- End Stack Trace -----------------\n");
}
