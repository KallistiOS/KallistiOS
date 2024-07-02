/* KallistiOS ##version##

   abort.c
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

#include <stdlib.h>
#include <arch/arch.h>

/* abort() causes abnormal/erroneous program termination
   WITHOUT calling the atexit() handlers. This will eventually
   return EXIT_FAILURE as the program's return code. */
__used void abort(void) {
    arch_abort();
}

