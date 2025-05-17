/* KallistiOS ##version##

   test.c
   Copyright (C) 2025 Donald Haase

   This is intended to validate that the API required by the toolchain
   to build correctly hasn't changed.
*/

/* For EXIT_SUCCESS */
#include <stdlib.h>

/* Include the actual headers from KOS */
#include <kos/thread.h>
#include <kos/tls.h>
#include <kos/mutex.h>
#include <kos/once.h>
#include <kos/cond.h>
#include <arch/irq.h>

/* Include the patch header to see if there are conflicts */
#include <gthr-kos.h>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    return EXIT_SUCCESS;
}
