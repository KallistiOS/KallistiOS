# Workqueue safety regression

This regression exercises the checked KOS workqueue interfaces. It cancels a
running callback while that callback cancels a different queued job and then
attempts to reschedule itself. This verifies that distinct per-job cancellation
barriers do not deadlock, cancellation suppresses the racing requeue, and both
final job states are idle.

It also covers duplicate-pending rejection, cancellation of an absent job,
self-cancellation deadlock detection, successful periodic self-requeue,
deadline ordering, a deadline beyond the condition-variable timeout range, and
enqueue rejection after queue shutdown.

Shutdown cases check callback self-destruction rejection, callback-initiated
stop followed by an external join, two concurrent killers draining a parked
callback, clearing the joined thread handle, and discarding pending jobs.
This runs real KOS threads and locks; it does not validate hardware I/O.

Success prints
`KOSWORKQUEUE cancel=1 cross=1 duplicate=1 self=1 periodic=1 order=21 stop=1 joins=2`.
