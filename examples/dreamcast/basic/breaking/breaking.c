#include <kos.h>
#include <stdio.h>
#include <assert.h>
#include <stdatomic.h>

static atomic_bool handled_ = false;

static bool handlerfunc(const ubc_breakpoint_t *bp, 
                        const irq_context_t *ctx,
                        void *ud) {

    printf("BREAKPOINT HIT!\n");

    handled_ = true;
    return true;
}

static void break_on_sized_data_write_value(void) {
    uint16_t var, tmp;

    ubc_breakpoint_t bp = {
        .address = &var,                  // address to break on
        .cond = {
            .access = ubc_access_operand, // instruction, operand, or both
            .rw     = ubc_rw_write,       // read, write, or both
            .size   = ubc_size_word       // byte, word, longword, quadword
        },
        .data = {
            .enabled = true,              // turn on data comparison
            .value   = 3                  // data to compare
        }
    };

    ubc_enable_breakpoint(&bp,handlerfunc, NULL);

    tmp = var; (void)tmp;
    assert(!handled_); //we only did a read

    var = 43;
    assert(!handled_); // we wrote the wrong value

    *(uint8_t*)&var = 3;
    assert(!handled_); //we accessed it as the wrong size

    var = 3;
    /* BREAKPOINT SHOULD TRIGGER HERE */
    assert(handled_); // wrote right value as the right size!

    printf("Success!\n");
}

int main(int argc, char* argv[]) {
    break_on_sized_data_write_value();

    return 0;
}