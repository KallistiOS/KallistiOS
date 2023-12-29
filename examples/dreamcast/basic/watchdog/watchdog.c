/* KallistiOS ##version##

   watchdog.c
   Copyright (C) 2023 Falco Girgis

*/

/* This program serves as both an example of using the Watchdog Timer
   peripheral on the Dreamcast's SH4 CPU as well as a test to validate
   the behavior of its driver in KOS.

   NOTE:
   At the time this is being written, there are no Dreamcast emulators
   out there which are bothering to emulate the SH4 WDT, since apparently
   no commercial games actually used it. Special care has been taken to
   fail the tests gracefully when there is no functioning WDT.
*/ 

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <time.h>

#include <arch/wdt.h>
#include <dc/maple/controller.h>

/* Microsecond-based units */
#define MSEC            1000
#define SEC             (1000 * MSEC)

/* Test configuration constants */
#define WDT_PET_COUNT   4000
#define WDT_INTERVAL    (500 * MSEC)
#define WDT_SECONDS     10 
#define WDT_COUNT_MAX   ((WDT_SECONDS * SEC) / WDT_INTERVAL)

/* User callback to be invoked from the WDT's interval timer 
   interrupt when the requested microsecond period has elapsed. 
   Our generic userdata pointer is passed back to us as void*. 
   We use it to store our atomic counter to count the number 
   of times this callback has been invoked. 

   NOTE: Since we're accessing both from within and outside
   of the interrupt, we make it atomic to ensure that it 
   doesn't get updated while it's being read or vice-versa. */
static void wdt_timeout(void *user_data) {
    ++(*(atomic_uint *)user_data);
}

/* Main entry point */
int main(int argc, char *argv[]) { 
    bool success = true;

    /* Press all buttons simultaneously to exit early. */
    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)exit);

    printf("\nEnabling WDT in watchdog mode!\n");

    /* Enable watchdog mode with a period of 5.25ms, causing a manual 
       reset interrupt signal to be raised upon timeout. This will 
       cause your Dreamcast to reboot immediately. */
    wdt_enable_watchdog(0, WDT_CLK_DIV_4096, WDT_RST_MANUAL);

    /* Continually "pet" the watchdog timer in a loop, resetting its
       counter value and preventing a timeout (and reboot). We also
       store its max value as we're iterating. */
    uint8_t max_count = 0;
    for(int p = 0; p < WDT_PET_COUNT; ++p) {
        const uint8_t current_count = wdt_get_counter();
        
        if(current_count > max_count)  
            max_count = current_count;

        if(current_count) 
            wdt_pet();
    }

    /* Immediately disable the WDT once we're done with it. */
    wdt_disable();

    /* Ensure that the WDT's counter wasn't stuck at zero the whole
       time and that it was actually updating as expected. */
    if(!max_count) {
        fprintf(stderr, "The WDT counter never even incremented!\n\n");
        success = false;
    }
    else {
        printf("Pet it %d times! Maximum counter value was %u.\n\n", 
               WDT_PET_COUNT, max_count);
    }

    printf("Enabling WDT timer with interval: %uus.\n", WDT_INTERVAL);
    printf("Expecting %u callbacks over %u seconds.\n",
           WDT_COUNT_MAX, WDT_SECONDS);

    /* Enable the WDT in interval counter mode, with a period of 500ms,
       an interrupt priority level of 15 (highest), and give it our 
       timeout callback plus pass it our counter as its userdata pointer. */
    atomic_uint counter = 0;
    wdt_enable_timer(0, WDT_INTERVAL, 15, wdt_timeout, &counter);

    /* Begin spinning in a loop until either condition is met:
       1) The counter becomes greater than or equal to the expected value
          (meaning our callback has been called the expected number of times
          from the Watchdog timer's interval interrupt).
       2) The elapsed time becomes greater than twice the expected number of
          seconds, meaning the watchdog interval timer is not behaving as we
          expected it to behave (probably running on emulator).
     */ 
    const time_t start_time = time(NULL);
    time_t current_time = start_time;
    time_t elapsed_time = 0;
    while(1) {
        /* Measure our current time (using the C standard library, 
           which internally uses the TMU2 peripheral). */
        current_time = time(NULL);
        elapsed_time = current_time - start_time;

        /* Check whether the WDT timer has incremented our counter the 
           expected number of times and exit if so. */
        if(counter >= WDT_COUNT_MAX) {
            break;
        }
        /* Check whether we should just give up on the WDT. */
        else if(elapsed_time > WDT_SECONDS * 2) {
            fprintf(stderr, "Test is taking too long... timing out!\n");
            success = false;
            break;
        }
    }

    printf("%u callbacks in %llu seconds!\n", counter, elapsed_time);

    /* Ensure that the amount of time elapsed based on WDT callbacks agrees
       with the amount of time elapsed as reported by TMU2. */
    const unsigned diff_seconds = abs((int)elapsed_time - (int)WDT_SECONDS);
    if(diff_seconds > 1) {
        printf("Watchdog timing did not match system timing!\n"
               "\t[%u sec delta]\n", diff_seconds);
        success = false;
    }

    /* Report results and return status code. */
    if(success) {
        printf("\n\n***** WATCHDOG TIMER TEST SUCCEEDED! *****\n\n");
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "\n\nXXXXX WATCHDOG TIMER TEST FAILED! XXXXX\n\n");
        return EXIT_FAILURE;
    }
}
