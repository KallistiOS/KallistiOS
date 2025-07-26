/* KallistiOS ##version##

   include/kos/tuple.h
   Copyright (C) 2025 Paul Cercueil
*/

/** \file    kos/tuple.h
    \brief   Tuple types definitions.
    \ingroup types

    This file contains definitions for tuple types.

    Tuple types are useful especially as return values; this is because on SH4
    the ABI allows a function to return two 32-bit values, one in r0, the other
    in r1. As a result, the functions that need to return two values do not need
    to do so through return value pointers, which generally cause the compiler
    to generate pretty bad code (due to aliasing etc.).

    \author Paul Cercueil
*/

#ifndef __KOS_TUPLE_H
#define __KOS_TUPLE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

/** \brief  Tuple type for two 32-bit values. */
typedef struct tuple_u32 {
    uint32_t u0, u1;
} tu32_t;

__END_DECLS

#endif /* __KOS_TUPLE_H */
