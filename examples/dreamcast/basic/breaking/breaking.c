/* KallistiOS ##version##

   breaking.c
   Copyright (C) 2024 Falco Girgis

*/

/*  This program serves as an example for using the API around the SH4's "User
    Break Controller" to create and manage breakpoints. It was also written to
    serve as a series of automatable tests validating the implementation of the
    driver in KOS.

    The following breakpoint configurations are tested:
        - breaking on instructions
        - breaking on reading from a region of memory
        - breaking on writing a particular value with a particular sized access
          to a memory location
        - breaking on accessing a region of memory with a particular sized
          access with a particular range of values
        - sequential breaking on accessing an instruction followed by
          accessing a region of memory with a particular sized access with a
          particular range of values
*/

#include <dc/ubc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include <stdatomic.h>

/*  Convenience macro we use throughout the test suite to ensure our breakpoint
    has or has not fired as expected at the given point in program execution. */
#define VERIFY(expr) \
    do { \
        if(!(expr)) { \
            fprintf(stderr, "%s failed at %s:%d!\n", \
                    #expr, __FUNCTION__, __LINE__); \
            return false; \
        } \
    } while(0);

/* Atomic used to flag whether our breakpoint has been handled. */
static atomic_bool handled = false;

/* Callback function used to handle a breakpoint request. */
static bool on_break(const ubc_breakpoint_t *bp,
                     const irq_context_t *ctx,
                     void *ud) {
    /* Don't warn about unused bp */
    (void)bp;

    /* Signal to the outside that the breakpoint has been handled. */
    handled = true;

    /* Print the location of the program counter when the breakpoint
       IRQ was signaled (minus 2 if we're breaking AFTER instruction
       execution!) */
    printf("\tBREAKPOINT HIT! [PC = %x]\n", (unsigned)CONTEXT_PC(*ctx) - 2);

    /* Userdata pointer used to hold a boolean used as the return value, which
       dictates whether a breakpoint persists or is removed after being
       handled. */
    return (bool)ud;
}

/* Dummy test function used as breakpoint instruction target. */
static __noinline int test_function(const char *str1, const char* str2) {
    return strcmp(str1, str2);
}

/* Configures and validates a generic breakpoint on an instruction address. */
static bool break_on_instruction(void) {
    /* Create a bare-minimal breakpoint which will break on any access to its
       address. */
    const ubc_breakpoint_t bp = {
        .address = test_function, /* Address of interest */
    };

    /* Reset our breakpoint handler flag. */
    handled = false;

    printf("Breaking on instruction...\n");

    /* Add the breakpoint with our handler and our userdata */
    VERIFY(ubc_add_breakpoint(&bp, on_break, (void *)true));

    /* Call our test_function, which should trigger the breakpoint! */
    volatile int result = test_function("Sega", "Nintendo");
    (void)result;

    /* BREAKPOINT EXPECTED HERE! */

    /* Verify our breakpoint was actually handled. */
    VERIFY(handled);

    printf("\tSUCCESS!\n");

    return true;
}

/* Configures and validates a read-only data watchpoint on a range of
   addresses. */
static bool break_on_data_region_read(void) {
    char upper_boundary = 0;
    alignas(1024) char vars[1024];
    char lower_boundary = 0;

    /* Creates a data breakpoint for anything that does a read from an address
       that matches the upper 22 bits of "vars." */
    const ubc_breakpoint_t bp = {
        /* Address of interest */
        .address      = vars,
        /* Low address bits to ignore, creating an address range */
        .address_mask = ubc_address_mask_10,
        /* Operand/Data access only */
        .access       = ubc_access_operand,
        /* Operand-access breakpoint settings */
        .operand = {
            /* Read-only */
            .rw = ubc_rw_read
        }
    };

    handled = false;

    printf("Breaking on data region read...\n");

    VERIFY(ubc_add_breakpoint(&bp, on_break, (void *)false));

    /* Read from above the specified range. */
    volatile char temp;
    temp = upper_boundary; (void)temp;
    VERIFY(!handled);

    /* Read from below the specified range. */
    temp = lower_boundary;
    VERIFY(!handled);

    /* Write to the specified range. */
    vars[512] = 1;
    VERIFY(!handled);

    /* Read from the start of the specified range. */
    temp = vars[0];
    VERIFY(handled);
    handled = false;

    /* Read from the middle of the specified range. */
    temp = vars[512];
    VERIFY(handled);
    handled = false;

    /* Read from the end of the specified range. */
    temp = vars[1023];
    VERIFY(handled);

    VERIFY(ubc_remove_breakpoint(&bp));

    printf("\tSUCCESS!\n");

    return true;
}

/* Configures and validates a data watchpoint looking for a particular value
   to be written to a particular address as a particular size. */
static bool break_on_sized_data_write_value(void) {
    uint16_t var;

    /* Create a write-only data breakpoint on the address of "var," looking for
       the value of `3` to be written. */
    const ubc_breakpoint_t bp = {
        .address = &var,                  /* address to break on */
        .access  = ubc_access_operand,    /* instruction, operand, or both */
        .operand = {
            .rw     = ubc_rw_write,       /* read, write, or both */
            .size   = ubc_size_16bit,     /* 8, 16, 32, 64-bit, or any */
            .data = {
                .enabled = true,          /* turn on data comparison */
                .value   = 3              /* data to compare */
            }
        }
    };

    handled = false;

    printf("Breaking on sized data write with value...\n");

    VERIFY(ubc_add_breakpoint(&bp, on_break, (void *)true));

    volatile uint16_t tmp;
    tmp = var; (void)tmp;
    VERIFY(!handled); /* We only did a read. */

    var = 43;
    VERIFY(!handled); /* We wrote the wrong value. */

    *(uint8_t*)&var = 3;
    VERIFY(!handled); /* We accessed it as the wrong size. */

    var = 3;
    /* BREAKPOINT SHOULD TRIGGER HERE! */
    VERIFY(handled); /* We wrote right value as the right size! */

    printf("\tSUCCESS!\n");

    return true;
}

/* Watching for a read or write of a certain size, of a range of values, to or
   from a region of memory. */
static bool break_on_sized_operand_region_access_value_range(void) {
    uint32_t upper_boundary = 0;
    alignas(1024) uint32_t vars[1024 / sizeof(uint32_t)];
    uint32_t lower_boundary = 0;

    /* Create a read/write data breakpoint which looks for a sized operand
       access to or from a range of addresses with a range of values. */
    const ubc_breakpoint_t bp = {
        .address      = vars,                /* address to break on */
        .address_mask = ubc_address_mask_10, /* create address range */
        .access       = ubc_access_operand,  /* break on data access */
        .operand = {
            .size     = ubc_size_32bit,      /* break on dword accesses */
            .data = {
                .enabled = true,   /* turn on data comparison */
                .value   = 0x7ff,  /* data to compare */
                .mask    = 0x3     /* mask off 2 bits to create a value range */
            }
        }
    };

    handled = false;

    printf("Breaking on sized operand region access with value range...\n");

    VERIFY(ubc_add_breakpoint(&bp, on_break, (void *)false));

    /* Read just above the region-of-interest. */
    volatile uint32_t tmp32;
    tmp32 = upper_boundary; (void)tmp32;
    VERIFY(!handled);

    /* Write just below the region-of-interest. */
    lower_boundary = tmp32; (void)lower_boundary;
    VERIFY(!handled);

    /* Write to the region-of-interest as the wrong data size. */
    *(bool *)vars = 0x3;
    VERIFY(!handled);

    /* Read from the region-of-interest as the wrong data size. */
    volatile uint16_t tmp16; (void)tmp16;
    tmp16 = ((uint16_t *)vars)[1023 / sizeof(uint16_t)];
    VERIFY(!handled);

    /* Write to the region-of-interest an incorrect value. */
    vars[512 / sizeof(uint32_t)] = 0x8fd;
    VERIFY(!handled);

    /* Write to the region-of-interest an incorrect value. */
    vars[512 / sizeof(uint32_t)] = 0x3;
    VERIFY(!handled);

    /* Write to the region-of-interest an in-range value. */
    vars[512 / sizeof(uint32_t)] = 0x7ff;
    VERIFY(handled);
    handled = false;

    /* Write to the region-of-interest an in-range value. */
    vars[512 / sizeof(uint32_t)] = 0x7fd;
    VERIFY(handled);
    handled = false;

    /* Read from the region-of-interest an in-range value. */
    tmp32 = vars[512 / sizeof(uint32_t)];
    VERIFY(handled);

    VERIFY(ubc_remove_breakpoint(&bp));

    printf("\tSUCCESS!\n");

    return true;
}

/* Configure and validate a sequential breakpoint. */
bool break_on_sequence(void) {
    alignas(1024) static uint32_t vars[1024 / sizeof(uint32_t)];

    /* Create a sequential breakpoint on the conditions:
            a. instruction - break on test_function
            b. operand - break on writing value range to data range */
    VERIFY(ubc_add_breakpoint(&(static ubc_breakpoint_t) {
                /* Address of first breakpoint */
                .address = test_function,
                /* Second breakpoint in the sequence */
                .next = &(static ubc_breakpoint_t) {
                    .address      = &vars,
                    .address_mask = ubc_address_mask_10,
                    .access       = ubc_access_operand,
                    .operand = {
                        .rw       = ubc_rw_write,
                        .size     = ubc_size_32bit,
                        .data = {
                            .enabled = true,
                            .value   = 0x7fc,
                            .mask    = 0x3
                        }
                    },
                }
            },
            on_break,
            (void *)true));

    /* Reset whether we've been handled. */
    handled = false;

    printf("Breaking on sequence...\n");

    vars[0] = 0xfc;
    VERIFY(!handled); /* Wrote wrong value without condition A first */

    test_function("Sega", "Sony");
    VERIFY(!handled);  /* Hit condition A, but not B. */

    *(uint16_t *)vars = 0x7fd;
    VERIFY(!handled); /* Wrote wrong value as wrong sized type .*/

    vars[512 / sizeof(uint32_t)] = 0xfc;
    VERIFY(!handled); /* Wrote wrong value. */

    vars[0] = 0x7fc;
    VERIFY(handled); /* Wrote correct value, fulfilling condition B */

    printf("\tSUCCESS!\n");

    return true;
}

/* Program entry point. */
int main(int argc, char* argv[]) {
    bool success = true;

    printf("Testing breakpoints...\n\n");

    /* Run test cases and accumulate results. */
    success &= break_on_instruction();
    success &= break_on_data_region_read();
    success &= break_on_sized_data_write_value();
    success &= break_on_sized_operand_region_access_value_range();
    success &= break_on_sequence();

    if(success) {
        printf("\n***** Breakpoint Test: SUCCESS *****\n");
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "\n***** Breakpoint Test: FAILURE *****\n");
        return EXIT_FAILURE;
    }
}
