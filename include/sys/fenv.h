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

/** Clears the given floating-point exception flags.
 * 
 *  \param  excepts     Bitwise OR'd list of FP exception flags
 *  \retval 0           Success
 */
int feclearexcept(int excepts);

/** Determines which of a subset of the FP exceptions are set.
 *  
 *  \param  excepts     Bitwise OR'd list of FP exception flags to test
 *  \returns            Bitwise OR of the FP exception macros that are both
 *                      in \p excepts and are currently set.
 */
int fetestexcept(int excepts);

/** Raises the specified floating-point exceptions.
 * 
 *  \param   excepts     Bitwise OR'd list of FP exception flags to raise
 *  \retval  0           Success   
 */
int feraiseexcept(int excepts);

/** Copies the state of the given FP flags from the environment.
 * 
 *  \param  flagp   Structure to store the FP state status within
 *  \param  excepts Bitwise OR'd list of FP exception flags to store
 *  \retval 0       Success
 * 
 *  \sa fesetexceptflag()
 */
int fegetexceptflag(fexcept_t *flagp, int excepts);

/** Copies the state of the given FP flags to the environment.
 * 
 *  \param  flagp   Structure to copy the FP state status from
 *  \param  excepts Bitwise OR'd list of FP exception flags to set
 *  \retval 0       Success
 * 
 *  \sa fegetexceptflag()
 */
int fesetexceptflag(const fexcept_t* flagp, int excepts);

/** Sets the floating-point rounding mode.
 * 
 *  \param  round   New rounding mode to set
 *  \retval 0       Success
 *  \retval Other   Failure; returns current rounding mode
 * 
 *  \sa fegetround()
 */
int fesetround(int round);

/** Gets the floating-point rounding mode.
 * 
 *  \returns    Value corresponding to current rounding mode 
 *  
 *  \sa fesetround() 
 */
int fegetround(void);

/** Stores the status of the FP environment.
 * 
 *  \param  envp    Structure to store FP status within
 *  \retval 0       Success
 *
 *  \sa fesetenv()
 */
int fegetenv(fenv_t *envp);

/** Restores the status of the FP environment.
 * 
 *  \note
 *  Does not raise any exceptions, only modifies the flags.
 * 
 *  \param  envp    Structure containing previous FP status
 *  \retval 0       Success
 * 
 *  \sa fegetenv()
 */
int fesetenv(const fenv_t* envp);

/** Saves the FP env, clears status flags, prevents exceptions from trapping.
 * 
 *  \param  envp    Structure where the FP environment will be stored
 * 
 *  \retval 0       Success  
 */
int feholdexcept(fenv_t *envp);

/** Restores FP environment and raises previously raised exceptions.
 * 
 *  \param  envp    FP environment to be restored
 *  \retval 0       Success
 */
int feupdateenv(const fenv_t *envp);

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
