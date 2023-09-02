/* KallistiOS ##version##

   include/kos/thread.h
   Copyright (C) 2000, 2001, 2002, 2003 Megan Potter
   Copyright (C) 2009, 2010, 2016 Lawrence Sebald
   Copyright (C) 2023 Colton Pawielski
   Copyright (C) 2023, 2024 Falco Girgis

*/

/** \file    kos/thread.h
    \brief   Threading support.
    \ingroup kthreads

    This file contains the interface to the threading system of KOS. Timer
    interrupts are used to reschedule threads within the system.

    \see    arch/timer.h
    \see    kos/genwait.h
    \see    kos/mutex.h
    \see    kos/once.h
    \see    kos/recursive_lock.h
    \see    kos/rwsem.h
    \see    kos/sem.h
    \see    kos/tls.h

    \todo
        - Remove deprecated thread mode API
        - Remove global extern pointer to current thread

    \author Megan Potter
    \author Lawrence Sebald
    \author Falco Girgis
*/

#ifndef __KOS_THREAD_H
#define __KOS_THREAD_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/cdefs.h>
#include <kos/tls.h>
#include <arch/irq.h>
#include <sys/queue.h>
#include <sys/reent.h>

#include <stdint.h>
#include <stdbool.h>

/** \defgroup kthreads  Kernel
    \brief              KOS Native Kernel Threading API
    \ingroup            threading

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

    \sa semaphore_t, mutex_t, kthread_once_t, kthread_key_t, rw_semaphore_t

    @{
*/

/** \brief  Process ID

    This macro defines the single process ID that encompasses all of KOS and the
    running application along with all of its threads.
*/
#define KOS_PID 1

/** \brief   Maximal thread priority

    This macro defines the maximum value for a thread's priority. Note that the
    larger this number, the lower the priority of the thread.
*/
#define PRIO_MAX 4096

/** \brief   Default thread priority

    Threads are created by default with the priority specified.
*/
#define PRIO_DEFAULT 10

/** \brief   Size of a kthread's label

    Maximum number of characters in a thread's label or name
    (including NULL terminator).
*/
#define KTHREAD_LABEL_SIZE  256

/** \brief   Size of a kthread's current directory

    Maximum number of characters in a thread's current working
    directory (including NULL terminator).
*/
#define KTHREAD_PWD_SIZE    256

/* Pre-define list/queue types */
struct kthread;

/* \cond */
TAILQ_HEAD(ktqueue, kthread);
LIST_HEAD(ktlist, kthread);
/* \endcond */

/** \name     Thread flag values
    \brief    Flags for kthread_flags_t

    These are possible values for the flags field on the kthread_t structure.
    These can be ORed together.

    @{
*/
#define THD_DEFAULTS    0  /**< \brief Defaults: no flags */
#define THD_USER        1  /**< \brief Thread runs in user mode */
#define THD_QUEUED      2  /**< \brief Thread is in the run queue */
#define THD_DETACHED    4  /**< \brief Thread is detached */
#define THD_OWNS_STACK  8  /**< \brief Thread manages stack lifetime */
/** @} */

/** \brief Kernel thread flags type */
typedef uint8_t kthread_flags_t;

/** \brief Kernel thread state

    Each thread in the system is in exactly one of this set of states.
*/
typedef enum kthread_state {
    STATE_ZOMBIE   = 0x0000,  /**< \brief Waiting to die */
    STATE_RUNNING  = 0x0001,  /**< \brief Process is "current" */
    STATE_READY    = 0x0002,  /**< \brief Ready to be scheduled */
    STATE_WAIT     = 0x0003,  /**< \brief Blocked on a genwait */
    STATE_FINISHED = 0x0004   /**< \brief Finished execution */
} kthread_state_t;



