/* KallistiOS ##version##
   POSIX clock argument/return-value regression probe.
   Copyright (C) 2026 Joseph Black
*/
#include <kos.h>
#include "../../../../../utils/posix-clock-test/contract.h"

KOS_INIT_FLAGS(INIT_DEFAULT & ~INIT_CDROM);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    check_pid();
    check_res();
    check_set();
    const clockid_t ids[] = {CLOCK_REALTIME, CLOCK_MONOTONIC,
        CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID};
    for(unsigned i = 0; i < 4; ++i) {
        struct timespec ts;
        CHECK(clock_gettime(ids[i], &ts) == 0);
        CHECK(ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000L);
    }
    printf("POSIX-CLOCK: PASS checks=%u rtc-write=0\n", checks);
    return 0;
}
