/* KallistiOS ##version##

   arch/dreamcast/kernel/gdb/gdb_ctrl.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

/*
   Implements continue and single-step control for the GDB remote stub.

   Supported control packets:
     - c / s         : continue and single-step
     - Cxx / Sxx     : continue and single-step with signal
     - Hg / Hc       : select threads for register or execution control

   Single-step is usually implemented by decoding the next SH4 instruction and
   patching the computed stop location with an internal TRAPA trap. Two
   special cases use different machinery: stepping a real TRAPA installs a
   temporary trap-handler shim, and stepping RTE uses a post-instruction UBC
   breakpoint because control resumes through the saved exception-return state.
*/

#include <arch/arch.h>
#include <arch/irq.h>
#include <arch/cache.h>
#include <dc/ubc.h>

#include "gdb_internal.h"


/* Hitachi SH architecture instruction encoding masks */
#define COND_BR_MASK    0xff00
#define UCOND_DBR_MASK  0xe000
#define UCOND_RBR_MASK  0xf0df
#define TRAPA_MASK      0xff00

#define COND_DISP       0x00ff
#define UCOND_DISP      0x0fff
#define UCOND_REG       0x0f00

/* Hitachi SH instruction opcodes */
#define BF_INSTR        0x8b00
#define BFS_INSTR       0x8f00
#define BT_INSTR        0x8900
#define BTS_INSTR       0x8d00
#define BRA_INSTR       0xa000
#define BSR_INSTR       0xb000
#define JMP_INSTR       0x402b
#define JSR_INSTR       0x400b
#define RTS_INSTR       0x000b
#define RTE_INSTR       0x002b
#define TRAPA_INSTR     0xc300
#define SSTEP_INSTR     0xc320

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

static bool do_single_step(void);
static void handle_step_trapa(irq_t code, irq_context_t *context, void *data);
static bool handle_step_rte_break(const ubc_breakpoint_t *bp,
                                  const irq_context_t *context,
                                  void *data);
static bool is_supported_ctrl_thread(int tid);

/*
   Resume the currently selected control thread.

   This helper is the shared backend for c/s, C/S, and parsed vCont actions.
   It optionally rewrites the resume PC and prepares single-step state when
   needed before returning control to the target.
*/
bool gdb_resume_target(bool stepping, bool set_pc, uint32_t pc) {
    setup_ctrl_context();

    if(set_pc)
        ctrl_irq_ctx->pc = pc;

    if(stepping) {
        if(!arch_valid_text_address(ctrl_irq_ctx->pc)) {
            gdb_error_with_code_str(GDB_EMEM_PROT, "s/S: invalid step PC");
            return false;
        }

        if(!do_single_step())
            return false;
    }

    return true;
}

/*
   Trap wrapper used when single-stepping a real TRAPA instruction.

   We stop once the trap has been taken but before its registered callback
   runs. After the debugger resumes, execution continues through the original
   handler exactly once.
*/
static void handle_step_trapa(irq_t code, irq_context_t *context, void *data) {
    irq_cb_t original = step_state.trapa.original;

    (void)data;

    gdb_enter_exception(context, EXC_TRAPA, false);

    if(original.hdl)
        original.hdl(code, context, original.data);
}

/*
   UBC callback used when single-stepping an RTE instruction.

   The breakpoint is armed on the RTE itself with break-after semantics, so
   this callback only runs after exception return has restored the frame that
   execution resumes from.
*/
static bool handle_step_rte_break(const ubc_breakpoint_t *bp,
                                  const irq_context_t *context,
                                  void *data) {
    (void)bp;
    (void)data;

    gdb_enter_exception((irq_context_t *)context, EXC_USER_BREAK_POST, false);

    return false;
}

/* Sets the target thread for control operations (continue/step). */
void set_ctrl_thread(int tid) {
    gdb_thread_for_ctrl = tid;
}

/*
   Sets the IRQ context used for control operations like continue/step.

   Control packets can only resume the currently stopped thread in this all-stop
   stub. Hc accepts only selectors that resolve to the live exception context:
   "any", "all", or the currently stopped thread's ID.
*/
void setup_ctrl_context(void) {
    ctrl_irq_ctx = gdb_resolve_thread_context(gdb_thread_for_ctrl);
}

