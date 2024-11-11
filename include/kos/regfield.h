/* KallistiOS ##version##

   kos/compiler.h
   Copyright (C) 2024 Paul Cercueil

   Macros to extract / insert bit fields
*/

/** \file    kos/regfield.h
    \brief   Macros to help dealing with register fields.
    \ingroup kernel

    \author Paul Cercueil
*/

#ifndef __KOS_REGFIELD_H
#define __KOS_REGFIELD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/** \brief  Create a mask with a bit set

    \param  bit             The bit to set (from 0 to 31)
    \return                 A 32-bit mask with the corresponding bit set
 */
#define BIT(bit)	(1u << (bit))

/** \brief  Create a mask with a range of bits set

    \param  h               The high bit of the range to set, included
    \param  l               The low bit of the range to set, included
    \return                 A 32-bit mask with the corresponding bits set
 */
#define GENMASK(h, l)	((0xffffffff << (l)) & (0xffffffff >> (31 - (h))))

/** \brief  Extract a field value from a variable

    \param  var             The 32-bit variable containing the field
    \param  field           A 32-bit mask that corresponds to the field
    \return                 The value of the field (shifted)
 */
#define FIELD_GET(var, field) \
	(((var) & (field)) >> __builtin_ctz(field))

/** \brief  Prepare a field with a given value

    \param  field           A 32-bit mask that corresponds to the field
    \param  value           The value to be put in the field
 */
#define FIELD_PREP(field, value) \
	(((value) << __builtin_ctz(field)) & (field))

__END_DECLS
#endif /* __KOS_REGFIELD_H */
