/* KallistiOS ##version##

   kernel/debug/dbgio.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2024 Falco Girgis
*/

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <kos/dbgio.h>

/*
  This module handles a swappable debug console. These functions used to be
  platform specific and define the most common interface, but on the DC for
  example, there are several valid choices, so something more generic is
  called for.

  See the dbgio.h header for more info on exactly how this works.
*/

extern dbgio_handler_t *dbgio_handlers[];
extern const size_t dbgio_handler_cnt;

/* Our currently selected handler (main device) */
static dbgio_handler_t *dbgio = NULL;
/* Our optional auxiliar handler */
static dbgio_handler_t *dbgio_aux = NULL;
/* List of all registered handlers */
static struct dbgio_list dbgio_registry = LIST_HEAD_INITIALIZER(dbgio_registry);
/* Whether we're currently enabling or disabling dbgio. */
static bool dbgio_enabled = true;

int dbgio_register(dbgio_handler_t *handler) {
    assert(!dbgio_find(handler->name));

    LIST_INSERT_HEAD(&dbgio_registry, handler, registry);

    return 0;
}

int dbgio_unregister(dbgio_handler_t *handler) {
    LIST_REMOVE(handler, registry);

    return 0;
}

dbgio_handler_t *dbgio_find(const char *name) {
    dbgio_handler_t *handler;

    LIST_FOREACH(handler, &dbgio_registry, registry) {
        if(!strcmp(handler->name, name))
            return handler;
    }

    return NULL;
}

int dbgio_dev_select(const char *name) {
    dbgio_handler_t *dev = dbgio_find(name);
    
    if(!dev || dev->init()) {
        errno = ENODEV;
        return -1;
    }

    dbgio = dev;

    return 0;
}

int dbgio_aux_select(const char *name) {
    dbgio_handler_t *aux = NULL;

    if(name) {
        aux = dbgio_find(name);
        if(!aux || aux->init()) {
            errno = ENODEV;
            return -1;
        }
    }

    dbgio_aux = aux;

    return 0;
}

const char *dbgio_dev_get(void) {
    return dbgio? dbgio->name : NULL;
}

const char *dbgio_aux_get(void) {
    return dbgio_aux? dbgio_aux->name : NULL;
}

void dbgio_enable(void) {
    dbgio_enabled = true;
}

void dbgio_disable(void) {
    dbgio_enabled = false;
}

int dbgio_init(void) {
    dbgio_handler_t *handler;

    for(int i = dbgio_handler_cnt; i >= 0; --i)
        dbgio_register(dbgio_handlers[i]);

    LIST_FOREACH(handler, &dbgio_registry, registry) {
        if(handler->detected() && !handler->init()) {
            dbgio = handler;
            return 0;
        }
    }

    errno = ENODEV;
    return -1;
}

int dbgio_set_irq_usage(dbgio_mode_t mode) {
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

int dbgio_write_buffer(const uint8_t *data, size_t len) {
    if(dbgio_enabled) {
        assert(dbgio);
        const int ret = dbgio->write_buffer(data, len, false);

        if(dbgio_aux)
            dbgio_aux->write_buffer(data, len, false);

        return ret;
    }

    return -1;
}

int dbgio_read_buffer(uint8_t *data, size_t len) {
    if(dbgio_enabled) {
        assert(dbgio);
        return dbgio->read_buffer(data, len);
    }

    return -1;
}

int dbgio_write_buffer_xlat(const uint8_t *data, size_t len) {
    if(dbgio_enabled) {
        assert(dbgio);
        const int ret = dbgio->write_buffer(data, len, true);
    
        if(dbgio_aux) 
            dbgio_aux->write_buffer(data, len, true);

        return ret;
    }

    return -1;
}

int dbgio_write_str(const char *str) {
    if(dbgio_enabled) {
        assert(dbgio);
        return dbgio_write_buffer_xlat((const uint8*)str, strlen(str));
    }

    return -1;
}

int dbgio_printf(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    const int ret = dbgio_vprintf(fmt, args);
    va_end(args);

    return ret;
}

int dbgio_vprintf(const char *fmt, va_list var_args) {
    char buffer[1024];
    
    const int i = vsnprintf(buffer, sizeof(buffer), fmt, var_args);

    dbgio_write_str(buffer);

    return i;
}

/* The null dbgio handler */
static bool null_detected(void) {
    return true;
}

static int null_init(void) {
    return 0;
}

static int null_shutdown(void) {
    return 0;
}

static int null_set_irq_usage(dbgio_mode_t mode) {
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

static int null_write_buffer(const uint8_t *data, size_t len, bool xlat) {
    (void)data;
    (void)len;
    (void)xlat;
    return len;
}

static int null_read_buffer(uint8_t *data, size_t len) {
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
    null_read_buffer,
    { NULL }
};
