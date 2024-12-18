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
    maple_device_t      *dev = NULL;

    /* Resolve some pointers first */
    resp = (maple_response_t *)det->recv_buf;
    devinfo = (maple_devinfo_t *)resp->data;

    /* Go through the list and look for a matching driver */
    LIST_FOREACH(i, &maple_state.driver_list, drv_list) {
        /* For now we just pick the first matching driver */
        if(i->functions & devinfo->functions) {
            /* Driver matches. Alloc a device. */
            dev = calloc(1, sizeof(*dev) + i->status_size);
            if (!dev)
                return 1;

            memcpy(&dev->info, devinfo, sizeof(maple_devinfo_t));
            dev->info.product_name[29] = 0;
            dev->info.product_license[59] = 0;
            dev->port = det->dst_port;
            dev->unit = det->dst_unit;
            dev->frame.state = MAPLE_FRAME_VACANT;

            /* Try to attach if we need to */
            if(!(i->attach) || (i->attach(i, dev) >= 0))
                break;

            /* Attach failed, free the device */
            free(dev);
            dev = NULL;
        }
    }

    /* Did we get any hits? */
    if(!dev)
        return -1;

    maple_state.ports[det->dst_port].units[det->dst_unit] = dev;

    /* Finish setting stuff up */
    dev->drv = i;
    dev->status_valid = 0;

    if(!(attach_callback_functions) || (dev->info.functions & attach_callback_functions)) {
        if(attach_callback) {
            attach_callback(dev);
        }
    }

    return 0;
}

static void maple_detach(maple_device_t *dev) {
    if(dev->drv && dev->drv->detach)
        dev->drv->detach(dev->drv, dev);

    dev->status_valid = 0;

    if(!(detach_callback_functions) || (dev->info.functions & detach_callback_functions)) {
        if(detach_callback) {
            detach_callback(dev);
        }
    }
}

/* Detach an attached maple device */
int maple_driver_detach(int p, int u) {
    maple_device_t *dev = maple_enum_dev(p, u);

    if(!dev)
        return -1;

    maple_state.ports[p].units[u] = NULL;
    maple_detach(dev);
    free(dev);
    return 0;
}

/* For each device which the given driver controls, call the callback */
int maple_driver_foreach(maple_driver_t *drv, int (*callback)(maple_device_t *)) {
    int     p, u;
    maple_device_t  *dev;

    for(p = 0; p < MAPLE_PORT_COUNT; p++) {
        for(u = 0; u < MAPLE_UNIT_COUNT; u++) {
            dev = maple_state.ports[p].units[u];

            if(dev && dev->drv == drv && !dev->frame.queued)
                if(callback(dev) < 0)
                    return -1;
        }
    }

    return 0;
}
