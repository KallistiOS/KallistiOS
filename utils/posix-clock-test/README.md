# POSIX clock argument regressions

This focused topic changes three API behaviors:

- `clock_getcpuclockid()` accepts PID zero and `KOS_PID`. Other PIDs return
  `ESRCH` directly, without modifying the output or `errno`.
- `clock_getres()` accepts a null result pointer for supported clocks. Unknown
  clocks still fail with `EINVAL`, even when the result pointer is null.
- `clock_settime(CLOCK_REALTIME)` rejects nanoseconds outside `[0, 1000000000)`
  before calling the RTC backend.

The null-resolution and nanosecond contracts are specified by
[The Open Group clock interfaces](https://pubs.opengroup.org/onlinepubs/000095399/functions/clock_getres.html).
This does not change RTC storage, fractional-second setting, clock resolution,
boot-offset arithmetic, scheduler accounting, or software timer policy. It is
not a claim of complete POSIX time conformance. In particular, valid realtime
sets still delegate whole seconds to the existing RTC backend.

## Host tests

Run `make -C utils/posix-clock-test test`. Override `CC` and `CFLAGS` for
GNU17, strict C23 (Clang: C2x), or ASan/UBSan lanes. Python 3 is required.
The runner compiles the real `kernel/libc/posix/clock_gettime.c` unchanged,
renames its four exported functions so host libc cannot satisfy the calls,
and replaces only thread/timer/RTC dependencies with deterministic spies.
It checks argument edges, direct error returns, unchanged output on failure,
all four read routes, valid set delegation and backend-error propagation.
Valid sets affect a spy only, never the host or console RTC.

`python3 utils/posix-clock-test/run.py --source /path/to/old/clock_gettime.c
--case pid` can test an old implementation. The `res` and `set` cases isolate
the other two regressions. The shim PID is checked against the real KOS header
during validation; these are software tests, not a timer or RTC hardware model.

## Target probe

Build `examples/dreamcast/basic/threading/posix-clock` with the normal KOS
environment and run its ELF. It links the actual KOS functions and shares the
argument checks with the host test, then reads all four clocks. It never makes
a valid `clock_settime()` call, so it does not intentionally change the RTC.
Do not run this probe against an unpatched kernel: the old invalid-nanosecond
bug could allow an RTC write if earlier checks were bypassed.

The marker is `POSIX-CLOCK: PASS checks=66 rtc-write=0`. Successful execution
establishes the API checks, not hardware timing precision, RTC persistence,
preemption accounting accuracy or long-duration clock stability.
