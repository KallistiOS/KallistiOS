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

   INIT_DEFAULT     -- do a normal init with the most common settings
   INIT_IRQ         -- Enable IRQs (implied by INIT_DEFAULT)
   INIT_NET         -- Enable networking (including sockets)
   INIT_MALLOCSTATS -- Enable a call to malloc_stats() right before shutdown
   INIT_NONE        -- don't do any auto init (you must manually init hardware)

   Refer to kos/init.h and arch/init_flags.h for the full list of flags.
   You can OR any or all of these together.

   If you wish to use the default initialization settings, specify
   INIT_DEFAULT in your KOS_INIT_FLAGS(). You may also omit KOS_INIT_FLAGS()
   from your program (as we are doing here since the above code is commented
   out), which will automatically use the default initialization settings.
*/

/* Your program's main entry point */
int main(int argc, char *argv[]) {
    /* The requisite line */
    printf("Hello world!\n");

    return 0;
}
