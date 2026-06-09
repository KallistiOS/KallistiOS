/* KallistiOS ##version##

   arch/dreamcast/kernel/gdb/gdb.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

/*
   Core Dreamcast GDB stub entry points and exception dispatch.

   This file owns:
     - stub initialization and shutdown
     - trap and exception registration
     - the main remote-protocol dispatch loop
     - connection lifecycle state for a live GDB session
*/

#include <arch/arch.h>
#include <arch/gdb.h>

#include <dc/dcload.h>
#include <dc/scif.h>

#include <stdio.h>

#include "gdb_internal.h"

/* TRAPA #0x20: internal single-step trap */
#define TRAPA_GDB_SINGLESTEP  32

/* TRAPA #0x3F: GDB-inserted software breakpoints (Z0) */
#define TRAPA_GDB_BREAKPOINT  63

/* TRAPA #0xFF: manually inserted via gdb_breakpoint() */
#define TRAPA_USER_BREAKPOINT 255

static irq_context_t *irq_ctx;
static bool initialized;
static bool connected;

/*
   Main GDB exception handler.

   Builds the initial stop reply for the current exception, then services
   remote packets until execution is resumed or the debugger detaches or kills
   the target.
*/
static void gdb_handle_exception(int exception_vector) {
    char command;
    char *ptr;

    gdb_handle_t_stop_reply(exception_vector);
    gdb_put_packet(remcom_out_buffer);

    gdb_undo_single_step();

    while(true) {
        remcom_out_buffer[0] = 0;
        ptr = (char *)gdb_get_packet();
        connected = true;

        command = *ptr++;

        switch(command) {
            case '?': gdb_handle_t_stop_reply(exception_vector); break;
            case 'p': gdb_handle_read_reg(ptr); break;
            case 'P': gdb_handle_write_reg(ptr); break;
            case 'q': gdb_handle_query(ptr); break;
            case 'Q': gdb_handle_set_query(ptr); break;
            case 'T': gdb_handle_thread_alive(ptr); break;
            case 'H': gdb_handle_thread_select(ptr); break;
            case 'g': gdb_handle_read_regs(ptr); break;
            case 'G': gdb_handle_write_regs(ptr); break;
            case 'm': gdb_handle_read_mem(ptr); break;
            case 'M': gdb_handle_write_mem(ptr); break;
            case 'x': gdb_handle_read_mem_binary(ptr); break;
            case 'X': gdb_handle_write_mem_binary(ptr); break;
            case 'c':
            case 's':
                if(gdb_handle_continue_step(command, ptr))
                    return;
                break;
            case 'C':
            case 'S':
                if(gdb_handle_continue_step_signal(command, ptr))
                    return;
                break;
            case 'Z':
            case 'z':
                gdb_handle_breakpoint(ptr); break;
            case 'v':
                if(gdb_handle_v_packet(ptr))
                    return;
                break;
            case 'D': gdb_handle_detach(); return;
            case 'k': gdb_handle_kill(); return;
            default:
                break;
        }

        gdb_put_packet(remcom_out_buffer);
    }
}

/* Returns the current IRQ context captured during a GDB exception. */
irq_context_t *gdb_get_irq_context(void) {
    return irq_ctx;
}

/*
   Enter the debugger for an already-captured exception context.

   This is used when we need to stop from helper code that is already running
   inside an exception handler, such as the stepped TRAPA shim in gdb_ctrl.c.
*/
void gdb_enter_exception(irq_context_t *context,
                         int exception_vector,
                         bool rewind_pc) {
    irq_ctx = context;

    if(rewind_pc)
        irq_ctx->pc -= 2;

    gdb_handle_exception(exception_vector);
}

/*
   Resolve a thread selector to the register context that GDB should edit.

   If the requested thread is the one currently stopped in the debugger, we
   must use the live exception frame rather than the parked kthread context;
   otherwise register reads and writes would not affect the state that resumes.
*/
irq_context_t *gdb_resolve_thread_context(int tid) {
    irq_context_t *default_context = gdb_get_irq_context();

    if(tid == GDB_THREAD_ALL || tid == GDB_THREAD_ANY)
        return default_context;

    kthread_t *target = thd_by_tid((tid_t)tid);

    if(!target)
        return default_context;

    if(default_context && target == thd_get_current())
        return default_context;

    return &target->context;
}

/* Updates whether a live debugger session is currently considered connected. */
void gdb_set_connected(bool is_connected) {
    connected = is_connected;
}

/*
   Handle a debugger-visible SH4 exception entry.

   This is the common IRQ callback for faults and traps that should stop in
   GDB. It forwards the captured exception frame and raw exception code into
   gdb_enter_exception() without rewriting the saved PC.
*/
static void handle_exception(irq_t code, irq_context_t *context, void *data) {
    (void)data;

    gdb_enter_exception(context, code, false);
}

/*
   Handle TRAPA #0xFF breakpoints raised by gdb_breakpoint().

   This path reports the stop as EXC_TRAPA without rewinding the saved PC.
   Unlike internal single-step and Z0 trap sites, the user breakpoint does not
   replace another instruction that needs to be virtually restored for GDB.
*/
static void handle_user_trapa(irq_t code, irq_context_t *context, void *data) {
    (void)code;
    (void)data;

    gdb_enter_exception(context, EXC_TRAPA, false);
}

/*
   Handle TRAPA exceptions used internally by the stub for single-step traps
   and patched Z0 software breakpoints.

   Because the trap instruction replaces the original instruction in memory,
   the saved PC is rewound by one instruction before reporting EXC_TRAPA.
*/
static void handle_gdb_trapa(irq_t code, irq_context_t *context, void *data) {
    (void)code;
    (void)data;

    gdb_enter_exception(context, EXC_TRAPA, true);
}

/*
   Triggers a software breakpoint using TRAPA #0xFF.

   Typically called at the start of a program to establish a connection
   with the debugger, or used to halt execution and enter the debugger manually.
*/
void gdb_breakpoint(void) {
    __asm__ __volatile__("trapa #0xff"::);
}

/*
   Initialize the Dreamcast GDB stub and register its exception handlers.

   The transport backend is chosen at runtime by probing for dc-load GDB
   support and otherwise falling back to SCIF. Initialization ends by raising
   an initial breakpoint so the debugger can attach immediately.
*/
void gdb_init(void) {
    if(initialized)
        return;

    initialized = true;
    connected = false;

    if(dcload_gdbpacket(NULL, 0, NULL, 0) == 0)
        using_dcl = 1;
    else {
        using_dcl = 0;
        scif_set_parameters(57600, 1);
    }

    irq_set_handler(EXC_ILLEGAL_INSTR, handle_exception, NULL);
    irq_set_handler(EXC_SLOT_ILLEGAL_INSTR, handle_exception, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_READ, handle_exception, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_WRITE, handle_exception, NULL);
    irq_set_handler(EXC_USER_BREAK_PRE, handle_exception, NULL);
    irq_set_handler(EXC_USER_BREAK_POST, handle_exception, NULL);

    irq_set_handler(IRQ_TRAP_CODE(TRAPA_GDB_SINGLESTEP), handle_gdb_trapa, NULL);
    irq_set_handler(IRQ_TRAP_CODE(TRAPA_GDB_BREAKPOINT), handle_gdb_trapa, NULL);
    irq_set_handler(IRQ_TRAP_CODE(TRAPA_USER_BREAKPOINT), handle_user_trapa, NULL);

    gdb_breakpoint();
}
