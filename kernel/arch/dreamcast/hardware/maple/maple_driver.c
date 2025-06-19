/* KallistiOS ##version##

   maple_driver.c
   (c)2002 Megan Potter
 */

#include <string.h>
#include <stdlib.h>
#include <dc/maple.h>

void maple_attach_callback(uint32 functions, maple_attach_callback_t cb) {
    maple_driver_t *i;

    if(!functions)
        functions = 0xffffffff;

    LIST_FOREACH(i, &maple_state.driver_list, drv_list) {
        if(i->functions & functions) {
            i->user_attach = cb;
            functions &= ~i->functions;

            if(!functions)
                break;
        }
    }
}

void maple_detach_callback(uint32 functions, maple_detach_callback_t cb) {
    maple_driver_t *i;

    if(!functions)
        functions = 0xffffffff;

    LIST_FOREACH(i, &maple_state.driver_list, drv_list) {
        if(i->functions & functions) {
            i->user_detach = cb;
            functions &= ~i->functions;

            if(!functions)
                break;
        }
    }
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
    maple_device_t      *dev = maple_state.ports[det->dst_port].units[det->dst_unit];
    bool                attached = false;

    /* Resolve some pointers first */
    resp = (maple_response_t *)det->recv_buf;
    devinfo = (maple_devinfo_t *)resp->data;

    /* Go through the list and look for a matching driver */
    LIST_FOREACH(i, &maple_state.driver_list, drv_list) {
        /* For now we just pick the first matching driver */
        if(i->functions & devinfo->functions) {

            /* Driver matches. Alloc a device if needed. */
            if(!dev) {
                dev = calloc(1, sizeof(*dev));
                if(!dev)
                    return 1;

                maple_state.ports[det->dst_port].units[det->dst_unit] = dev;

                /* Add the basics for the initial version of the struct */
                dev->port = det->dst_port;
                dev->unit = det->dst_unit;
                dev->frame.state = MAPLE_FRAME_VACANT;
            }

            memcpy(&dev->info, devinfo, sizeof(maple_devinfo_t));

            /* Now lets allocate a new status buffer */
            if(i->status_size && !dev->status) {
                dev->status = calloc(1, i->status_size);
                if(!dev->status)
                    return 1;
            }

            if(!i->status_size || dev->status) {
                /* Try to attach if we need to then break out. */
                if(!(i->attach) || (i->attach(i, dev) >= 0)) {
                    attached = true;
                    break;
                }
            }
        }
    }

    if(!dev)
        return -1;

    /* Did we get any hits? */
    if(!attached) {
        free(dev->status);
        dev->status = NULL;

        return -1;
    }

    /* Finish setting stuff up */
    dev->drv = i;
    dev->status_valid = 0;
    dev->valid = true;

    if(i->user_attach)
        i->user_attach(dev);

    return 0;
}

/* Detach an attached maple device */
int maple_driver_detach(int p, int u) {
    maple_device_t *dev = maple_enum_dev(p, u);

    if(!dev)
        return -1;

    dev->valid = false;

    if(dev->drv) {
        if(dev->drv->user_detach)
            dev->drv->user_detach(dev);
        if(dev->drv->detach)
            dev->drv->detach(dev->drv, dev);
    }

    dev->status_valid = 0;

    if(dev->drv->status_size) {
        free(dev->status);
        dev->status = NULL;
    }

    dev->probe_mask = 0;
    dev->dev_mask = 0;

    return 0;
}

/* For each device which the given driver controls, call the callback */
int maple_driver_foreach(maple_driver_t *drv, int (*callback)(maple_device_t *)) {
    int     p, u;
    maple_device_t  *dev;

    for(p = 0; p < MAPLE_PORT_COUNT; p++) {
        for(u = 0; u < MAPLE_UNIT_COUNT; u++) {
            dev = maple_enum_dev(p, u);

            if(dev && dev->drv == drv && !dev->frame.queued)
                if(callback(dev) < 0)
                    return -1;
        }
    }

    return 0;
}
