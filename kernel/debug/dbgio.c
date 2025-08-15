/* KallistiOS ##version##

   kernel/debug/dbgio.c
   Copyright (C) 2004 Megan Potter
*/

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <kos/dbgio.h>
#include <arch/spinlock.h>

/*
  This module handles a swappable debug console. These functions used to be
  platform specific and define the most common interface, but on the DC for
  example, there are several valid choices, so something more generic is
  called for.

  See the dbgio.h header for more info on exactly how this works.
*/

// Our currently selected handler.
static dbgio_handler_t * dbgio = NULL;

int dbgio_dev_select(const char * name) {
    size_t i;

    for(i = 0; i < dbgio_handler_cnt; i++) {
        if(!strcmp(dbgio_handlers[i]->name, name)) {
            /* Try to initialize the device, and if we can't then bail. */
            if(dbgio_handlers[i]->init()) {
                errno = ENODEV;
                return -1;
            }

            dbgio = dbgio_handlers[i];
            return 0;
        }
    }

    errno = ENODEV;
    return -1;
}

const char * dbgio_dev_get(void) {
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
        if(dbgio_handlers[i]->detected()) {
            // Select this device.
            dbgio = dbgio_handlers[i];

            // Try to init it. If it fails, then move on to the
            // next one anyway.
            if(!dbgio->init()) {
                // Worked.
                dbgio_enable();
                return 0;
            }

            // Failed... nuke it and continue.
            dbgio = NULL;
        }
    }

    // Didn't find an interface.
    errno = ENODEV;
    return -1;
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

// Not re-entrant
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


// The null dbgio handler
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
static int null_read_buffer(uint8_t * data, int len) {
    (void)data;
    (void)len;
    errno = EAGAIN;
    return -1;
}

dbgio_handler_t dbgio_null = {
    "null",
    null_detected,
    null_init,
    null_shutdown,
    null_set_irq_usage,
    null_read,
    null_write,
    null_flush,
    null_write_buffer,
    null_read_buffer
};
