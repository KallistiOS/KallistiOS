/* KallistiOS ##version##

   arch/dreamcast/kernel/gdb/gdb_break.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

/*
   Implements GDB breakpoint and watchpoint packets for the Dreamcast target.

   Supported packet families:
     - Z0/z0: software breakpoints via TRAPA instruction patching
     - Z1-Z4/z1-z4: hardware breakpoints and watchpoints via the SH4 UBC

   Notes:
     - software breakpoints require 2-byte alignment
     - Z1 instruction breakpoints require kind 2 on SH4
     - Z2-Z4 watchpoints support operand sizes of 1, 2, 4, or 8 bytes
     - the hardware breakpoint path uses the KOS UBC driver backend with two tracked slots
     - 8-byte operand watchpoints are synthesized as paired 32-bit UBC watches
*/

#include <arch/arch.h>
#include <arch/cache.h>

#include <dc/memory.h>
#include <dc/ubc.h>

#include "gdb_internal.h"

#define MAX_SW_BREAKPOINTS  32
#define GDB_SW_BREAK_OPCODE 0xC33f

#define GDB_BRK_SW    0
#define GDB_BRK_HW    1
#define GDB_WATCH_W   2
#define GDB_WATCH_R   3
#define GDB_WATCH_RW  4

typedef struct {
    uintptr_t addr;
    uint16_t original;
    bool active;
} sw_breakpoint_t;

typedef struct {
    ubc_breakpoint_t bp;
    uintptr_t base_addr;
    int type;
    size_t length;
    int partner;
    bool primary;
    bool active;
} hw_breakpoint_t;

static sw_breakpoint_t sw_breakpoints[MAX_SW_BREAKPOINTS];
static hw_breakpoint_t gdb_hw_bps[2];

/* Normalizes an address to the cached P1 alias used for code patching. */
static uintptr_t normalize_cached_address(uintptr_t addr) {
    return (addr & MEM_AREA_CACHE_MASK) | MEM_AREA_P1_BASE;
}

/* Returns whether the requested software-breakpoint range lies in valid text. */
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

/*
   Insert or remove a software breakpoint.

   Replaces the instruction at a 2-byte aligned address with TRAPA and keeps
   the original instruction word so it can be restored on z0 removal.

   The target address is normalized to the cached P1 alias before validation,
   tracking, and patching. Removal only restores the original instruction if
   the live site still contains the TRAPA opcode inserted by the stub.

   Duplicate Z0 requests for the same address are treated as success without
   consuming an extra slot.
*/
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

                /* Remove the stub's TRAPA and restore the original instruction. */
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

            /* Patch the target instruction with the software-break TRAPA. */
            *site = GDB_SW_BREAK_OPCODE;
            icache_flush_range(normalized_addr, 2);
            gdb_put_ok();
            return;
        }
    }

    gdb_error_with_code_str(GDB_EBKPT_SW_NORES, "Z0: no free breakpoint slots");
}

/*
   Maps a GDB watchpoint kind to the closest UBC operand-size selector.

   Only byte, word, longword, and quadword operand sizes are supported by
   this backend.
*/
static bool encode_hw_watch_length(size_t length, ubc_size_t *size) {
    switch(length) {
        case 1u:
            *size = ubc_size_8bit;
            return true;
        case 2u:
            *size = ubc_size_16bit;
            return true;
        case 4u:
            *size = ubc_size_32bit;
            return true;
        case 8u:
            *size = ubc_size_64bit;
            return true;
        default:
            return false;
    }
}

/*
   Builds a UBC breakpoint descriptor from a GDB Z1-Z4 request.

   Z1 becomes a pre-instruction hardware breakpoint. Z2-Z4 become operand
   watchpoints with the requested size and read/write mode.
*/
static bool build_hw_breakpoint(ubc_breakpoint_t *bp, int brk_type,
                                uintptr_t addr, size_t length) {
    ubc_size_t size;

    memset(bp, 0, sizeof(ubc_breakpoint_t));
    bp->address = (void *)addr;

    switch(brk_type) {
        case GDB_BRK_HW:
            if(length != 2u)
                return false;

            bp->access = ubc_access_instruction;
            bp->instruction.break_before = true;
            return true;

        case GDB_WATCH_W:
        case GDB_WATCH_R:
        case GDB_WATCH_RW:
            if(!encode_hw_watch_length(length, &size))
                return false;

            bp->access = ubc_access_operand;
            bp->operand.size = size;
            bp->operand.rw = (brk_type == GDB_WATCH_W)  ? ubc_rw_write :
                             (brk_type == GDB_WATCH_R)  ? ubc_rw_read :
                                                         ubc_rw_either;
            return true;

        default:
            return false;
    }
}

