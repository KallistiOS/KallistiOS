/* KallistiOS ##version##

   lightgun.c
   Copyright (C) 2015 Lawrence Sebald
*/

#include <assert.h>
#include <kos/dbglog.h>
#include <kos/genwait.h>
#include <dc/maple.h>
#include <dc/maple/lightgun.h>

/* Device Driver Struct */
static maple_driver_t lightgun_drv = {
    .functions = MAPLE_FUNC_LIGHTGUN,
    .name = "Lightgun",
    .periodic = NULL,
    .attach = NULL,
    .detach = NULL
};

/* Add the lightgun to the driver chain */
void lightgun_init(void) {
    maple_driver_reg(&lightgun_drv);
}

void lightgun_shutdown(void) {
    maple_driver_unreg(&lightgun_drv);
}
