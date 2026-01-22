/* KallistiOS ##version##

   maple_enum.c
   (c)2002 Megan Potter
   (c)2008 Lawrence Sebald
 */

#include <dc/maple.h>
#include <kos/thread.h>
#include <kos/regfield.h>
#include <assert.h>

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
maple_device_t *maple_enum_dev(int p, int u) {
    maple_device_t *rv = maple_state.ports[p].units[u];
    if(rv && rv->valid)
        return rv;
    return NULL;
}

/* Return the Nth device of the requested type (where N is zero-indexed) */
maple_device_t *maple_enum_type(int n, uint32_t func) {
    int p, u;
    maple_device_t *dev;

    for(p = 0; p < MAPLE_PORT_COUNT; p++) {
        for(u = 0; u < MAPLE_UNIT_COUNT; u++) {
            dev = maple_enum_dev(p, u);

            if(dev != NULL && (dev->info.functions & func)) {
                if(!n) return dev;

                n--;
            }
        }
    }

    return NULL;
}

/* Return the Nth device that is of the requested type and supports the list of
   capabilities given. */
maple_device_t *maple_enum_type_ex(int n, uint32_t func, uint32_t cap) {
    int p, u, d;
    maple_device_t *dev;
    uint32_t funcmask;

    /* If func is 0, there can be no match (and it's UB for clz below) */
    if(!func) return NULL;

    /* Create a mask that leaves only the bits above func. */
    funcmask = ~GENMASK(31 - __builtin_clz(func), 0);

    for(p = 0; p < MAPLE_PORT_COUNT; ++p) {
        for(u = 0; u < MAPLE_UNIT_COUNT; ++u) {
            dev = maple_enum_dev(p, u);

            /* If the device supports the function code we passed in, check
               if it supports the capabilities that the user requested. */
            if(dev != NULL && (dev->info.functions & func)) {

                /* Figure out which function data we want to look at. Function
                   data entries are arranged by the function code, most
                   significant bit first. So we count the bits above func. */
                d = __builtin_popcount(dev->info.functions & funcmask);

                /* Ensure that the result is in-bounds */
                assert((d >= 0) && (d < 3));

                /* Check if the function data for the function type checks out
                   with what it should be. */
                cap = __builtin_bswap32(cap);

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

/* Get the status struct for the requested maple device.
   Cast to the appropriate type you're expecting. */
void *maple_dev_status(maple_device_t *dev) {
    /* The device must be valid. */
    if(!dev || !dev->valid || !dev->drv)
        return NULL;

    /* Cast and return the status buffer */
    return dev->status;
}