/* Returns the active tracked hardware breakpoint matching type, address, and kind. */
static hw_breakpoint_t *find_hw_breakpoint(int brk_type, uintptr_t addr,
                                           size_t length) {
    for(int i = 0; i < 2; ++i) {
        if(gdb_hw_bps[i].active &&
           gdb_hw_bps[i].primary &&
           gdb_hw_bps[i].type == brk_type &&
           gdb_hw_bps[i].length == length &&
           gdb_hw_bps[i].base_addr == addr) {
            return &gdb_hw_bps[i];
        }
    }

    return NULL;
}

/* Returns the first unused tracked hardware breakpoint slot, if any. */
static hw_breakpoint_t *find_free_hw_breakpoint(void) {
    for(int i = 0; i < 2; ++i) {
        if(!gdb_hw_bps[i].active)
            return &gdb_hw_bps[i];
    }

    return NULL;
}

/*
   Returns true when an operand watchpoint needs both UBC channels.

   SH4-generated accesses to 64-bit C objects are commonly emitted as paired
   32-bit loads and stores, so a single quadword watchpoint would miss half of
   the logical range requested by GDB.
*/
static bool needs_split_operand_watchpoint(int brk_type, size_t length) {
    return length == 8u &&
           (brk_type == GDB_WATCH_W ||
            brk_type == GDB_WATCH_R ||
            brk_type == GDB_WATCH_RW);
}

/* Clears bookkeeping for one tracked hardware breakpoint slot. */
static void clear_hw_breakpoint_slot(hw_breakpoint_t *slot) {
    memset(slot, 0, sizeof(hw_breakpoint_t));
}

/*
   Programs both UBC channels to cover an 8-byte operand watchpoint as two
   adjacent 32-bit watches. Both slots share the same logical address range so
   insertion, duplicate detection, and removal continue to behave like a single
   GDB watchpoint.
*/
static void set_split_operand_watchpoint(int brk_type, uintptr_t addr,
                                         size_t length) {
    hw_breakpoint_t *low = &gdb_hw_bps[0];
    hw_breakpoint_t *high = &gdb_hw_bps[1];

    if(low->active || high->active) {
        gdb_error_with_code_str(GDB_EBKPT_HW_NORES,
                                "Z/z: wide watchpoint requires both UBC channels");
        return;
    }

    if(!build_hw_breakpoint(&low->bp, brk_type, addr, 4u) ||
       !build_hw_breakpoint(&high->bp, brk_type, addr + 4u, 4u)) {
        clear_hw_breakpoint_slot(low);
        clear_hw_breakpoint_slot(high);
        gdb_error_with_code_str(GDB_EMEM_SIZE,
                                "Z/z: unsupported wide hardware watchpoint");
        return;
    }

    if(!ubc_add_breakpoint(&low->bp, NULL, NULL)) {
        clear_hw_breakpoint_slot(low);
        clear_hw_breakpoint_slot(high);
        gdb_error_with_code_str(GDB_EBKPT_SET_FAIL,
                                "Z/z: failed to program low half of wide watchpoint");
        return;
    }

    if(!ubc_add_breakpoint(&high->bp, NULL, NULL)) {
        ubc_remove_breakpoint(&low->bp);
        clear_hw_breakpoint_slot(low);
        clear_hw_breakpoint_slot(high);
        gdb_error_with_code_str(GDB_EBKPT_SET_FAIL,
                                "Z/z: failed to program high half of wide watchpoint");
        return;
    }

    low->base_addr = addr;
    low->type = brk_type;
    low->length = length;
    low->partner = 1;
    low->primary = true;
    low->active = true;

    high->base_addr = addr;
    high->type = brk_type;
    high->length = length;
    high->partner = 0;
    high->primary = false;
    high->active = true;

    gdb_put_ok();
}

