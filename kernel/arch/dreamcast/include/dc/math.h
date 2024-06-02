/* KallistiOS ##version##

   dc/math.h
   Copyright (C) 2023 Paul Cercueil

*/

/**
    \file    dc/math.h
    \brief   Prototypes for optimized math functions written in ASM
    \ingroup math_general

    \author Paul Cercueil
*/

#ifndef __DC_MATH_H
#define __DC_MATH_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \defgroup math_general  General
    \brief                  Optimized general-purpose math utilities
    \ingroup                math    
    @{
*/

/**
    \brief  Returns the bit-reverse value of the argument (where MSB
            becomes LSB and vice-versa).

    \return the bit-reverse value of the argument.
*/
unsigned int bit_reverse(unsigned int value);

/** @} */

__END_DECLS

#endif /* __DC_MATH_H */
