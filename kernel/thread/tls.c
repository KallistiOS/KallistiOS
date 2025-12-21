/* KallistiOS ##version##

   kernel/thread/tls.c
   Copyright (C) 2009 Lawrence Sebald
*/

/* This file defines methods for accessing thread-local storage, added in KOS
   1.3.0. */

#include <stdatomic.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>

#include <kos/irq.h>
#include <kos/tls.h>
#include <kos/spinlock.h>
#include <kos/thread.h>
#include <kos/mutex.h>

static _Atomic kthread_key_t next_key = 1;

typedef struct kthread_tls_dest {
    /* List handle */
    LIST_ENTRY(kthread_tls_dest) dest_list;

    /* The key */
    kthread_key_t key;

    /* Destructor for the key */
    void (*destructor)(void *);
} kthread_tls_dest_t;

LIST_HEAD(kthread_tls_dest_list, kthread_tls_dest);

static struct kthread_tls_dest_list dest_list;
static mutex_t dlist_mtx;

/* What is the next key that will be given out? */
kthread_key_t kthread_key_next(void) {
    return next_key;
}

typedef void (*destructor)(void *);

/* Get the destructor for a given key. */
static destructor kthread_key_get_destructor(kthread_key_t key) {
    kthread_tls_dest_t *i;

    mutex_lock_scoped(&dlist_mtx);

    LIST_FOREACH(i, &dest_list, dest_list) {
        if(i->key == key) {
            return i->destructor;
        }
    }

    return NULL;
}

/* Delete the destructor for a given key. */
static void kthread_key_delete_destructor(kthread_key_t key) {
    kthread_tls_dest_t *i, *tmp;

    mutex_lock_scoped(&dlist_mtx);

    LIST_FOREACH_SAFE(i, &dest_list, dest_list, tmp) {
        if(i->key == key) {
            LIST_REMOVE(i, dest_list);
            free(i);
            return;
        }
    }
}

/* Create a new TLS key. */
int kthread_key_create(kthread_key_t *key, void (*destructor)(void *)) {
    kthread_tls_dest_t *dest;

    if(irq_inside_int() && destructor &&
        (!malloc_irq_safe() || mutex_is_locked(&dlist_mtx))) {
        errno = EPERM;
        return -1;
    }

    /* Store the destructor if need be. */
    if(destructor) {
        dest = (kthread_tls_dest_t *)malloc(sizeof(kthread_tls_dest_t));

        if(!dest) {
            errno = ENOMEM;
            return -1;
        }
    }

    /* Now that the destructor's ready, we can get our key */
    *key = atomic_fetch_add(&next_key, 1);

    if(destructor) {
        dest->key = *key;
        dest->destructor = destructor;
        mutex_lock_scoped(&dlist_mtx);
        LIST_INSERT_HEAD(&dest_list, dest, dest_list);
    }

    return 0;
}

/* Always returns 0 as we want to iterate over all threads. */
static int key_delete_cb(kthread_t *thd, void *user_data) {
    kthread_tls_kv_t *i, *tmp;
    kthread_key_t key = *(kthread_key_t *)(user_data);

    LIST_FOREACH_SAFE(i, &thd->tls_list, kv_list, tmp) {
            if(i->key == key) {
                LIST_REMOVE(i, kv_list);
                free(i);
                return 0;
            }
        }

    return 0;
}

/* Delete a TLS key. Note that currently this doesn't prevent you from reusing
   the key after deletion. This seems ok, as the pthreads standard states that
   using the key after deletion results in "undefined behavior".
*/
int kthread_key_delete(kthread_key_t key) {

    irq_disable_scoped();

    /* Make sure the key is valid. */
    if(key >= kthread_key_next() || key < 1) {
        errno = EINVAL;
        return -1;
    }

    /* Make sure we can actually delete things below. */
    if(!malloc_irq_safe() || mutex_is_locked(&dlist_mtx)) {
        errno = EPERM;
        return -1;
    }

    /* Go through each thread searching for (and removing) the data. */
    thd_each(key_delete_cb, (void *)&key);

    kthread_key_delete_destructor(key);

    return 0;
}

/* Get the value stored for a given TLS key. Returns NULL if the key is invalid
   or there is no data there for the current thread. */
void *kthread_getspecific(kthread_key_t key) {
    kthread_t *cur = thd_get_current();
    kthread_tls_kv_t *i;

    LIST_FOREACH(i, &cur->tls_list, kv_list) {
        if(i->key == key) {
            return i->data;
        }
    }

    return NULL;
}

/* Set the value for a given TLS key. Returns -1 on failure. errno will be
   EINVAL if the key is not valid, ENOMEM if there is no memory available to
   allocate for storage, or EPERM if run inside an interrupt and the a call is
   in progress already. */
int kthread_setspecific(kthread_key_t key, const void *value) {
    kthread_t *cur = thd_get_current();
    kthread_tls_kv_t *i;

    if(key >= next_key || key < 1) {
        errno = EINVAL;
        return -1;
    }

    /* Check if we already have an entry for this key. */
    LIST_FOREACH(i, &cur->tls_list, kv_list) {
        if(i->key == key) {
            i->data = (void *)value;
            return 0;
        }
    }

    if(irq_inside_int() &&
        (!malloc_irq_safe() || mutex_is_locked(&dlist_mtx))) {
        errno = EPERM;
        return -1;
    }

    /* No entry, create a new one. */
    i = (kthread_tls_kv_t *)malloc(sizeof(kthread_tls_kv_t));

    if(!i) {
        errno = ENOMEM;
        return -1;
    }

    i->key = key;
    i->data = (void *)value;
    i->destructor = kthread_key_get_destructor(key);
    LIST_INSERT_HEAD(&cur->tls_list, i, kv_list);

    return 0;
}

int kthread_tls_init(void) {
    /* Initialize the destructor list. */
    LIST_INIT(&dest_list);

    mutex_init(&dlist_mtx, MUTEX_TYPE_DEFAULT);

    return 0;
}

void kthread_tls_shutdown(void) {
    kthread_tls_dest_t *n1, *n2;

    /* If we can't get it, shut down anyways */
    mutex_lock_irqsafe(&dlist_mtx);

    LIST_FOREACH_SAFE(n1, &dest_list, dest_list, n2) {
        LIST_REMOVE(n1, dest_list);
        free(n1);
    }
}
