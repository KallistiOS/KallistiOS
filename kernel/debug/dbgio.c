/* KallistiOS ##version##

   kernel/debug/dbgio.c
   Copyright (C) 2004 Megan Potter
*/

#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <kos/dbgio.h>
#include <kos/mutex.h>

/*
  This module handles a swappable debug console. These functions used to be
  platform specific and define the most common interface, but on the DC for
  example, there are several valid choices, so something more generic is
  called for.

  See the dbgio.h header for more info on exactly how this works.
*/

// Our currently selected handler.
static dbgio_handler_t *dbgio = NULL;

static bool dbgio_dev_assign(dbgio_handler_t *d) {

    if(d && d->init())
        return false;

    dbgio = d;
    return true;
}

int dbgio_dev_select(const char *name) {
    size_t i;

    for(i = 0; i < dbgio_handler_cnt; i++) {
        if(!strcmp(dbgio_handlers[i]->name, name)) {
            /* Try to assign the device, and if we can't then bail. */
            if(!dbgio_dev_assign(dbgio_handlers[i])) {
                errno = ENODEV;
                return -1;
            }

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

int dbgio_init(void) {
    size_t i;

    // Look for a valid interface.
    for(i = 0; i < dbgio_handler_cnt; i++) {
        /* Stop looking once we hit an empty slot */
        if(!dbgio_handlers[i]) break;

        if(dbgio_handlers[i]->detected()) {

            // Try to assign it. If it fails, then move on to the
            // next one anyway.
            if(!dbgio_dev_assign(dbgio_handlers[i])) {
                // Worked.
                dbgio_enable();
                return 0;
            }
        }
    }

    // Didn't find an interface.
    errno = ENODEV;
    return -1;
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
        if(dbgio && dbgio->write)
            return dbgio->write(c);
    }

    return -1;
}

int dbgio_flush(void) {
    if(dbgio_enabled) {
        if(dbgio && dbgio->flush)
            return dbgio->flush();
    }

    return -1;
}

int dbgio_write_buffer(const uint8_t *data, int len) {
    if(dbgio_enabled) {
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
        assert(dbgio);
        return dbgio_write_buffer_xlat((const uint8_t *)str, strlen(str));
    }

    return -1;
}

// Not re-entrant
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
