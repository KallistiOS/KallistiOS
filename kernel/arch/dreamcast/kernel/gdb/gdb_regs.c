/* KallistiOS ##version##

   arch/dreamcast/kernel/gdb/gdb_regs.c

   Copyright (C) Megan Potter
   Copyright (C) Richard Moats
   Copyright (C) 2026 Andy Barajas

*/

/*
   Implements SH4 register access for the GDB remote stub.

   This module provides:
     - g / G access to the raw SH4 register block expected by GDB
     - p / P access to individual raw registers and the implemented pseudo registers
     - register formatting for T stop replies
     - thread-selected register contexts via Hg packets

   Bulk g/G packets cover only the raw SH4 register block. Implemented pseudo
   registers such as drN and fvN are available through single-register access.
*/

#include "gdb_internal.h"

/* Map from KOS register context order to GDB sh4 order */
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
   GDB's raw SH4 remote register block is 67 32-bit registers:
     - 41 directly mapped registers we expose from irq_context_t
     - 18 unavailable raw slots for ssr, spc, and banked r0-r7 values
     - 8 reserved blank raw slots

   Pseudo registers like dr0-dr14 and fv0-fv12 are not part of the g/G block.
 */
#define BASE_REG_COUNT         ((int)(sizeof(kos_reg_map) / sizeof(kos_reg_map[0])))
#define UNAVAILABLE_REG_COUNT  18
#define RESERVED_REG_COUNT     8
#define RAW_REG_COUNT          (BASE_REG_COUNT + UNAVAILABLE_REG_COUNT + RESERVED_REG_COUNT)
#define PSEUDO_ZERO_REGNUM     RAW_REG_COUNT
#define DOUBLE_REG_BASE        (PSEUDO_ZERO_REGNUM + 1)
#define DOUBLE_REG_COUNT       8
#define VECTOR_REG_BASE        (DOUBLE_REG_BASE + DOUBLE_REG_COUNT)
#define VECTOR_REG_COUNT       4

static int32_t gdb_thread_for_regs = GDB_THREAD_ANY;
static irq_context_t *regs_irq_ctx;

/* Sets the current thread ID used for register operations. */
void set_regs_thread(int tid) {
    gdb_thread_for_regs = tid;
}

/*
   Selects the appropriate IRQ context for the current register thread.

   When the selected thread is the one currently stopped in the debugger, this
   must use the live exception frame rather than the dormant kthread context.
*/
void setup_regs_context(void) {
    regs_irq_ctx = gdb_resolve_thread_context(gdb_thread_for_regs);
}

/* Packs two FR register words into one pseudo double register (drN). */
static uint64_t build_dr(uint32_t fr_low, uint32_t fr_high) {
    return ((uint64_t)fr_high << 32) | fr_low;
}

/* Packs four FR register words into one pseudo vector register (fvN). */
static void build_fv(uint32_t *fv, const irq_context_t *context, int base) {
    fv[0] = context->fr[base + 0];
    fv[1] = context->fr[base + 1];
    fv[2] = context->fr[base + 2];
    fv[3] = context->fr[base + 3];
}

/*
   Appends a single register to the GDB T packet in stop reply format.

   Format: nn:vvvvvvvv;
     - nn: register number (2 hex digits)
     - vvvvvvvv: value encoded in hex (endianness-respecting)
     - Ends with a semicolon

   Returns pointer to the end of the output buffer, or NULL if the full field
   would not fit in the remaining space.
*/
static char *append_reg(char *out, size_t *remaining, int regnum,
                        const void *value, size_t size) {
    size_t needed = 2u + 1u + (size * 2u) + 1u;

    if(!remaining || (needed + 1u) > *remaining)
        return NULL;

    *out++ = highhex(regnum);
    *out++ = lowhex(regnum);
    *out++ = ':';
    out = mem_to_hex((const char *)value, out, size);
    *out++ = ';';
    *out = '\0';
    *remaining -= needed;

    return out;
}

