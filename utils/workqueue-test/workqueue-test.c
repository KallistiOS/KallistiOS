/* Host regression of production workqueue.c; pthread shims, not SH-4 proof. */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <kos/workqueue.h>
#include <kos/timer.h>

_Thread_local bool test_irq;
static _Thread_local kthread_t *current;
static _Thread_local kthread_t foreign_thread;
atomic_int test_long_waits;
static atomic_int fail_create, fail_join, joins;
static atomic_int entered, release_job, runs, requeue, cross_cancel;
static atomic_int self_destroy, self_cancel;
static workqueue_t *queue;
static workqueue_job_t active, other;

static void *launch(void *arg) {
    current = arg;
    return current->routine(current->data);
}
kthread_t *thd_create_ex(const kthread_attr_t *attr,
                       void *(*routine)(void *), void *data) {
    (void)attr;
    if(atomic_exchange(&fail_create, 0)) { errno = ENOMEM; return NULL; }
    kthread_t *t = calloc(1, sizeof(*t));
    assert(t);
    t->routine = routine; t->data = data;
    assert(!pthread_create(&t->id, NULL, launch, t));
    return t;
}
kthread_t *thd_get_current(void) { return current ? current : &foreign_thread; }
int thd_join(kthread_t *t, void **result) {
    if(atomic_exchange(&fail_join, 0)) return -1;
    assert(!pthread_equal(t->id, pthread_self()));
    assert(!pthread_join(t->id, result));
    atomic_fetch_add(&joins, 1);
    free(t);
    return 0;
}
static void pause_briefly(void) {
    const struct timespec delay = {0, 1000000};
    nanosleep(&delay, NULL);
}
static void wait_flag(atomic_int *flag) {
    uint64_t deadline = timer_ms_gettime64() + 3000;
    while(!atomic_load(flag)) {
        assert(timer_ms_gettime64() < deadline);
        pause_briefly();
    }
}
static void idle(workqueue_job_t *job) {
    uint64_t deadline = timer_ms_gettime64() + 3000;
    workqueue_job_info_t info;
    do {
        assert(!workqueue_job_get_info(queue, job, &info));
        if(!info.running && !info.queued && !info.cancelling) return;
        assert(timer_ms_gettime64() < deadline);
        pause_briefly();
    } while(true);
}
static void unexpected(workqueue_t *wq, workqueue_job_t *job) {
    (void)wq; (void)job; assert(!"unexpected callback");
}
static void active_callback(workqueue_t *wq, workqueue_job_t *job) {
    atomic_fetch_add(&runs, 1);
    atomic_store(&entered, 1);
    wait_flag(&release_job);
    atomic_store(&cross_cancel, workqueue_cancel_ex(wq, &other));
    job->time_ms = timer_ms_gettime64() + 1000;
    assert(workqueue_enqueue_ex(wq, job) == -1);
    atomic_store(&requeue, errno);
}
static void *cancel(void *job) {
    assert(!workqueue_cancel_ex(queue, job));
    return NULL;
}
static void self_stop(workqueue_t *wq, workqueue_job_t *job) {
    assert(workqueue_cancel_ex(wq, job) == -1);
    atomic_store(&self_cancel, errno);
    errno = 0;
    workqueue_destroy(wq);
    atomic_store(&self_destroy, errno);
    workqueue_kill(wq);
    assert(workqueue_enqueue_ex(wq, job) == -1 && errno == ECANCELED);
    atomic_store(&entered, 1);
}
static void *kill_queue(void *arg) { workqueue_kill(arg); return NULL; }
static void blocking(workqueue_t *wq, workqueue_job_t *job) {
    (void)wq; (void)job;
    atomic_store(&entered, 1);
    wait_flag(&release_job);
}
int main(void) {
    alarm(20);
    atomic_store(&fail_create, 1);
    assert(!workqueue_create() && errno == ENOMEM);
    queue = workqueue_create(); assert(queue);
    workqueue_job_info_t info;
    workqueue_job_t invalid = {0};
    assert(workqueue_enqueue_ex(queue, &invalid) == -1 && errno == EINVAL);
    other = (workqueue_job_t){ .cb = unexpected, .time_ms = UINT64_MAX - 1 };
    assert(!workqueue_enqueue_ex(queue, &other));
    wait_flag(&test_long_waits);
    assert(workqueue_enqueue_ex(queue, &other) == -1 && errno == EBUSY);
    test_irq = true;
    assert(!workqueue_create() && errno == EPERM);
    assert(workqueue_enqueue_ex(queue, &other) == -1 && errno == EPERM);
    assert(workqueue_cancel_ex(queue, &other) == -1 && errno == EPERM);
    assert(workqueue_job_get_info(queue, &other, &info) == -1 && errno == EPERM);
    errno = 0; workqueue_kill(queue); assert(errno == EPERM);
    errno = 0; workqueue_destroy(queue); assert(errno == EPERM);
    assert(!workqueue_get_thread(queue) && errno == EPERM);
    test_irq = false;
    active = (workqueue_job_t){ .cb = active_callback };
    assert(!workqueue_enqueue_ex(queue, &active)); wait_flag(&entered);
    pthread_t canceller;
    assert(!pthread_create(&canceller, NULL, cancel, &active));
    uint64_t deadline = timer_ms_gettime64() + 3000;
    do {
        assert(!workqueue_job_get_info(queue, &active, &info));
        assert(timer_ms_gettime64() < deadline);
        if(!info.cancelling) pause_briefly();
    } while(!info.cancelling);
    atomic_store(&release_job, 1);
    assert(!pthread_join(canceller, NULL));
    idle(&active); idle(&other);
    assert(atomic_load(&runs) == 1 && atomic_load(&requeue) == ECANCELED);
    assert(atomic_load(&cross_cancel) == 0);
    assert(workqueue_cancel_ex(queue, &active) == -1 && errno == ENOENT);
    workqueue_destroy(queue);

    queue = workqueue_create(); assert(queue);
    atomic_store(&entered, 0);
    active = (workqueue_job_t){ .cb = self_stop };
    assert(!workqueue_enqueue_ex(queue, &active)); wait_flag(&entered);
    workqueue_kill(queue);
    assert(atomic_load(&self_cancel) == EDEADLK);
    assert(atomic_load(&self_destroy) == EDEADLK);
    assert(workqueue_get_thread(queue) == NULL);
    workqueue_kill(queue); workqueue_destroy(queue);

    queue = workqueue_create(); assert(queue);
    atomic_store(&entered, 0); atomic_store(&release_job, 0);
    active = (workqueue_job_t){ .cb = blocking };
    assert(!workqueue_enqueue_ex(queue, &active)); wait_flag(&entered);
    pthread_t killers[2]; int previous_joins = atomic_load(&joins);
    for(unsigned i = 0; i < 2; ++i)
        assert(!pthread_create(&killers[i], NULL, kill_queue, queue));
    /* Admission closes before the callback is released. */
    other.time_ms = UINT64_MAX - 1;
    deadline = timer_ms_gettime64() + 3000;
    while(workqueue_enqueue_ex(queue, &other) != -1 || errno != ECANCELED) {
        assert(timer_ms_gettime64() < deadline); pause_briefly();
    }
    atomic_store(&release_job, 1);
    for(unsigned i = 0; i < 2; ++i) assert(!pthread_join(killers[i], NULL));
    assert(atomic_load(&joins) == previous_joins + 1);
    assert(!workqueue_get_thread(queue));
    workqueue_destroy(queue);

    queue = workqueue_create(); assert(queue);
    atomic_store(&fail_join, 1); errno = 0;
    workqueue_destroy(queue);
    assert(errno == EIO && workqueue_get_thread(queue) != NULL);
    workqueue_destroy(queue); /* retry must join and free, not use freed storage */
    puts("workqueue production-source tests passed");
    return 0;
}
