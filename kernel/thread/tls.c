/* KallistiOS ##version##

   kernel/thread/tls.c
   Copyright (C) 2009 Lawrence Sebald
*/

/* This file defines methods for accessing thread-local storage, added in KOS
   1.3.0. */

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>

#include <kos/tls.h>
#include <kos/thread.h>
#include <arch/irq.h>
#include <arch/spinlock.h>

static spinlock_t mutex = SPINLOCK_INITIALIZER;
static kthread_key_t next_key = 1;

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

/* What is the next key that will be given out? */
kthread_key_t kthread_key_next(void) {
    return next_key;
}

typedef void (*destructor)(void *);

/* Get the destructor for a given key. */
static destructor kthread_key_get_destructor(kthread_key_t key) {
    kthread_tls_dest_t *i;

    LIST_FOREACH(i, &dest_list, dest_list) {
        if(i->key == key) {
            return i->destructor;
        }
    }

    return NULL;
}

/* Delete the destructor for a given key. */
void kthread_key_delete_destructor(kthread_key_t key) {
    kthread_tls_dest_t *i, *tmp;

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

    if(irq_inside_int() &&
       (spinlock_is_locked(&mutex) || !malloc_irq_safe())) {
        errno = EPERM;
        return -1;
    }

    spinlock_lock_scoped(&mutex);

    /* Store the destructor if need be. */
    if(destructor) {
        dest = (kthread_tls_dest_t *)malloc(sizeof(kthread_tls_dest_t));

        if(!dest) {
            errno = ENOMEM;
            return -1;
        }

        dest->key = next_key;
        dest->destructor = destructor;
        LIST_INSERT_HEAD(&dest_list, dest, dest_list);
    }

    *key = next_key++;

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

    if(irq_inside_int() && spinlock_is_locked(&mutex)) {
        errno = EPERM;
        return -1;
    }

    /* Make sure the key is valid. */
    {
        spinlock_lock_scoped(&mutex);

        if(key >= next_key || key < 1) {
            errno = EINVAL;
            return -1;
        }
    }

    /* Check if we already have an entry for this key. */
    LIST_FOREACH(i, &cur->tls_list, kv_list) {
        if(i->key == key) {
            i->data = (void *)value;
            return 0;
        }
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

    return 0;
}

void kthread_tls_shutdown(void) {
    kthread_tls_dest_t *n1, *n2;

    /* Tear down the destructor list. */
    n1 = LIST_FIRST(&dest_list);

    while(n1 != NULL) {
        n2 = LIST_NEXT(n1, dest_list);
        free(n1);
        n1 = n2;
    }
}
