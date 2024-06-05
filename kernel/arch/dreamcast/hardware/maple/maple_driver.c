/* KallistiOS ##version##

   maple_driver.c
   (c)2002 Megan Potter
 */

#include <string.h>
#include <stdlib.h>
#include <dc/maple.h>

static maple_attach_callback_t attach_callback = NULL;
static uint32 attach_callback_functions = 0;

static maple_detach_callback_t detach_callback = NULL;
static uint32 detach_callback_functions = 0;

void maple_attach_callback(uint32 functions, maple_attach_callback_t cb) {
    attach_callback_functions = functions;
    attach_callback = cb;
}

void maple_detach_callback(uint32 functions, maple_detach_callback_t cb) {
    detach_callback_functions = functions;
    detach_callback = cb;
}

/* Register a maple device driver; do this before maple_init() */
int maple_driver_reg(maple_driver_t *driver) {
    /* Don't add two drivers for the same function */
    maple_driver_t *i;

    if(driver->drv_list.le_prev)
        return -1;

    LIST_FOREACH(i, &maple_state.driver_list, drv_list)
        if(i->functions & driver->functions)
            return -1;

    /* Insert it into the device list */
    LIST_INSERT_HEAD(&maple_state.driver_list, driver, drv_list);
    return 0;
}

/* Unregister a maple device driver */
int maple_driver_unreg(maple_driver_t *driver) {
    /* Remove it from the list */
    LIST_REMOVE(driver, drv_list);
    return 0;
}

/* Attach a maple device to a driver, if possible */
int maple_driver_attach(maple_frame_t *det) {
    maple_driver_t      *i;
    maple_response_t    *resp;
    maple_devinfo_t     *devinfo;
    maple_device_t      *dev;
    int         attached;

    /* Resolve some pointers first */
    resp = (maple_response_t *)det->recv_buf;
    devinfo = (maple_devinfo_t *)resp->data;
    attached = 0;
    dev = &maple_state.ports[det->dst_port].units[det->dst_unit];
    memcpy(&dev->info, devinfo, sizeof(maple_devinfo_t));
    memset(dev->status, 0, sizeof(dev->status));
    dev->drv = NULL;

    /* Go through the list and look for a matching driver */
    LIST_FOREACH(i, &maple_state.driver_list, drv_list) {
        /* For now we just pick the first matching driver */
        if(i->functions & devinfo->functions) {
            /* Driver matches, try an attach if we need to */
            if(!(i->attach) || (i->attach(i, dev) >= 0)) {
                /* Success: make it permanent */
                attached = 1;
                break;
            }
        }
    }

    /* Did we get any hits? */
    if(!attached)
        return -1;

    /* Finish setting stuff up */
    dev->drv = i;
    dev->status_valid = 0;
    dev->valid = 1;

    if(!(attach_callback_functions) || (dev->info.functions & attach_callback_functions)) {
        if(attach_callback) {
            attach_callback(dev);
        }
    }

    return 0;
}

/* Detach an attached maple device */
int maple_driver_detach(int p, int u) {
    maple_device_t  *dev;

    dev = &maple_state.ports[p].units[u];

    if(!dev->valid)
        return -1;

    if(dev->drv && dev->drv->detach)
        dev->drv->detach(dev->drv, dev);

    dev->valid = 0;
    dev->status_valid = 0;

    if(!(detach_callback_functions) || (dev->info.functions & detach_callback_functions)) {
        if(detach_callback) {
            detach_callback(dev);
        }
    }

    return 0;
}

/* For each device which the given driver controls, call the callback */
int maple_driver_foreach(maple_driver_t *drv, int (*callback)(maple_device_t *)) {
    int     p, u;
    maple_device_t  *dev;

    for(p = 0; p < MAPLE_PORT_COUNT; p++) {
        for(u = 0; u < MAPLE_UNIT_COUNT; u++) {
            dev = &maple_state.ports[p].units[u];

            if(!dev->valid) continue;

            if(dev->drv == drv && !dev->frame.queued)
                if(callback(dev) < 0)
                    return -1;
        }
    }

    return 0;
}
