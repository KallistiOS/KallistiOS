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
    debugging. On Dreamcast, the stack tracing functions only work if frame
    pointers have been enabled at compile time (-DFRAME_POINTERS and no
    -fomit-frame-pointer flag).

    \author Megan Potter
    \author Eric Fradella
*/

#ifndef __ARCH_STACK_H
#define __ARCH_STACK_H

#include <sys/cdefs.h>
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
*/
void arch_stk_trace(int n);

/** \brief  Do a stack trace from the current function.

    This function does a stack trace from the the specified frame pointer,
    printing the results to stdout. This could be used for doing something like
    stack tracing a main thread from inside an IRQ handler.

    \param  fp              The frame pointer to start from.
    \param  n               The number of frames to leave off.
*/
void arch_stk_trace_at(uint32_t fp, size_t n);

/** @} */

__END_DECLS

#endif  /* __ARCH_EXEC_H */

