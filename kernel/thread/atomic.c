/* KallistiOS ##version##

   kernel/thread/atomic.c
   Copyright (C) 2024 Paul Cercueil <paul@crapouillou.net>
*/

/* Lock-less compare-and-swap implementation */

#include <kos/atomic.h>

#include "cas.h"

uint32_t compare_and_swap(uint32_t *addr, uint32_t old, uint32_t new)
{
	return _sh_cas_atomic_begin(addr, old, new);
}
