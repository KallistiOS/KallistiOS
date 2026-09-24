# KOS workqueue safety and service boundaries

This focused proposal is based on upstream `804b3195`. It changes the existing
workqueue implementation and its exports, not fibers, the Service Executor,
SH4ZAM, networking policy, or a global lazy-worker service. Each workqueue still
creates its dedicated worker eagerly. The separate `thd_worker_*` API is unchanged.

KOS workqueues provide one-shot or deadline-scheduled callbacks on a dedicated
thread. They are intended for finite thread-context work and do not replace
ordinary KOS threads whose persistent state or blocking behavior is part of
their contract.

## Checked job operations

`workqueue_enqueue_ex()` rejects malformed jobs, a duplicate pending instance,
a stopped queue, and requeue suppressed by an active cancellation. A callback
may still enqueue its own job once to implement periodic work. Jobs remain
caller-owned and must stay alive while queued, running, or cancelling.
One job may belong to only one queue at a time. Do not mutate pending job fields.
After requeueing itself, a callback must not change those fields again until its
next invocation. Independent producers must be quiesced before a caller relies
on cancellation to release storage; cancellation cannot prohibit future calls.

`workqueue_cancel_ex()` first removes a pending instance. If the callback is
already running, cancellation establishes a barrier, waits for the callback to
return, and removes any requeue that raced establishment of the barrier. An
attempt by the running callback to requeue after the barrier returns
`ECANCELED`. Self-cancellation reports `EDEADLK` instead of blocking the worker
on itself. Barriers are tracked per job, so that callback may still cancel a
different pending job while its own external cancellation is draining.

`workqueue_job_get_info()` copies queued, running, and cancelling state while
holding the queue lock. A periodic callback which has already requeued itself
can be both running and queued; these states describe distinct instances of the
same caller-owned job record.

The original void enqueue and cancel functions remain as source-compatible
wrappers around the checked operations. They deliberately discard the result.
New code should use the checked forms when rejection changes program behavior.

## Deadlines and shutdown

The worker keeps the pending list in absolute millisecond order. Deadlines
farther away than the signed condition-variable timeout range are waited in
bounded slices and re-evaluated, avoiding integer truncation or premature
execution.

Queue shutdown rejects new work, wakes the worker, joins it, and is serialized
when multiple threads request shutdown concurrently. A callback may request
that its queue stop, but cannot join or destroy its own worker thread.
Pending jobs are discarded without running; the active callback must return
before an external kill completes. Failed joins leave the stopped queue alive
for a retry, and successful joins clear the borrowed thread handle. Never join,
detach, or destroy that worker independently. Queue destruction must be serialized
against all other users, including concurrent kill/cancel calls. No forcibly
terminated callback or general stop-all-workers guarantee is introduced.

## Interrupt ingress

Workqueue enqueue, cancel, and state-query operations take a KOS mutex and are
not interrupt-safe. Interrupt handlers should acknowledge hardware, copy a
small bounded event into an IRQ-safe subsystem queue when one exists, and wake
thread-context processing. That ingress queue must provide explicit
backpressure when it is full.

## Regression coverage

`examples/dreamcast/basic/threading/workqueue-safety` verifies active-callback
cancellation, racing periodic requeue suppression, duplicate rejection,
self-cancel deadlock detection, ordinary periodic self-requeue, deadline
ordering, long-deadline handling, and post-shutdown admission rejection.
It also checks callback-initiated stop, self-destruction rejection, two external
killers waiting for an active callback, discarded pending jobs, and a cleared
thread handle after joining.

`utils/workqueue-test` compiles the production implementation with pthread
shims. It adds IRQ-context rejection, worker-creation failure, exact single-join
accounting, and injected join failure followed by retry without freeing live
queue storage. GCC 14 GNU17 and strict C23, plus Clang GNU17 ASan/UBSan, pass.

The full SH-4 GCC 16.2 build and the target probe build pass. The target probe
passes in Flycast interpreter and dynarec modes. Host shims do not prove SH-4
ABI behavior, and emulator tests do not establish hardware I/O shutdown safety.
