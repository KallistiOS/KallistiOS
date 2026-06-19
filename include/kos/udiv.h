/* KallistiOS ##version##

   kos/udiv.h
   Copyright (C) 2024, 2026 Paul Cercueil
*/

/** \file    kos/udiv.h
    \brief   Fast unsigned integer division
    \ingroup intmath

    This file contains an API that can be used to pre-process an unsigned value
    into an udiv_t struct. This struct can later be used to divide another
    unsigned value much faster than by doing an actual division.

    This API is very useful when a rarely-changing divider is used often.

    \author Paul Cercueil
*/

#ifndef __KOS_UDIV_H
#define __KOS_UDIV_H

#include <kos/cdefs.h>

__BEGIN_DECLS

#include <kos/intmath.h>
#include <kos/regfield.h>

/** \struct  udiv_t
    \brief   Pre-processed unsigned integer divider value.
*/
typedef struct {
    unsigned int p;
    unsigned int m;
} udiv_t;

/** \brief       Create a udiv_t from an unsigned int divider.

    Use this function to create the preleminary udiv_t object, that can then
    be passed to udiv_divide() or udiv_divide_fast().

    \param  div             The unsigned integer divider value
    \return                 A pre-processed divider as a udiv_t
*/
static inline udiv_t udiv_set_divider(unsigned int div)
{
    unsigned int p = 31 - __builtin_clz(div) + !is_power_of_two(div);
    unsigned int m = (BITLL(32 + p) + div - 1) / (unsigned long long)div;

    return (udiv_t){ .p = p, .m = m, };
}

/** \brief       Perform a division using the udiv_t, without bound checking

    This function is similar to udiv_divide(), except that it only works for
    values 1 < div < 0x80000001.

    \param  val             The dividend
    \param  udiv            The divider, as a udiv_t
    \return                 The result of the division operation
*/
static inline unsigned int udiv_divide_fast(unsigned int val, udiv_t udiv) {
    unsigned int q = ((unsigned long long)udiv.m * val) >> 32;
    unsigned int t = ((val - q) >> 1) + q;

    return t >> (udiv.p - 1);
}

/** \brief       Perform a division using the udiv_t, with bound checking

    This function will perform a division using the provided udiv_t.
    The pre-processed divider can correspond to any non-zero integer value.

    \param  val             The dividend
    \param  udiv            The divider, as a udiv_t
    \return                 The result of the division operation
*/
static inline unsigned int udiv_divide(unsigned int val, udiv_t udiv)
{
    /* Divide by 0x80000001 or higher: the algorithm does not work, so
     * udiv.m contains the full divider value, and we just need to check
     * if the dividend is >= the divider. */
    if (__predict_false(udiv.p == 0x20))
        return val >= udiv.m;

    /* Divide by 1: the algorithm does not work, so handle this special case. */
    if (__predict_false(udiv.m == 0 && udiv.p == 0))
        return val;

    return udiv_divide_fast(val, udiv);
}

__END_DECLS

#endif /* __KOS_UDIV_H */
