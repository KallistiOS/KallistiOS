/* KallistiOS ##version##

   arch/xbox/kernel/tls_static.c
   Copyright (C) 2026 The KallistiOS Contributors
*/

#include <arch/tls_static.h>

#include <stdbool.h>
#include <stddef.h>

void arch_tls_init(void) {
    /* libgcc emutls allocates its per-thread vector lazily through KOS keys. */
}

bool arch_tls_setup_data(kthread_t *thread) {
    /* There is no eagerly allocated ELF TLS block on this GCC target. */
    thread->tls_hnd = NULL;
    return true;
}

void arch_tls_destroy_data(kthread_t *thread) {
    /* emutls registered a KOS key destructor for its lazily allocated vector.
       kthread's normal TLS destructor pass releases it before this callback. */
    thread->tls_hnd = NULL;
}

size_t arch_tls_data_offset(void) {
    return 0;
}
