/* KallistiOS ##version##

   arch/xbox/include/arch/types.h
   Copyright (C) 2026 The KallistiOS Contributors
*/

/** \file    arch/types.h
    \brief   Xbox architecture integer types.
    \ingroup system_types
*/

#ifndef __ARCH_TYPES_H
#define __ARCH_TYPES_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/** \addtogroup system_types
    @{
*/

/* Preserve KOS's legacy base types, including their printf-compatible C type
   identities. The i686 ABI makes both int and long 32 bits, but existing KOS
   code intentionally formats uint32/int32 as long. */
typedef unsigned long long uint64; /**< \brief 64-bit unsigned integer */
typedef unsigned long uint32;      /**< \brief 32-bit unsigned integer */
typedef unsigned short uint16;     /**< \brief 16-bit unsigned integer */
typedef unsigned char uint8;       /**< \brief 8-bit unsigned integer */
typedef long long int64;           /**< \brief 64-bit signed integer */
typedef long int32;                /**< \brief 32-bit signed integer */
typedef short int16;               /**< \brief 16-bit signed integer */
typedef char int8;                 /**< \brief 8-bit signed integer */

typedef volatile uint64 vuint64; /**< \brief Volatile 64-bit unsigned type */
typedef volatile uint32 vuint32; /**< \brief Volatile 32-bit unsigned type */
typedef volatile uint16 vuint16; /**< \brief Volatile 16-bit unsigned type */
typedef volatile uint8 vuint8;   /**< \brief Volatile 8-bit unsigned type */
typedef volatile int64 vint64;   /**< \brief Volatile 64-bit signed type */
typedef volatile int32 vint32;   /**< \brief Volatile 32-bit signed type */
typedef volatile int16 vint16;   /**< \brief Volatile 16-bit signed type */
typedef volatile int8 vint8;     /**< \brief Volatile 8-bit signed type */

/** @} */

__END_DECLS

#endif /* __ARCH_TYPES_H */
