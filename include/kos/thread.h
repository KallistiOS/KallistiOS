/* KallistiOS ##version##

   include/kos/thread.h
   Copyright (C) 2000, 2001, 2002, 2003 Megan Potter
   Copyright (C) 2009, 2010, 2016 Lawrence Sebald

*/

#ifndef __KOS_THREAD_H
#define __KOS_THREAD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/tls.h>
#include <arch/types.h>
#include <arch/irq.h>
#include <arch/arch.h>
#include <sys/queue.h>
#include <sys/reent.h>

/** \file   kos/thread.h
    \brief  Threading support.

    This file contains the interface to the threading system of KOS. Timer
    interrupts are used to reschedule threads within the system while in
    preemptive mode. There is also some support for a cooperative threading
    mode (where each thread must manually give up its timeslice to swap out
    threads).

    The thread scheduler itself is a relatively simplistic priority scheduler.
    There is no provision for priorities to erode over time, so keep that in
    mind. That practically means that if you have 2 high priority threads that
    are always runnable and one low priority thread that is always runnable, the
    low priority thread will never actually run (since it will never get to the
    front of the run queue because of the high priority threads).

    The scheduler supports two distinct types of threads: joinable and detached
    threads. A joinable thread is one that can return a value to the creating
    thread (or for that matter, any other thread that wishes to join it). A
    detached thread is one that is completely detached from the rest of the
    system and cannot return values by "normal" means. Detached threads
    automatically clean up all of the internal resources associated with the
    thread when it exits. Joinable threads, on the other hand, must keep some
    state available for the ability to return values. To make sure that all
    memory allocated by the thread's internal structures gets freed, you must
    either join with the thread (with thd_join()) or detach it (with
    thd_detach()). The old KOS threading system only had what would be
    considered detached threads.

    \author Megan Potter
    \author Lawrence Sebald
    \see    arch/timer.h
    \see    kos/genwait.h
    \see    kos/mutex.h
    \see    kos/once.h
    \see    kos/recursive_lock.h
    \see    kos/rwsem.h
    \see    kos/sem.h
    \see    kos/tls.h
*/

/** \brief  Maximal thread priority.
    This macro defines the maximum value for a thread's priority. Note that the
    larger this number, the lower the priority of the thread.
*/
#define PRIO_MAX 4096

/** \brief  Default thread priority.
    Threads are created by default with the priority specified.
*/
#define PRIO_DEFAULT 10

/* Pre-define list/queue types */
struct kthread;

/* \cond */
TAILQ_HEAD(ktqueue, kthread);
LIST_HEAD(ktlist, kthread);
/* \endcond */

/** \brief  Structure describing one running thread.

    Each thread has one of this structure assigned to it, which hold all the
    data associated with the thread. There are various functions to manipulate
    the data in here, so you shouldn't generally do so manually.

    \headerfile kos/thread.h
*/
typedef struct kthread {
    /** \brief  Thread list handle. Not a function. */
    LIST_ENTRY(kthread) t_list;

    /** \brief  Run/Wait queue handle. Once again, not a function. */
    TAILQ_ENTRY(kthread) thdq;

    /** \brief  Timer queue handle (if applicable). Also not a function. */
    TAILQ_ENTRY(kthread) timerq;

    /** \brief  Kernel thread id. */
    tid_t tid;

    /** \brief  Static priority: 0..PRIO_MAX (higher means lower priority). */
    prio_t prio;

    /** \brief  Thread flags.
        \see    thd_flags   */
    uint32 flags;

    /** \brief  Process state.
        \see    thd_states  */
    int state;

    /** \brief  Generic wait target, if waiting.
        \see    kos/genwait.h   */
    void * wait_obj;

    /** \brief  Generic wait message, if waiting.
        \see    kos/genwait.h   */
    const char * wait_msg;

    /** \brief  Wait timeout callback.

        If the genwait times out while waiting, this function will be called.
        This allows hooks for things like fixing up semaphore count values, etc.

        \param  obj         The object that we were waiting on.
    */
    void (*wait_callback)(void * obj);

    /** \brief  Next scheduled time.
        This value is used for sleep and timed block operations. This value is
        in milliseconds since the start of timer_ms_gettime(). This should be
        enough for something like 2 million years of wait time. ;) */
    uint64 wait_timeout;

    /** \brief  Thread label.
        This value is used when printing out a user-readable process listing. */
    char label[256];

    /** \brief  Current file system path. */
    char pwd[256];

    /** \brief  Register store -- used to save thread context. */
    irq_context_t context;

    /** \brief  Thread private stack.
        This should be a pointer to the base of a stack page. */
    uint32 *stack;

    /** \brief  Size of the thread's stack, in bytes. */
    uint32 stack_size;

    /** \brief  Thread errno variable. */
    int thd_errno;

    /** \brief  Our reent struct for newlib. */
    struct _reent thd_reent;

    /** \brief  Thread-local storage.
        \see    kos/tls.h   */
    struct kthread_tls_kv_list tls_list;

    /** \brief  Return value of the thread function.
        This is only used in joinable threads.  */
    void *rv;
} kthread_t;

