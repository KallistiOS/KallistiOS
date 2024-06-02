/* KallistiOS ##version##

   fpu_exc.c
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

/*
    This file serves as both an example of and an automatable test case for
    working with the SH4's FPU state and exceptions.
*/

#include <kos.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#ifdef __SH4_SINGLE_ONLY__
#   define SH4_FPU_CONFIG_NAME  "-m4-single-only"
#elif defined(__SH4_SINGLE__)
#   define SH4_FPU_CONFIG_NAME  "-m4-single"
#else
#   define SH4_FPU_CONFIG_NAME  "unknown"
#endif

static const char*
fpscr_stringify(unsigned int value, char* buffer, size_t bytes) {

    snprintf(buffer, bytes,
            "\tFPSCR [%x]:\n"
            "\t\tFR     = %d\n"
            "\t\tSZ     = %d\n"
            "\t\tPR     = %d\n"
            "\t\tDN     = %d\n"
            "\t\tCause  = %x\n"
            "\t\tEnable = %x\n"
            "\t\tFlag   = %x\n"
            "\t\tRM     = %x\n",
            value,
            !!(value & (1 << 21)),
            !!(value & (1 << 20)),
            !!(value & (1 << 19)),
            !!(value & (1 << 18)),
            (value >> 12) & 0x3f,
            (value >> 7) & 0x1f,
            (value >> 2) & 0x1f,
            value & 0x3);

    return buffer;
}

static bool fpscr_test(const char *name, unsigned mask, void (*test)(void)) {
    char buffer[512];
    bool success = true;

    const unsigned begin_fpscr = __builtin_sh_get_fpscr();

    printf("Beginning %s test!\n", name);

    test();

    unsigned fpscr = __builtin_sh_get_fpscr();
    printf("%s", fpscr_stringify(fpscr, buffer, sizeof(buffer)));

    if(!(fpscr & mask)) {
        fprintf(stderr, "\tFAILURE: %s flag not asserted!\n", name);
        success = false;
    }
    else {
        printf("\tSUCCESS!\n");
    }

    __builtin_sh_set_fpscr(begin_fpscr);

    return success;
}

static void fpscr_underflow(void) {
    volatile double d = 1.0;
    while(d > 0.0) {
//        printf("\t%.15lf\n", d);
        d *= 0.01;
    }
}

static void fpscr_overflow(void) {
    volatile double d = 1.0;
    while(d < DBL_MAX) {
//        printf("\t%.15lf\n", d);
        d *= 1.1;
    }
}

static void fpscr_nan(void) {
    volatile double d = 0.0;
    volatile double c = 0.0;
    volatile double e = d / c;
    (void)e;
}

static void fpscr_div_zero(void) {
    volatile double d = 1.0;
    volatile double c = 0.0;
    volatile double e = d / c;
    (void)e;
}

int main(int argc, char **argv) {
    char buffer[512];
    bool success = true;

    /* Exit parachute */
    cont_btn_callback(0, CONT_START, (cont_btn_callback_t)arch_exit);

    printf("Beginning the FPU exception test!\n");
    printf("\tFPU Config: %s\n", SH4_FPU_CONFIG_NAME);
    printf("\tsizeof(float): %zu\n", sizeof(float));
    printf("\tsizeof(double): %zu\n", sizeof(double));

    unsigned fpscr_start = __builtin_sh_get_fpscr();
    printf("Original Value:\n%s", fpscr_stringify(fpscr_start, buffer, sizeof(buffer)));

    success &= fpscr_test("underflow",     (1 << 3), fpscr_underflow);
    success &= fpscr_test("overflow",      (1 << 4), fpscr_overflow);
    success &= fpscr_test("divde-by-zero", (1 << 5), fpscr_div_zero);
    success &= fpscr_test("NAN",           (1 << 6), fpscr_nan);

    if(success) {
        printf("\nTEST SUCCEEDED!\n");
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "\nTEST FAILED!\n");
        return EXIT_FAILURE;
    }

}


