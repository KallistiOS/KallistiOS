/* SPDX-License-Identifier: BSD-3-Clause

   KallistiOS ##version##

   include/libudiv/udiv.h
   Copyright (C) 2024 Paul Cercueil <paul@crapouillou.net>
*/

/** \file    libudiv/udiv.h
    \brief   Fast uint32_t division routines
    \ingroup libudiv

    This file was originally from `libudiv`.

    \author     Paul Cercueil
*/

#ifndef __LIBUDIV_H__
#define __LIBUDIV_H__

#include <kos/cdefs.h>

/** \defgroup   libudiv     libudiv
    \brief      Optimized unsigned 32-bit division routines
    \ingroup    math_general

    libudiv is an optimized math library that provides routines for efficiently
    dividing an unsigned 32-bit variable by a known unsigned 32-bit constant.

    @{
*/

/** \brief Divider for a uint32_t

    This structure represents a cached, optimized form of a constant uint32_t
    divisor value.
*/
typedef struct {
    /** \cond */
    unsigned int p;
    unsigned int m;
    /** \endcond */
} udiv_t;

/** \cond */
#define UDIV_P(div) \
    (31 - __builtin_clz(div) + !!((div) & ((div) - 1)))

#define UDIV_M(div, p) \
    ((p) == 0x20 ? (div) : \
     (unsigned int)(((0x1ull << (32 + (p))) + (div) - 1) / (unsigned long long)(div)))

static inline udiv_t __udiv_set_divider(unsigned int div) {
    unsigned int p = UDIV_P(div);
    unsigned int m = UDIV_M(div, p);

    return (udiv_t){ .p = p, .m = m, };
}
/** \endcond */

/** \brief Returns a udiv_t from a constant uint32_t

    Used to convert a constant uint32_t value into an optimized udiv_t form.
*/
#define udiv_set_divider(div) \
    __builtin_choose_expr(__builtin_constant_p(div), \
                  (udiv_t){ .p = UDIV_P(div), .m = UDIV_M((div), UDIV_P(div)) }, \
                  __udiv_set_divider(div))

/** \brief Efficiently divides a uint32_t by a udiv_t (fast)

    Performs the fastest version of fast uint32_t division, dividing
    \p val by \p udiv.

    \warning
    This algorithm only works for values: `1 < div < 0x80000001`.

    \param  val     Dividend or numerator.
    \param  udiv    Divisor or denomenator.

    \returns        Quotient or result of the division.

    \sa udiv_divide()
*/
static inline unsigned int udiv_divide_fast(unsigned int val, udiv_t udiv) {
    unsigned int q, t;

    /* This algorithm only works for values 1 < div < 0x80000001. */

    q = ((unsigned long long)udiv.m * val) >> 32;
    t = ((val - q) >> 1) + q;

    return t >> (udiv.p - 1);
}

/** \brief Efficiently divides a uint32_t by a udiv_t (generic)

    Performs the generic version of fast uint32_t division, dividing \p val by
    \p udiv. This version has no restrictions on the \p udiv source value.

    \param  val     Dividend or numerator.
    \param  udiv    Divisor or denomenator

    \returns        Quotient or result of the division.

    \sa udiv_divide_fast()
*/
static inline unsigned int udiv_divide(unsigned int val, udiv_t udiv) {
    /* Divide by 0x80000001 or higher: the algorithm does not work, so
     * udiv.m contains the full divider value, and we just need to check
     * if the dividend is >= the divider. */
    if (__unlikely(udiv.p == 0x20))
        return val >= udiv.m;

    /* Divide by 1: the algorithm does not work, so handle this special case. */
    if (__unlikely(udiv.m == 0 && udiv.p == 0))
        return val;

    return udiv_divide_fast(val, udiv);
}

/** @) */

#endif /* __LIBUDIV_H__ */