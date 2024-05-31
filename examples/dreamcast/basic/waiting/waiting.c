/* KallistiOS ##version##

   waiting.c
   Copyright (C) 2024 Falco Girgis

*/

/*  This program serves as an example for using the API around the SH4's "User
    Break Controller" to create and manage breakpoints. It was also written to
    serve as a series of automatable tests validating the implementation of the
    driver in KOS.
*/

#include <kos.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include <stdatomic.h>


static inline double percent_diff(double v1, double v2) {
    return fabs(v1 - v2) / ((v1 + v2) / 2.0) * 100.0;
}

#define GENERATE_TESTER(name, type, measure)                        \
    bool (name)(const char *str, void (*delay_fn)(type),            \
                unsigned factor, unsigned ns, unsigned max)         \
    {                                                               \
        printf("Testing: %s\n", str);                               \
        printf("   REQUESTED      ACTUAL      DIFF %%\n");          \
                                                                    \
        for(type t = 0; t < (type)max; t = t * factor + 1) {        \
            const uint64_t perf_start = measure();                  \
            delay_fn(t);                                            \
            const uint64_t perf_end = measure();                    \
                                                                    \
            const unsigned actual =                                 \
                ceil((double)(perf_end - perf_start) / (double)ns); \
                                                                    \
            printf("%12u%12u%12.3lf\n",                             \
                   (unsigned)t, actual, percent_diff(t, actual));   \
            fflush(stdout);                                         \
        }                                                           \
                                                                \
        printf("\n");                                               \
                                                                    \
        return true;                                                \
    }

GENERATE_TESTER(test_spin_delay, unsigned short, perf_cntr_timer_ns)
GENERATE_TESTER(test_sleep, uint64_t, timer_ns_gettime64)

#define test_spin_delay(func, ...) (test_spin_delay)(#func, func, __VA_ARGS__)
#define test_sleep(func, ...)      (test_sleep)(#func, func, __VA_ARGS__)

/* Program entry point. */
int main(int argc, char* argv[]) {
    bool success = true;
#if 0
    printf("Testing Spin Delay Latencies...\n\n");
    thd_set_hz(0);
    success &= test_spin_delay(timer_spin_delay_ns, 10, 1, 1000000000);
    success &= test_spin_delay(timer_spin_delay_us, 10, 1000, 1000000);
    success &= test_spin_delay(timer_spin_delay_ms, 2, 1000000, 1000);
#endif
    printf("Testing Sleep Delay Latencies...\n\n");
    //thd_set_hz(10);
    //thd_sleep_ms(3);
    success &= test_sleep(thd_sleep_ns, 2, 1, 2000000000);
    success &= test_sleep(thd_sleep_us, 2, 1000, 5000000);
    //success &= test_sleep(thd_sleep_ms, 2, 1000000, 5000);

    if(success) {
        printf("***** Delay Test: SUCCESS *****\n");
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "***** Delay Test: FAILURE *****\n");
        return EXIT_FAILURE;
    }
}
