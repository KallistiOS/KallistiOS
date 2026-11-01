/* KallistiOS ##version##

   resource.c
   Copyright (C) 2025 Donald Haase
*/

#include <errno.h>
#include <stdint.h>
#include <kos/thread.h>
#include <kos/timer.h>
#include <sys/resource.h>

int getpriority(int which, id_t who) {
    kthread_t *thd;

    if(which > PRIO_USER || which < PRIO_PROCESS) {
        errno = EINVAL;
        return -1;
    }

    if(!who)
        thd = thd_get_current();
    else
        thd = thd_by_tid((tid_t)who);
    if(!thd) {
        errno = ESRCH;
        return -1;
    }

    /* Scale from KOS to POSIX ranges */
    int prio = (int)thd_get_prio(thd) * 2 - 20;
    if(prio > 19)
        prio = 19;

    return prio;
}

int setpriority(int which, id_t who, int value) {
    kthread_t *thd;

    if(which > PRIO_USER || which < PRIO_PROCESS || value < -20 || value > 19) {
        errno = EINVAL;
        return -1;
    }

    if(!who)
        thd = thd_get_current();
    else
        thd = thd_by_tid((tid_t)who);
    if(!thd) {
        errno = ESRCH;
        return -1;
    }

    /* Scale from POSIX to KOS ranges */
    int prio = (value + 20) / 2;
    if(prio == 0)
        prio = 1;

    return thd_set_prio(thd, prio);
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

/* The structure of this is based around thd_pslist */
int getrusage(int who, struct rusage *r_usage) {
    uint64_t ms_time, cpu_total = 0;

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
    ms_time = timer_ms_gettime64();

    cpu_total = thd_get_total_cpu_time();
    ms_time -= cpu_total;

    r_usage->ru_utime.tv_sec  = cpu_total / 1000;
    r_usage->ru_utime.tv_usec = (cpu_total % 1000) * 1000;

    r_usage->ru_stime.tv_sec  = ms_time / 1000;
    r_usage->ru_stime.tv_usec = (ms_time % 1000) * 1000;

    return 0;
}
