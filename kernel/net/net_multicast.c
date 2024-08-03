/* KallistiOS ##version##

   kernel/net/net_multicast.c
   Copyright (C) 2010, 2013 Lawrence Sebald

*/

/* This file deals with setting up multicasting support on the ethernet level.
   Basically, this is just a set of convenience functions around the if_set_mc
   function that got added to the netif_t structure. */

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <kos/net.h>
#include <kos/mutex.h>
#include <arch/irq.h>

typedef struct mc_entry {
    LIST_ENTRY(mc_entry)    entry;
    uint8                   mac[6];
} mc_entry_t;

LIST_HEAD(mc_list, mc_entry);
static struct mc_list multicasts = LIST_HEAD_INITIALIZER(0);
static int mc_count = 0;
static mutex_t mc_mutex = MUTEX_INITIALIZER;

static void multicast_commit(void) {
    mc_entry_t *i;
    int tmp = 0;
    uint8 macs[mc_count * 6];

    if(!net_default_dev) {
        return;
    }

    /* Add each one into the list */
    LIST_FOREACH(i, &multicasts, entry) {
        memcpy(&macs[6 * tmp++], i->mac, 6);
    }

    /* Commit the final result to the card */
    net_default_dev->if_set_mc(net_default_dev, macs, tmp);
}

int net_multicast_add(const uint8 mac[6]) {
    mc_entry_t *ent;

    ent = (mc_entry_t *)malloc(sizeof(mc_entry_t));

    if(!ent) {
        return -1;
    }

    memcpy(ent->mac, mac, 6);

    if(mutex_lock_irqsafe(&mc_mutex)) {
        free(ent);
        return -1;
    }

    LIST_INSERT_HEAD(&multicasts, ent, entry);
    ++mc_count;

    multicast_commit();
    mutex_unlock(&mc_mutex);

    return 0;
}

int net_multicast_del(const uint8 mac[6]) {
    mc_entry_t *i, *tmp;

    if(mutex_lock_irqsafe(&mc_mutex))
        return -1;

    /* Look for the one in question */
    i = LIST_FIRST(&multicasts);

    while(i) {
        tmp = LIST_NEXT(i, entry);

        if(!memcmp(mac, i->mac, 6)) {
            LIST_REMOVE(i, entry);
            free(i);
            --mc_count;
        }

        i = tmp;
    }

    multicast_commit();

    mutex_unlock(&mc_mutex);

    return 0;
}

int net_multicast_check(const uint8 mac[6]) {
    mc_entry_t *i;
    int rv = 0;

    if(mutex_lock_irqsafe(&mc_mutex))
        return -1;

    /* Look for the one in question */
    LIST_FOREACH(i, &multicasts, entry) {
        if(!memcmp(mac, i->mac, 6)) {
            rv = 1;
            break;
        }
    }

    mutex_unlock(&mc_mutex);
    return rv;
}

int net_multicast_init(void) {
    return 0;
}

void net_multicast_shutdown(void) {
    mc_entry_t *i, *tmp;

    /* Free all entries */
    i = LIST_FIRST(&multicasts);

    while(i) {
        tmp = LIST_NEXT(i, entry);
        free(i);
        i = tmp;
    }

    LIST_INIT(&multicasts);
    mc_count = 0;

    /* Clear the device's multicast list */
    if(net_default_dev) {
        net_default_dev->if_set_mc(net_default_dev, NULL, 0);
    }
}
