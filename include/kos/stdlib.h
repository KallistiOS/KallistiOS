/* KallistiOS ##version##

   kos/stdlib.h
   Copyright (C) 2023 Falco Girgis
*/

/* 
   Add select POSIX extensions to stdlib.h which are not present within Newlib.
*/ 

#ifndef _STDLIB_H_
#error "Do not include this file directly. Use <stdlib.h> instead."
#endif /* !_STDLIB_H_ */

#ifndef __KOS_STDLIB_H
#define __KOS_STDLIB_H

#if !defined(__STRICT_ANSI__)

#include <kos/cdefs.h>

__BEGIN_DECLS

extern int posix_memalign(void **memptr, size_t alignment, size_t size);

__END_DECLS

#endif /* !defined(__STRICT_ANSI__) */
#endif /* !__KOS_STDLIB_H */
