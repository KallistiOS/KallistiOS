/* KallistiOS ##version##

   dc/math.h
   Copyright (C) 2023 Paul Cercueil

*/

#ifndef __DC_MATH_H
#define __DC_MATH_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/**
    \file   dc/math.h
    \brief  Prototypes for optimized math functions written in ASM
    \author Paul Cercueil
*/

/**
    \brief  Returns the bit-reverse value of the argument (where MSB
            becomes LSB and vice-versa).
    \return the bit-reverse value of the argument.
*/
unsigned int bit_reverse(unsigned int value);

#endif /* __DC_MATH_H */
