/* KallistiOS ##version##

   arch/xbox/include/arch/tls_static.h
   Copyright (C) 2026 The KallistiOS Contributors
*/

/** \file    arch/tls_static.h
    \brief   Xbox compiler thread-local storage integration.
    \ingroup kthreads

    The i686-pc-xbox GCC target lowers C/C++ thread-local variables through
    libgcc's emutls runtime. That runtime is configured for KOS and stores its
    per-thread vector with kthread_key_create()/kthread_getspecific(), so Xbox
    does not require a segment-register thread pointer or an ELF TLS block.
*/

#ifndef __ARCH_TLS_STATIC_H
#define __ARCH_TLS_STATIC_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>
#include <stddef.h>

#include <kos/thread.h>

void arch_tls_init(void);
bool arch_tls_setup_data(kthread_t *thread);
void arch_tls_destroy_data(kthread_t *thread);

/** Return zero because emutls variables do not occupy one contiguous static
    TLS image with a meaningful module-relative data offset. */
size_t arch_tls_data_offset(void);

__END_DECLS

#endif /* __ARCH_TLS_STATIC_H */
