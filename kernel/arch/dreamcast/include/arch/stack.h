/* KallistiOS ##version##

   arch/dreamcast/include/arch/stack.h
   (c)2002 Megan Potter
   (c)2025 Eric Fradella

*/

/** \file    arch/stack.h
    \brief   Stack functions
    \ingroup debugging_stacktrace

    This file contains arch-specific stack implementations, including defining
    stack sizes and alignments, as well as functions for stack tracing and
    debugging. On Dreamcast, the stack tracing functions use heuristic stack
    pointer scanning rather than frame pointer chains, because GCC for SH4
    does not reliably maintain r14-based frame pointers, even with
    -fno-omit-frame-pointer.

    \author Megan Potter
    \author Eric Fradella
*/

#ifndef __ARCH_STACK_H
#define __ARCH_STACK_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <kos/thread.h>

/** \defgroup debugging_stacktrace  Stack Traces
    \brief                          API for managing stack backtracing
    \ingroup                        debugging

    @{
*/

#ifndef THD_STACK_ALIGNMENT
/** \brief  Required alignment for stack. */
#define THD_STACK_ALIGNMENT 8
#endif

#ifndef THD_STACK_SIZE
/** \brief  Default thread stack size. */
#define THD_STACK_SIZE  32768
#endif

#ifndef THD_KERNEL_STACK_SIZE
/** \brief Main/kernel thread's stack size. */
#define THD_KERNEL_STACK_SIZE (64 * 1024)
#endif

/** \brief   DC specific "function" to get the return address from the current
             function.

    \return                 The return address of the current function.
*/
static __always_inline uintptr_t arch_get_ret_addr(void) {
    uintptr_t pr;

    __asm__ __volatile__("sts pr,%0\n" : "=r"(pr));

    return pr;
}

/* Please note that all of the following frame pointer macros are ONLY
   valid if you have compiled your code WITHOUT -fomit-frame-pointer. These
   are mainly useful for getting a stack trace from an error. */

/** \brief   DC specific "function" to get the frame pointer from the current
             function.

    \return                 The frame pointer from the current function.
    \note                   This only works if you don't disable frame pointers.
*/
static __always_inline uintptr_t arch_get_fptr(void) {
    register uintptr_t fp __asm__("r14");

    return fp;
}

/** \brief   Pass in a frame pointer value to get the return address for the
             given frame.

    \param  fptr            The frame pointer to look at.
    \return                 The return address of the pointer.
*/
static inline uintptr_t arch_fptr_ret_addr(uintptr_t fptr) {
    return *(uintptr_t *)fptr;
}

/** \brief   Pass in a frame pointer value to get the previous frame pointer for
             the given frame.

    \param  fptr            The frame pointer to look at.
    \return                 The previous frame pointer.
*/
static inline uintptr_t arch_fptr_next(uintptr_t fptr) {
    return arch_fptr_ret_addr(fptr + 4);
}

/** \brief   Find the next return address by scanning the stack.

    Scans upward from the given stack pointer for saved PR values
    validated as return addresses from call instructions.

    \param  sp              Stack pointer to start scanning from.
    \param  ret_addr_out    On success, receives the found return address.
    \param  next_sp_out     On success, receives the stack position to
                            continue scanning from.
    \return                 `true` if a return address was found.
*/
bool arch_stk_unwind_step(uintptr_t sp, uintptr_t *ret_addr_out,
                      uintptr_t *next_sp_out);

/** \brief  Set up new stack before running.

    This function does nothing as it is unnecessary on Dreamcast.

    \param nt               A pointer to the new thread for which a stack
                            is to be set up.
*/
void arch_stk_setup(kthread_t *nt);

/** \brief  Do a stack trace from the current function.

    This function does a stack trace from the current function, printing the
    results to stdout. This is used, for instance, when an assertion fails in
    assert().

    \param  n               The number of frames to leave off. Each frame is a
                            jump to subroutine or branch to subroutine. assert()
                            leaves off 2 frames, for reference.

    \see    arch_stk_trace_at
*/
void arch_stk_trace(int n);

/** \brief  Do a stack trace from the given stack pointer.

    This function does a stack trace from the specified stack pointer,
    printing the results to stdout. This could be used for doing something like
    stack tracing a main thread from inside an IRQ handler.

    \param  sp              The stack pointer to start scanning from.
    \param  n               The number of frames to leave off.
*/
void arch_stk_trace_at(uintptr_t sp, size_t n);

/** @} */

__END_DECLS

#endif  /* __ARCH_EXEC_H */
