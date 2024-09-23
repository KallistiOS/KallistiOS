/* KallistiOS ##version##

   arch/dreamcast/include/arch/args.h
   Copyright (C) 2023 Paul Cercueil <paul@crapouillou.net>

*/

/** \file   arch/args.h
    \brief  Macros for getting argument registers in inline assembly

    This file contains the KOS_FPARG(n) macro, which resolves to the register
    name that corresponds to the nth floating-point argument of a function.

    \author Paul Cercueil
*/

#ifdef __SH4_SINGLE_ONLY__
#define KOS_SH4_SINGLE_ONLY 1
#else
#define KOS_SH4_SINGLE_ONLY 0
#endif

#define __KOS_FPARG_0_1 "fr4"
#define __KOS_FPARG_0_0 "fr5"
#define __KOS_FPARG_1_1 "fr5"
#define __KOS_FPARG_1_0 "fr4"
#define __KOS_FPARG_2_1 "fr6"
#define __KOS_FPARG_2_0 "fr7"
#define __KOS_FPARG_3_1 "fr7"
#define __KOS_FPARG_3_0 "fr6"
#define __KOS_FPARG_4_1 "fr8"
#define __KOS_FPARG_4_0 "fr9"
#define __KOS_FPARG_5_1 "fr9"
#define __KOS_FPARG_5_0 "fr8"
#define __KOS_FPARG_6_1 "fr10"
#define __KOS_FPARG_6_0 "fr11"
#define __KOS_FPARG_7_1 "fr11"
#define __KOS_FPARG_7_0 "fr10"

#define __KOS_FPARG(n,single) __KOS_FPARG_##n##_##single
#define _KOS_FPARG(n,single) __KOS_FPARG(n,single)

/** \brief  Get the name of the nth floating-point argument register
 *
 *  This macro resolves to the register name that corresponds to the nth
 *  floating-point argument of a function (starting from n=0).
 */
#define KOS_FPARG(n) _KOS_FPARG(n, KOS_SH4_SINGLE_ONLY)
