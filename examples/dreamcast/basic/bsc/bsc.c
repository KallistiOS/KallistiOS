/* KallistiOS ##version##

   bsc.c
   Copyright (C) 2023 Falco Girgis

*/

/* 
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <time.h>
#include <kos.h>

#include <arch/bsc.h>
#include <dc/maple/controller.h>


typedef struct bsc_isr_state {
    atomic_size_t match_counter;
    atomic_size_t overflow_counter;
} bsc_isr_state_t;

void on_comp_match(void* userdata) { 
    bsc_isr_state_t *state = userdata;
    ++state->match_counter;
}

void on_overflow(void* userdata) {
    bsc_isr_state_t *state = userdata;
    ++state->overflow_counter;
    state->match_counter = 0;
}

/* Main entry point */
int main(int argc, char *argv[]) { 
    maple_device_t *cont1;
    cont_state_t *state;
    bsc_isr_state_t bsc_state = { 0 };

    bsc_set_isrs(0x1, 
                 on_comp_match, &bsc_state,
                 on_overflow, &bsc_state);

    size_t prev_match = 0, prev_overflow = 0;

    while(1) {
        if((cont1 = maple_enum_type(0, MAPLE_FUNC_CONTROLLER))) {
            if((state = (cont_state_t *)maple_dev_status(cont1))) {
                if(state->start)
                    goto exit;
            }
        }

        if(prev_match != bsc_state.match_counter || prev_overflow != bsc_state.overflow_counter) { 
            printf("Counter incremented: %zu, %zu\n", 
                    bsc_state.overflow_counter, bsc_state.match_counter);
            prev_match = bsc_state.match_counter;
            prev_overflow = bsc_state.overflow_counter;
        }
    }
exit:
    bsc_shutdown();

    return EXIT_SUCCESS;
}