/*
   Returns whether an Hc thread selector can be honored without scheduler help.

   The stub may inspect other threads' saved register contexts, but execution
   control still returns through the current exception frame. Without core
   scheduler support, resuming a different thread would be misleading.
*/
static bool is_supported_ctrl_thread(int tid) {
    kthread_t *current = thd_get_current();

    if(tid == GDB_THREAD_ANY || tid == GDB_THREAD_ALL)
        return true;

    if(!current || tid <= GDB_THREAD_ANY)
        return false;

    return current->tid == (tid_t)tid;
}

/*
   Prepares for single-step execution.

   Most instructions are stepped by patching the computed next stop address
   with an internal TRAPA. Branches, delay slots, and returns are decoded so
   the trap lands where execution would naturally continue. Real TRAPA
   instructions and RTE are handled specially, because neither case can be
   modeled safely by simply overwriting the next instruction word.
*/
static bool do_single_step(void) {
    short *instr_mem;
    int displacement;
    int reg;
    unsigned short opcode, br_opcode;

    setup_ctrl_context();

    instr_mem = (short *)ctrl_irq_ctx->pc;
    opcode = *instr_mem;
    br_opcode = opcode & COND_BR_MASK;
    step_state.kind = STEP_PATCHED_INSTR;

    if(br_opcode == BT_INSTR || br_opcode == BTS_INSTR) {
        if(ctrl_irq_ctx->sr & T_BIT_MASK) {
            displacement = (opcode & COND_DISP) << 1;

            if(displacement & 0x80)
                displacement |= 0xffffff00;

            /*
               Remember PC points to second instr.
               after PC of branch ... so add 4
            */
            instr_mem = (short *)(ctrl_irq_ctx->pc + displacement + 4);
        }
        else {
            /* Can't safely place trapa in BT/S delay slot */
            instr_mem += (br_opcode == BTS_INSTR) ? 2 : 1;
        }
    }
    else if(br_opcode == BF_INSTR || br_opcode == BFS_INSTR) {
        if(ctrl_irq_ctx->sr & T_BIT_MASK) {
            /* Can't put a trapa in the delay slot of a bf/s instruction */
            instr_mem += (br_opcode == BFS_INSTR) ? 2 : 1;
        }
        else {
            displacement = (opcode & COND_DISP) << 1;

            if(displacement & 0x80)
                displacement |= 0xffffff00;

            /*
               Remember PC points to second instr.
               after PC of branch ... so add 4
            */
            instr_mem = (short *)(ctrl_irq_ctx->pc + displacement + 4);
        }
    }
    else if((opcode & UCOND_DBR_MASK) == BRA_INSTR) {
        displacement = (opcode & UCOND_DISP) << 1;

        if(displacement & 0x0800)
            displacement |= 0xfffff000;

        /*
          Remember PC points to second instr.
          after PC of branch ... so add 4
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
            gdb_error_with_code_str(GDB_EBKPT_HW_NORES,
                                    "s/S: unable to arm RTE post-step breakpoint");
            return false;
        }

        stepped = true;
        return true;
    }
    else if((opcode & TRAPA_MASK) == TRAPA_INSTR) {
        step_state.kind = STEP_TRAPA_HANDLER;
        step_state.trapa.code = IRQ_TRAP_CODE(opcode & COND_DISP);
        step_state.trapa.original = irq_get_handler(step_state.trapa.code);
        irq_set_handler(step_state.trapa.code, handle_step_trapa, NULL);
        stepped = true;
        return true;
    }
    else
        instr_mem += 1;

    step_state.patch.mem_addr = instr_mem;
    step_state.patch.old_instr = *instr_mem;
    *instr_mem = SSTEP_INSTR;
    icache_flush_range((uint32_t)instr_mem, 2);
    stepped = true;
    return true;
}

/*
   Undo whichever helper state was armed for the previous single-step.

   Depending on the step kind, this restores the patched instruction, removes
   the temporary TRAPA wrapper, or disarms the UBC post-step breakpoint.
*/
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
   Handle the 'c' (continue) and 's' (single-step) GDB commands.

   These commands resume execution of the program, optionally from a new PC.
   - 'c' continues execution normally.
   - 's' performs a single instruction step.

   Format:
     - 'c'           → continue from current PC
     - 'cXXXX'       → continue from address XXXX
     - 's'           → single-step from current PC
     - 'sXXXX'       → single-step from address XXXX

   command is the packet opcode ('c' or 's'), and ptr points to the optional
   address payload that follows it. This function parses that address when
   present, updates the live exception context's PC, and resumes execution. If
   single-stepping, it prepares the computed next stop location for trapping.
*/
bool handle_continue_step(char command, char *ptr) {
    bool stepping = (command == 's');
    uint32_t addr = 0;
    bool set_pc = hex_to_int(&ptr, &addr) != 0;

    if(*ptr != '\0') {
        gdb_error_with_code_str(GDB_EINVAL, "c/s: invalid packet");
        return false;
    }

    return gdb_resume_target(stepping, set_pc, addr);
}

/*
   Handle the 'C' and 'S' commands.

   Format:
     - 'Cxx'         → continue with signal xx
     - 'Sxx'         → step one instruction with signal xx
     - 'Cxx;ADDR'    → continue from address ADDR with signal xx
     - 'Sxx;ADDR'    → step from address ADDR with signal xx

   Signals are ignored on SH4; this just resumes or steps the currently stopped
   thread as needed. If 'S' is used, single-step mode is enabled before
   continuing.

   command is the packet opcode ('C' or 'S'), and ptr points to the character
   after it.
*/
bool handle_continue_step_signal(char command, char *ptr) {
    bool stepping = (command == 'S');
    uint32_t signal = 0;
    uint32_t addr = 0;
    bool set_pc = false;

    /* Parse signal (always two hex digits) */
    if(hex_to_int(&ptr, &signal) != 2) {
        gdb_error_with_code_str(GDB_EINVAL, "C/S: invalid signal packet");
        return false;
    }

    /* Optional: skip semicolon and parse new PC if present */
    if(*ptr == ';') {
        ++ptr;

        if(!hex_to_int(&ptr, &addr) || *ptr != '\0') {
            gdb_error_with_code_str(GDB_EINVAL, "C/S: invalid address packet");
            return false;
        }

        set_pc = true;
    }
    else if(*ptr != '\0') {
        gdb_error_with_code_str(GDB_EINVAL, "C/S: invalid packet");
        return false;
    }

    /* SH4 does not use the supplied signal value here. */
    (void)signal;
    return gdb_resume_target(stepping, set_pc, addr);
}

/*
   Handle the 'H' packet to select the active thread for GDB operations.

   Format:
     - HgXX → Set thread for register ops (g, G, p, P)
     - HcXX → Select the live thread for control ops when it is the current stop

   XX is a thread ID in hex. Special values:
     - 0        → Any thread (default)
     - 0xFFFFFFFF (or -1) → All threads

   Register selection supports dormant thread contexts. Control selection is
   intentionally narrower: without scheduler help, continue/step can only act
   on the currently stopped thread (or the equivalent "any/all" selectors).
*/
void handle_thread_select(char *ptr) {
    int tid = GDB_THREAD_ANY;
    char type = *ptr++;
    uint32_t parsed_tid = 0;

    if(*ptr == '-' && ptr[1] == '1' && ptr[2] == '\0')
        tid = GDB_THREAD_ALL;
    else if(hex_to_int(&ptr, &parsed_tid) && *ptr == '\0')
        tid = (int)parsed_tid;
    else {
        gdb_error_with_code_str(GDB_EINVAL, "H: invalid thread selector");
        return;
    }

    if(tid > GDB_THREAD_ANY && !thd_by_tid((tid_t)tid)) {
        gdb_error_with_code_str(GDB_EINVAL, "H: unknown thread");
        return;
    }

    if(type == 'g')
        set_regs_thread(tid);
    else if(type == 'c') {
        if(!is_supported_ctrl_thread(tid)) {
            gdb_error_with_code_str(GDB_EUNIMPL,
                                    "Hc: non-current thread execution unsupported");
            return;
        }

        set_ctrl_thread(tid);
    }
    else {
        gdb_error_with_code_str(GDB_EINVAL, "H: unsupported selector");
        return;
    }

    gdb_put_ok();
}
