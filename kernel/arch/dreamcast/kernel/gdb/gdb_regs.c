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

static bool hex_to_mem_checked_n(const char *src, void *dest, size_t count) {
    unsigned char *out = (unsigned char *)dest;

    if(!src || !dest)
        return false;

    for(size_t i = 0; i < count; ++i) {
        int high = hex(src[(i * 2u)]);
        int low = hex(src[(i * 2u) + 1u]);

        if(high < 0 || low < 0)
            return false;

        out[i] = (unsigned char)((high << 4) | low);
    }

    return true;
}

static bool validate_hex_span(const char *in, size_t hex_chars) {
    if(!in)
        return false;

    for(size_t i = 0; i < hex_chars; ++i) {
        if(hex(in[i]) < 0)
            return false;
    }

    return in[hex_chars] == '\0';
}

/*
 * Handle the 'g' command.
 * Returns the full set of general-purpose registers.
 */
void handle_read_regs(char *ptr) {
    (void)ptr;

    char *out = remcom_out_buffer;
    for(int i = 0; i < NUM_REG_BYTES / 4; i++)
        out = mem_to_hex((char *)((uintptr_t)irq_ctx + kos_reg_map[i]), out, 4);
    *out = 0;
}

/*
 * Handle the 'G' command.
 * Writes to all general-purpose registers.
 * Format: Gxxxxxxxx.... (entire register state in hex).
 */
void handle_write_regs(char *ptr) {
    char *in = ptr;

    if(!validate_hex_span(ptr, NUM_REG_BYTES * 2u)) {
        strcpy(remcom_out_buffer, "E01");
        return;
    }

    for(int i = 0; i < NUM_REG_BYTES / 4; i++, in += 8) {
        if(!hex_to_mem_checked_n(in, (char *)((uintptr_t)irq_ctx + kos_reg_map[i]), 4)) {
            strcpy(remcom_out_buffer, "E01");
            return;
        }
    }

    strcpy(remcom_out_buffer, GDB_OK);
}
