/* KallistiOS ##version##

   util/vmu_printf.c
   Copyright (C) 2024 Paul Cercueil
*/

#include <stdarg.h>
#include <stdio.h>

#include <dc/vmu_fb.h>

static vmufb_t vmufb;

void vmu_printf(const char *fmt, ...) {
    maple_device_t *dev;
    unsigned int vmu;
    char buf[256];
    va_list va;

    buf[sizeof(buf) - 1] = '\0';

    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    va_end(va);

    vmufb_clear(&vmufb);
    vmufb_print_string(&vmufb, vmu_get_font(), buf);

    for(vmu = 0; ; vmu++) {
        dev = maple_enum_type(vmu, MAPLE_FUNC_LCD);
        if(!dev)
            break;

        vmufb_present(&vmufb, dev);
    }
}
