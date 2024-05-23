/* KallistiOS ##version##

   kernel/thread/cas.h
   Copyright (C) 2024 Paul Cercueil
*/

#ifndef __KOS_KERNEL_CAS_H
#define __KOS_KERNEL_CAS_H

#include <stdint.h>

/* Compare-and-swap labels exported from cas.s */
extern uint32_t _sh_cas_atomic_begin(uint32_t *addr, uint32_t old, uint32_t new);
extern uint32_t _sh_cas_atomic_end;

#endif /* __KOS_KERNEL_CAS_H */
