/* KallistiOS ##version##

   hello.c
   Copyright (C) 2001 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

#include <stdio.h>

/* #include <kos.h>
   KOS_INIT_FLAGS(INIT_DEFAULT);
*/

/* KOS_INIT_FLAGS() tells KOS how to initialize itself and which drivers to
   pull into your executable. All of this initialization happens before main()
   gets called, and the shutdown happens afterwards.

   Here are some possible flags to pass to KOS_INIT_FLAGS():

   INIT_NONE        -- don't do any auto init
   INIT_IRQ         -- Enable IRQs
   INIT_NET         -- Enable networking (including sockets)
   INIT_MALLOCSTATS -- Enable a call to malloc_stats() right before shutdown

   Refer to kos/init.h and arch/init_flags.h for the full list of flags.
   You can OR any or all of these together.

   If you want to start out with the current KOS defaults, use INIT_DEFAULT.
   You may now also omit KOS_INIT_FLAGS() altogether, which will also use
   INIT_DEFAULT automatically, as we're doing here. This can be useful if
   you're lazy or if you're building regular C or C++ code and don't want to
   introduce KOS-specific DC code into the file.
*/

/* Your program's main entry point */
int main(int argc, char *argv[]) {
    /* The requisite line */
    printf("\nHello world!\n\n");

    return 0;
}
