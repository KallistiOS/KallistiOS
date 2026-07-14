/* KallistiOS ##version##

   resource.c
   Copyright (C) 2026 Donald Haase
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/resource.h>

#include <kos.h>

struct priority_data {
    int rv;
    int which;
    id_t who;
    int value; /* Only when setting */
};

/* Samples for a plain test of getpriority() */
static struct priority_data gp_samples[] = {
    {0, PRIO_PROCESS, 0},
    {0, PRIO_PGRP, 0},
    {0, PRIO_USER, 0},
    {-1, 42, 0},
    {-1, PRIO_USER, 1}
};

/* Samples for testing setpriority */
static struct priority_data sp_samples[] = {
    {0, PRIO_PROCESS, 0, 77},
    {-1, 42, 0, 99},
    {-1, PRIO_USER, 1, 101}
};

int main(int argc, char **argv) {
    int retval = EXIT_SUCCESS;

    printf("First testing out getpriority()\n");

    for(size_t i = 0; i < __array_size(gp_samples); i++) {
        int rv = gp_samples[i].rv;
        int which = gp_samples[i].which;
        id_t who = gp_samples[i].who;

        int actual_rv = getpriority(which, who);

        if(rv != actual_rv) {
            fprintf(stderr, "FAILED: ");
            retval = EXIT_FAILURE;
        }
        else
            printf("PASSED: ");

        printf("getpriority(%i, %lu) expected %i got %i\n", which,
            who, rv, actual_rv);
    }

    printf("Next testing out setpriority()\n");

    for(size_t i = 0; i < __array_size(sp_samples); i++) {
        int rv = sp_samples[i].rv;
        int which = sp_samples[i].which;
        id_t who = sp_samples[i].who;
        int value = sp_samples[i].value;

        int actual_rv = setpriority(which, who, value);

        if(rv != actual_rv) {
            fprintf(stderr, "FAILED: ");
            retval = EXIT_FAILURE;
        }
        else
            printf("PASSED: ");

        printf("setpriority(%i, %lu, %i) expected %i got %i\n", which,
            who, value, rv, actual_rv);

        actual_rv = getpriority(which, who);

        /* If the failure happens when failure is expected, it's a pass */
        if((actual_rv != value) != (rv == -1)) {
            fprintf(stderr, "FAILED: ");
            retval = EXIT_FAILURE;
        }
        else
            printf("PASSED: ");

        printf("getpriority(%i, %lu) expected %i got %i\n", which,
            who, value, actual_rv);
    }

    printf("Last testing out getrusage()\n");

    struct rusage use;

    int getrv = getrusage(RUSAGE_CHILDREN, &use);

    /* Everything should be 0 for 'children' */
    if(getrv || use.ru_utime.tv_sec || use.ru_utime.tv_usec ||
            use.ru_stime.tv_sec || use.ru_stime.tv_usec) {
        fprintf(stderr, "FAILED: ");
        retval = EXIT_FAILURE;
    }
    else
        printf("PASSED: ");

    /* Reset our r_usage */
    memset(&use, 0, sizeof(use));

    getrv = getrusage(RUSAGE_SELF, &use);

    printf("getrusage(RUSAGE_SELF, &use) times: \n"
            "utime: %llus and %luu\n"
            "stime: %llus and %luu\n"
            "Total system time: %lluu\n",
            use.ru_utime.tv_sec, use.ru_utime.tv_usec,
            use.ru_stime.tv_sec, use.ru_stime.tv_usec,
            timer_ns_gettime64() / 1000);

    return retval;
}
