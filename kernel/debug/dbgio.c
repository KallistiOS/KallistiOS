/* KallistiOS ##version##

   kernel/debug/dbgio.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2026 Eric Fradella
*/

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <kos/dbgio.h>
#include <kos/spinlock.h>

/*
  This module handles a swappable debug console. These functions used to be
  platform specific and define the most common interface, but on the DC for
  example, there are several valid choices, so something more generic is
  called for.

  See the dbgio.h header for more info on exactly how this works.
*/

/* A singly linked list of dbgio handlers */
SLIST_HEAD(dbgio_handlers_list, dbgio_handler);
struct dbgio_handlers_list dbgio_handlers;

/* Our currently selected handler. */
static dbgio_handler_t *dbgio = NULL;

int dbgio_dev_select(const char * name) {
    dbgio_handler_t *cur;

    SLIST_FOREACH(cur, &dbgio_handlers, entry) {
        if(!strcmp(cur->name, name)) {
            /* Try to initialize the device, and if we can't then bail. */
            if(cur->init()) {
                errno = ENODEV;
                return -1;
            }

            dbgio = cur;
            return 0;
        }
    }

    errno = ENODEV;
    return -1;
}

const char *dbgio_dev_get(void) {
    if(!dbgio)
        return NULL;
    else
        return dbgio->name;
}

static int dbgio_enabled = 0;
void dbgio_enable(void) {
    dbgio_enabled = 1;
}
void dbgio_disable(void) {
    dbgio_enabled = 0;
}

int dbgio_add_handler(dbgio_handler_t *handler) {
    SLIST_INSERT_HEAD(&dbgio_handlers, handler, entry);
    return 0;
}

int dbgio_remove_handler(dbgio_handler_t *handler) {
    SLIST_REMOVE(&dbgio_handlers, handler, dbgio_handler, entry);

    if(dbgio == handler) {
        dbgio_dev_select_auto();
    }

    return 0;
}

int dbgio_dev_select_auto(void) {
    dbgio_handler_t *cur;

    /* Look for a valid interface. */
    SLIST_FOREACH(cur, &dbgio_handlers, entry) {
        if(cur->detected()) {
            /* Select this device. */
            dbgio = cur;

            /* Try to init it. If it fails,
               then move on to the next one anyway. */
            if(!dbgio->init()) {
                /* Worked. */
                return 0;
            }

            /* Failed... nuke it and continue. */
            dbgio = NULL;
        }
    }

    /* Didn't find an interface. */
    errno = ENODEV;
    return -1;
}

/* Override with a non-weak symbol if you want to
   add or adjust your own debug I/O handler code */
int __weak_symbol dbgio_init(void) {
    dbgio_dev_select_auto();
    dbgio_enable();

    return 0;
}

int dbgio_set_irq_usage(int mode) {
    if(dbgio_enabled) {
        assert(dbgio);
        return dbgio->set_irq_usage(mode);
    }

    return -1;
}

int dbgio_read(void) {
    if(dbgio_enabled) {
        assert(dbgio);
        return dbgio->read();
    }

    return -1;
}

int dbgio_write(int c) {
    if(dbgio_enabled) {
        assert(dbgio);
        return dbgio->write(c);
    }

    return -1;
}

int dbgio_flush(void) {
    if(dbgio_enabled) {
        assert(dbgio);
        return dbgio->flush();
    }

    return -1;
}

int dbgio_write_buffer(const uint8_t *data, int len) {
    if(dbgio_enabled) {
        assert(dbgio);
        return dbgio->write_buffer(data, len, 0);
    }

    return -1;
}

int dbgio_read_buffer(uint8_t *data, int len) {
    if(dbgio_enabled) {
        assert(dbgio);
        return dbgio->read_buffer(data, len);
    }

    return -1;
}

int dbgio_write_buffer_xlat(const uint8_t *data, int len) {
    if(dbgio_enabled) {
        assert(dbgio);
        return dbgio->write_buffer(data, len, 1);
    }

    return -1;
}

int dbgio_write_str(const char *str) {
    if(dbgio_enabled) {
        assert(dbgio);
        return dbgio_write_buffer_xlat((const uint8_t *)str, strlen(str));
    }

    return -1;
}

/* Not re-entrant */
static char printf_buf[1024];
static spinlock_t lock = SPINLOCK_INITIALIZER;

int dbgio_printf(const char *fmt, ...) {
    va_list args;
    int i;

    /* XXX This isn't correct. We could be inside an int with IRQs
      enabled, and we could be outside an int with IRQs disabled, which
      would cause a deadlock here. We need an irq_is_enabled()! */
    if(!irq_inside_int())
        spinlock_lock(&lock);

    va_start(args, fmt);
    i = vsnprintf(printf_buf, sizeof(printf_buf), fmt, args);
    va_end(args);

    if(i >= 0)
        dbgio_write_str(printf_buf);

    if(!irq_inside_int())
        spinlock_unlock(&lock);

    return i;
}


/* The null dbgio handler */
static int null_detected(void) {
    return 1;
}
static int null_init(void) {
    return 0;
}
static int null_shutdown(void) {
    return 0;
}
static int null_set_irq_usage(int mode) {
    (void)mode;
    return 0;
}
static int null_read(void) {
    errno = EAGAIN;
    return -1;
}
static int null_write(int c) {
    (void)c;
    return 1;
}
static int null_flush(void) {
    return 0;
}
static int null_write_buffer(const uint8_t *data, int len, int xlat) {
    (void)data;
    (void)len;
    (void)xlat;
    return len;
}
static int null_read_buffer(uint8_t *data, int len) {
    (void)data;
    (void)len;
    errno = EAGAIN;
    return -1;
}

dbgio_handler_t dbgio_null = {
    .name = "null",
    .detected = null_detected,
    .init = null_init,
    .shutdown = null_shutdown,
    .set_irq_usage = null_set_irq_usage,
    .read = null_read,
    .write = null_write,
    .flush = null_flush,
    .write_buffer = null_write_buffer,
    .read_buffer = null_read_buffer
};
