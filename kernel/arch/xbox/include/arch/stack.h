/* KallistiOS ##version##

   arch/xbox/include/arch/stack.h
   Copyright (C) 2026 The KallistiOS Contributors
*/

/** \file    arch/stack.h
    \brief   Original Xbox thread-stack and stack-unwinding support.
    \ingroup debugging_stacktrace
*/

#ifndef __ARCH_STACK_H
#define __ARCH_STACK_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kos/thread.h>

/** GCC's i386 ABI uses a preferred 16-byte stack boundary. */
#ifndef THD_STACK_ALIGNMENT
#define THD_STACK_ALIGNMENT 16U
#endif

/** Default stack allocated for an application thread. */
#ifndef THD_STACK_SIZE
#define THD_STACK_SIZE (32U * 1024U)
#endif

/** Stack reservation associated with the initial kernel thread. */
#ifndef THD_KERNEL_STACK_SIZE
#define THD_KERNEL_STACK_SIZE (256U * 1024U)
#endif

/** Internal scheduler threads need room for the i386 ABI, FPU-enabled
    interrupt paths, and portable wait/reaping call chains. */
#define THD_IDLE_STACK_SIZE (4U * 1024U)
#define THD_REAPER_STACK_SIZE (4U * 1024U)

/** Linker-reserved stack on which startup.S enters arch_main(). */
extern uint8_t arch_stack_base[];
extern uint8_t arch_stack_top[];

/** Override the portable Dreamcast-era assumption that the initial stack is
    always the final THD_KERNEL_STACK_SIZE bytes below _arch_mem_top. */
#define THD_KERNEL_STACK_BASE ((void *)arch_stack_base)

/** Canary stored at the low end of newly allocated thread stacks. */
#define XBOX_STACK_CANARY 0x4b4f5358U

/** Return address saved by the current function's caller. */
static __always_inline uintptr_t arch_get_ret_addr(void) {
    return (uintptr_t)__builtin_return_address(0);
}

/** Current EBP frame pointer.

    Xbox KOS is compiled with -fno-omit-frame-pointer so an ordinary frame is:

        [EBP + 0] previous EBP
        [EBP + 4] return address
*/
static __always_inline uintptr_t arch_get_fptr(void) {
    uintptr_t frame;

    __asm__ volatile("movl %%ebp, %0" : "=r"(frame));
    return frame;
}

/** Return address stored in an i386 frame. */
static inline uintptr_t arch_fptr_ret_addr(uintptr_t frame) {
    return *(const uintptr_t *)(frame + sizeof(uintptr_t));
}

/** Previous (caller's) i386 frame pointer. */
static inline uintptr_t arch_fptr_next(uintptr_t frame) {
    return *(const uintptr_t *)frame;
}

/** Validate and advance one EBP frame-chain entry. */
bool arch_stk_unwind_step(uintptr_t frame, uintptr_t *ret_addr_out,
                          uintptr_t *next_frame_out);

/** Install architecture stack metadata after a new context is constructed. */
void arch_stk_setup(kthread_t *thread);

/** Check architecture stack metadata without modifying the thread. */
bool arch_stk_check(const kthread_t *thread);

/** Trace the current EBP frame chain, omitting the first \p skip frames. */
void arch_stk_trace(int skip);

/** Trace an EBP frame chain beginning at \p frame. */
void arch_stk_trace_at(uintptr_t frame, size_t skip);

__END_DECLS

#endif /* __ARCH_STACK_H */
