/* KallistiOS ##version##

   nmmgr.c
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2026 Joseph Black

*/

/*

This module manages "names". A name is a generic identifier that corresponds
to a handler for that name. These names can correspond to services exported
by a module or the kernel, they can be VFS handlers, they can be just about
anything. The only requirement is that they implement the nmmgr_handler_t
interface at the front of their struct.

*/

#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <kos/init_base.h>
#include <kos/nmmgr.h>
#include <kos/mutex.h>
#include <kos/cond.h>
#include <kos/exports.h>
#include <kos/irq.h>
#include <arch/timer.h>

/* Thread mutex for our name handler list */
static mutex_t mutex = MUTEX_INITIALIZER;

/* Name handler structures; these structs contain path/type pairs that
   describe how to handle a given path name. */
static nmmgr_list_t nmmgr_handlers;

typedef struct nmmgr_lifetime {
    nmmgr_handler_t *handler;
    size_t references;
    bool published;
    bool removing;
    condvar_t idle;
    SLIST_ENTRY(nmmgr_lifetime) list_entry;
} nmmgr_lifetime_t;

static SLIST_HEAD(nmmgr_lifetime_list, nmmgr_lifetime) lifetimes;

static nmmgr_lifetime_t *lifetime_find_locked(nmmgr_handler_t *handler) {
    nmmgr_lifetime_t *lifetime;

    SLIST_FOREACH(lifetime, &lifetimes, list_entry) {
        if(lifetime->handler == handler)
            return lifetime;
    }

    return NULL;
}

static nmmgr_handler_t *lookup_locked(const char *fn) {
    nmmgr_handler_t *current = NULL;
    nmmgr_handler_t *candidate;
    size_t current_length = 0;

    SLIST_FOREACH(candidate, &nmmgr_handlers, list_ent) {
        const size_t candidate_length = strlen(candidate->pathname);

        if(!strncasecmp(candidate->pathname, fn, candidate_length) &&
           current_length < candidate_length) {
            current_length = candidate_length;
            current = candidate;
        }
    }

    if(current && (current->flags & NMMGR_FLAGS_ALIAS))
        current = ((alias_handler_t *)current)->alias;

    return current;
}

/* Locate a name handler for a given path name */
nmmgr_handler_t * nmmgr_lookup(const char *fn) {
    nmmgr_handler_t *handler;

    if(!fn) {
        errno = EINVAL;
        return NULL;
    }

    if(mutex_lock(&mutex) < 0)
        return NULL;

    handler = lookup_locked(fn);
    mutex_unlock(&mutex);
    return handler;
}

nmmgr_handler_t *nmmgr_lookup_ref(const char *fn) {
    nmmgr_handler_t *handler;
    nmmgr_lifetime_t *lifetime;

    if(!fn) {
        errno = EINVAL;
        return NULL;
    }

    if(mutex_lock(&mutex) < 0)
        return NULL;

    handler = lookup_locked(fn);
    lifetime = handler ? lifetime_find_locked(handler) : NULL;

    if(!lifetime || !lifetime->published) {
        handler = NULL;
        errno = ENOENT;
    }
    else {
        ++lifetime->references;
    }

    mutex_unlock(&mutex);
    return handler;
}

nmmgr_list_t * nmmgr_get_list(void) {
    return &nmmgr_handlers;
}

/* Add a name handler */
int nmmgr_handler_add(nmmgr_handler_t *hnd) {
    nmmgr_lifetime_t *lifetime;
    nmmgr_handler_t *other;

    if(!hnd || !hnd->pathname[0] ||
       !memchr(hnd->pathname, '\0', sizeof(hnd->pathname))) {
        errno = EINVAL;
        return -1;
    }

    lifetime = calloc(1, sizeof(*lifetime));

    if(!lifetime)
        return -1;

    lifetime->handler = hnd;
    lifetime->published = true;
    cond_init(&lifetime->idle);

    if(mutex_lock(&mutex) < 0) {
        cond_destroy(&lifetime->idle);
        free(lifetime);
        return -1;
    }

    if(lifetime_find_locked(hnd)) {
        errno = EALREADY;
        goto fail;
    }

    SLIST_FOREACH(other, &nmmgr_handlers, list_ent) {
        if(other->type == hnd->type &&
           !strcasecmp(other->pathname, hnd->pathname)) {
            errno = EEXIST;
            goto fail;
        }
    }

    SLIST_INSERT_HEAD(&nmmgr_handlers, hnd, list_ent);
    SLIST_INSERT_HEAD(&lifetimes, lifetime, list_entry);

    mutex_unlock(&mutex);

    return 0;

fail:
    mutex_unlock(&mutex);
    cond_destroy(&lifetime->idle);
    free(lifetime);
    return -1;
}

int nmmgr_handler_retain(nmmgr_handler_t *hnd) {
    nmmgr_lifetime_t *lifetime;
    int rv = -1;

    if(!hnd) {
        errno = EINVAL;
        return -1;
    }

    if(mutex_lock(&mutex) < 0)
        return -1;

    lifetime = lifetime_find_locked(hnd);

    if(lifetime && lifetime->published) {
        ++lifetime->references;
        rv = 0;
    }
    else {
        errno = ENOENT;
    }

    mutex_unlock(&mutex);
    return rv;
}

