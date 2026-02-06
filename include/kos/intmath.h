/* KallistiOS ##version##

   kos/intmath.h
   Copyright (C) 2026 Paul Cercueil

   Integer math helper functions
*/

/** \file    kos/intmath.h
    \brief   Functions to help with integer math.
    \ingroup intmath

    \author Paul Cercueil
*/

#ifndef __KOS_INTMATH_H
#define __KOS_INTMATH_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>

static inline bool is_power_of_two(unsigned int val)
{
    return (val & (val - 1)) == 0;
}

static inline unsigned int log2_rdown(unsigned int val)
{
    return 31 - __builtin_clz(val);
}

static inline unsigned int log2_rup(unsigned int val)
{
    return log2_rdown(val) + !is_power_of_two(val);
}

__END_DECLS
#endif /* __KOS_INTMATH_H */
