/* KallistiOS ##version##

   arch/xbox/kernel/stack.c
   Copyright (C) 2026 The KallistiOS Contributors
*/

#include <arch/arch.h>
#include <arch/stack.h>

#include <kos/dbgio.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool stack_bounds_for_frame(uintptr_t frame, uintptr_t *base_out,
                                   uintptr_t *end_out) {
    uintptr_t base;
    uintptr_t end;

    if(thd_current && thd_current->stack && thd_current->stack_size) {
        base = (uintptr_t)thd_current->stack;
        end = base + thd_current->stack_size;

        if(frame >= base && frame < end) {
            *base_out = base;
            *end_out = end;
            return true;
        }
    }

    base = (uintptr_t)arch_stack_base;
    end = (uintptr_t)arch_stack_top;
    if(frame >= base && frame < end) {
        *base_out = base;
        *end_out = end;
        return true;
    }

    return false;
}

bool arch_stk_unwind_step(uintptr_t frame, uintptr_t *ret_addr_out,
                          uintptr_t *next_frame_out) {
    uintptr_t stack_base;
    uintptr_t stack_end;
    uintptr_t next;
    uintptr_t ret;

    if(!ret_addr_out || !next_frame_out)
        return false;

    *ret_addr_out = 0;
    *next_frame_out = 0;

    if((frame & (sizeof(uintptr_t) - 1U)) ||
       !stack_bounds_for_frame(frame, &stack_base, &stack_end) ||
       frame > stack_end - 2U * sizeof(uintptr_t))
        return false;

    (void)stack_base;
    next = arch_fptr_next(frame);
    ret = arch_fptr_ret_addr(frame);

    /* Older frames must be at higher addresses on a downward-growing stack.
       The return address must point into this image's executable text. */
    if(next <= frame || (next & (sizeof(uintptr_t) - 1U)) ||
       next > stack_end - 2U * sizeof(uintptr_t) ||
       ret < (uintptr_t)&_executable_start ||
       ret >= (uintptr_t)&_etext)
        return false;

    *ret_addr_out = ret;
    *next_frame_out = next;
    return true;
}

void arch_stk_setup(kthread_t *thread) {
    uintptr_t base;

    if(!thread || !thread->stack ||
       thread->stack_size < 2U * sizeof(uintptr_t))
        arch_panic("Invalid Xbox thread stack");

    base = (uintptr_t)thread->stack;

    if(base & (THD_STACK_ALIGNMENT - 1U))
        arch_panic("Unaligned Xbox thread stack");

    *(uint32_t *)base = XBOX_STACK_CANARY;
}

bool arch_stk_check(const kthread_t *thread) {
    if(!thread || !thread->stack ||
       thread->stack_size < 2U * sizeof(uintptr_t))
        return false;

    return *(const uint32_t *)thread->stack == XBOX_STACK_CANARY;
}

void arch_stk_trace(int skip) {
    arch_stk_trace_at(arch_get_fptr(), (size_t)skip);
}

void arch_stk_trace_at(uintptr_t frame, size_t skip) {
    uintptr_t ret;
    uintptr_t next;
    unsigned int count = 0;

    dbgio_printf("-------- Stack Trace (innermost first) ---------\n");

    while(count < 32U &&
          arch_stk_unwind_step(frame, &ret, &next)) {
        if(skip)
            --skip;
        else {
            dbgio_printf("   %08lx\n", (unsigned long)ret);
            ++count;
        }

        frame = next;
    }

    if(count == 0)
        dbgio_printf("   (no frames found)\n");
    else if(count == 32U)
        dbgio_printf("   ... (stack trace truncated)\n");

    dbgio_printf("-------------- End Stack Trace -----------------\n");
}
