/* Deterministic backend spies; no host or console clock is set. */
#include <string.h>
#include <kos/rtc.h>
#include <kos/timer.h>
#include "contract.h"

static kthread_t current;
static unsigned rtc_writes;
static time_t last_write;
static int write_failure;
kthread_t *thd_get_current(void) { return &current; }
uint64_t thd_get_cpu_time(kthread_t *thread) {
    CHECK(thread == &current);
    return 2345;
}
uint64_t thd_get_total_cpu_time(void) { return 1234567; }
time_t rtc_boot_time(void) { return 1700000000; }
void timer_ns_gettime(uint32_t *seconds, uint32_t *nanoseconds) {
    *seconds = 42;
    *nanoseconds = 987654321;
}
int rtc_set_unix_secs(time_t seconds) {
    ++rtc_writes;
    last_write = seconds;
    if(write_failure) {
        errno = EPERM;
        return -1;
    }
    return 0;
}

static void check_backends(void) {
    const clockid_t ids[] = {CLOCK_REALTIME, CLOCK_MONOTONIC,
        CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID};
    const time_t sec[] = {1700000042, 42, 1234, 2};
    const long ns[] = {987654321, 987654321, 567000000, 345000000};
    struct timespec ts;
    for(unsigned i = 0; i < 4; ++i) {
        CHECK(clock_gettime(ids[i], &ts) == 0);
        CHECK(ts.tv_sec == sec[i] && ts.tv_nsec == ns[i]);
        errno = 0;
        CHECK(clock_gettime(ids[i], NULL) == -1);
        CHECK(errno == EFAULT);
    }
    ts = (struct timespec){123, 456};
    CHECK(clock_gettime((clockid_t)-1, &ts) == -1);
    CHECK(errno == EINVAL);
    CHECK(ts.tv_sec == 123 && ts.tv_nsec == 456);
    CHECK(rtc_writes == 0);
    const long valid_ns[] = {0, 1, 999999999};
    for(unsigned i = 0; i < 3; ++i) {
        ts = (struct timespec){1700000000 + i, valid_ns[i]};
        CHECK(clock_settime(CLOCK_REALTIME, &ts) == 0);
        CHECK(rtc_writes == i + 1 && last_write == ts.tv_sec);
    }
    write_failure = 1;
    CHECK(clock_settime(CLOCK_REALTIME, &ts) == -1);
    CHECK(errno == EPERM && rtc_writes == 4);
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "all";
    if(!strcmp(mode, "all") || !strcmp(mode, "pid")) check_pid();
    if(!strcmp(mode, "all") || !strcmp(mode, "res")) check_res();
    if(!strcmp(mode, "all") || !strcmp(mode, "set")) {
        check_set();
        CHECK(rtc_writes == 0);
    }
    if(!strcmp(mode, "all")) check_backends();
    printf("POSIX-CLOCK-HOST: PASS checks=%u case=%s\n", checks, mode);
    return 0;
}