/*
   Appends the mapped base SH4 registers to a GDB T stop reply.

   Includes:
     - General-purpose registers (r0-r15)
     - Control registers (pc, pr, gbr, vbr, mach, macl, sr, fpul, fpscr)
     - Floating-point registers (fr0-fr15)

   Does not include unavailable raw g/G slots or pseudo registers.

   Returns a pointer to the end of the output buffer. If the buffer fills up,
   only complete register fields are emitted and the reply remains
   null-terminated.
*/
char *append_regs(char *out, size_t *remaining, const irq_context_t *context) {
    for(int i = 0; i < BASE_REG_COUNT; ++i) {
        const uint32_t *reg_ptr =
            (const uint32_t *)((uintptr_t)context + kos_reg_map[i]);
        char *next = append_reg(out, remaining, i, reg_ptr, sizeof(*reg_ptr));

        if(!next)
            break;

        out = next;
    }

    return out;
}

static char *append_zero_reg_hex(char *out) {
    static const uint32_t zero;
    return mem_to_hex((const char *)&zero, out, sizeof(zero));
}

/*
   Decodes a fixed number of hex bytes after validating every nibble.

   This helper is used for register payloads where silent garbage would be
   unsafe. It converts exactly count output bytes and fails if any input
   character is not hexadecimal.
*/
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

/*
   Parses a register payload whose encoded width must match exactly.

   The input must be exactly size * 2 hex characters long and terminate
   immediately after the encoded value. This prevents partial or overlong
   single-register writes from being accepted accidentally.
*/
static bool parse_register_hex_exact(const char *in, void *out, size_t size) {
    if(!in || in[size * 2u] != '\0')
        return false;

    return hex_to_mem_checked_n(in, out, size);
}

/*
   Validates that a fixed-width span consists only of hexadecimal digits.

   This is used for the bulk G packet before consuming the raw SH4 register
   block, so malformed characters can be rejected before any register state is
   written back into irq_context_t.
*/
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
   Encodes one raw or pseudo register into GDB hex form.

   Base SH4 registers are read from the selected IRQ context, unavailable and
   reserved raw slots are reported as zero, and implemented pseudo registers
   such as drN and fvN are synthesized from the floating-point register bank.
*/
static bool read_register_hex(char *out, int regnum) {
    irq_context_t *context;

    if(regnum < 0)
        return false;

    setup_regs_context();
    context = regs_irq_ctx;

    if(regnum < BASE_REG_COUNT) {
        uint32_t *reg_ptr =
            (uint32_t *)((uintptr_t)context + kos_reg_map[regnum]);
        mem_to_hex((const char *)reg_ptr, out, sizeof(*reg_ptr));
        return true;
    }

    regnum -= BASE_REG_COUNT;

    if(regnum < UNAVAILABLE_REG_COUNT + RESERVED_REG_COUNT) {
        append_zero_reg_hex(out);
        return true;
    }

    regnum -= UNAVAILABLE_REG_COUNT + RESERVED_REG_COUNT;

    if(regnum == 0) {
        append_zero_reg_hex(out);
        return true;
    }

    --regnum;

    if(regnum < DOUBLE_REG_COUNT) {
        int base = regnum * 2;
        uint64_t dr = build_dr(context->fr[base], context->fr[base + 1]);
        mem_to_hex((const char *)&dr, out, sizeof(dr));
        return true;
    }

    regnum -= DOUBLE_REG_COUNT;

    if(regnum < VECTOR_REG_COUNT) {
        int base = regnum * 4;
        uint32_t fv[4];
        build_fv(fv, context, base);
        mem_to_hex((const char *)fv, out, sizeof(fv));
        return true;
    }

    return false;
}

/*
   Decodes and writes one raw or pseudo register from GDB hex form.

   Base SH4 registers are written directly into the selected IRQ context.
   Unavailable and reserved raw slots are accepted only for layout
   compatibility and ignored. Implemented pseudo registers such as drN and fvN
   are unpacked back into the underlying floating-point register words.
*/
static bool write_register_hex(int regnum, const char *in) {
    irq_context_t *context;
    uint32_t value32;

    if(regnum < 0)
        return false;

    setup_regs_context();
    context = regs_irq_ctx;

    if(regnum < BASE_REG_COUNT) {
        if(!parse_register_hex_exact(in, &value32, sizeof(value32)))
            return false;

        *(uint32_t *)((uintptr_t)context + kos_reg_map[regnum]) = value32;
        return true;
    }

    regnum -= BASE_REG_COUNT;

    if(regnum < UNAVAILABLE_REG_COUNT + RESERVED_REG_COUNT) {
        uint32_t ignored;
        return parse_register_hex_exact(in, &ignored, sizeof(ignored));
    }

    regnum -= UNAVAILABLE_REG_COUNT + RESERVED_REG_COUNT;

    if(regnum == 0) {
        uint32_t ignored;
        return parse_register_hex_exact(in, &ignored, sizeof(ignored));
    }

    --regnum;

    if(regnum < DOUBLE_REG_COUNT) {
        int base = regnum * 2;
        uint64_t dr = 0;

        if(!parse_register_hex_exact(in, &dr, sizeof(dr)))
            return false;

        context->fr[base] = (uint32_t)(dr & 0xffffffffu);
        context->fr[base + 1] = (uint32_t)(dr >> 32);
        return true;
    }

    regnum -= DOUBLE_REG_COUNT;

    if(regnum < VECTOR_REG_COUNT) {
        int base = regnum * 4;
        uint32_t fv[4];

        if(!parse_register_hex_exact(in, fv, sizeof(fv)))
            return false;

        for(int i = 0; i < 4; i++)
            context->fr[base + i] = fv[i];
        return true;
    }

    return false;
}

