/* KallistiOS ##version##

   kernel/gdb/gdb_mem.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

#include <arch/arch.h>
#include <arch/cache.h>
#include <dc/memory.h>

#include "gdb_internal.h"

static int dofault;  /* Non zero, bus errors will raise exception */

static bool is_valid_memory_address(uintptr_t addr, bool is_write) {
    (void)is_write;

    if(addr >= MEM_AREA_P4_BASE)
        return false;

    addr = (addr & MEM_AREA_CACHE_MASK) | MEM_AREA_P1_BASE;
    return arch_valid_address(addr) || arch_valid_text_address(addr);
}

static bool is_valid_memory_range(uint32_t addr, uint32_t len, bool is_write) {
    uintptr_t end_addr;

    if(len == 0)
        return true;

    end_addr = (uintptr_t)addr + (uintptr_t)len - 1u;
    if(end_addr < addr)
        return false;

    return is_valid_memory_address(addr, is_write) &&
           is_valid_memory_address(end_addr, is_write);
}

static bool hex_to_mem_checked(const char *src, void *dest, uint32_t count) {
    unsigned char *out = (unsigned char *)dest;

    if(!src || !dest)
        return false;

    for(uint32_t i = 0; i < count; ++i) {
        int high = hex(src[i * 2u]);
        int low = hex(src[(i * 2u) + 1u]);

        if(high < 0 || low < 0)
            return false;

        out[i] = (unsigned char)((high << 4) | low);
    }

    return true;
}

/*
 * Handle the 'm' command.
 * Reads memory from the target at a given address and length.
 * Format: mADDR,LEN
 */
void handle_read_mem(char *ptr) {
    uint32_t addr = 0;
    uint32_t len = 0;

    dofault = 0;

    if(hex_to_int(&ptr, &addr) && *ptr++ == ',' && hex_to_int(&ptr, &len) &&
       *ptr == '\0' && is_valid_memory_range(addr, len, false))
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
    size_t payload_len;

    dofault = 0;

    if(hex_to_int(&ptr, &addr) && *ptr++ == ',' &&
       hex_to_int(&ptr, &len) && *ptr++ == ':') {
        payload_len = strlen(ptr);

        if(is_valid_memory_range(addr, len, true) &&
           ((len == 0 && *ptr == '\0') ||
            (payload_len == (size_t)len * 2u &&
             hex_to_mem_checked(ptr, (char *)addr, len)))) {
            icache_flush_range(addr, len);
            strcpy(remcom_out_buffer, GDB_OK);
        }
        else {
            strcpy(remcom_out_buffer, "E02");
        }
    }
    else
        strcpy(remcom_out_buffer, "E02");

    dofault = 1;
}
