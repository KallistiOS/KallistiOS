/* KallistiOS ##version##

   arch/dreamcast/kernel/perf_monitor.c
   Copyright (C) 2024 Paul Cercueil
*/

#include <arch/timer.h>
#include <dc/perf_monitor.h>

extern struct perf_monitor _monitors_start, _monitors_end;

void __stop_perf_monitor(struct perf_monitor **monitor) {
    struct perf_monitor *data = *monitor;

    data->event0 += perf_cntr_count(PRFC0) - data->event0_start;
    data->event1 += perf_cntr_count(PRFC1) - data->event1_start;
    data->time_ns += timer_ns_gettime64() - data->time_start;
}

struct perf_monitor *__start_perf_monitor(struct perf_monitor *data) {
    data->calls++;
    data->time_start = timer_ns_gettime64();
    data->event0_start = perf_cntr_count(PRFC0);
    data->event1_start = perf_cntr_count(PRFC1);

    return data;
}

void perf_monitor_init(perf_cntr_event_t event1, perf_cntr_event_t event2) {
    perf_cntr_timer_disable();

    perf_cntr_clear(PRFC0);
    perf_cntr_clear(PRFC1);

    perf_cntr_start(PRFC0, event1, PMCR_COUNT_CPU_CYCLES);
    perf_cntr_start(PRFC1, event2, PMCR_COUNT_CPU_CYCLES);
}

void perf_monitor_exit(void) {
    perf_cntr_stop(PRFC0);
    perf_cntr_stop(PRFC1);

    perf_cntr_clear(PRFC0);
    perf_cntr_clear(PRFC1);

    perf_cntr_timer_enable();
}

void perf_monitor_print(FILE *f) {
    struct perf_monitor *monitor;

    if((uintptr_t)&_monitors_end != (uintptr_t)&_monitors_start)
        fprintf(f, "Performance monitors:\n");

    for(monitor = &_monitors_end - 1; monitor >= &_monitors_start; monitor--) {
        fprintf(f, "\t%s L%u: %llu calls\n\t\t%llu ns (%f ns/call)\n\t\tevent 0: %llu (%f event/call)\n\t\tevent 1: %llu (%f event/call)\n",
                monitor->fn, monitor->line, monitor->calls, monitor->time_ns,
                monitor->calls ? (float)monitor->time_ns / (float)monitor->calls : 0.0f,
                monitor->event0,
                monitor->event0 ? (float)monitor->event0 / (float)monitor->calls : 0.0f,
                monitor->event1,
                monitor->event1 ? (float)monitor->event1 / (float)monitor->calls : 0.0f);
    }
}
