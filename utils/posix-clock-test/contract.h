/* Shared host/target checks. Never performs a valid RTC write. */
#ifndef POSIX_CLOCK_TEST_CONTRACT_H
#define POSIX_CLOCK_TEST_CONTRACT_H
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <kos/thread.h>

/* Some host libcs do not declare this optional interface. */
int clock_getcpuclockid(pid_t pid, clockid_t *id);

static unsigned checks;
#define CHECK(condition) do { \
    ++checks; \
    if(!(condition)) { \
        printf("POSIX-CLOCK: FAIL line=%d: %s\n", __LINE__, #condition); \
        abort(); \
    } \
} while(0)

static void check_pid(void) {
    const pid_t valid[] = {0, KOS_PID};
    const pid_t invalid[] = {-1, KOS_PID + 1, INT_MAX};
    clockid_t id;
    for(unsigned i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i) {
        id = (clockid_t)-123;
        errno = EDOM;
        CHECK(clock_getcpuclockid(valid[i], &id) == 0);
        CHECK(id == CLOCK_PROCESS_CPUTIME_ID);
        CHECK(errno == EDOM);
    }
    for(unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        id = (clockid_t)-123;
        errno = EDOM;
        CHECK(clock_getcpuclockid(invalid[i], &id) == ESRCH);
        CHECK(id == (clockid_t)-123);
        CHECK(errno == EDOM);
    }
}

static void check_res(void) {
    const clockid_t ids[] = {CLOCK_REALTIME, CLOCK_MONOTONIC,
        CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID};
    struct timespec ts;
    for(unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        errno = EDOM;
        CHECK(clock_getres(ids[i], NULL) == 0);
        CHECK(errno == EDOM);
        ts = (struct timespec){123, 456};
        CHECK(clock_getres(ids[i], &ts) == 0);
        CHECK(ts.tv_sec == 0);
        CHECK(ts.tv_nsec == (i < 2 ? 1 : 1000000));
    }
    ts = (struct timespec){123, 456};
    errno = 0;
    CHECK(clock_getres((clockid_t)-1, &ts) == -1);
    CHECK(errno == EINVAL);
    CHECK(ts.tv_sec == 123 && ts.tv_nsec == 456);
    errno = 0;
    CHECK(clock_getres((clockid_t)-1, NULL) == -1);
    CHECK(errno == EINVAL);
}

static void check_set(void) {
    const long bad_ns[] = {-1, LONG_MIN, 1000000000L, LONG_MAX};
    const clockid_t readonly[] = {CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID,
        CLOCK_THREAD_CPUTIME_ID, (clockid_t)-1};
    struct timespec ts = {1700000000, 0};
    for(unsigned i = 0; i < sizeof(bad_ns) / sizeof(bad_ns[0]); ++i) {
        ts.tv_nsec = bad_ns[i];
        errno = 0;
        CHECK(clock_settime(CLOCK_REALTIME, &ts) == -1);
        CHECK(errno == EINVAL);
    }
    errno = 0;
    CHECK(clock_settime(CLOCK_REALTIME, NULL) == -1);
    CHECK(errno == EFAULT);
    ts.tv_nsec = 0;
    for(unsigned i = 0; i < sizeof(readonly) / sizeof(readonly[0]); ++i) {
        errno = 0;
        CHECK(clock_settime(readonly[i], &ts) == -1);
        CHECK(errno == EINVAL);
    }
}
#endif
