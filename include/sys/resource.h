/* KallistiOS ##version##

   sys/resource.h
   Copyright (C) 2025 Donald Haase
*/

/** \file   sys/resource.h
    \brief  Basic definitions for resource operations

    This file provides a basic implementation of the POSIX/XSI
    standard for resource operations. These are meant to be
    per-process, so are basically stubs for the purposes of KOS
    which is single-process.
*/

#ifndef __SYS_RESOURCE_H
#define __SYS_RESOURCE_H

__BEGIN_DECLS

#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>

/* Possible values for the `which` argument of
    `getpriority()` and`setpriority()` */
enum {
    PRIO_PROCESS,
    PRIO_PGRP,
    PRIO_USER
};

/** \brief  Obtain the nice value of a process, process group, or user.

    In KOS we don't have a concept currently of any of these things, so
    this is effectively a stub that will only return a value if requested
    for the default/current id.

    \param  which   Which type of ID to get the nice value of.
    \param  who     Only 0 is currently valid.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EINVAL - `which` was an invalid value.
    \em     EINVAL - `who` was a value other than 0.
*/
int getpriority(int which, id_t who);

/** \brief  Sets the nice value of a process, process group, or user.

    In KOS we don't have a concept currently of any of these things, so
    this is effectively a stub that will only set a value if requested
    for the default/current id. This also currently has no impact aside
    from changing the return of `getpriority`.

    \param  which   Which type of ID to set the nice value of.
    \param  who     Only 0 is currently valid.
    \param  value   nice value to set.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EINVAL - `which` was an invalid value.
    \em     EINVAL - `who` was a value other than 0.
*/
int setpriority(int which, id_t who, int value);

/* An unsigned integer type used for limit values */
typedef unsigned int rlim_t;

#define RLIM_INFINITY UINT_MAX
#define RLIM_SAVED_MAX RLIM_INFINITY
#define RLIM_SAVED_CUR RLIM_INFINITY

struct rlimit {
    rlim_t rlim_cur;    /* The current (soft) limit. */
    rlim_t rlim_max;    /* The hard limit. */
};

/* Possible values for the `resource` argument of
    `getrlimit()` and`setrlimit()` */
enum {
    RLIMIT_CORE,    /* Limit on size of core image. */
    RLIMIT_CPU,     /* Limit on CPU time per process. */
    RLIMIT_DATA,    /* Limit on data segment size. */
    RLIMIT_FSIZE,   /* Limit on file size. */
    RLIMIT_NOFILE,  /* Limit on number of open files. */
    RLIMIT_STACK,   /* Limit on stack size. */
    RLIMIT_AS       /* Limit on address space size. */
};

/** \brief  Gets the maximum resource consumption limits.

    Everything will return `RLIM_INFINITY` as we impose no such
    limits.

    \param  resource   The type of resource to get the rlimit for.
    \param  rlp        Where to place the rlimit data.

    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EINVAL - `resource` was an invalid value.
*/
int getrlimit(int resource, struct rlimit *rlp);

/** \brief  Sets the maximum resource consumption limits.

    Just a stub. Errors on bad input, otherwise always succeeds.

    \param  resource   The type of resource to set the rlimit for.
    \param  rlp        Ignored.

    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EINVAL - `resource` was an invalid value.
*/
int setrlimit(int resource, const struct rlimit *rlp);

/* Possible values for the `who` argument of `getrusage()` */
enum {
    RUSAGE_SELF,    /* Returns information about the current process. */
    RUSAGE_CHILDREN /* Returns information about children of the current process. */
};

struct rusage {
  	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
};

/** \brief  Get information about cpu utilization.

    For the purposes of this function, KOS is treated as having a
    single process that all threads are running under. As such `RUSAGE_SELF`
    will return user time that is the sum of all living threads and
    system time that is all the rest (IRQs + dead threads).
    `RUSAGE_CHILDREN` will return zero.

    \param  who     `RUSAGE_SELF` or `RUSAGE_CHILDREN`.
    \param  r_usage rusage data to be filled.
    \retval 0       On success.
    \retval -1      On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     EINVAL - `who` was an invalid value.
*/
int getrusage(int who, struct rusage *r_usage);

__END_DECLS

#endif /* !__SYS_RESOURCE_H */
