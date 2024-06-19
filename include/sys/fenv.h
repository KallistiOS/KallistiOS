/* KallistiOS ##version##

   sys/fenv.h
   Copyright (C) 2024 Falco Girgis

*/


/** \file
    \brief   SH4-Specific FENV definitions and extensions.

*/

#ifndef __SYS_FENV_H
#define __SYS_FENV_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

#define FE_INEXACT      0x01 
#define FE_UNDERFLOW    0x02
#define FE_OVERFLOW     0x04
#define FE_DIVBYZERO    0x08
#define FE_INVALID      0x10

#define FE_ALL_ACCEPT   0x1f

#define FE_TONEAREST    0x00
#define FE_TOWARDZERO   0x01
#define FE_DOWNWARD     0x02
#define FE_UPWARD       0x03

typedef uint8_t fenv_t;
typedef uint8_t fexcept_t;

extern const fenv_t *_fe_dfl_env;
#define FE_DFL_ENV   _fe_dfl_env

/* Extended POSIX */
typedef uint8_t femode_t;
int fegetmode(femode_t *mode);
int fesetmode(const femode_t *mode);

extern const femode_t *_fe_dfl_mode;
#define FE_DFL_MODE     _fe_dfl_mode

/* GNU Extensions */
int feenableexcept(int excepts);
int fedisableexcept(int excepts);
int fegetexcept(void);

__END_DECLS

#endif