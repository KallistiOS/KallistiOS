/* KallistiOS ##version##

   kernel/gdb/gdb_mem.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

#include <arch/cache.h>

#include "gdb_internal.h"

static int dofault;  /* Non zero, bus errors will raise exception */

/*
 * Handle the 'm' command.
 * Reads memory from the target at a given address and length.
 * Format: mADDR,LEN
 */
void handle_read_mem(char *ptr) {
    uint32_t addr = 0;
    uint32_t len = 0;

    dofault = 0;

    if(hex_to_int(&ptr, &addr) && *ptr++ == ',' && hex_to_int(&ptr, &len))
        mem_to_hex((char *)addr, remcom_out_buffer, len);
    else
        strcpy(remcom_out_buffer, "E01");

    dofault = 1;
}

/*
 * Handle the 'M' command.
 * Writes memory to the target at a given address.
 * Format: MADDR,LEN:DATA
 */
void handle_write_mem(char *ptr) {
    uint32_t addr = 0;
    uint32_t len = 0;

    dofault = 0;

    if(hex_to_int(&ptr, &addr) && *ptr++ == ',' &&
       hex_to_int(&ptr, &len) && *ptr++ == ':') {
        hex_to_mem(ptr, (char *)addr, len);
        icache_flush_range(addr, len);
        strcpy(remcom_out_buffer, GDB_OK);
    }
    else
        strcpy(remcom_out_buffer, "E02");

    dofault = 1;
}