/** \brief   Structure describing one running thread.

    Each thread has one of these structures assigned to it, which holds all the
    data associated with the thread. There are various functions to manipulate
    the data in here, so you shouldn't generally do so manually.
*/
typedef __attribute__((aligned(32))) struct kthread {
    /** \brief  Register store -- used to save thread context. */
    irq_context_t context;

    /** \brief  Thread list handle. Not a function. */
    LIST_ENTRY(kthread) t_list;

    /** \brief  Run/Wait queue handle. Once again, not a function. */
    TAILQ_ENTRY(kthread) thdq;

    /** \brief  Timer queue handle (if applicable). Also not a function. */
    TAILQ_ENTRY(kthread) timerq;

    /** \brief  Kernel thread id. */
    tid_t tid;

    /** \brief  Dynamic priority */
    prio_t prio;

    /** \brief  Static priority: 0..PRIO_MAX (higher means lower priority). */
    prio_t real_prio;

    /** \brief  Thread flags. */
    kthread_flags_t flags;

    /** \brief  Process state */
    kthread_state_t state;

    /** \brief  Generic wait target, if waiting.

        \see    kos/genwait.h
    */
    void *wait_obj;

    /** \brief  Generic wait message, if waiting.

        \see    kos/genwait.h
    */
    const char *wait_msg;

    /** \brief  Wait timeout callback.

        If the genwait times out while waiting, this function will be called.
        This allows hooks for things like fixing up semaphore count values, etc.

        \param  obj         The object that we were waiting on.
    */
    void (*wait_callback)(void *obj);

    /** \brief  Next scheduled time.

        This value is used for sleep and timed block operations. This value is
        in milliseconds since the start of timer_ms_gettime(). This should be
        enough for something like 2 million years of wait time. ;)
    */
    uint64_t wait_timeout;

    /** \brief Per-Thread CPU Time. */
    struct {
        uint64_t scheduled; /**< \brief time when the thread became active */
        uint64_t total;     /**< \brief total running CPU time for thread */
    } cpu_time;

    /** \brief  Thread label.

        This value is used when printing out a user-readable process listing.
    */
    char label[KTHREAD_LABEL_SIZE];

    /** \brief  Current file system path. */
    char pwd[KTHREAD_PWD_SIZE];

    /** \brief  Thread private stack.

        This should be a pointer to the base of a stack page.
    */
    void *stack;

    /** \brief  Size of the thread's stack, in bytes. */
    size_t stack_size;

    /** \brief  Thread errno variable. */
    int thd_errno;

    /** \brief  Our reent struct for newlib. */
    struct _reent thd_reent;

    /** \brief  OS-level thread-local storage.

        \see    kos/tls.h
    */
    struct kthread_tls_kv_list tls_list;

    /** \brief Compiler-level thread-local storage. */
    void *tls_hnd;

    /** \brief  Return value of the thread function.

        This is only used in joinable threads.
    */
    void *rv;
} kthread_t;

/** \brief   Thread creation attributes.

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
    bool create_detached;

    /** \brief  Set the size of the stack to be created. */
    size_t stack_size;

    /** \brief  Pre-allocate a stack for the thread.
        \note   If you use this attribute, you must also set stack_size. */
    void *stack_ptr;

    /** \brief  Set the thread's priority. */
    prio_t prio;

    /** \brief  Thread label. */
    const char *label;
} kthread_attr_t;

/** \brief  kthread mode values

    \deprecated
    Only preemptive scheduling is still supported!

    The threading system will always be in one of the following modes. This
    represents either pre-emptive scheduling or an un-initialized state.
*/
typedef enum kthread_mode {
    THD_MODE_NONE    = -1,  /**< \brief Threads not running */
    THD_MODE_COOP    =  0,  /**< \brief Cooperative mode \deprecated */
    THD_MODE_PREEMPT =  1   /**< \brief Preemptive threading mode */
} kthread_mode_t;

/** \cond The currently executing thread -- Do not manipulate directly! */
extern kthread_t *thd_current;
/** \endcond */

/** \brief   Block the current thread.

    Blocks the calling thread and performs a reschedule as if a context switch
    timer had been executed. This is useful for, e.g., blocking on sync
    primitives. The param 'mycxt' should point to the calling thread's context
    block. This is implemented in arch-specific code.

    The meaningfulness of the return value depends on whether the unblocker set
    a return value or not.

    \param  mycxt           The IRQ context of the calling thread.

    \return                 Whatever the unblocker deems necessary to return.
*/
int thd_block_now(irq_context_t *mycxt);

/** \brief   Find a new thread to swap in.

    This function looks at the state of the system and returns a new thread
    context to swap in. This is called from thd_block_now() and from the
    preemptive context switcher. Note that thd_current might be NULL on entering
    this function, if the caller blocked itself.

    It is assumed that by the time this returns, the irq_srt_addr and
    thd_current will be updated.

    \return                 The IRQ context of the thread selected.
*/
irq_context_t *thd_choose_new(void);

