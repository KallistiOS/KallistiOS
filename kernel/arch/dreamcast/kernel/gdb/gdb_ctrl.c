/* KallistiOS ##version##

   kernel/gdb/gdb_ctrl.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

#include <arch/arch.h>
#include <arch/irq.h>
#include <arch/cache.h>
#include <dc/ubc.h>

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

typedef enum {
    STEP_NONE = 0,
    STEP_PATCHED_INSTR,
    STEP_TRAPA_HANDLER,
    STEP_UBC_POST,
} step_kind_t;

typedef struct {
    short *mem_addr;
    short old_instr;
} step_patch_t;

typedef struct {
    irq_t code;
    irq_cb_t original;
} step_trapa_t;

typedef struct {
    ubc_breakpoint_t bp;
} step_ubc_t;

typedef struct {
    step_kind_t kind;
    step_patch_t patch;
    step_trapa_t trapa;
    step_ubc_t ubc;
} step_data_t;

static bool stepped;
static step_data_t step_state;
static int32_t gdb_thread_for_ctrl = GDB_THREAD_ANY;
static irq_context_t *ctrl_irq_ctx;

void set_ctrl_thread(int tid) {
    gdb_thread_for_ctrl = tid;
}

void setup_ctrl_context(void) {
    ctrl_irq_ctx = gdb_resolve_thread_context(gdb_thread_for_ctrl);
}

static bool is_supported_ctrl_thread(int tid) {
    kthread_t *current = thd_get_current();

    if(tid == GDB_THREAD_ANY || tid == GDB_THREAD_ALL)
        return true;

    if(!current || tid <= GDB_THREAD_ANY)
        return false;

    return current->tid == (tid_t)tid;
}

static void handle_step_trapa(irq_t code, irq_context_t *context, void *data) {
    irq_cb_t original = step_state.trapa.original;

    (void)data;

    gdb_enter_exception(context, EXC_TRAPA, false);

    if(original.hdl)
        original.hdl(code, context, original.data);
}

static bool handle_step_rte_break(const ubc_breakpoint_t *bp,
                                  const irq_context_t *context,
                                  void *data) {
    (void)bp;
    (void)data;

    gdb_enter_exception((irq_context_t *)context, EXC_USER_BREAK_POST, false);

    return false;
}

static bool do_single_step(void) {
    short *instr_mem;
    int displacement;
    int reg;
    unsigned short opcode, br_opcode;

    setup_ctrl_context();
    instr_mem = (short *)ctrl_irq_ctx->pc;
    stepped = true;
    opcode = *instr_mem;
    br_opcode = opcode & COND_BR_MASK;
    step_state.kind = STEP_PATCHED_INSTR;

    if(br_opcode == BT_INSTR || br_opcode == BTS_INSTR) {
        if(ctrl_irq_ctx->sr & T_BIT_MASK) {
            displacement = (opcode & COND_DISP) << 1;

            if(displacement & 0x80)
                displacement |= 0xffffff00;

            /*
               * Remember PC points to second instr.
               * after PC of branch ... so add 4
               */
            instr_mem = (short *)(ctrl_irq_ctx->pc + displacement + 4);
        }
        else {
            /* can't put a trapa in the delay slot of a bt/s instruction */
            instr_mem += (br_opcode == BTS_INSTR) ? 2 : 1;
        }
    }
    else if(br_opcode == BF_INSTR || br_opcode == BFS_INSTR) {
        if(ctrl_irq_ctx->sr & T_BIT_MASK) {
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
            instr_mem = (short *)(ctrl_irq_ctx->pc + displacement + 4);
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
        instr_mem = (short *)(ctrl_irq_ctx->pc + displacement + 4);
    }
    else if((opcode & UCOND_RBR_MASK) == JSR_INSTR) {
        reg = (char)((opcode & UCOND_REG) >> 8);

        instr_mem = (short *)ctrl_irq_ctx->r[reg];
    }
    else if(opcode == RTS_INSTR)
        instr_mem = (short *)ctrl_irq_ctx->pr;
    else if(opcode == RTE_INSTR) {
        memset(&step_state.ubc.bp, 0, sizeof(ubc_breakpoint_t));
        step_state.kind = STEP_UBC_POST;
        step_state.ubc.bp.address = (void *)ctrl_irq_ctx->pc;
        step_state.ubc.bp.access = ubc_access_instruction;
        step_state.ubc.bp.instruction.break_before = false;

        if(!ubc_add_breakpoint(&step_state.ubc.bp, handle_step_rte_break, NULL)) {
            step_state.kind = STEP_NONE;
            strcpy(remcom_out_buffer, "E52");
            stepped = false;
            return false;
        }

        return true;
    }
    else if((opcode & TRAPA_MASK) == TRAPA_INSTR) {
        step_state.kind = STEP_TRAPA_HANDLER;
        step_state.trapa.code = IRQ_TRAP_CODE(opcode & COND_DISP);
        step_state.trapa.original = irq_get_handler(step_state.trapa.code);
        irq_set_handler(step_state.trapa.code, handle_step_trapa, NULL);
        return true;
    }
    else
        instr_mem += 1;

    step_state.patch.mem_addr = instr_mem;
    step_state.patch.old_instr = *instr_mem;
    *instr_mem = SSTEP_INSTR;
    icache_flush_range((uint32_t)instr_mem, 2);
    return true;
}

