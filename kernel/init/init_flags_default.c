/* KallistiOS ##version##

   init_defaults.c
   (c)2002 Megan Potter
*/

#include <kos/init.h>

/* Default values which will be used if the user doesn't declare anything */
#ifdef _arch_xbox
/*
 * PE/COFF weak externals cannot satisfy references from other archive members
 * with GNU ld. This object is in libkallisti.a, so a strong default has the
 * intended semantics: it is extracted only when the application has not
 * supplied the complete KOS_INIT_FLAGS symbol set itself.
 */
KOS_INIT_FLAGS(INIT_DEFAULT);
#else
__weak_symbol KOS_INIT_FLAGS(INIT_DEFAULT);
#endif
