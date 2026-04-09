/* KallistiOS ##version##

   kernel/gdb/gdb_break.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

#include <arch/arch.h>
#include <arch/cache.h>

#include <dc/memory.h>

#include "gdb_internal.h"

/* Handle inserting/removing a hardware breakpoint.
   Using the SH4 User Break Controller (UBC) we can have
   two breakpoints, each set for either instruction and/or operand access.
   Break channel B can match a specific data being moved, but there is
   no GDB remote protocol spec for utilizing this functionality. */

#define LREG(r, o) (*((uint32_t*)((r)+(o))))
#define WREG(r, o) (*((uint16_t*)((r)+(o))))
#define BREG(r, o) (*((uint8_t*)((r)+(o))))

#define MAX_SW_BREAKPOINTS 32
#define GDB_SW_BREAK_OPCODE 0xc33f

typedef struct {
    uintptr_t addr;
    uint16_t original;
    bool active;
} sw_breakpoint_t;

static sw_breakpoint_t sw_breakpoints[MAX_SW_BREAKPOINTS];

static uintptr_t normalize_cached_address(uintptr_t addr) {
    return (addr & MEM_AREA_CACHE_MASK) | MEM_AREA_P1_BASE;
}

static bool is_valid_sw_breakpoint_range(uintptr_t addr, size_t length) {
    uintptr_t end_addr;

    if(length == 0)
        return false;

    end_addr = addr + length - 1u;
    if(end_addr < addr)
        return false;

    return arch_valid_text_address(normalize_cached_address(addr)) &&
           arch_valid_text_address(normalize_cached_address(end_addr));
}

static void soft_breakpoint(bool set, uintptr_t addr, size_t length) {
    uintptr_t normalized_addr;
    volatile uint16_t *site;

    if((addr & 1u) || length != 2u) {
        gdb_error_with_code_str(GDB_EINVAL,
                                "Z0: address must be 2-byte aligned");
        return;
    }

    if(!is_valid_sw_breakpoint_range(addr, length)) {
        gdb_error_with_code_str(GDB_EMEM_PROT,
                                "Z0: invalid software breakpoint address");
        return;
    }

    normalized_addr = normalize_cached_address(addr);
    site = (volatile uint16_t *)normalized_addr;

    for(int i = 0; i < MAX_SW_BREAKPOINTS; ++i) {
        if(sw_breakpoints[i].active &&
           sw_breakpoints[i].addr == normalized_addr) {
            if(set) {
                gdb_put_ok();
            }
            else {
                if(*site != GDB_SW_BREAK_OPCODE) {
                    gdb_error_with_code_str(GDB_EBKPT_CLEAR_ID,
                                            "z0: breakpoint site changed while active");
                    return;
                }

                *site = sw_breakpoints[i].original;
                icache_flush_range(normalized_addr, 2);
                sw_breakpoints[i].active = false;
                gdb_put_ok();
            }

            return;
        }
    }

    if(!set) {
        gdb_error_with_code_str(GDB_EBKPT_CLEAR_ADDR,
                                "z0: no breakpoint at requested address");
        return;
    }

    for(int i = 0; i < MAX_SW_BREAKPOINTS; ++i) {
        if(!sw_breakpoints[i].active) {
            sw_breakpoints[i].addr = normalized_addr;
            sw_breakpoints[i].original = *site;
            sw_breakpoints[i].active = true;
            *site = GDB_SW_BREAK_OPCODE;
            icache_flush_range(normalized_addr, 2);
            gdb_put_ok();
            return;
        }
    }

    gdb_error_with_code_str(GDB_EBKPT_SW_NORES, "Z0: no free breakpoint slots");
}

static void hard_breakpoint(bool set, int brk_type, uintptr_t addr, size_t length, char* res_buffer) {
    char* const ucb_base = (char*)0xff200000;
    static const int ucb_step = 0xc;
    static const char BAR = 0x0, BAMR = 0x4, BBR = 0x8, /*BASR = 0x14,*/ BRCR = 0x20;

    static const uint8_t bbrBrk[] = {
        0x0,  /* type 0, memory breakpoint -- unsupported */
        0x14, /* type 1, hardware breakpoint */
        0x28, /* type 2, write watchpoint */
        0x24, /* type 3, read watchpoint */
        0x2c  /* type 4, access watchpoint */
    };

    uint8_t bbr = 0;
    char* ucb;
    int i;

    if(brk_type < 0 || brk_type > 4) {
        strcpy(res_buffer, "E02");
        return;
    }

    if(length <= 8) {
        do {
            bbr++;
        } while(length >>= 1);
    }

    bbr |= bbrBrk[brk_type];

    if(addr == 0) {  /* GDB tries to watch 0, wasting a UCB channel */
        strcpy(res_buffer, GDB_OK);
    }
    else if(brk_type == 0) {
        /* we don't support memory breakpoints -- the debugger
           will use the manual memory modification method */
        *res_buffer = '\0';
    }
    else if(length > 8) {
        strcpy(res_buffer, "E22");
    }
    else if(set) {
        WREG(ucb_base, BRCR) = 0;

        /* find a free UCB channel */
        for(ucb = ucb_base, i = 2; i > 0; ucb += ucb_step, i--)
            if(WREG(ucb, BBR) == 0)
                break;

        if(i) {
            LREG(ucb, BAR) = addr;
            BREG(ucb, BAMR) = 0x4; /* no BASR bits used, all BAR bits used */
            WREG(ucb, BBR) = bbr;
            strcpy(res_buffer, GDB_OK);
        }
        else
            strcpy(res_buffer, "E12");
    }
    else {
        /* find matching UCB channel */
        for(ucb = ucb_base, i = 2; i > 0; ucb += ucb_step, i--)
            if(LREG(ucb, BAR) == addr && WREG(ucb, BBR) == bbr)
                break;

        if(i) {
            WREG(ucb, BBR) = 0;
            strcpy(res_buffer, GDB_OK);
        }
        else
            strcpy(res_buffer, "E06");
    }
}

#undef LREG
#undef WREG

/*
 * Handle the 'Z' and 'z' commands.
 * Inserts or removes a breakpoint or watchpoint.
 * Format: Ztype,addr,kind or ztype,addr,kind
 */
void handle_breakpoint(char *ptr) {
    bool set = (ptr[-1] == 'Z');
    int brk_type = *ptr++ - '0';
    uint32_t addr;
    uint32_t length;

    if(*ptr++ == ',' && hex_to_int(&ptr, &addr) &&
       *ptr++ == ',' && hex_to_int(&ptr, &length) && *ptr == '\0') {
        if(brk_type == 0)
            soft_breakpoint(set, addr, length);
        else
            hard_breakpoint(set, brk_type, addr, length, remcom_out_buffer);
    }
    else {
        gdb_error_with_code_str(GDB_EINVAL, "Z/z: invalid packet");
    }
}