/* Undo the effect of a previous do_single_step.  If we single stepped,
   restore the old instruction. */
void undo_single_step(void) {
    if(stepped) {
        if(step_state.kind == STEP_PATCHED_INSTR) {
            short *instr_mem = step_state.patch.mem_addr;

            *instr_mem = step_state.patch.old_instr;
            icache_flush_range((uint32_t)instr_mem, 2);
        }
        else if(step_state.kind == STEP_TRAPA_HANDLER) {
            irq_set_handler(step_state.trapa.code,
                            step_state.trapa.original.hdl,
                            step_state.trapa.original.data);
        }
        else if(step_state.kind == STEP_UBC_POST) {
            ubc_remove_breakpoint(&step_state.ubc.bp);
        }
    }

    stepped = false;
    memset(&step_state, 0, sizeof(step_data_t));
}

/*
 * Handle the 'c' (continue) and 's' (single-step) commands.
 * Optionally takes a new PC value. Updates the PC and steps if needed.
 */
bool handle_continue_step(char *ptr) {
    bool stepping = (ptr[-1] == 's');
    uint32_t addr = 0;
    bool set_pc = hex_to_int(&ptr, &addr) != 0;
    setup_ctrl_context();

    if(*ptr != '\0') {
        strcpy(remcom_out_buffer, "E01");
        return false;
    }

    if(set_pc)
        ctrl_irq_ctx->pc = addr;

    if(stepping) {
        if(!arch_valid_text_address(ctrl_irq_ctx->pc)) {
            strcpy(remcom_out_buffer, "E35");
            return false;
        }

        if(!do_single_step())
            return false;
    }

    return true;
}

void handle_thread_select(char *ptr) {
    int tid = GDB_THREAD_ANY;
    char type = *ptr++;
    uint32_t parsed_tid = 0;

    if(*ptr == '-' && ptr[1] == '1' && ptr[2] == '\0')
        tid = GDB_THREAD_ALL;
    else if(hex_to_int(&ptr, &parsed_tid) && *ptr == '\0')
        tid = (int)parsed_tid;
    else {
        strcpy(remcom_out_buffer, "E01");
        return;
    }

    if(tid > GDB_THREAD_ANY && !thd_by_tid((tid_t)tid)) {
        strcpy(remcom_out_buffer, "E01");
        return;
    }

    if(type == 'g')
        set_regs_thread(tid);
    else if(type == 'c') {
        if(!is_supported_ctrl_thread(tid)) {
            strcpy(remcom_out_buffer, "E02");
            return;
        }

        set_ctrl_thread(tid);
    }
    else {
        strcpy(remcom_out_buffer, "E01");
        return;
    }

    strcpy(remcom_out_buffer, GDB_OK);
}
