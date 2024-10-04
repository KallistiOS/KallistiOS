/* KallistiOS ##version##
   sys/fenv.h
   Copyright (C) 2024 Falco Girgis
*/

/** \file
    \brief SH4-Specific FENV definitions and extensions.
*/

#ifndef __SYS_FENV_H
#define __SYS_FENV_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

/** \name  Exception Types
 *  \brief Flags for floating-point exception types
 *  @{
 */
#define FE_INEXACT    0x01 /**< Rounding was necessary to store result */
#define FE_UNDERFLOW  0x02 /**< Result was subnomral with precision loss */
#define FE_OVERFLOW   0x04 /**< Result too large to be representable */
#define FE_DIVBYZERO  0x08 /**< Pole error occurred */
#define FE_INVALID    0x10 /**< Domain error occurred */
#define FE_ALL_ACCEPT 0x1f /**< Mask which includes all exception types */
/** @} */

/** \name  Rounding Modes 
 *  \brief Values for floating-point rounding modes
 *  \note  
 *  If the unrounded value is larger than the maximum expressible absolute value, 
 *  the value will be the maximum expressible absolute value.
 *  @{
 */
#define FE_TONEAREST  0x0 /**< Round to nearest value. If two, use LSB of 0. */
#define FE_TOWARDZERO 0x1 /**< Discard digits below the round bit of value. */
#define FE_DOWNWARD   0x2 /**< Unsupported. */
#define FE_UPWARD     0x3 /**< Unsupported. */
/** @} */

/** Default floating-point environment at program start. */
#define FE_DFL_ENV   _fe_dfl_env
/** Default floating-point control modes at program start. */
#define FE_DFL_MODE  _fe_dfl_mode

/** Type representing entire FP environment. */
typedef uint8_t fenv_t;
/** Type representing all FP status flags collectively. */
typedef uint8_t fexcept_t;

/** \name  POSIX Extensions
 *  \brief Extended functionality for POSIX compliance.
 *  @{
 */
typedef uint8_t femode_t;
int fegetmode(femode_t *mode);
int fesetmode(const femode_t *mode);
/** @} */

/** \name  GNU Extensions
 *  \brief Extended GNU functionality support.
 *  @{
 */
int feenableexcept(int excepts);
int fedisableexcept(int excepts);
int fegetexcept(void);
/** @} */

/** \cond INTERNAL */
extern const fenv_t *_fe_dfl_env;
extern const femode_t *_fe_dfl_mode;
/** \endcond */

__END_DECLS

#endif
