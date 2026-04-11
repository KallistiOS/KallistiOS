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
#define GDB_BRK_SW 0
#define GDB_BRK_HW 1
#define GDB_WATCH_W 2
#define GDB_WATCH_R 3
#define GDB_WATCH_RW 4

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

static bool encode_hw_break_length(size_t length, uint8_t *size_code) {
    switch(length) {
        case 1u:
            *size_code = 1u;
            return true;
        case 2u:
            *size_code = 2u;
            return true;
        case 4u:
            *size_code = 3u;
            return true;
        case 8u:
            *size_code = 4u;
            return true;
        default:
            return false;
    }
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

static void hard_breakpoint(bool set, int brk_type, uintptr_t addr,
                            size_t length, char *res_buffer) {
    char * const ucb_base = (char *)0xff200000;
    static const int ucb_step = 0xc;
    static const char BAR = 0x0, BAMR = 0x4, BBR = 0x8, BRCR = 0x20;
    static const uint8_t bbrBrk[] = {
        0x0,
        0x14,
        0x28,
        0x24,
        0x2c
    };
    uint8_t bbr;
    char *ucb;
    int i;

    if(brk_type < GDB_BRK_HW || brk_type > GDB_WATCH_RW) {
        gdb_error_with_code_str(GDB_EINVAL,
                                "Z/z: invalid hardware breakpoint type");
        return;
    }

    if(addr == 0) {
        strcpy(res_buffer, GDB_OK);
    }
    else if(!encode_hw_break_length(length, &bbr)) {
        gdb_error_with_code_str(GDB_EMEM_SIZE,
                                "Z/z: unsupported hardware breakpoint length");
    }
    else if(set) {
        bbr |= bbrBrk[brk_type];
        WREG(ucb_base, BRCR) = 0;

        for(ucb = ucb_base, i = 2; i > 0; ucb += ucb_step, i--) {
            if(WREG(ucb, BBR) == 0)
                break;
        }

        if(i) {
            LREG(ucb, BAR) = addr;
            BREG(ucb, BAMR) = 0x4;
            WREG(ucb, BBR) = bbr;
            strcpy(res_buffer, GDB_OK);
        }
        else
            strcpy(res_buffer, "E12");
    }
    else {
        bbr |= bbrBrk[brk_type];

        for(ucb = ucb_base, i = 2; i > 0; ucb += ucb_step, i--) {
            if(LREG(ucb, BAR) == addr && WREG(ucb, BBR) == bbr)
                break;
        }

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

void handle_breakpoint(char *ptr) {
    bool set = (ptr[-1] == 'Z');
    int brk_type = *ptr++ - '0';
    uint32_t addr;
    uint32_t length;

    if(*ptr++ == ',' && hex_to_int(&ptr, &addr) &&
       *ptr++ == ',' && hex_to_int(&ptr, &length) && *ptr == '\0') {
        if(brk_type < GDB_BRK_SW || brk_type > GDB_WATCH_RW) {
            gdb_error_with_code_str(GDB_EINVAL, "Z/z: invalid breakpoint type");
        }
        else if(brk_type == GDB_BRK_SW)
            soft_breakpoint(set, addr, length);
        else
            hard_breakpoint(set, brk_type, addr, length, remcom_out_buffer);
    }
    else {
        gdb_error_with_code_str(GDB_EINVAL, "Z/z: invalid packet");
    }
}
