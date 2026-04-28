/* KallistiOS ##version##

   arch/dreamcast/include/arch/tls_static.h
   Copyright (C) 2025 Donald Haase

*/

/** \file    arch/tls_static.h
    \brief   Compiler Thread-local storage.
    \ingroup kthreads

    The functions in this file deal with managing static
    TLS for C99's __thread or C11's _Thread_local.

    This should not be exported or accessed externally.

    \author Donald Haase
*/

#ifndef __ARCH_TLS_STATIC_H
#define __ARCH_TLS_STATIC_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <kos/thread.h>

/** \brief  Initialize tls.

    This function takes the steps necessary to initialize tls after
    the creation of the k_thread for the main kernel thread.

    For SH, this forces the setting of the GBR register to the TLS
    of the kernel thread. That isn't necessary afterwards as it is
    handled during context switching.

*/
void arch_tls_init(void);

/** \brief  Set up tls data for a new kthread.

    This function allocates the memory for a kthread's TLS,
    copies static data from the executable, and sets the
    kthread's IRQ context to match.

    It will be called by `thd_create_ex`

    \param  thd             The thread to setup tls data for.
*/
bool arch_tls_setup_data(kthread_t *thd);

/** \brief  Destroy tls data for an old kthread.

    This function undoes arch_tls_setup_data.

    It will be called by `thd_destroy`

    \param  thd             The thread to destroy tls data for.
*/
void arch_tls_destroy_data(kthread_t *thd);

/** \brief  Returns the byte offset to static TLS data.

    Computes the offset from the thread control block (TCB)
    pointer to where the thread's static TLS data begins. The
    offset accounts for the size of the TCB header.

    \return                 Byte offset to the static TLS data
                            block.
*/
size_t arch_tls_data_offset(void);

__END_DECLS

#endif  /* __ARCH_TLS_STATIC_H */

