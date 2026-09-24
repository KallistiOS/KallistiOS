/* KallistiOS ##version##

   workqueue-safety.c
   Copyright (C) 2026 Joseph Black
*/

#include <kos.h>
#include <kos/workqueue.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_TIMEOUT_MS 2000u

static workqueue_t *queue;
static semaphore_t active_started;
static semaphore_t active_release;
static semaphore_t cancel_finished;
static semaphore_t self_finished;
static semaphore_t periodic_finished;
static semaphore_t ordered_finished;
static workqueue_job_t active_job;
static workqueue_job_t cross_cancel_job;
static workqueue_job_t self_job;
static workqueue_job_t periodic_job;
static volatile unsigned active_runs;
static volatile int active_requeue_result;
static volatile int active_requeue_errno;
static volatile int cross_cancel_result;
static volatile int cross_cancel_errno;
static volatile int cancel_result;
static volatile int cancel_errno;
static volatile int self_result;
static volatile int self_errno;
static volatile unsigned periodic_runs;
static volatile int periodic_requeue_result;
static volatile unsigned order_count;
static volatile unsigned order[2];
static volatile unsigned unexpected_runs;
static volatile int self_destroy_errno;
static semaphore_t kill_finished;

typedef struct ordered_job {
    workqueue_job_t job;
    unsigned value;
} ordered_job_t;

static void active_callback(workqueue_t *wq, workqueue_job_t *job) {
    ++active_runs;
    sem_signal(&active_started);
    sem_wait(&active_release);

    errno = 0;
    cross_cancel_result = workqueue_cancel_ex(wq, &cross_cancel_job);
    cross_cancel_errno = errno;

    job->time_ms = timer_ms_gettime64() + 1000;
    errno = 0;
    active_requeue_result = workqueue_enqueue_ex(wq, job);
    active_requeue_errno = errno;
}

static void *cancel_thread(void *data) {
    workqueue_job_t *job = data;

    errno = 0;
    cancel_result = workqueue_cancel_ex(queue, job);
    cancel_errno = errno;
    sem_signal(&cancel_finished);
    return NULL;
}

static void self_cancel_callback(workqueue_t *wq, workqueue_job_t *job) {
    errno = 0;
    self_result = workqueue_cancel_ex(wq, job);
    self_errno = errno;
    sem_signal(&self_finished);
}

static void periodic_callback(workqueue_t *wq, workqueue_job_t *job) {
    if(++periodic_runs == 1) {
        job->time_ms = timer_ms_gettime64() + 10;
        periodic_requeue_result = workqueue_enqueue_ex(wq, job);
    }
    else {
        sem_signal(&periodic_finished);
    }
}

static void ordered_callback(workqueue_t *wq, workqueue_job_t *job) {
    ordered_job_t *ordered = (ordered_job_t *)job;
    unsigned slot = order_count++;

    (void)wq;
    if(slot < 2)
        order[slot] = ordered->value;
    sem_signal(&ordered_finished);
}

static void unexpected_callback(workqueue_t *wq, workqueue_job_t *job) {
    (void)wq;
    (void)job;
    ++unexpected_runs;
}

static void stop_callback(workqueue_t *wq, workqueue_job_t *job) {
    (void)job;
    errno = 0;
    workqueue_destroy(wq);
    self_destroy_errno = errno;
    workqueue_kill(wq);
    sem_signal(&self_finished);
}

static void parked_callback(workqueue_t *wq, workqueue_job_t *job) {
    (void)wq; (void)job;
    sem_signal(&active_started);
    sem_wait(&active_release);
}

static void *kill_thread(void *data) {
    workqueue_kill(data);
    sem_signal(&kill_finished);
    return NULL;
}

static bool wait_for_cancellation(workqueue_job_t *job) {
    uint64_t deadline = timer_ms_gettime64() + TEST_TIMEOUT_MS;
    workqueue_job_info_t info;

    while(timer_ms_gettime64() < deadline) {
        if(workqueue_job_get_info(queue, job, &info) == 0 && info.cancelling)
            return true;
        thd_pass();
    }

    return false;
}