/** \defgroup thd_flags             Thread flag values

    These are possible values for the flags field on the kthread_t structure.
    These can be ORed together.

    @{
*/
#define THD_DEFAULTS    0       /**< \brief Defaults: no flags */
#define THD_USER        1       /**< \brief Thread runs in user mode */
#define THD_QUEUED      2       /**< \brief Thread is in the run queue */
#define THD_DETACHED    4       /**< \brief Thread is detached */
/** @} */

/** \defgroup thd_states            Thread states

    Each thread in the system is in exactly one of this set of states.

    @{
*/
#define STATE_ZOMBIE    0x0000  /**< \brief Waiting to die */
#define STATE_RUNNING   0x0001  /**< \brief Process is "current" */
#define STATE_READY     0x0002  /**< \brief Ready to be scheduled */
#define STATE_WAIT      0x0003  /**< \brief Blocked on a genwait */
#define STATE_FINISHED  0x0004  /**< \brief Finished execution */
/** @} */

/** \brief  Thread creation attributes.

    This structure allows you to specify the various attributes for a thread to
    have when it is created. These can only be modified (in general) at thread
    creation time (with the exception of detaching a thread, which can be done
    later with thd_detach()).

    Leaving any of the attributes in this structure 0 will set them to their
    default value.

    \headerfile kos/thread.h
*/
typedef struct kthread_attr {
    /** \brief  1 for a detached thread. */
    int create_detached;

    /** \brief  Set the size of the stack to be created. */
    uint32 stack_size;

    /** \brief  Pre-allocate a stack for the thread.
        \note   If you use this attribute, you must also set stack_size. */
    void *stack_ptr;

    /** \brief  Set the thread's priority. */
    prio_t prio;

    /** \brief  Thread label. */
    const char *label;
} kthread_attr_t;

/** \defgroup thd_modes             Threading system modes

    The threading system will always be in one of the following modes. This
    represents the type of scheduling done by the system (or the special case of
    threads not having been initialized yet).

    @{
*/
#define THD_MODE_NONE       -1  /**< \brief Threads not running */
#define THD_MODE_COOP       0   /**< \brief Cooperative threading mode */
#define THD_MODE_PREEMPT    1   /**< \brief Preemptive threading mode */
/** @} */

/** \brief  The currently executing thread.

    Do not manipulate this variable directly!
*/
extern kthread_t *thd_current;

/** \brief  Block the current thread.

    Blocks the calling thread and performs a reschedule as if a context switch
    timer had been executed. This is useful for, e.g., blocking on sync
    primitives. The param 'mycxt' should point to the calling thread's context
    block. This is implemented in arch-specific code.

    The meaningfulness of the return value depends on whether the unblocker set
    a return value or not.

    \param  mycxt           The IRQ context of the calling thread.

    \return                 Whatever the unblocker deems necessary to return.
*/
int thd_block_now(irq_context_t * mycxt);

/** \brief  Find a new thread to swap in.

    This function looks at the state of the system and returns a new thread
    context to swap in. This is called from thd_block_now() and from the
    preemptive context switcher. Note that thd_current might be NULL on entering
    this function, if the caller blocked itself.

    It is assumed that by the time this returns, the irq_srt_addr and
    thd_current will be updated.

    \return                 The IRQ context of the thread selected.
*/
irq_context_t * thd_choose_new();

/** \brief  Given a thread ID, locates the thread structure.
    \param  tid             The thread ID to retrieve.

    \return                 The thread on success, NULL on failure.
*/
kthread_t *thd_by_tid(tid_t tid);

