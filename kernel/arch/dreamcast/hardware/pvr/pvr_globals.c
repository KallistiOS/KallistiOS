/* KallistiOS ##version##

   pvr_globals.c
   (C)2002 Megan Potter

 */

#include <dc/pvr.h>
#include "pvr_internal.h"

/*

  Global variables internal to the PVR module go here.

*/

/* Our global state -- by default it's initialized to zeros, so the
   valid flag will be zero. */
volatile pvr_state_t pvr_state;


