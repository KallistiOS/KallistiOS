/* KallistiOS ##version##

   conio_dbgio.c
   Copyright (C) 2026 Eric Fradella
*/

#include <stdlib.h>
#include <kos/dbgio.h>
#include <kos/dbglog.h>
#include <kos/thread.h>
#include <dc/pvr.h>
#include <conio/conio.h>
#include <errno.h>

static int conio_dbgio_detected(void) {
    return conio_ttymode != CONIO_TTY_NONE;
}
static int conio_dbgio_init(void) {
    return 0;
}
static int conio_dbgio_shutdown(void) {
    return 0;
}
static int conio_dbgio_set_irq_usage(int mode) {
    (void)mode;
    return 0;
}
static int conio_dbgio_read(void) {
    errno = EAGAIN;
    return -1;
}
static int conio_dbgio_write(int c) {
    conio_putch(c);
    return 1;
}
static int conio_dbgio_flush(void) {
    return 0;
}
static int conio_dbgio_write_buffer(const uint8_t *data, int len, int xlat) {
    (void)len;
    (void)xlat;
    conio_putstr((char *)data);
    return len;
}
static int conio_dbgio_read_buffer(uint8_t * data, int len) {
    (void)data;
    (void)len;
    errno = EAGAIN;
    return -1;
}

dbgio_handler_t dbgio_conio = {
    .name = "conio",
    .detected = conio_dbgio_detected,
    .init = conio_dbgio_init,
    .shutdown = conio_dbgio_shutdown,
    .set_irq_usage = conio_dbgio_set_irq_usage,
    .read = conio_dbgio_read,
    .write = conio_dbgio_write,
    .flush = conio_dbgio_flush,
    .write_buffer = conio_dbgio_write_buffer,
    .read_buffer = conio_dbgio_read_buffer
};

/* The main program */
int main(int argc, char **argv) {
    /* Setup the console */
    pvr_init_defaults();
    conio_init(CONIO_TTY_PVR, CONIO_INPUT_LINE);
    conio_set_theme(CONIO_THEME_MATRIX);

    dbglog(DBG_INFO, "Let's add a conio dbgio inteface...\n");

    dbgio_add_handler(&dbgio_conio);
    dbgio_dev_select("conio");

    dbglog(DBG_INFO, kos_get_banner());
    dbglog(DBG_INFO, "This is KOS dbglog() output via conio!\n");

    dbgio_remove_handler(&dbgio_conio);

    dbglog(DBG_INFO, "conio dbgio has now been removed.\n");
    dbglog(DBG_INFO, "This is KOS dbglog() output via the first valid dbgio interface!\n");

    thd_sleep(5000);

    return 0;
}
