/* KallistiOS ##version##

   gdb_breaking.c
   Copyright (C) 2025 Donald Haase

*/

/*  This program demonstrates setting up a debugging connection with KOS to
    a host-side gdb server. If sending using dc-tool, you must pass the `-g`
    flag to enable connection to the gdb server. Alternatively, this will
    attempt to establish the gdb connection over the scif port directly.

    Once this program is sent to the DC or emulator, you'd start up `kos-gdb`
    or gdb-multiarch and pass it the elf to the program so that it can see
    symbols in it. gdb will inform of each of the two breakpoints below and
    allow you to inspect the local variables and states by using commands like
    `where full` and moving on from the breakpoint with `continue`.
*/

#include <arch/gdb.h>
#include <kos/dbglog.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>

int main(int argc, char* argv[]) {
    size_t step = 0;

    /* Initialize the connection. This informs dc-tool to connect in
    gdb, or sets up the scif connection, then installs the various IRQ
    handlers needed for monitoring and breaking. Then it breaks. */
    dbglog(DBG_INFO, "Step %u: call gdb_init()\n", ++step);
    gdb_init();

    /* Now try a manual gdb breakpoint */
    dbglog(DBG_INFO, "Step %u: call gdb_breakpoint()\n", ++step);
    gdb_breakpoint();

    return step;
}
