/* KallistiOS ##version##

   resource.c   
   Copyright (C) 2025 Donald Haase
*/

#include <errno.h>
#include <kos/thread.h>
#include <kos/timer.h>
#include <sys/resource.h>

static int priorities[PRIO_USER + 1] = { 0 };

int getpriority(int which, id_t who) {
    if(who || (which > PRIO_USER) || (which < PRIO_PROCESS)) {
        errno = EINVAL;
        return -1;
    }
    
    return priorities[which];
}

int setpriority(int which, id_t who, int value) {
    if(who || (which > PRIO_USER) || (which < PRIO_PROCESS)) {
        errno = EINVAL;
        return -1;
    }

    priorities[which] = value;
    return 0;
}

int getrlimit(int resource, struct rlimit *rlp) {
    if((resource > RLIMIT_AS) || (resource < RLIMIT_CORE)) {
        errno = EINVAL;
        return -1;
    }

    rlp->rlim_cur = RLIM_INFINITY;
    rlp->rlim_max = RLIM_INFINITY;
    return 0;
}
        
int setrlimit(int resource, const struct rlimit *rlp) {
    (void)rlp;

    if((resource > RLIMIT_AS) || (resource < RLIMIT_CORE)) {
        errno = EINVAL;
        return -1;
    }

    /* Do nothing */
    return 0;
}

/* Helper to sum up thread usage time */
static int get_user_time(kthread_t *thd, void *user_data) {
    *((uint64_t *)user_data) += thd->cpu_time.total;
    return 1;
}

/* The structure of this is based around thd_pslist */
int getrusage(int who, struct rusage *r_usage) {
    uint64_t ns_time, cpu_total = 0;

    if((who > RUSAGE_CHILDREN) || (who < RUSAGE_SELF)) {
        errno = EINVAL;
        return -1;
    }

    /* No child processes in KOS */
    if(who == RUSAGE_CHILDREN) {
        r_usage->ru_utime.tv_sec  = 0;
        r_usage->ru_utime.tv_usec = 0;

        r_usage->ru_stime.tv_sec  = 0;
        r_usage->ru_stime.tv_usec = 0;

        return 0;
    }

    irq_disable_scoped();
    thd_get_cpu_time(thd_get_current());
    ns_time = timer_ns_gettime64();

    /* Sum up the time in each thread */
    thd_each(get_user_time, &cpu_total);

    ns_time -= cpu_total;

    r_usage->ru_utime.tv_sec  = cpu_total / 1000000000;
    r_usage->ru_utime.tv_usec = (cpu_total / 1000) % 1000000;

    r_usage->ru_stime.tv_sec  = ns_time / 1000000000;
    r_usage->ru_stime.tv_usec = (ns_time / 1000) % 1000000;

    return 0;
}
