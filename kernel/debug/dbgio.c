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
#include <kos/irq.h>
#include <kos/mutex.h>

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

int dbgio_dev_select(const char *name) {
    dbgio_handler_t *cur;

    SLIST_FOREACH(cur, &dbgio_handlers, entry) {
        if(!strcmp(cur->name, name)) {
            /* If it has init, try to and kick on failure. */
            if(!dbgio->output && cur->init && cur->init()) {
                errno = ENODEV;
                return -1;
            }

            /* If it won't be used, and has a shutdown, do so */
            if(!dbgio->output && dbgio->shutdown)
                dbgio->shutdown();

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

int dbgio_dev_output(const char *name, bool output) {
    dbgio_handler_t *cur;

    SLIST_FOREACH(cur, &dbgio_handlers, entry) {
        if(!strcmp(cur->name, name)) {
            /* Nothing to do */
            if(output == cur->output)
                return 0;

            /* Already set as primary so just pass through */
            if(cur == dbgio) {
                cur->output = output;
                return 0;
            }

            /* Enable the outputter */
            if(output) {
                /* If it has init, and can't, fail. */
                if(cur->init && cur->init()) {
                    errno = ENODEV;
                    return -1;
                }
                cur->output = true;

                return 0;
            }
            /* Disable the outputter */
            else {
                cur->output = false;

                if(cur->shutdown)
                    cur->shutdown();

                return 0;
            }
        }
    }

    errno = ENODEV;
    return -1;
}

static int dbgio_enabled = 0;
void dbgio_enable(void) {
    dbgio_enabled = 1;
}
void dbgio_disable(void) {
    dbgio_enabled = 0;
}

int dbgio_add_handler(dbgio_handler_t *handler) {
    int rv = 0;
    /* The handler's set by default to always output */
    if(handler->output)
        if(handler->init)
            rv = handler->init();

    SLIST_INSERT_HEAD(&dbgio_handlers, handler, entry);
    return rv;
}

int dbgio_remove_handler(dbgio_handler_t *handler) {
    dbgio_handler_t *t;

    SLIST_FOREACH(t, &dbgio_handlers, entry) {
        if(t == handler) {

            SLIST_REMOVE(&dbgio_handlers, handler, dbgio_handler, entry);
            if(dbgio == handler) {
                /* Stop forced output, if set */
                handler->output = false;
                dbgio_dev_select_auto();
            }
            else {
                /* If we're outputting clean up */
                if(handler->output) {
                    handler->output = false;
                    if(handler->shutdown)
                        handler->shutdown();
                }
            }
            return 0;
        }
    }

    /* Handler hadn't been added */
    return -1;
}

int dbgio_dev_select_auto(void) {
    dbgio_handler_t *cur;

    /* Look for a valid interface. */
    SLIST_FOREACH(cur, &dbgio_handlers, entry) {
        if(cur->detected && cur->detected()) {
            /* If inittable, try to init it. If it fails,
               then move on to the next one anyway. */
            if(cur->output || !cur->init || !cur->init()) {
                /* If it has a shutdown, do so */
                if(!dbgio->output && dbgio->shutdown)
                    dbgio->shutdown();

                /* Worked, so assign it */
                dbgio = cur;
                return 0;
            }
        }
    }

    /* Didn't find an interface. */
    dbgio = NULL;
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
        if(dbgio && dbgio->set_irq_usage)
            return dbgio->set_irq_usage(mode);
    }

    return -1;
}

int dbgio_read(void) {
    if(dbgio_enabled) {
        if(dbgio && dbgio->read)
            return dbgio->read();
    }

    return -1;
}

int dbgio_write(int c) {
    if(dbgio_enabled) {
        dbgio_handler_t *cur;

        /* Output on all outputters. */
        SLIST_FOREACH(cur, &dbgio_handlers, entry) {
            if((cur != dbgio) && cur->write)
                cur->write(c);
        }

        if(dbgio && dbgio->write)
            return dbgio->write(c);
    }

    return -1;
}

int dbgio_flush(void) {
    if(dbgio_enabled) {
        dbgio_handler_t *cur;

        /* Flush on all outputters. */
        SLIST_FOREACH(cur, &dbgio_handlers, entry) {
            if((cur != dbgio) && cur->flush)
                cur->flush();
        }

        if(dbgio && dbgio->flush)
            return dbgio->flush();
    }

    return -1;
}

int dbgio_write_buffer(const uint8_t *data, int len) {
    if(dbgio_enabled) {
        dbgio_handler_t *cur;

        /* Output on all outputters. */
        SLIST_FOREACH(cur, &dbgio_handlers, entry) {
            if((cur != dbgio) && cur->write_buffer)
                cur->write_buffer(data, len, 0);
        }

        if(dbgio && dbgio->write_buffer)
            return dbgio->write_buffer(data, len, 0);
    }

    return -1;
}

int dbgio_read_buffer(uint8_t *data, int len) {
    if(dbgio_enabled) {
        if(dbgio && dbgio->read_buffer)
            return dbgio->read_buffer(data, len);
    }

    return -1;
}

int dbgio_write_buffer_xlat(const uint8_t *data, int len) {
    if(dbgio_enabled) {
        if(dbgio && dbgio->write_buffer)
            return dbgio->write_buffer(data, len, 1);
    }

    return -1;
}

int dbgio_write_str(const char *str) {
    if(dbgio_enabled) {
        if(dbgio && dbgio->write_buffer)
            return dbgio_write_buffer_xlat((const uint8_t *)str, strlen(str));
    }

    return -1;
}

/* Not re-entrant */
static char printf_buf[1024];
static mutex_t lock = MUTEX_INITIALIZER;

int dbgio_printf(const char *fmt, ...) {
    va_list args;
    int i;

    /* XXX This isn't correct. We could be inside an int with IRQs
      enabled, and we could be outside an int with IRQs disabled, which
      would cause a deadlock here. We need an irq_is_enabled()! */
    if(!irq_inside_int())
        mutex_lock(&lock);

    va_start(args, fmt);
    i = vsnprintf(printf_buf, sizeof(printf_buf), fmt, args);
    va_end(args);

    if(i >= 0)
        dbgio_write_str(printf_buf);

    if(!irq_inside_int())
        mutex_unlock(&lock);

    return i;
}