/*
   Handle the 'p' command.

   Reads one raw GDB register slot from the current register-selection context.
   Format: pNN where NN is the register number in hex.

   The reply width depends on the selected raw register definition. Invalid,
   unmapped, or malformed register requests return EINVAL.
*/
void handle_read_reg(char *ptr) {
    uint32_t regnum = 0;

    if(!hex_to_int(&ptr, &regnum) || *ptr != '\0' ||
       !read_register_hex(remcom_out_buffer, (int)regnum)) {
        gdb_error_with_code_str(GDB_EINVAL, "p: invalid register request");
    }
}

/*
   Handle the 'P' command.

   Writes one raw GDB register slot in the current register-selection context.
   Format: PNN=... where NN is the register number in hex.

   The payload width must match the selected register exactly. Invalid register
   numbers, malformed packet syntax, and bad hex payloads return EINVAL.
*/
void handle_write_reg(char *ptr) {
    uint32_t regnum = 0;

    if(!hex_to_int(&ptr, &regnum) || *ptr++ != '=' ||
       !write_register_hex((int)regnum, ptr)) {
        gdb_error_with_code_str(GDB_EINVAL, "P: invalid register write");
        return;
    }

    gdb_put_ok();
}

/*
   Handle the 'g' command.

   Returns the full raw SH4 g/G register block expected by GDB.
   Format: g

   This includes the mapped base registers plus zero-filled unavailable and
   reserved raw slots; pseudo registers are not part of the bulk packet.
*/
void handle_read_regs(char *ptr) {
    (void)ptr;

    char *out = remcom_out_buffer;
    irq_context_t *context;

    setup_regs_context();
    context = regs_irq_ctx;

    for(int i = 0; i < BASE_REG_COUNT; i++)
        out = mem_to_hex((char *)((uintptr_t)context + kos_reg_map[i]), out, 4);

    for(int i = 0; i < UNAVAILABLE_REG_COUNT + RESERVED_REG_COUNT; i++)
        out = append_zero_reg_hex(out);

    *out = 0;
}

/*
   Handle the 'G' command.

   Consumes the full raw SH4 g/G register block supplied by GDB.
   Format: G<raw-register-hex-block>

   Only the mapped base registers are written back to irq_context_t;
   unavailable and reserved raw slots are accepted and ignored.
*/
void handle_write_regs(char *ptr) {
    char *in = ptr;
    size_t remaining = strlen(in);
    uint32_t values[BASE_REG_COUNT];
    irq_context_t *context;

    if(remaining < (size_t)RAW_REG_COUNT * 8) {
        gdb_error_with_code_str(GDB_EINVAL, "G: short register payload");
        return;
    }

    if(!validate_hex_span(in, (size_t)RAW_REG_COUNT * 8u)) {
        gdb_error_with_code_str(GDB_EINVAL, "G: invalid register payload");
        return;
    }

    setup_regs_context();
    context = regs_irq_ctx;

    for(int i = 0; i < BASE_REG_COUNT; i++, in += 8) {
        if(!hex_to_mem_checked_n(in, &values[i], sizeof(values[i]))) {
            gdb_error_with_code_str(GDB_EINVAL, "G: invalid register payload");
            return;
        }
    }

    for(int i = 0; i < BASE_REG_COUNT; ++i)
        *(uint32_t *)((uintptr_t)context + kos_reg_map[i]) = values[i];

    in += (UNAVAILABLE_REG_COUNT + RESERVED_REG_COUNT) * 8;

    gdb_put_ok();
}
