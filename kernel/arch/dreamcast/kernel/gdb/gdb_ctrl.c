/* KallistiOS ##version##

   kernel/gdb/gdb_ctrl.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

#include <arch/irq.h>
#include <arch/cache.h>

#include "gdb_internal.h"


/* Hitachi SH architecture instruction encoding masks */
#define COND_BR_MASK   0xff00
#define UCOND_DBR_MASK 0xe000
#define UCOND_RBR_MASK 0xf0df
#define TRAPA_MASK     0xff00

#define COND_DISP      0x00ff
#define UCOND_DISP     0x0fff
#define UCOND_REG      0x0f00

/* Hitachi SH instruction opcodes */
#define BF_INSTR       0x8b00
#define BFS_INSTR      0x8f00
#define BT_INSTR       0x8900
#define BTS_INSTR      0x8d00
#define BRA_INSTR      0xa000
#define BSR_INSTR      0xb000
#define JMP_INSTR      0x402b
#define JSR_INSTR      0x400b
#define RTS_INSTR      0x000b
#define RTE_INSTR      0x002b
#define TRAPA_INSTR    0xc300
#define SSTEP_INSTR    0xc320

/* Hitachi SH processor register masks */
#define T_BIT_MASK     0x0001

typedef struct {
    short *mem_addr;
    short old_instr;
} step_data_t;

static bool stepped;
static step_data_t instr_buffer;

void do_single_step(void) {
    short *instr_mem;
    int displacement;
    int reg;
    unsigned short opcode, br_opcode;

    stepped = true;
    instr_mem = (short *) irq_ctx->pc;
    opcode = *instr_mem;
    br_opcode = opcode & COND_BR_MASK;

    if(br_opcode == BT_INSTR || br_opcode == BTS_INSTR) {
        if(irq_ctx->sr & T_BIT_MASK) {
            displacement = (opcode & COND_DISP) << 1;

            if(displacement & 0x80)
                displacement |= 0xffffff00;

            /*
               * Remember PC points to second instr.
               * after PC of branch ... so add 4
               */
            instr_mem = (short *)(irq_ctx->pc + displacement + 4);
        }
        else {
            /* can't put a trapa in the delay slot of a bt/s instruction */
            instr_mem += (br_opcode == BTS_INSTR) ? 2 : 1;
        }
    }
    else if(br_opcode == BF_INSTR || br_opcode == BFS_INSTR) {
        if(irq_ctx->sr & T_BIT_MASK) {
            /* can't put a trapa in the delay slot of a bf/s instruction */
            instr_mem += (br_opcode == BFS_INSTR) ? 2 : 1;
        }
        else {
            displacement = (opcode & COND_DISP) << 1;

            if(displacement & 0x80)
                displacement |= 0xffffff00;

            /*
               * Remember PC points to second instr.
               * after PC of branch ... so add 4
               */
            instr_mem = (short *)(irq_ctx->pc + displacement + 4);
        }
    }
    else if((opcode & UCOND_DBR_MASK) == BRA_INSTR) {
        displacement = (opcode & UCOND_DISP) << 1;

        if(displacement & 0x0800)
            displacement |= 0xfffff000;

        /*
         * Remember PC points to second instr.
         * after PC of branch ... so add 4
         */
        instr_mem = (short *)(irq_ctx->pc + displacement + 4);
    }
    else if((opcode & UCOND_RBR_MASK) == JSR_INSTR) {
        reg = (char)((opcode & UCOND_REG) >> 8);

        instr_mem = (short *) irq_ctx->r[reg];
    }
    else if(opcode == RTS_INSTR)
        instr_mem = (short *) irq_ctx->pr;
    else if(opcode == RTE_INSTR)
        instr_mem = (short *) irq_ctx->r[15];
    else if((opcode & TRAPA_MASK) == TRAPA_INSTR)
        instr_mem = (short *)((opcode & ~TRAPA_MASK) << 2);
    else
        instr_mem += 1;

    instr_buffer.mem_addr = instr_mem;
    instr_buffer.old_instr = *instr_mem;
    *instr_mem = SSTEP_INSTR;
    icache_flush_range((uint32_t)instr_mem, 2);
}

/* Undo the effect of a previous do_single_step.  If we single stepped,
   restore the old instruction. */
void undo_single_step(void) {
    if(stepped) {
        short *instr_mem;
        instr_mem = instr_buffer.mem_addr;
        *instr_mem = instr_buffer.old_instr;
        icache_flush_range((uint32_t)instr_mem, 2);
    }

    stepped = false;
}

/*
 * Handle the 'c' (continue) and 's' (single-step) commands.
 * Optionally takes a new PC value. Updates the PC and steps if needed.
 */
void handle_continue_step(char *ptr) {
    bool stepping = (ptr[-1] == 's');
    uint32_t addr;

    if(hex_to_int(&ptr, &addr))
        irq_ctx->pc = addr;

    if(stepping)
        do_single_step();
}
