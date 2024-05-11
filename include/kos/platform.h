/* KallistiOS ##version##

   include/kos/platform.h
   Copyright (C) 2024 Paul Cercueil

*/

/** \file   kos/platform.h
    \brief  Platform detection macros.
    \author Paul Cercueil
*/

#ifndef __KOS_PLATFORM_H
#define __KOS_PLATFORM_H

#ifdef __NAOMI__
#  define KOS_PLATFORM_IS_NAOMI 1
#else
#  define KOS_PLATFORM_IS_NAOMI 0
#endif

#ifdef __DREAMCAST__
#  define KOS_PLATFORM_IS_DREAMCAST 1
#else
#  define KOS_PLATFORM_IS_DREAMCAST 0
#endif

#endif /* __KOS_PLATFORM_H */