static bool wait_until_idle(workqueue_job_t *job) {
    uint64_t deadline = timer_ms_gettime64() + TEST_TIMEOUT_MS;
    workqueue_job_info_t info;

    while(timer_ms_gettime64() < deadline) {
        if(workqueue_job_get_info(queue, job, &info) == 0 &&
           !info.queued && !info.running && !info.cancelling)
            return true;
        thd_pass();
    }

    return false;
}

int main(int argc, char **argv) {
    workqueue_job_info_t info;
    workqueue_job_t duplicate_job = { 0 };
    workqueue_job_t stopped_job = { 0 };
    ordered_job_t first = { .value = 1 };
    ordered_job_t second = { .value = 2 };
    kthread_t *canceller = NULL;
    uint64_t now;
    bool failed = false;
    int duplicate_errno = 0;
    int missing_errno = 0;
    int stopped_errno = 0;

    (void)argc;
    (void)argv;

    sem_init(&active_started, 0);
    sem_init(&active_release, 0);
    sem_init(&cancel_finished, 0);
    sem_init(&self_finished, 0);
    sem_init(&periodic_finished, 0);
    sem_init(&ordered_finished, 0);

    queue = workqueue_create();
    if(!queue) {
        printf("Unable to create work queue\n");
        return EXIT_FAILURE;
    }

    cross_cancel_job.cb = unexpected_callback;
    cross_cancel_job.time_ms = UINT64_MAX - 1;
    active_job.cb = active_callback;
    if(workqueue_enqueue_ex(queue, &cross_cancel_job) < 0 ||
       workqueue_enqueue_ex(queue, &active_job) < 0 ||
       sem_wait_timed(&active_started, TEST_TIMEOUT_MS) < 0)
        failed = true;

    if(!failed) {
        canceller = thd_create(0, cancel_thread, &active_job);
        if(!canceller || !wait_for_cancellation(&active_job))
            failed = true;
    }

    sem_signal(&active_release);
    if(canceller) {
        if(sem_wait_timed(&cancel_finished, TEST_TIMEOUT_MS) < 0 ||
           thd_join(canceller, NULL) < 0)
            failed = true;
    }

    if(cancel_result != 0 || cancel_errno != 0 || active_runs != 1 ||
       cross_cancel_result != 0 || cross_cancel_errno != 0 ||
       active_requeue_result != -1 || active_requeue_errno != ECANCELED ||
       !wait_until_idle(&active_job) || !wait_until_idle(&cross_cancel_job))
        failed = true;

    duplicate_job.cb = unexpected_callback;
    duplicate_job.time_ms = UINT64_MAX - 1;
    if(workqueue_enqueue_ex(queue, &duplicate_job) < 0)
        failed = true;
    errno = 0;
    if(workqueue_enqueue_ex(queue, &duplicate_job) == 0)
        failed = true;
    duplicate_errno = errno;
    if(duplicate_errno != EBUSY ||
       workqueue_job_get_info(queue, &duplicate_job, &info) < 0 ||
       !info.queued || info.running || info.cancelling ||
       workqueue_cancel_ex(queue, &duplicate_job) < 0)
        failed = true;
    errno = 0;
    if(workqueue_cancel_ex(queue, &duplicate_job) == 0)
        failed = true;
    missing_errno = errno;
    if(missing_errno != ENOENT)
        failed = true;

    self_job.cb = self_cancel_callback;
    if(workqueue_enqueue_ex(queue, &self_job) < 0 ||
       sem_wait_timed(&self_finished, TEST_TIMEOUT_MS) < 0 ||
       self_result != -1 || self_errno != EDEADLK ||
       !wait_until_idle(&self_job))
        failed = true;

    periodic_job.cb = periodic_callback;
    if(workqueue_enqueue_ex(queue, &periodic_job) < 0 ||
       sem_wait_timed(&periodic_finished, TEST_TIMEOUT_MS) < 0 ||
       periodic_runs != 2 || periodic_requeue_result != 0 ||
       !wait_until_idle(&periodic_job))
        failed = true;

    first.job.cb = ordered_callback;
    second.job.cb = ordered_callback;
    now = timer_ms_gettime64();
    first.job.time_ms = now + 40;
    second.job.time_ms = now + 20;
    if(workqueue_enqueue_ex(queue, &first.job) < 0 ||
       workqueue_enqueue_ex(queue, &second.job) < 0 ||
       sem_wait_timed(&ordered_finished, TEST_TIMEOUT_MS) < 0 ||
       sem_wait_timed(&ordered_finished, TEST_TIMEOUT_MS) < 0 ||
       order_count != 2 || order[0] != 2 || order[1] != 1)
        failed = true;

    workqueue_kill(queue);
    stopped_job.cb = unexpected_callback;
    errno = 0;
    if(workqueue_enqueue_ex(queue, &stopped_job) == 0)
        failed = true;
    stopped_errno = errno;
    if(stopped_errno != ECANCELED || unexpected_runs != 0)
        failed = true;

    workqueue_destroy(queue);

    /* A callback may stop, but must not destroy or join its own worker. */
    queue = workqueue_create();
    if(!queue) return EXIT_FAILURE;
    self_job.time_ms = 0;
    self_job.cb = stop_callback;
    if(workqueue_enqueue_ex(queue, &self_job) < 0 ||
       sem_wait_timed(&self_finished, TEST_TIMEOUT_MS) < 0)
        failed = true;
    workqueue_kill(queue);
    if(self_destroy_errno != EDEADLK || workqueue_get_thread(queue) != NULL)
        failed = true;
    workqueue_destroy(queue);

    /* Concurrent stops must both wait for the parked callback and join once. */
    queue = workqueue_create();
    if(!queue) return EXIT_FAILURE;
    sem_init(&kill_finished, 0);
    active_job.time_ms = 0;
    active_job.cb = parked_callback;
    if(workqueue_enqueue_ex(queue, &active_job) < 0 ||
       sem_wait_timed(&active_started, TEST_TIMEOUT_MS) < 0)
        failed = true;
    kthread_t *killers[2] = {
        thd_create(0, kill_thread, queue), thd_create(0, kill_thread, queue),
    };
    uint64_t deadline = timer_ms_gettime64() + TEST_TIMEOUT_MS;
    stopped_job.time_ms = UINT64_MAX - 1;
    bool stopped = false;
    do {
        if(workqueue_enqueue_ex(queue, &stopped_job) < 0 && errno == ECANCELED) {
            stopped = true;
            break;
        }
        thd_pass();
    } while(timer_ms_gettime64() < deadline);
    sem_signal(&active_release);
    for(unsigned i = 0; i < 2; ++i) {
        if(!killers[i] || sem_wait_timed(&kill_finished, TEST_TIMEOUT_MS) < 0 ||
           thd_join(killers[i], NULL) < 0)
            failed = true;
    }
    if(!stopped || workqueue_get_thread(queue) != NULL ||
       workqueue_job_get_info(queue, &stopped_job, &info) < 0 ||
       info.queued || info.running || info.cancelling)
        failed = true;
    workqueue_destroy(queue);
    sem_destroy(&kill_finished);
    sem_destroy(&ordered_finished);
    sem_destroy(&periodic_finished);
    sem_destroy(&self_finished);
    sem_destroy(&cancel_finished);
    sem_destroy(&active_release);
    sem_destroy(&active_started);

    if(failed) {
        printf("Workqueue safety failed: cancel=%d/%d cross=%d/%d "
               "requeue=%d/%d duplicate=%d missing=%d self=%d/%d "
               "periodic=%u/%d order=%u,%u\n",
               cancel_result, cancel_errno,
               cross_cancel_result, cross_cancel_errno,
               active_requeue_result, active_requeue_errno,
               duplicate_errno, missing_errno, self_result, self_errno,
               periodic_runs, periodic_requeue_result,
               order[0], order[1]);
        return EXIT_FAILURE;
    }

    printf("KOSWORKQUEUE cancel=1 cross=1 duplicate=1 self=1 periodic=1 "
           "order=%u%u stop=1 joins=2\n",
           order[0], order[1]);
    return EXIT_SUCCESS;
}
