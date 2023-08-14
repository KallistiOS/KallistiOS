/* KallistiOS ##version##

   kos/stdlib.h
   Copyright (C) 2023 Falco Girgis
*/

#ifndef _STDLIB_H_
#error "Do not include this file directly. Use <stdlib.h> instead."
#endif /* !_STDLIB_H_ */

#ifndef __KOS_STDLIB_H
#define __KOS_STDLIB_H

#if !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L) || (__cplusplus >= 201703L)

#include <kos/cdefs.h>

__BEGIN_DECLS

extern int posix_memalign(void **memptr, size_t alignment, size_t size);

#endif /* !defined(__STRICT_ANSI__) || (__STDC_VERSION__ >= 201112L) || (__cplusplus >= 201703L) */
#endif /* !__KOS_STDLIB_H */