/** \brief  Enqueue a process in the runnable queue.

    This function adds a thread to the runnable queue after the process group of
    the same priority if front_of_line is zero, otherwise queues it at the front
    of its priority group. Generally, you will not have to do this manually.

    \param  t               The thread to queue.
    \param  front_of_line   Set to 1 to put this thread in front of other
                            threads of the same priority, 0 to put it behind the
                            other threads (normal behavior).
*/
void thd_add_to_runnable(kthread_t *t, int front_of_line);

/** \brief  Removes a thread from the runnable queue, if it's there.

    This function removes a thread from the runnable queue, if it is currently
    in that queue. Generally, you shouldn't have to do this manually, as waiting
    on synchronization primitives and the like will do this for you if needed.

    \param  thd             The thread to remove from the runnable queue.

    \retval 0               On success, or if the thread isn't runnable.
*/
int thd_remove_from_runnable(kthread_t *thd);

/** \brief  Create a new thread.

    This function creates a new kernel thread with default parameters to run the
    given routine. The thread will terminate and clean up resources when the
    routine completes if the thread is created detached, otherwise you must
    join the thread with thd_join() to clean up after it.

    \param  detach          Set to 1 to create a detached thread. Set to 0 to
                            create a joinable thread.
    \param  routine         The function to call in the new thread.
    \param  param           A parameter to pass to the function called.

    \return                 The new thread on success, NULL on failure.
*/
kthread_t *thd_create(int detach, void * (*routine)(void *param), void *param);

/** \brief  Create a new thread with the specified set of attributes.

    This function creates a new kernel thread with the specified set of
    parameters to run the given routine.

    \param  attr            A set of thread attributes for the created thread.
                            Passing NULL will initialize all attributes to their
                            default values.
    \param  routine         The function to call in the new thread.
    \param  param           A parameter to pass to the function called.

    \return                 The new thread on success, NULL on failure.
*/
kthread_t *thd_create_ex(kthread_attr_t *attr, void *(*routine)(void *),
                         void *param);

/** \brief  Brutally kill the given thread.

    This function kills the given thread, removing it from the execution chain,
    cleaning up thread-local data and other internal structures. You should
    never call this function on the current thread. In general, you shouldn't
    call this function at all.

    \param  thd             The thread to destroy.
    \retval 0               On success.
*/
int thd_destroy(kthread_t *thd);

/** \brief  Exit the current thread.

    This function ends the execution of the current thread, removing it from all
    execution queues. This function will never return to the thread. Returning
    from the thread's function is equivalent to calling this function.

    \param  rv              The return value of the thread.
*/
void thd_exit(void *rv) __noreturn;

/** \brief  Force a thread reschedule.

    This function is the thread scheduler, and is generally called from a timer
    interrupt, at least in preemptive mode. You will most likely never have a
    reason to call this function directly.

    For most cases, you'll want to set front_of_line to zero, but read the
    comments in kernel/thread/thread.c for more info, especially if you need to
    guarantee low latencies. This function just updates irq_srt_addr and
    thd_current. Set 'now' to non-zero if you want to use a particular system
    time for checking timeouts.

    \param  front_of_line   Set to 0, unless you have a good reason not to.
    \param  now             Set to 0, unless you have a good reason not to.
*/
void thd_schedule(int front_of_line, uint64 now);

/** \brief  Force a given thread to the front of the queue.

    This function promotes the given thread to be the next one that will be
    swapped in by the scheduler. This function is only callable inside an
    interrupt context (it simply returns otherwise).
*/
void thd_schedule_next(kthread_t *thd);

/** \brief  Throw away the current thread's timeslice.

    This function manually yields the current thread's timeslice to the system,
    forcing a reschedule to occur.
*/
void thd_pass();

/** \brief  Sleep for a given number of milliseconds.

    This function puts the current thread to sleep for the specified amount of
    time. The thread will be removed from the runnable queue until the given
    number of milliseconds passes. That is to say that the thread will sleep for
    at least the given number of milliseconds. If another thread is running, it
    will likely sleep longer.

    \param  ms              The number of milliseconds to sleep.
*/
void thd_sleep(int ms);

/** \brief  Set a thread's priority value.

    This function is used to change the priority value of a thread. If the
    thread is scheduled already, it will be rescheduled with the new priority
    value.

    \param  thd             The thread to change the priority of.
    \param  prio            The priority value to assign to the thread.

    \retval 0               On success.
*/
int thd_set_prio(kthread_t *thd, prio_t prio);

/** \brief  Retrieve the current thread's kthread struct.
    \return                 The current thread's structure.
*/
kthread_t *thd_get_current();

