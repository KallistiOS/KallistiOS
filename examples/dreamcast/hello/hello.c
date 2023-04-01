/* KallistiOS ##version##

   hello.c
   Copyright (C) 2001 Megan Potter
*/

#include <kos.h>

/* You can safely remove this line if you don't use a ROMDISK */
extern uint8 romdisk[];

/* These macros tell KOS how to initialize itself. All of this initialization
   happens before main() gets called, and the shutdown happens afterwards. So
   you need to set any flags you want here. Here are some possibilities:

   INIT_NONE        -- don't do any auto init
   INIT_IRQ     -- Enable IRQs
   INIT_THD_PREEMPT -- Enable pre-emptive threading
   INIT_NET     -- Enable networking (including sockets)
   INIT_MALLOCSTATS -- Enable a call to malloc_stats() right before shutdown

   You can OR any or all of those together. If you want to start out with
   the current KOS defaults, use INIT_DEFAULT (or leave it out entirely). */
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/* And specify a romdisk, if you want one (or leave it out) */
KOS_INIT_ROMDISK(romdisk);

/* Your program's main entry point */
int main(int argc, char **argv) {
    /* The requisite line */
    printf("\nHello world!\n\n");

    return 0;
}
