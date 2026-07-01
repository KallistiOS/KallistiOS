/* KallistiOS ##version##

   arch/dreamcast/trap.c
   Copyright (C) 2025 Andress Barajas

   SH4/Dreamcast gprof backend.

   When built with -pg, GCC emits the following at each function entry:

       Offset   Bytes    Meaning
       --------------------------------------------------
         +0     2 bytes  trapa #33
         +2     2 bytes  alignment pad (a nop)
         +4     4 bytes  .long LABELNO
       --------------------------------------------------
         +8              real function body

   GCC forces the trapa to a 4-byte boundary, so the trap always lands at a
   fixed layout. EXC_TRAPA is a POST exception, so the saved context PC points
   at +2 (the pad). The real body is therefore at saved_pc + 6, regardless of
   whether the toolchain emits an explicit nop (older GCC) or an alignment pad
   (newer GCC).

*/

#include <stdint.h>

#include <arch/irq.h>
#include <kos/irq.h>

#include "../../gprof_internal.h"

/* Trap code GCC emits for -pg instrumentation on the SH4 (trapa #33). */
#define GPROF_TRAPA_CODE     33

/* Bytes from the saved (post-trapa) PC to the real function body:
   2 bytes of alignment padding + 4 bytes of the LABELNO constant. */
#define GPROF_RESUME_OFFSET  6

static void handle_gprof_trapa(irq_t code, irq_context_t *ctx, void *data) {
    (void)code;
    (void)data;

    uintptr_t resume_pc = CONTEXT_PC(*ctx) + GPROF_RESUME_OFFSET;

    /* Patch the saved PC so RTE resumes in the real function body. */
    CONTEXT_PC(*ctx) = resume_pc;

    /* At function entry, pr still holds the caller's return address. */
    gprof_mcount(ctx->pr, resume_pc);
}

void gprof_arch_start(void) {
    irq_set_handler(IRQ_TRAP_CODE(GPROF_TRAPA_CODE), handle_gprof_trapa, NULL);
}

void gprof_arch_stop(void) {
    irq_set_handler(IRQ_TRAP_CODE(GPROF_TRAPA_CODE), NULL, NULL);
}