int nmmgr_handler_release(nmmgr_handler_t *hnd) {
    nmmgr_lifetime_t *lifetime;
    int rv = -1;

    if(!hnd) {
        errno = EINVAL;
        return -1;
    }

    if(mutex_lock(&mutex) < 0)
        return -1;

    lifetime = lifetime_find_locked(hnd);

    if(lifetime && lifetime->references) {
        --lifetime->references;
        rv = 0;

        if(!lifetime->references)
            cond_broadcast(&lifetime->idle);
    }
    else {
        errno = EINVAL;
    }

    mutex_unlock(&mutex);
    return rv;
}

int nmmgr_handler_remove_timed(nmmgr_handler_t *hnd, unsigned int timeout) {
    nmmgr_lifetime_t *lifetime;
    int wait_result = 0;
    const uint64_t deadline = timeout ? timer_ms_gettime64() + timeout : 0;

    if(!hnd) {
        errno = EINVAL;
        return -1;
    }

    /* Removal can sleep while retained users drain and can free its sidecar;
       unlike the historical unlink-only operation it is thread-context only. */
    if(irq_inside_int()) {
        errno = EPERM;
        return -1;
    }

    if(mutex_lock(&mutex) < 0)
        return -1;

    lifetime = lifetime_find_locked(hnd);

    if(!lifetime) {
        mutex_unlock(&mutex);
        errno = ENOENT;
        return -1;
    }

    if(lifetime->removing) {
        mutex_unlock(&mutex);
        errno = EBUSY;
        return -1;
    }

    lifetime->removing = true;

    if(lifetime->published) {
        SLIST_REMOVE(&nmmgr_handlers, hnd, nmmgr_handler, list_ent);
        lifetime->published = false;
    }

    while(lifetime->references && wait_result == 0) {
        int wait_timeout = 0;
        if(timeout) {
            const uint64_t now = timer_ms_gettime64();
            if(now >= deadline) {
                errno = ETIMEDOUT;
                wait_result = -1;
                break;
            }
            const uint64_t remaining = deadline - now;
            wait_timeout = remaining > INT_MAX ? INT_MAX : (int)remaining;
        }
        wait_result = cond_wait_timed(&lifetime->idle, &mutex,
                                      wait_timeout);
        /* A chunked wait may expire before an unsigned timeout does. */
        if(wait_result < 0 && errno == ETIMEDOUT && timeout &&
           timer_ms_gettime64() < deadline)
            wait_result = 0;
    }

    if(wait_result < 0 && lifetime->references) {
        lifetime->removing = false;
        mutex_unlock(&mutex);
        return -1;
    }

    SLIST_REMOVE(&lifetimes, lifetime, nmmgr_lifetime, list_entry);
    mutex_unlock(&mutex);
    cond_destroy(&lifetime->idle);
    free(lifetime);

    return 0;
}

/* Preserve the original unbounded removal contract while making completion
   mean that no retained user can still call through the handler. */
int nmmgr_handler_remove(nmmgr_handler_t *hnd) {
    return nmmgr_handler_remove_timed(hnd, 0);
}

int nmmgr_handler_get_path(size_t index, uint32_t type,
                           uint32_t required_flags,
                           uint32_t excluded_flags, char *path,
                           size_t path_size) {
    nmmgr_handler_t *handler;
    bool too_long = false;
    int rv = -1;

    if(!path || path_size == 0) {
        errno = EINVAL;
        return -1;
    }

    path[0] = '\0';

    if(mutex_lock(&mutex) < 0)
        return -1;

    SLIST_FOREACH(handler, &nmmgr_handlers, list_ent) {
        const bool type_matches = type == NMMGR_TYPE_UNKNOWN ||
                                  handler->type == type;
        const bool flags_match =
            (handler->flags & required_flags) == required_flags &&
            !(handler->flags & excluded_flags);

        if(!type_matches || !flags_match)
            continue;

        if(index--)
            continue;

        if(strlen(handler->pathname) >= path_size) {
            too_long = true;
            break;
        }

        strcpy(path, handler->pathname);
        rv = 0;
        break;
    }

    if(rv < 0)
        errno = too_long ? ENAMETOOLONG : ENOENT;

    mutex_unlock(&mutex);
    return rv;
}

KOS_INIT_FLAG_WEAK(export_init, false);

/* Initialize structures */
void nmmgr_init(void) {
    /* Start with no handlers */
    SLIST_INIT(&nmmgr_handlers);
    SLIST_INIT(&lifetimes);

    /* Initialize our internal exports */
    KOS_INIT_FLAG_CALL(export_init);
}

void nmmgr_shutdown(void) {
    for(;;) {
        nmmgr_handler_t *handler;
        bool needs_free;

        mutex_lock(&mutex);
        handler = SLIST_FIRST(&lifetimes) ?
                  SLIST_FIRST(&lifetimes)->handler : NULL;
        needs_free = handler && (handler->flags & NMMGR_FLAGS_NEEDSFREE);
        mutex_unlock(&mutex);

        if(!handler)
            break;

        if(nmmgr_handler_remove(handler) < 0)
            break;

        if(needs_free)
            free(handler);
    }
}