/** \brief       Given a thread ID, locates the thread structure.
    \relatesalso kthread_t

    \param  tid             The thread ID to retrieve.

    \return                 The thread on success, NULL on failure.
*/
kthread_t *thd_by_tid(tid_t tid);

/** \brief       Enqueue a process in the runnable queue.
    \relatesalso kthread_t

    This function adds a thread to the runnable queue after the process group of
    the same priority if front_of_line is zero, otherwise queues it at the front
    of its priority group. Generally, you will not have to do this manually.

    \param  t               The thread to queue.
    \param  front_of_line   Set to true to put this thread in front of other
                            threads of the same priority, false to put it
                            behind the other threads (normal behavior).

    \sa thd_remove_from_runnable
*/
void thd_add_to_runnable(kthread_t *t, bool front_of_line);

/** \brief       Removes a thread from the runnable queue, if it's there.
    \relatesalso kthread_t

    This function removes a thread from the runnable queue, if it is currently
    in that queue. Generally, you shouldn't have to do this manually, as waiting
    on synchronization primitives and the like will do this for you if needed.

    \param  thd             The thread to remove from the runnable queue.

    \retval 0               On success, or if the thread isn't runnable.

    \sa thd_add_to_runnable
*/
int thd_remove_from_runnable(kthread_t *thd);

/** \brief       Create a new thread.
    \relatesalso kthread_t

    This function creates a new kernel thread with default parameters to run the
    given routine. The thread will terminate and clean up resources when the
    routine completes if the thread is created detached, otherwise you must
    join the thread with thd_join() to clean up after it.

    \param  detach          Set to true to create a detached thread. Set to
                            false to create a joinable thread.
    \param  routine         The function to call in the new thread.
    \param  param           A parameter to pass to the function called.

    \return                 The new thread on success, NULL on failure.

    \sa thd_create_ex, thd_destroy
*/
kthread_t *thd_create(bool detach, void *(*routine)(void *param), void *param);

/** \brief       Create a new thread with the specified set of attributes.
    \relatesalso kthread_t

    This function creates a new kernel thread with the specified set of
    parameters to run the given routine.

    \param  attr            A set of thread attributes for the created thread.
                            Passing NULL will initialize all attributes to their
                            default values.
    \param  routine         The function to call in the new thread.
    \param  param           A parameter to pass to the function called.

    \return                 The new thread on success, NULL on failure.

    \sa thd_create, thd_destroy
*/
kthread_t *thd_create_ex(const kthread_attr_t *__RESTRICT attr,
                         void *(*routine)(void *param), void *param);

/** \brief       Brutally kill the given thread.
    \relatesalso kthread_t

    This function kills the given thread, removing it from the execution chain,
    cleaning up thread-local data and other internal structures. In general, you
    shouldn't call this function at all.

    \warning
    You should never call this function on the current thread.

    \param  thd             The thread to destroy.
    \retval 0               On success.

    \sa thd_create
*/
int thd_destroy(kthread_t *thd);

/** \brief   Exit the current thread.

    This function ends the execution of the current thread, removing it from all
    execution queues. This function will never return to the thread. Returning
    from the thread's function is equivalent to calling this function.

    \param  rv              The return value of the thread.
*/
void thd_exit(void *rv) __noreturn;

/** \brief   Force a thread reschedule.

    This function is the thread scheduler, and is generally called from a timer
    interrupt. You will most likely never have a reason to call this function
    directly.

    For most cases, you'll want to set front_of_line to zero, but read the
    comments in kernel/thread/thread.c for more info, especially if you need to
    guarantee low latencies. This function just updates irq_srt_addr and
    thd_current. Set 'now' to non-zero if you want to use a particular system
    time for checking timeouts.

    \param  front_of_line   Set to false, unless you have a good reason not to.
    \param  now             Set to 0, unless you have a good reason not to.

    \sa thd_schedule_next
*/
void thd_schedule(bool front_of_line, uint64_t now);

/** \brief       Force a given thread to the front of the queue.
    \relatesalso kthread_t

    This function promotes the given thread to be the next one that will be
    swapped in by the scheduler. This function is only callable inside an
    interrupt context (it simply returns otherwise).

    \param  thd     The thread to schedule next.
*/
void thd_schedule_next(kthread_t *thd);

