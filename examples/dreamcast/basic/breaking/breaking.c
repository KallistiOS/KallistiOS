#include <kos.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdalign.h>
#include <stdatomic.h>

#define VERIFY(expr) \
    do { \
        if(!(expr)) { \
            fprintf(stderr, "%s failed at %s:%d!\n", \
                    #expr, __FUNCTION__, __LINE__); \
            return false; \
        } \
    } while(0);

static atomic_bool handled_ = false;

static bool handlerfunc(const ubc_breakpoint_t *bp, 
                        const irq_context_t *ctx,
                        void *ud) {

    bool return_value = (bool)ud;
    handled_ = true;
    return return_value;
}

static int test_function(const char *str1, const char* str2) {
    return strcmp(str1, str2);
}

static bool break_on_instruction(void) {
    const ubc_breakpoint_t bp = {
        .address = test_function,
        .cond.access = ubc_access_instruction
    };

    handled_ = false;

    printf("Breaking on instructions... ");

    ubc_add_breakpoint(&bp, handlerfunc, (void *)true);

    volatile uintptr_t func;
    func = (uintptr_t)test_function; (void)func;
    VERIFY(!handled_);

    test_function("Sega", "Nintendo");
    /* BREAKPOINT EXPECTED HERE! */
    VERIFY(handled_);

    printf("SUCCESS!\n");

    return true;
}

static bool break_on_data_region_read(void) {
    char upper_boundary = 0;
    alignas(1024) volatile char vars[1024];
    char lower_boundary = 0;

    const ubc_breakpoint_t bp = {
        .address = (void *)vars,
        .cond = {
            .address_mask = ubc_address_mask_10,
            .rw           = ubc_rw_read
        }
    };

    handled_ = false;

    printf("Breaking on data region read... ");
    VERIFY(!((uintptr_t)vars & 0x3ff));

    ubc_add_breakpoint(&bp, handlerfunc, (void *)false);

    volatile char temp;
    temp = upper_boundary; (void)temp;
    VERIFY(!handled_);

    temp = lower_boundary;
    VERIFY(!handled_);

    vars[512] = 1;
    VERIFY(!handled_);

    temp = vars[0];
    VERIFY(handled_);

    handled_ = false;
    temp = vars[512];
    VERIFY(handled_);

    handled_ = false;
    temp = vars[1023];
    VERIFY(handled_);

    ubc_remove_breakpoint(&bp);

    printf("SUCCESS!\n");
    return true;
}

static bool break_on_sized_data_write_value(void) {
    uint16_t var;

    const ubc_breakpoint_t bp = {
        .address = &var,                  // address to break on
        .cond = {
            .access = ubc_access_operand, // instruction, operand, or both
            .rw     = ubc_rw_write,       // read, write, or both
            .size   = ubc_size_16bit      // 8, 16, 32, 64-bit, or any
        },
        .data = {
            .enabled = true,              // turn on data comparison
            .value   = 3                  // data to compare
        }
    };

    handled_ = false;

    printf("Breaking on sized data write with value... ");

    ubc_add_breakpoint(&bp, handlerfunc, (void *)true);

    volatile uint16_t tmp;
    tmp = var; (void)tmp;
    VERIFY(!handled_); //we only did a read

    var = 43;
    VERIFY(!handled_); // we wrote the wrong value

    *(uint8_t*)&var = 3;
    VERIFY(!handled_); //we accessed it as the wrong size

    var = 3;
    /* BREAKPOINT SHOULD TRIGGER HERE! */
    VERIFY(handled_); // wrote right value as the right size!

    printf("SUCCESS!\n");

    return true;
}

/* Watching for a read or write of a certain size, of a range of values, to or
   from a region of memory. */
static bool break_on_sized_operand_region_access_value_range(void) {
    uint32_t upper_boundary = 0;
    alignas(1024) volatile uint32_t vars[1024 / sizeof(uint32_t)];
    uint32_t lower_boundary = 0;

    const ubc_breakpoint_t bp = {
        .address = (void *)&vars,  // address to break on
        .cond = {
            .address_mask = ubc_address_mask_10, // don't care aabout lower 10 bits
            .access       = ubc_access_operand,  // instruction, operand, or both
            .size         = ubc_size_32bit       // 8, 16, 32, 64-bit, or any
        },
        .data = {
            .enabled = true,   // turn on data comparison
            .value   = 0x7ff,  // data to compare
            .mask    = 0x3     // mask off bottom two value bits to create a range
        }
    };

    handled_ = false;

    printf("Breaking on sized operand region access with value range... ");

    ubc_add_breakpoint(&bp, handlerfunc, (void *)false);

    // Read just above the region of interest
    volatile uint32_t tmp32;
    tmp32 = upper_boundary; (void)tmp32;
    VERIFY(!handled_);

    // Write just below the region of interest
    lower_boundary = tmp32; (void)lower_boundary;
    VERIFY(!handled_);

    // Write to the region of interest as the wrong data size
    *(bool *)vars = 0x3;
    VERIFY(!handled_);

    // Read from the region of interest as the wrong data size
    volatile int16_t tmp16; (void)tmp16;
    tmp16 = ((uint16_t *)vars)[1023 / sizeof(uint16_t)];
    VERIFY(!handled_);

    // Write to the region of interest an incorrect value
    vars[512 / sizeof(uint32_t)] = 0x8fd;
    VERIFY(!handled_);

    // Write to the region of interest an incorrect value
    vars[512 / sizeof(uint32_t)] = 0x3;
    VERIFY(!handled_);

    // Write to the region of interest an in-range value
    vars[512 / sizeof(uint32_t)] = 0x7ff;
    VERIFY(handled_);
    handled_ = false;

    // Write to the region of interest an in-range value
    vars[512 / sizeof(uint32_t)] = 0x7fd;
    VERIFY(handled_);
    handled_ = false;

    // Read from the region of interest an in-range value
    tmp32 = vars[512 / sizeof(uint32_t)];
    VERIFY(handled_);

    printf("SUCCESS!\n");

    ubc_remove_breakpoint(&bp);

    return true;
}

//test_function

// Sequential breakpoint
bool break_on_sequence(void) {
    alignas(1024) static uint32_t vars[1024 / sizeof(uint32_t)];

    /* Create a sequential breakpoint on the conditions:
            a. instruction - break on test_function
            b. operand - break on writing value range to data range */
    ubc_add_breakpoint(&(static ubc_breakpoint_t) {
            .address = test_function,
            .next = &(static ubc_breakpoint_t) {
                .address = (void *)&vars,
                .cond = {
                    .address_mask = ubc_address_mask_10,
                    .access       = ubc_access_operand,
                    .rw           = ubc_rw_write,
                    .size         = ubc_size_32bit
                },
                .data = {
                    .enabled = true,
                    .value   = 0x7fc,
                    .mask    = 0x3
                }
            }
        },
        handlerfunc,
        (void *)false);

    /* Reset whether we've been handled. */
    handled_ = false;

    printf("Breaking on sequence... ");

    vars[0] = 0xfc;
    VERIFY(!handled_); /* Wrote wrong value without condition A first */

    test_function("Sega", "Sony");
    VERIFY(!handled_);  /* Hit condition A, but not B. */

    *(uint16_t *)vars = 0x7fd;
    VERIFY(!handled_); /* Wrote wrong value as wrong sized type .*/

    vars[512 / sizeof(uint32_t)] = 0xfc;
    VERIFY(!handled_); /* Wrote wrong value. */

    vars[0] = 0x7fc;
    VERIFY(handled_); /* Wrote correct value, fulfilling condition B */

    printf("SUCCESS!\n");

    return true;
}

int main(int argc, char* argv[]) {
    bool success = true;

    success &= break_on_instruction();
    success &= break_on_data_region_read();
    success &= break_on_sized_data_write_value();
    success &= break_on_sized_operand_region_access_value_range();
    success &= break_on_sequence();
    // Verify ubc_add_breakpoint() fails gracefully

    if(success) {
        printf("\n***** Breakpoint Test: SUCCESS *****\n");
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "\n***** Breakpoint Test: FAILURE *****\n");
        return EXIT_FAILURE;
    }
}