/** \brief  Retrieve the thread's label.
    \param  thd             The thread to retrieve from.

    \return                 The human-readable label of the thread.
*/
const char *thd_get_label(kthread_t *thd);

/** \brief  Set the thread's label.

    This function sets the label of a thread, which is simply a human-readable
    string that is used to identify the thread. These labels aren't used for
    anything internally, and you can give them any label you want. These are
    mainly seen in the printouts from thd_pslist() or thd_pslist_queue().

    \param  thd             The thread to set the label of.
    \param  label           The string to set as the label.
*/
void thd_set_label(kthread_t *thd, const char *label);

/** \brief  Retrieve the thread's current working directory.

    This function retrieves the working directory of a thread. Generally, you
    will want to use either fs_getwd() or one of the standard C functions for
    doing this, but this is here in case you need it when the thread isn't
    active for some reason.

    \param  thd             The thread to retrieve from.

    \return                 The thread's working directory.
*/
const char *thd_get_pwd(kthread_t *thd);

/** \brief  Set the thread's current working directory.

    This function will set the working directory of a thread. Generally, you
    will want to use either fs_chdir() or the standard C chdir() function to
    do this, but this is here in case you need to do it while the thread isn't
    active for some reason.

    \param  thd             The thread to set the working directory of.
    \param  pwd             The directory to set as active.
*/
void thd_set_pwd(kthread_t *thd, const char *pwd);

/** \brief  Retrieve a pointer to the thread errno.

    This function retrieves a pointer to the errno value for the thread. You
    should generally just use the errno variable to access this.

    \param  thd             The thread to retrieve from.

    \return                 A pointer to the thread's errno.
*/
int * thd_get_errno(kthread_t *thd);

/** \brief  Retrieve a pointer to the thread reent struct.

    This function is used to retrieve some internal state that is used by
    newlib to provide a reentrant libc.

    \param  thd             The thread to retrieve from.

    \return                 The thread's reent struct.
*/
struct _reent * thd_get_reent(kthread_t *thd);

/** \brief  Change threading modes.

    This function changes the current threading mode of the system.

    \param  mode            One of the \ref thd_modes values.

    \return                 The old mode of the threading system.
*/
int thd_set_mode(int mode);

/** \brief  Fetch the current threading mode.

    \return                 The current mode of the threading system.
*/
int thd_get_mode(void);

/** \brief  Wait for a thread to exit.

    This function "joins" a joinable thread. This means effectively that the
    calling thread blocks until the speified thread completes execution. It is
    invalid to join a detached thread, only joinable threads may be joined.

    \param  thd             The joinable thread to join.
    \param  value_ptr       A pointer to storage for the thread's return value,
                            or NULL if you don't care about it.

    \return                 0 on success, or less than 0 if the thread is
                            non-existant or not joinable.
*/
int thd_join(kthread_t * thd, void **value_ptr);

/** \brief  Detach a joinable thread.

    This function switches the specified thread's mode from THD_MODE_JOINABLE
    to THD_MODE_DETACHED. This will ensure that the thread cleans up all of its
    internal resources when it exits.

    \param  thd             The joinable thread to detach.

    \return                 0 on success or less than 0 if the thread is
                            non-existant or already detached.
    \see    thd_join()
*/
int thd_detach(kthread_t *thd);

/** \brief Iterate all threads and call the passed callback for each

    \param cb               The callback to call for each thread
    \param data             User data to be passed to the callback

    \retval 0               On success.
*/
int thd_each(int (*cb)(kthread_t* thd, void* user_data), void* data);

/** \brief  Print a list of all threads using the given print function.

    \param  pf              The printf-like function to print with.

    \retval 0               On success.
*/
int thd_pslist(int (*pf)(const char *fmt, ...));

/** \brief  Print a list of all queued threads using the given print function.

    \param  pf              The printf-like function to print with.

    \retval 0               On success.
*/
int thd_pslist_queue(int (*pf)(const char *fmt, ...));

/** \brief  Initialize the threading system.

    This is normally done for you by default when KOS starts. This will also
    initialize all the various synchronization primitives.

    \param  mode            One of the \ref thd_modes values.

    \retval -1              If threads are already initialized.
    \retval 0               On success.
*/
int thd_init(int mode);

/** \brief  Shutdown the threading system.

    This is done for you by the normal shutdown procedure of KOS. This will
    also shutdown all the various synchronization primitives.
*/
void thd_shutdown();

__END_DECLS

#endif  /* __KOS_THREAD_H */
