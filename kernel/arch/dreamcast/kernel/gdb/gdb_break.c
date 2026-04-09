/* KallistiOS ##version##

   kernel/gdb/gdb_break.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

#include "gdb_internal.h"

/* Handle inserting/removing a hardware breakpoint.
   Using the SH4 User Break Controller (UBC) we can have
   two breakpoints, each set for either instruction and/or operand access.
   Break channel B can match a specific data being moved, but there is
   no GDB remote protocol spec for utilizing this functionality. */

#define LREG(r, o) (*((uint32_t*)((r)+(o))))
#define WREG(r, o) (*((uint16_t*)((r)+(o))))
#define BREG(r, o) (*((uint8_t*)((r)+(o))))

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
       *ptr++ == ',' && hex_to_int(&ptr, &length)) {
        hard_breakpoint(set, brk_type, addr, length, remcom_out_buffer);
    } else {
        strcpy(remcom_out_buffer, "E02");
    }
}