/*
   Sets or clears a hardware breakpoint/watchpoint using SH4 UBC.

   Supports instruction, read, write, or access break types.

   Requests are tracked by type, address, and kind so inserting the same
   breakpoint again does not create a duplicate, and removals clear the
   matching active entry.
*/
static void hard_breakpoint(bool set, int brk_type, uintptr_t addr,
                            size_t length) {
    hw_breakpoint_t *slot;
    hw_breakpoint_t *partner = NULL;

    if(brk_type < GDB_BRK_HW || brk_type > GDB_WATCH_RW) {
        gdb_error_with_code_str(GDB_EINVAL,
                                "Z/z: invalid hardware breakpoint type");
        return;
    }

    /* GDB sometimes probes address 0; do not waste a UBC channel on it. */
    if(addr == 0) {
        gdb_put_ok();
        return;
    }

    slot = find_hw_breakpoint(brk_type, addr, length);

    if(!set) {
        if(!slot) {
            gdb_error_with_code_str(GDB_EBKPT_CLEAR_ADDR,
                                    "z/z: no matching hardware breakpoint");
            return;
        }

        if(slot->partner >= 0 && slot->partner < 2 &&
           gdb_hw_bps[slot->partner].active) {
            partner = &gdb_hw_bps[slot->partner];
        }

        if(partner && !ubc_remove_breakpoint(&partner->bp)) {
            gdb_error_with_code_str(GDB_EBKPT_CLEAR_ID,
                                    "z/z: failed to clear paired hardware breakpoint");
            return;
        }

        if(!ubc_remove_breakpoint(&slot->bp)) {
            gdb_error_with_code_str(GDB_EBKPT_CLEAR_ID,
                                    "z/z: failed to clear hardware breakpoint");
            return;
        }

        if(partner)
            clear_hw_breakpoint_slot(partner);

        clear_hw_breakpoint_slot(slot);
        gdb_put_ok();
        return;
    }

    if(slot) {
        gdb_put_ok();
        return;
    }

    if(needs_split_operand_watchpoint(brk_type, length)) {
        set_split_operand_watchpoint(brk_type, addr, length);
        return;
    }

    slot = find_free_hw_breakpoint();
    if(!slot) {
        gdb_error_with_code_str(GDB_EBKPT_HW_NORES,
                                "Z/z: no free hardware breakpoint slots");
        return;
    }

    if(!build_hw_breakpoint(&slot->bp, brk_type, addr, length)) {
        gdb_error_with_code_str(GDB_EMEM_SIZE,
                                "Z/z: unsupported hardware breakpoint length");
        memset(slot, 0, sizeof(hw_breakpoint_t));
        return;
    }

    if(!ubc_add_breakpoint(&slot->bp, NULL, NULL)) {
        memset(slot, 0, sizeof(hw_breakpoint_t));
        gdb_error_with_code_str(GDB_EBKPT_SET_FAIL,
                                "Z/z: failed to program hardware breakpoint");
        return;
    }

    slot->type = brk_type;
    slot->base_addr = addr;
    slot->length = length;
    slot->partner = -1;
    slot->primary = true;
    slot->active = true;

    gdb_put_ok();
}

/*
   Handle the 'Z' and 'z' commands.

   Inserts or removes a breakpoint or watchpoint request from GDB.
   Format: Ztype,addr,kind or ztype,addr,kind

   Supported types:
     - 0: software breakpoint
     - 1: hardware instruction breakpoint
     - 2: write watchpoint
     - 3: read watchpoint
     - 4: access watchpoint

   After parsing the type, address, and kind fields, this dispatcher forwards
   the request into the software-breakpoint or UBC-backed hardware/watchpoint
   path. Invalid packet syntax and unsupported breakpoint kinds return EINVAL.
*/
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
            hard_breakpoint(set, brk_type, addr, length);
    }
    else {
        gdb_error_with_code_str(GDB_EINVAL, "Z/z: invalid packet");
    }
}
