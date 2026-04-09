/* KallistiOS ##version##

   kernel/gdb/gdb.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

/* Remote communication protocol.

   A debug packet whose contents are <data>
   is encapsulated for transmission in the form:

    $ <data> # CSUM1 CSUM2

    <data> must be ASCII alphanumeric and cannot include characters
    '$' or '#'.  If <data> starts with two characters followed by
    ':', then the existing stubs interpret this as a sequence number.

    CSUM1 and CSUM2 are ascii hex representation of an 8-bit
    checksum of <data>, the most significant nibble is sent first.
    the hex digits 0-9,a-f are used.

   Receiver responds with:

    +   - if CSUM is correct and ready for next packet
    -   - if CSUM is incorrect

   <data> is as follows:
   All values are encoded in ascii hex digits.
**********************************************************************
    REQUEST     PACKET
**********************************************************************
    read regs   g

    reply       XX....X     Each byte of register data
                            is described by two hex digits.
                            Registers are in the internal order
                            for GDB, and the bytes in a register
                            are in the same order the machine uses.
                or ENN      for an error.
**********************************************************************
    write regs  GXX..XX     Each byte of register data
                            is described by two hex digits.

    reply       OK          for success
                ENN         for an error
**********************************************************************
    write reg   Pn...=r...  Write register n... with value r...,
                            which contains two hex digits for each
                            byte in the register (target byte
                            order).

    reply       OK          for success
                ENN         for an error
    (not supported by all stubs).
**********************************************************************
    read mem    mAA..AA,LLLL
                            AA..AA is address, LLLL is length.
    reply       XX..XX      XX..XX is mem contents

                            Can be fewer bytes than requested
                            if able to read only part of the data.
                or ENN      NN is errno
**********************************************************************
    write mem   MAA..AA,LLLL:XX..XX
                            AA..AA is address,
                            LLLL is number of bytes,
                            XX..XX is data

    reply       OK          for success
                ENN         for an error (this includes the case
                            where only part of the data was
                            written).
**********************************************************************
    cont        cAA..AA     AA..AA is address to resume
                            If AA..AA is omitted,
                            resume at same address.
**********************************************************************
    step        sAA..AA     AA..AA is address to resume
                            If AA..AA is omitted,
                            resume at same address.
**********************************************************************
    last signal     ?               Reply the current reason for stopping.
                                        This is the same reply as is generated
                    for step or cont : SAA where AA is the
                    signal number.

    There is no immediate reply to step or cont.
    The reply comes when the machine stops.
    It is       SAA     AA is the "signal number"

    or...       TAAn...:r...;n:r...;n...:r...;
                    AA = signal number
                    n... = register number
                    r... = register contents
    or...       WAA     The process exited, and AA is
                    the exit status.  This is only
                    applicable for certains sorts of
                    targets.
    kill request    k

    toggle debug    d       toggle debug flag (see 386 & 68k stubs)
    reset       r       reset -- see sparc stub.
    reserved    <other>     On other requests, the stub should
                    ignore the request and send an empty
                    response ($#<checksum>).  This way
                    we can extend the protocol and GDB
                    can tell whether the stub it is
                    talking to uses the old or the new.
    search      tAA:PP,MM   Search backwards starting at address
                    AA for a match with pattern PP and
                    mask MM.  PP and MM are 4 bytes.
                    Not supported by all stubs.

    general query   qXXXX       Request info about XXXX.
    general set QXXXX=yyyy  Set value of XXXX to yyyy.
    query sect offs qOffsets    Get section offsets.  Reply is
                    Text=xxx;Data=yyy;Bss=zzz
    console output  Otext       Send text to stdout.  Only comes from
                    remote target.

    Responses can be run-length encoded to save space.  A '*' means that
    the next character is an ASCII encoding giving a repeat count which
    stands for that many repetitions of the character preceding the '*'.
    The encoding is n+29, yielding a printable character where n >=3
    (which is where rle starts to win).  Don't use an n > 126.

    So
    "0* " means the same as "0000".  */

#include <arch/arch.h>
#include <arch/gdb.h>

#include <dc/dcload.h>
#include <dc/scif.h>

#include "gdb_internal.h"

/* TRAPA #0x20: internal single-step or resume */
#define TRAPA_GDB_SINGLESTEP 32

