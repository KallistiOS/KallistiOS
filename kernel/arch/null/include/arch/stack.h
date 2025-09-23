/* KallistiOS ##version##

   arch/null/include/arch/stack.h
   (c)2002 Megan Potter
   (c)2025 Eric Fradella

*/

/** \file    arch/stack.h
    \brief   Stack functions
    \ingroup debugging_stacktrace

    This file contains arch-specific stack implementations, including defining
    stack sizes and alignments, as well as functions for stack tracing and
    debugging.

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

/* Please note that all of the following frame pointer macros are ONLY
   valid if you have compiled your code WITHOUT -fomit-frame-pointer. These
   are mainly useful for getting a stack trace from an error. */

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

