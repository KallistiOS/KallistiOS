/* KallistiOS ##version##

   kernel/gdb/gdb_regs.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

#include "gdb_internal.h"

/*
 * Number of bytes for registers
 */
#define NUM_REG_BYTES    41*4

/* map from KOS register context order to GDB sh4 order */
#define KOS_REG(r)      offsetof(irq_context_t, r)

uint32_t kos_reg_map[] = {
    KOS_REG(r[0]), KOS_REG(r[1]), KOS_REG(r[2]), KOS_REG(r[3]),
    KOS_REG(r[4]), KOS_REG(r[5]), KOS_REG(r[6]), KOS_REG(r[7]),
    KOS_REG(r[8]), KOS_REG(r[9]), KOS_REG(r[10]), KOS_REG(r[11]),
    KOS_REG(r[12]), KOS_REG(r[13]), KOS_REG(r[14]), KOS_REG(r[15]),

    KOS_REG(pc), KOS_REG(pr), KOS_REG(gbr), KOS_REG(vbr),
    KOS_REG(mach), KOS_REG(macl), KOS_REG(sr),
    KOS_REG(fpul), KOS_REG(fpscr),

    KOS_REG(fr[0]), KOS_REG(fr[1]), KOS_REG(fr[2]), KOS_REG(fr[3]),
    KOS_REG(fr[4]), KOS_REG(fr[5]), KOS_REG(fr[6]), KOS_REG(fr[7]),
    KOS_REG(fr[8]), KOS_REG(fr[9]), KOS_REG(fr[10]), KOS_REG(fr[11]),
    KOS_REG(fr[12]), KOS_REG(fr[13]), KOS_REG(fr[14]), KOS_REG(fr[15])
};

#undef KOS_REG

/*
 * Handle the 'g' command.
 * Returns the full set of general-purpose registers.
 */
void handle_read_regs(char *ptr) {
    (void)ptr;

    char *out = remcom_out_buffer;
    for(int i = 0; i < NUM_REG_BYTES / 4; i++)
        out = mem_to_hex((char *)((uint32_t)irq_ctx + kos_reg_map[i]), out, 4);
    *out = 0;
}

/*
 * Handle the 'G' command.
 * Writes to all general-purpose registers.
 * Format: Gxxxxxxxx.... (entire register state in hex).
 */
void handle_write_regs(char *ptr) {
    char *in = ptr;
    for(int i = 0; i < NUM_REG_BYTES / 4; i++, in += 8)
        hex_to_mem(in, (char *)((uint32_t)irq_ctx + kos_reg_map[i]), 4);
    strcpy(remcom_out_buffer, GDB_OK);
}