/* TRAPA #0xFF: manually inserted via gdb_breakpoint() */
#define TRAPA_USER_BREAKPOINT 255

irq_context_t *irq_ctx;
static int remote_debug;

/*
    This function does all exception handling.  It only does two things -
    it figures out why it was called and tells gdb, and then it reacts
    to gdb's requests.
*/
static void gdb_handle_exception(int exception_vector) {
    char *ptr;

    handle_t_stop_reply(exception_vector);
    put_packet(remcom_out_buffer);

    undo_single_step();

    while(true) {
        remcom_out_buffer[0] = 0;
        ptr = (char *)get_packet();

        switch(*ptr++) {
            case '?': handle_t_stop_reply(exception_vector); break;
            case 'd':
                remote_debug = !remote_debug;
                break;
            case 'p': handle_read_reg(ptr); break;
            case 'P': handle_write_reg(ptr); break;
            case 'q': handle_query(ptr); break;
            case 'Q': handle_set_query(ptr); break;
            case 'T': handle_thread_alive(ptr); break;
            case 'H': handle_thread_select(ptr); break;
            case 'g': handle_read_regs(ptr); break;
            case 'G': handle_write_regs(ptr); break;
            case 'm': handle_read_mem(ptr); break;
            case 'M': handle_write_mem(ptr); break;
            case 'c':
            case 's':
                if(handle_continue_step(ptr))
                    return;
                break;
            case 'Z':
            case 'z':
                handle_breakpoint(ptr); break;
            case 'v':
                if(handle_v_packet(ptr))
                    return;
                break;
            case 'D': handle_detach(); return;
            case 'k': handle_kill(); return;
            default:
                break;
        }

        put_packet(remcom_out_buffer);
    }
}

void gdb_enter_exception(irq_context_t *context,
                         int exception_vector,
                         bool rewind_pc) {
    irq_ctx = context;

    if(rewind_pc)
        irq_ctx->pc -= 2;

    gdb_handle_exception(exception_vector);
}

irq_context_t *gdb_get_irq_context(void) {
    return irq_ctx;
}

irq_context_t *gdb_resolve_thread_context(int tid) {
    irq_context_t *default_context = gdb_get_irq_context();
    kthread_t *target;

    if(tid == GDB_THREAD_ALL || tid == GDB_THREAD_ANY)
        return default_context;

    target = thd_by_tid((tid_t)tid);
    if(!target)
        return default_context;

    if(default_context && target == thd_get_current())
        return default_context;

    return &target->context;
}


static void handle_exception(irq_t code, irq_context_t *context, void *data) {
    (void)data;

    gdb_enter_exception(context, code, false);
}

static void handle_user_trapa(irq_t code, irq_context_t *context, void *data) {
    (void)code;
    (void)data;

    gdb_enter_exception(context, EXC_TRAPA, false);
}

static void handle_gdb_trapa(irq_t code, irq_context_t *context, void *data) {
    /*
    * trapa 0x20 indicates a software trap inserted in
    * place of code ... so back up PC by one
    * instruction, since this instruction will
    * later be replaced by its original one!
    */
    (void)code;
    (void)data;

    gdb_enter_exception(context, EXC_TRAPA, true);
}

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger. */

void gdb_breakpoint(void) {
    __asm__ __volatile__("trapa	#0xff"::);
}

void gdb_init(void) {
    if(dcload_gdbpacket(NULL, 0, NULL, 0) == 0)
        using_dcl = 1;
    else
        scif_set_parameters(57600, 1);

    irq_set_handler(EXC_ILLEGAL_INSTR, handle_exception, NULL);
    irq_set_handler(EXC_SLOT_ILLEGAL_INSTR, handle_exception, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_READ, handle_exception, NULL);
    irq_set_handler(EXC_DATA_ADDRESS_WRITE, handle_exception, NULL);
    irq_set_handler(EXC_USER_BREAK_PRE, handle_exception, NULL);

    irq_set_handler(IRQ_TRAP_CODE(TRAPA_GDB_SINGLESTEP), handle_gdb_trapa, NULL);
    irq_set_handler(IRQ_TRAP_CODE(TRAPA_USER_BREAKPOINT), handle_user_trapa, NULL);

    gdb_breakpoint();
}
