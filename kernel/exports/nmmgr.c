/* KallistiOS ##version##

   nmmgr.c
   Copyright (C) 2003 Megan Potter

*/

/*

This module manages "names". A name is a generic identifier that corresponds
to a handler for that name. These names can correspond to services exported
by a module or the kernel, they can be VFS handlers, they can be just about
anything. The only requirement is that they implement the nmmgr_handler_t
interface at the front of their struct.

*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <kos/init_base.h>
#include <kos/nmmgr.h>
#include <kos/mutex.h>
#include <kos/exports.h>

/* Thread mutex for our name handler list */
static mutex_t mutex = MUTEX_INITIALIZER;

/* Name handler structures; these structs contain path/type pairs that
   describe how to handle a given path name. */
static nmmgr_list_t nmmgr_handlers;

/* Locate a name handler for a given path name */
nmmgr_handler_t * nmmgr_lookup(const char *fn) {
    nmmgr_handler_t *cur = NULL, *tmp;
    size_t          cur_len = 0, tmp_len;

    /* Scan the handler table and look for the best path match */
    SLIST_FOREACH(tmp, &nmmgr_handlers, list_ent) {
        tmp_len = strlen(tmp->pathname);
        if(!strncasecmp(tmp->pathname, fn, tmp_len)) {
            if(cur_len < tmp_len) {
                cur_len = tmp_len;
                cur = tmp;
            }
        }
    }

    if(cur == NULL) {
        /* Couldn't find a handler */
        return NULL;
    }
    else {
        /* If we found an alias, return its referent */
        if(cur->flags & NMMGR_FLAGS_ALIAS)
            return ((alias_handler_t*)cur)->alias;

        return cur;
    }
}

nmmgr_list_t * nmmgr_get_list(void) {
    return &nmmgr_handlers;
}

/* Add a name handler */
int nmmgr_handler_add(nmmgr_handler_t *hnd) {
    mutex_lock(&mutex);

    SLIST_INSERT_HEAD(&nmmgr_handlers, hnd, list_ent);

    mutex_unlock(&mutex);

    return 0;
}

/* Remove a name handler */
int nmmgr_handler_remove(nmmgr_handler_t *hnd) {
    nmmgr_handler_t *tmp;
    int rv = -1;

    if(mutex_lock_irqsafe(&mutex) < 0)
        return -1;

    /* Verify that it's actually in there */
    SLIST_FOREACH(tmp, &nmmgr_handlers, list_ent) {
        if(tmp == hnd)
            break;
    }

    if(tmp) {
        SLIST_REMOVE(&nmmgr_handlers, hnd, nmmgr_handler, list_ent);
        rv = 0;
    }

    mutex_unlock(&mutex);

    return rv;
}

KOS_INIT_FLAG_WEAK(export_init, false);

/* Initialize structures */
void nmmgr_init(void) {
    /* Start with no handlers */
    SLIST_INIT(&nmmgr_handlers);

    /* Initialize our internal exports */
    KOS_INIT_FLAG_CALL(export_init);
}

void nmmgr_shutdown(void) {
    nmmgr_handler_t *c, *n;

    c = SLIST_FIRST(&nmmgr_handlers);

    while(c != NULL) {
        n = SLIST_NEXT(c, list_ent);

        if(c->flags & NMMGR_FLAGS_NEEDSFREE)
            free(c);

        c = n;
    }
}