/** \brief   Throw away the current thread's timeslice.

    This function manually yields the current thread's timeslice to the system,
    forcing a reschedule to occur.
*/
void thd_pass(void);

/** \brief   Sleep for a given number of milliseconds.

    This function puts the current thread to sleep for the specified amount of
    time. The thread will be removed from the runnable queue until the given
    number of milliseconds passes. That is to say that the thread will sleep for
    at least the given number of milliseconds. If another thread is running, it
    will likely sleep longer.

    \note
    When \p ms is given a value of `0`, this is equivalent to thd_pass().

    \param  ms              The number of milliseconds to sleep.
*/
void thd_sleep(unsigned ms);

/** \brief       Set a thread's priority value.
    \relatesalso kthread_t

    This function is used to change the priority value of a thread. If the
    thread is scheduled already, it will be rescheduled with the new priority
    value.

    \param  thd             The thread to change the priority of.
    \param  prio            The priority value to assign to the thread.

    \retval 0               On success.
    \retval -1              thd is NULL.
    \retval -2              prio requested was out of range.
*/
int thd_set_prio(kthread_t *thd, prio_t prio);

/** \brief       Retrieve the current thread's kthread struct.
    \relatesalso kthread_t

    \return                 The current thread's structure.
*/
kthread_t *thd_get_current(void);

/** \brief       Retrieve the thread's label.
    \relatesalso kthread_t

    \param  thd             The thread to retrieve from.

    \return                 The human-readable label of the thread.

    \sa thd_set_label
*/
const char *thd_get_label(kthread_t *thd);

/** \brief       Set the thread's label.
    \relatesalso kthread_t

    This function sets the label of a thread, which is simply a human-readable
    string that is used to identify the thread. These labels aren't used for
    anything internally, and you can give them any label you want. These are
    mainly seen in the printouts from thd_pslist() or thd_pslist_queue().

    \param  thd             The thread to set the label of.
    \param  label           The string to set as the label.

    \sa thd_get_label
*/
void thd_set_label(kthread_t *__RESTRICT thd, const char *__RESTRICT label);

/** \brief       Retrieve the thread's current working directory.
    \relatesalso kthread_t

    This function retrieves the working directory of a thread. Generally, you
    will want to use either fs_getwd() or one of the standard C functions for
    doing this, but this is here in case you need it when the thread isn't
    active for some reason.

    \param  thd             The thread to retrieve from.

    \return                 The thread's working directory.

    \sa thd_set_pd
*/
const char *thd_get_pwd(kthread_t *thd);

/** \brief       Set the thread's current working directory.
    \relatesalso kthread_t

    This function will set the working directory of a thread. Generally, you
    will want to use either fs_chdir() or the standard C chdir() function to
    do this, but this is here in case you need to do it while the thread isn't
    active for some reason.

    \param  thd             The thread to set the working directory of.
    \param  pwd             The directory to set as active.

    \sa thd_get_pwd
*/
void thd_set_pwd(kthread_t *__RESTRICT thd, const char *__RESTRICT pwd);

/** \brief       Retrieve a pointer to the thread errno.
    \relatesalso kthread_t

    This function retrieves a pointer to the errno value for the thread. You
    should generally just use the errno variable to access this.

    \param  thd             The thread to retrieve from.

    \return                 A pointer to the thread's errno.
*/
int *thd_get_errno(kthread_t *thd);

/** \brief       Retrieve a pointer to the thread reent struct.
    \relatesalso kthread_t

    This function is used to retrieve some internal state that is used by
    newlib to provide a reentrant libc.

    \param  thd             The thread to retrieve from.

    \return                 The thread's reent struct.
*/
struct _reent *thd_get_reent(kthread_t *thd);

/** \brief       Retrieves the thread's elapsed CPU time
    \relatesalso kthread_t

    Returns the amount of active CPU time the thread has consumed in
    nanoseconds.

    \param thd          The thead to retrieve the CPU time for.

    \retval             Total utilized CPU time in nanoseconds.
*/
uint64_t thd_get_cpu_time(kthread_t *thd);

