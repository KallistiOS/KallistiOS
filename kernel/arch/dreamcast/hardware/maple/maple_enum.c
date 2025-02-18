/* KallistiOS ##version##

   maple_enum.c
   (c)2002 Megan Potter
   (c)2008 Lawrence Sebald
 */

#include <dc/maple.h>
#include <kos/thread.h>

/* Return the number of connected devices */
int maple_enum_count(void) {
    int p, u, cnt;

    for(cnt = 0, p = 0; p < MAPLE_PORT_COUNT; p++)
        for(u = 0; u < MAPLE_UNIT_COUNT; u++) {
            if(maple_dev_valid(p,u))
                cnt++;
        }

    return cnt;
}

/* Return a raw device info struct for the given device */
maple_device_t * maple_enum_dev(int p, int u) {
    maple_device_t *rv = maple_state.ports[p].units[u];
    if(rv && rv->valid)
        return rv;
    return NULL;
}

/* Return the Nth device of the requested type (where N is zero-indexed) */
maple_device_t * maple_enum_type(int n, uint32 func) {
    int p, u;
    maple_device_t *dev;

    for(p = 0; p < MAPLE_PORT_COUNT; p++) {
        for(u = 0; u < MAPLE_UNIT_COUNT; u++) {
            dev = maple_enum_dev(p, u);

            if(dev != NULL && dev->info.functions & func) {
                if(!n) return dev;

                n--;
            }
        }
    }

    return NULL;
}

/* Return the Nth device that is of the requested type and supports the list of
   capabilities given. */
maple_device_t * maple_enum_type_ex(int n, uint32 func, uint32 cap) {
    int p, u, d;
    maple_device_t *dev;
    uint32 f, tmp;

    for(p = 0; p < MAPLE_PORT_COUNT; ++p) {
        for(u = 0; u < MAPLE_UNIT_COUNT; ++u) {
            dev = maple_enum_dev(p, u);

            /* If the device supports the function code we passed in, check
               if it supports the capabilities that the user requested. */
            if(dev != NULL && (dev->info.functions & func)) {
                f = dev->info.functions;
                d = 0;
                tmp = func;

                /* Figure out which function data we want to look at. Function
                   data entries are arranged by the function code, most
                   significant bit first. This is really not pretty, and is
                   rather inefficient, but its the best I could think of off the
                   top of my head (i.e, replace me later). */
                while(tmp != 0x80000000) {
                    if(f & 0x80000000) {
                        ++d;
                    }

                    f <<= 1;
                    tmp <<= 1;
                }

                /* Check if the function data for the function type checks out
                   with what it should be. */
                cap = ((cap >> 24) & 0xFF) | ((cap >> 8) & 0xFF00) |
                      ((cap & 0xFF00) << 8) | ((cap & 0xFF) << 24);

                if((dev->info.function_data[d] & cap) == cap) {
                    if(!n)
                        return dev;

                    --n;
                }
            }
        }
    }

    return NULL;
}

/* Get the status struct for the requested maple device; wait until it's
   valid before returning. Cast to the appropriate type you're expecting. */
void * maple_dev_status(maple_device_t *dev) {
    /* The device must be valid, and must get periodic updates. */
    if(!dev || !dev->valid || !dev->drv || !dev->drv->periodic)
        return NULL;

    /* Waits until the first DMA happens: crude but effective (replace me later) */
    while(!dev->status_valid)
        thd_pass();

    /* Cast and return the status buffer */
    return dev->status;
}

