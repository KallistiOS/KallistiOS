#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <kos.h>
#include <time.h>

static void quit(void) {
     printf("DISABLING WATCHDOG!\n");
     wdt_disable();
     exit(EXIT_SUCCESS);
}

static atomic_bool triggered = false;
static atomic_int counter = 0;

static void timer_callback(void* user_data) {
    (void)user_data;
    
    ++counter;
    triggered = true;
}

int main(int argc, char* argv[]) { 
    cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                      (cont_btn_callback_t)quit);

    wdt_enable_timer(0, 100, 5, timer_callback, NULL);

    while(1) {
        if(triggered) {
            struct timespec spec;    
            timespec_get(&spec, TIME_UTC);
            printf("WDT event fired at [%llu]: %d\n", 
                    spec.tv_sec, counter);
            
            triggered = false;
        }
    }

    return EXIT_SUCCESS;
}