/** \brief       Retrieves all thread's elapsed CPU time
    \relatesalso kthread_t

    Returns the amount of active CPU time all threads have consumed in
    nanoseconds.

    \retval             Total utilized CPU time in nanoseconds.
*/
uint64_t thd_get_total_cpu_time(void);

/** \brief   Change threading modes.

    This function changes the current threading mode of the system.
    With preemptive threading being the only mode.

    \deprecated
    This is now deprecated

    \param  mode            One of the THD_MODE values.

    \return                 The old mode of the threading system.

    \sa thd_get_mode
*/
int thd_set_mode(kthread_mode_t mode) __deprecated;

/** \brief   Fetch the current threading mode.

    With preemptive threading being the only mode.

    \deprecated
    This is now deprecated.

    \return                 The current mode of the threading system.

    \sa thd_set_mode
*/
kthread_mode_t thd_get_mode(void) __deprecated;

/** \brief   Set the scheduler's frequency.

    Sets the frequency of the scheduler interrupts in hertz.

    \param hertz    The new frequency in hertz (1-1000)

    \retval 0       The frequency was updated successfully.
    \retval -1      \p hertz is invalid.

    \sa thd_get_hz(), HZ
*/
int thd_set_hz(unsigned int hertz);

/** \brief   Fetch the scheduler's current frequency.

    Queries the scheduler for its interrupt frequency in hertz.

    \return                 Scheduler frequency in hertz.

    \sa thd_set_hz(), HZ
*/
unsigned thd_get_hz(void);

/** \brief       Wait for a thread to exit.
    \relatesalso kthread_t

    This function "joins" a joinable thread. This means effectively that the
    calling thread blocks until the speified thread completes execution. It is
    invalid to join a detached thread, only joinable threads may be joined.

    \param  thd             The joinable thread to join.
    \param  value_ptr       A pointer to storage for the thread's return value,
                            or NULL if you don't care about it.

    \return                 0 on success, or less than 0 if the thread is
                            non-existent or not joinable.

    \sa thd_detach
*/
int thd_join(kthread_t *thd, void **value_ptr);

/** \brief       Detach a joinable thread.
    \relatesalso kthread_t

    This function switches the specified thread's mode from THD_MODE_JOINABLE
    to THD_MODE_DETACHED. This will ensure that the thread cleans up all of its
    internal resources when it exits.

    \param  thd             The joinable thread to detach.

    \return                 0 on success or less than 0 if the thread is
                            non-existent or already detached.
    \sa thd_join()
*/
int thd_detach(kthread_t *thd);

/** \brief       Iterate all threads and call the passed callback for each
    \relatesalso kthread_t

    \param cb               The callback to call for each thread. 
                            If a nonzero value is returned, iteration
                            ceases immediately.
    \param data             User data to be passed to the callback

    \retval                 0 or the first nonzero value returned by \p cb.

    \sa thd_pslist
*/
int thd_each(int (*cb)(kthread_t *thd, void *user_data), void *data);

/** \brief   Print a list of all threads using the given print function.

    Each thread is printed with its address, tid, priority level, flags,
    it's wait timeout (if sleeping) the amount of cpu time usage in ns
    (this includes time in IRQs), state, and name.

    In addition a '[system]' item is provided that represents time since
    initialization not spent in a thread (context switching, updating
    wait timeouts, etc).

    \param  pf              The printf-like function to print with.

    \retval 0               On success.

    \sa thd_pslist_queue
*/
int thd_pslist(int (*pf)(const char *fmt, ...));

/** \brief   Print a list of all queued threads using the given print function.

    \param  pf              The printf-like function to print with.

    \retval 0               On success.

    \sa thd_pslist
*/
int thd_pslist_queue(int (*pf)(const char *fmt, ...));

/** \cond INTERNAL */

/** \brief  Initialize the threading system.
    
    This is normally done for you by default when KOS starts. This will also
    initialize all the various synchronization primitives.
    \retval -1              If threads are already initialized.
    \retval 0               On success.
    \sa thd_shutdown
*/
int thd_init(void);


/** \brief   Shutdown the threading system.
 
    This is done for you by the normal shutdown procedure of KOS. This will
    also shutdown all the various synchronization primitives.
 
    \sa thd_init
*/
void thd_shutdown(void);

/** \endcond */

/** @} */

__END_DECLS

#endif  /* __KOS_THREAD_H */
