/* KallistiOS ##version##

   vmu.c
   Copyright (C) 2002, 2003 Megan Potter
   Copyright (C) 2008 Donald Haase
   Copyright (C) 2023, 2025 Falco Girgis
 */

/*
   This module deals with the VMU.  It provides functionality for
   filesystem, LCD screen, buzzer, and date/time access.

   Thanks to Marcus Comstedt for VMU/Maple information.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <kos/thread.h>
#include <kos/genwait.h>
#include <kos/platform.h>
#include <kos/dbglog.h>
#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <dc/math.h>
#include <dc/biosfont.h>
#include <dc/vmufs.h>
#include <arch/timer.h>

#define VMU_BLOCK_WRITE_RETRY_TIME  100     /* time to sleep until retrying a failed write */

/* This is the value that official VMUs report for function_data[0]. Have not 
   found any official ones that report a different value nor any third party
   that report it.
*/
static const uint32_t vmu_official_function_data0 = 0x403f7e7e;

/* Distinguish between VMU (only official, with screen/clock/buttons)
   and VMS (memcard only). */
static bool vmu_is_vmu(const maple_device_t *dev) {
    /* If it doesn't have the known magic number, it's no official VMU. */
    if(dev->info.function_data[0] != vmu_official_function_data0)
        return false;

    /* Give a secondary test to confirm that it's presenting all three
    components of an official VMU. This alone is not sufficient to test
    for VMU vs VMS though as most (all?) 3rd party VMS report that they
    have the other functions.
    */
    if(dev->info.functions & (MAPLE_FUNC_MEMCARD | MAPLE_FUNC_LCD | MAPLE_FUNC_CLOCK))
        return true;

    return false;
}

/* VMU's raw condition data: 0 = PRESSED, 1 = RELEASED */
typedef struct vmu_cond {
    uint8_t raw_buttons;
    uint8_t dummy[3];
} vmu_cond_t;

typedef struct vmu_datetime {
    uint16_t year;    /* 0 - 9999 */
    uint8_t  month;   /* 1 - 12 */
    uint8_t  day;     /* 1 - 31 */
    uint8_t  hour;    /* 0 - 23 */
    uint8_t  minute;  /* 0 - 59 */
    uint8_t  second;  /* 0 - 59 */
    uint8_t  weekday; /* 0 - 6 (Starting with Monday) */
} vmu_datetime_t;

static void vmu_datetime_to_tm(const vmu_datetime_t *dt, struct tm *bt) {
    bt->tm_sec  = dt->second;
    bt->tm_min  = dt->minute;
    bt->tm_hour = dt->hour;
    bt->tm_mday = dt->day;
    bt->tm_mon  = dt->month - 1;
    bt->tm_year = dt->year - 1900;
    bt->tm_wday = dt->weekday != 6? dt->weekday + 1 : 0;
}

static void vmu_datetime_from_tm(vmu_datetime_t *dt, const struct tm *bt) {
    dt->second  = bt->tm_sec;
    dt->minute  = bt->tm_min;
    dt->hour    = bt->tm_hour;
    dt->day     = bt->tm_mday;
    dt->month   = bt->tm_mon + 1;
    dt->year    = bt->tm_year + 1900;
    dt->weekday = bt->tm_wday? dt->weekday - 1 : 6;
}

static int vmu_attach(maple_driver_t *drv, maple_device_t *dev) {
    (void)drv;
    dev->status_valid = 1;
    return 0;
}

static void vmu_poll_reply(maple_state_t *st, maple_frame_t *frm) {
    (void)st;

    maple_response_t   *resp;
    uint32_t           *respbuf;
    vmu_cond_t         *raw;
    vmu_state_t        *cooked;

    /* Unlock the frame now (it's ok, we're in an IRQ) */
    maple_frame_unlock(frm);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frm->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf = (uint32_t *)resp->data;

    if(respbuf[0] != MAPLE_FUNC_CLOCK)
        return;

    if(!frm->dev)
        return;

    /* Verify the size of the frame and grab a pointer to it */
    assert(sizeof(vmu_cond_t) == ((resp->data_len - 1) * sizeof(uint32_t)));
    raw = (vmu_cond_t *)(respbuf + 1);

    /* Fill the "nice" struct from the raw data */
    cooked = (vmu_state_t *)(frm->dev->status);
    /* Copy over current button states to previous states. */
    cooked->buttons.previous = cooked->buttons.current;
    /* Invert raw struct as nice struct */
    cooked->buttons.current.raw = ~(raw->raw_buttons);

    /* Check to see if the VMU is upside-down in the controller and readjust
       its directional buttons accordingly. */
    const maple_device_t *cont = maple_enum_dev(frm->dev->port, 0);

    if(cont && (cont->info.functions & MAPLE_FUNC_CONTROLLER) &&
       (frm->dev->info.connector_direction == cont->info.connector_direction)) {
        cooked->buttons.current.raw =
            (cooked->buttons.current.raw & 0xf0)      |
            (cooked->buttons.current.dpad_up    << 1) | /* down */
            (cooked->buttons.current.dpad_down  << 0) | /* up */
            (cooked->buttons.current.dpad_left  << 3) | /* right */
            (cooked->buttons.current.dpad_right << 2);  /* left */
    }

    frm->dev->status_valid = 1;
}

static int vmu_poll(maple_device_t *dev) {
    uint32_t *send_buf;

    /* Only query for button input on the front VMU of each controller 
       AND the device actually has the functionality. */
    if((dev->unit == 1) && vmu_is_vmu(dev)) {
        if(maple_frame_lock(&dev->frame) < 0)
            return 0;

        maple_frame_init(&dev->frame);
        send_buf = (uint32_t *)dev->frame.recv_buf;
        send_buf[0] = MAPLE_FUNC_CLOCK;
        dev->frame.cmd = MAPLE_COMMAND_GETCOND;
        dev->frame.dst_port = dev->port;
        dev->frame.dst_unit = dev->unit;
        dev->frame.length = 1;
        dev->frame.callback = vmu_poll_reply;
        dev->frame.send_buf = send_buf;
        maple_queue_frame(&dev->frame);

    }

    return 0;
}

static void vmu_periodic(maple_driver_t *drv) {
    maple_driver_foreach(drv, vmu_poll);
}

/* Device Driver Struct */
static maple_driver_t vmu_drv = {
    .functions = MAPLE_FUNC_MEMCARD | MAPLE_FUNC_LCD | MAPLE_FUNC_CLOCK,
    .name = "VMU Driver",
    .periodic = NULL,
    .status_size = sizeof(vmu_state_t),
    .attach = vmu_attach,
    .detach = NULL
};

/* Add the VMU to the driver chain */
void vmu_init(void) {
    maple_driver_reg(&vmu_drv);
}

void vmu_shutdown(void) {
    maple_driver_unreg(&vmu_drv);
}

/* Dynamically add the periodic polling callback to the driver when button input is enabled. */
void vmu_set_buttons_enabled(int enable) {
    vmu_drv.periodic = enable ? vmu_periodic : NULL;
}

/* Determine whether polling for button input is enabled or not by presence of periodic callback. */
int vmu_get_buttons_enabled(void) {
    return !!vmu_drv.periodic;
}

int vmu_has_241_blocks(maple_device_t *dev) {
    vmu_root_t root;

    if(vmufs_root_read(dev, &root) < 0)
        return -1;

    if(root.blk_cnt == 241)
        return 1;

    return 0;
}

int vmu_toggle_241_blocks(maple_device_t *dev, int enable) {
    vmu_root_t root;

    if(vmufs_root_read(dev, &root) < 0)
        return -1;

    root.blk_cnt = (enable != 0) ? 241 : 200;

    if(vmufs_root_write(dev, &root) < 0)
        return -1;

    return 0;
}

int vmu_use_custom_color(maple_device_t *dev, int enable) {
    vmu_root_t root;

    if(vmufs_root_read(dev, &root) < 0)
        return -1;

    /* 1 - Enables the use of the custom color. 0 - Disables */
    root.use_custom = (enable != 0) ? 1 : 0;

    if(vmufs_root_write(dev, &root) < 0)
        return -1;

    return 0;
}

/* The custom color is used while navigating the Dreamcast's file manager.
   You set the RGBA parameters, each with valid range of 0-255 */
int vmu_set_custom_color(maple_device_t *dev, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    vmu_root_t root;

    if(vmufs_root_read(dev, &root) < 0)
        return -1;

    /* 1 - Enables the use of the custom color. 0 - Disables */
    root.use_custom = 1;
    root.custom_color[0] = blue;
    root.custom_color[1] = green;
    root.custom_color[2] = red;
    root.custom_color[3] = alpha;

    if(vmufs_root_write(dev, &root) < 0)
        return -1;

    return 0;
}

/* The icon shape is used while navigating the BIOS menu. The values
   for icon_shape are listed in the biosfont.h and start with
   BFONT_ICON_VMUICON. */
int vmu_set_icon_shape(maple_device_t *dev, uint8_t icon_shape) {
    vmu_root_t root;

    if(KOS_PLATFORM_IS_NAOMI)
        return -1;

    if(icon_shape < BFONT_ICON_VMUICON || icon_shape > BFONT_ICON_EMBROIDERY)
        return -1;

    if(vmufs_root_read(dev, &root) < 0)
        return -1;

    /* Valid value range is 0-123 and starts with BFONT_ICON_VMUICON which
       has a value of 5.  This is because we can't use the first 5 icons
       found in the bios so we must subtract 5 */
    root.icon_shape = icon_shape - BFONT_ICON_VMUICON;

    if(vmufs_root_write(dev, &root) < 0)
        return -1;

    return 0;
}

/* These interfaces will probably change eventually, but for now they
   can stay the same */

/* Callback that unlocks the frame, general use */
static void vmu_gen_callback(maple_state_t *st, maple_frame_t *frame) {
    (void)st;

    /* Unlock the frame for the next usage */
    maple_frame_unlock(frame);

    /* Wakey, wakey! */
    genwait_wake_all(frame);
}

/* Set the tone to be generated by the VMU's speaker.
   Only last two bytes are used. This might necessitate
   refactoring as the clock is a separate device from
   the screen and storage. */
int vmu_beep_raw(maple_device_t *dev, uint32_t beep) {
    uint32_t *send_buf;

    assert(dev);

    /* Only send a beep if this is a real VMU */
    if(!vmu_is_vmu(dev))
        return MAPLE_EINVALID;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CLOCK;
    send_buf[1] = beep;
    dev->frame.cmd = MAPLE_COMMAND_SETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = NULL;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return MAPLE_EOK;
}

int vmu_beep_waveform(maple_device_t *dev, uint8_t period1, uint8_t duty_cycle1, uint8_t period2, uint8_t duty_cycle2) {
    const uint32_t raw_beep = (((period2 - duty_cycle2) << 24) | (period2 << 16) |
                               ((period1 - duty_cycle1) <<  8) | (period1));

    return vmu_beep_raw(dev, raw_beep);
}

/* Draw a 1-bit bitmap on the LCD screen (48x32). return a -1 if
   an error occurs */
int vmu_draw_lcd(maple_device_t *dev, const void *bitmap) {
    uint32_t *send_buf;

    assert(dev != NULL);

    /* Only try to draw to screen if this is a real VMU */
    if(!vmu_is_vmu(dev))
        return MAPLE_EINVALID;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_LCD;
    send_buf[1] = 0;    /* Block / phase / partition */
    memcpy(send_buf + 2, bitmap, VMU_SCREEN_WIDTH * 4);
    dev->frame.cmd = MAPLE_COMMAND_BWRITE;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2 + VMU_SCREEN_WIDTH;
    dev->frame.callback = NULL;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return MAPLE_EOK;
}

int vmu_draw_lcd_rotated(maple_device_t *dev, const void *bitmap) {
    uint32_t bitmap_inverted[48];
    unsigned int i;

    for(i = 0; i < 48; i++) {
        bitmap_inverted[i] = bit_reverse(((uint32 *)bitmap)[47 - i]);
    }

    return vmu_draw_lcd(dev, bitmap_inverted);
}

/* This function converts a xbm image to a 1-bit bitmap that can
   be displayed on LCD screen of VMU */
static void vmu_xbm_to_bitmap(uint8_t *bitmap, const char *vmu_icon) {
    int x, y, xi, xb;
    memset(bitmap, 0, VMU_SCREEN_WIDTH * VMU_SCREEN_HEIGHT / 8);

    if(vmu_icon) {
        for(y = 0; y < VMU_SCREEN_HEIGHT; y++)
            for(x = 0; x < VMU_SCREEN_WIDTH; x++) {
                xi = x / 8;
                xb = 0x80 >> (x % 8);

                if(vmu_icon[((VMU_SCREEN_HEIGHT - 1) - y) * VMU_SCREEN_WIDTH 
                          + ((VMU_SCREEN_WIDTH  - 1) - x)] == '.')
                    bitmap[y * (VMU_SCREEN_WIDTH / 8) + xi] |= xb;
            }
    }
}

int vmu_draw_lcd_xbm(maple_device_t *dev, const char *vmu_icon) {
    uint8_t  bitmap[VMU_SCREEN_WIDTH * VMU_SCREEN_HEIGHT / 8];
    vmu_xbm_to_bitmap(bitmap, vmu_icon);

    return vmu_draw_lcd(dev, bitmap);
}

/* Utility function which sets the icon on all available VMUs
   from an Xwindows XBM. Imported from libdcutils. */
void vmu_set_icon(const char *vmu_icon) {
    int            i = 0;
    maple_device_t *dev;
    uint8_t        bitmap[VMU_SCREEN_WIDTH * VMU_SCREEN_HEIGHT / 8];

    vmu_xbm_to_bitmap(bitmap, vmu_icon);

    while((dev = maple_enum_type(i++, MAPLE_FUNC_LCD))) {
        vmu_draw_lcd(dev, bitmap);
    }
}

/* Read the data in block blocknum into buffer, return a -1
   if an error occurs, for now we ignore MAPLE_RESPONSE_FILEERR,
   which will be changed shortly */
static void vmu_block_read_callback(maple_state_t *st, maple_frame_t *frm) {
    (void)st;

    /* Wakey, wakey! */
    genwait_wake_all(frm);
}

int vmu_block_read(maple_device_t *dev, uint16_t blocknum, uint8_t *buffer) {
    maple_response_t *resp;
    int              rv;
    uint32_t         *send_buf;
    uint32_t         blkid;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* This is (block << 24) | (phase << 8) | (partition (0 for all vmu)) */
    blkid = ((blocknum & 0xff) << 24) | ((blocknum >> 8) << 16);

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MEMCARD;
    send_buf[1] = blkid;
    dev->frame.cmd = MAPLE_COMMAND_BREAD;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = vmu_block_read_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the VMU to accept it */
    if(genwait_wait(&dev->frame, "vmu_block_read", 100, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_RESPONDED) {
            /* It's probably never coming back, so just unlock the frame */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vmu_block_read: timeout to unit %c%c, block %d\n",
                   dev->port + 'A', dev->unit + '0', (int)blocknum);
            return MAPLE_ETIMEOUT;
        }
    }

    if(dev->frame.state != MAPLE_FRAME_RESPONDED) {
        dbglog(DBG_ERROR, "vmu_block_read: incorrect state for unit %c%c, block %d (%d)\n",
               dev->port + 'A', dev->unit + '0', (int)blocknum, dev->frame.state);
        dev->frame.state = MAPLE_FRAME_VACANT;
        return MAPLE_EFAIL;
    }

    /* Copy out the response */
    resp = (maple_response_t *)dev->frame.recv_buf;
    send_buf = (uint32_t *)resp->data;

    if(resp->response != MAPLE_RESPONSE_DATATRF
            || send_buf[0] != MAPLE_FUNC_MEMCARD
            || send_buf[1] != blkid) {
        rv = MAPLE_EFAIL;
        dbglog(DBG_ERROR, "vmu_block_read failed: %s(%d)/%08lx\r\n",
               maple_perror(resp->response), resp->response, send_buf[0]);
    }
    else {
        rv = MAPLE_EOK;
        memcpy(buffer, send_buf + 2, (resp->data_len - 2) * 4);
    }

    maple_frame_unlock(&dev->frame);

    return rv;
}

/* writes buffer into block blocknum.  ret a -1 on error.  We don't do anything about the
   maple bus returning file errors, etc, right now, but that will change soon. */
static void vmu_block_write_callback(maple_state_t *st, maple_frame_t *frm) {
    (void)st;

    /* Reset the frame status (but still keep it for us to use) */
    frm->state = MAPLE_FRAME_UNSENT;

    /* Wakey, wakey! */
    genwait_wake_all(frm);
}

static int vmu_block_write_internal(maple_device_t *dev, uint16_t blocknum, const uint8_t *buffer) {
    maple_response_t *resp;
    int              rv, phase, r;
    uint32_t         *send_buf;
    uint32_t         blkid;

    assert(dev != NULL);

    /* Assume success */
    rv = MAPLE_EOK;

    /* Lock the frame. XXX: Priority inversion issues here. */
    while(maple_frame_lock(&dev->frame) < 0)
        thd_pass();

    /* Writes have to occur in four phases per block -- this is the
       way of flash memory, which you must erase an entire block
       at once to write; the blocks in this case are 128 bytes long. */
    for(phase = 0; phase < 4; phase++) {
        /* this is (block << 24) | (phase << 8) | (partition (0 for all vmu)) */
        blkid = ((blocknum & 0xff) << 24) | ((blocknum >> 8) << 16) | (phase << 8);

        /* Reset the frame */
        maple_frame_init(&dev->frame);
        send_buf = (uint32_t *)dev->frame.recv_buf;
        send_buf[0] = MAPLE_FUNC_MEMCARD;
        send_buf[1] = blkid;
        memcpy(send_buf + 2, buffer + 128 * phase, 128);
        dev->frame.cmd = MAPLE_COMMAND_BWRITE;
        dev->frame.dst_port = dev->port;
        dev->frame.dst_unit = dev->unit;
        dev->frame.length = 2 + (128 / 4);
        dev->frame.callback = vmu_block_write_callback;
        dev->frame.send_buf = send_buf;
        maple_queue_frame(&dev->frame);

        /* Wait for the VMU to accept it */
        if(genwait_wait(&dev->frame, "vmu_block_write", 100, NULL) < 0) {
            if(dev->frame.state != MAPLE_FRAME_UNSENT) {
                /* It's probably never coming back, so just unlock the frame */
                dev->frame.state = MAPLE_FRAME_VACANT;
                dbglog(DBG_ERROR, "vmu_block_write: timeout to unit %c%c, block %d\n",
                       dev->port + 'A', dev->unit + '0', (int)blocknum);
                return MAPLE_ETIMEOUT;
            }
        }

        if(dev->frame.state != MAPLE_FRAME_UNSENT) {
            dbglog(DBG_ERROR, "vmu_block_read: incorrect state for unit %c%c, block %d (%d)\n",
                   dev->port + 'A', dev->unit + '0', (int)blocknum, dev->frame.state);
            dev->frame.state = MAPLE_FRAME_VACANT;
            return MAPLE_EFAIL;
        }

        /* Check the response */
        resp = (maple_response_t *)dev->frame.recv_buf;
        r = resp->response;

        if(r != MAPLE_RESPONSE_OK) {
            rv = MAPLE_EFAIL;
            dbglog(DBG_ERROR, "Incorrect response writing phase %d:\n", phase);
            dbglog(DBG_ERROR, "response:      %s(%d)\n", maple_perror(resp->response), resp->response);
            dbglog(DBG_ERROR, "datalen:       %d\n", resp->data_len);
        }
    }

    /* Finally a "sync" command -- thanks Nagra */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MEMCARD;
    send_buf[1] = ((blocknum & 0xff) << 24)
                  | (((blocknum >> 8) & 0xff) << 16)
                  | (4 << 8);
    dev->frame.cmd = MAPLE_COMMAND_BSYNC;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = vmu_block_write_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the VMU to accept it */
    if(genwait_wait(&dev->frame, "vmu_block_write", 100, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_UNSENT) {
            /* It's probably never coming back, so just unlock the frame */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vmu_block_write: timeout to unit %c%c, block %d\n",
                   dev->port + 'A', dev->unit + '0', (int)blocknum);
            return MAPLE_ETIMEOUT;
        }
    }

    if(dev->frame.state != MAPLE_FRAME_UNSENT) {
        dbglog(DBG_ERROR, "vmu_block_read: incorrect state for unit %c%c, block %d (%d)\n",
               dev->port + 'A', dev->unit + '0', (int)blocknum, dev->frame.state);
        dev->frame.state = MAPLE_FRAME_VACANT;
        return MAPLE_EFAIL;
    }

    dev->frame.state = MAPLE_FRAME_VACANT;

    return rv;
}

/* Sometimes a flaky or stubborn card can be recovered by trying a couple
   of times... */
int vmu_block_write(maple_device_t *dev, uint16_t blocknum, const uint8_t *buffer) {
    int i, rv;

    for(i = 0; i < 4; i++) {
        // Try the write.
        rv = vmu_block_write_internal(dev, blocknum, buffer);

        if(rv == MAPLE_EOK)
            return rv;

        /* It failed -- wait a bit and try again. */
        thd_sleep(VMU_BLOCK_WRITE_RETRY_TIME);
    }

    /* Well, looks like it's really toasty... return the most recent
       error. */
    return rv;
}

int vmu_set_datetime(maple_device_t *dev, time_t unix) {
    uint32_t *send_buf;
    struct tm *btime;

    assert(dev);

    /* Only set datetime if this is a real VMU */
    if(!vmu_is_vmu(dev))
        return MAPLE_EINVALID;

    btime = localtime(&unix);
    assert(btime); /* A failure here means an invalid unix timestamp was given. */

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CLOCK;
    send_buf[1] = 0;
    vmu_datetime_from_tm((vmu_datetime_t *)(send_buf + 2), btime);

    dev->frame.cmd = MAPLE_COMMAND_BWRITE;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 4;
    dev->frame.callback = vmu_gen_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the timer to accept it */
    if(genwait_wait(&dev->frame, "vmu_set_datetime", 500, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_VACANT)  {
            /* Something went wrong.... */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vmu_set_datetime: timeout to unit %c%c\n",
                   dev->port + 'A', dev->unit + '0');
            return MAPLE_ETIMEOUT;
        }
    }

    return MAPLE_EOK;
}

static void vmu_get_datetime_callback(maple_state_t *st, maple_frame_t *frm) {
    (void)st;

    /* Wakey, wakey! */
    genwait_wake_all(frm);
}

int vmu_get_datetime(maple_device_t *dev, time_t *unix) {
    maple_response_t *resp;
    int               rv;
    uint32_t          *send_buf;

    assert(dev);

    /* Only get datetime if this is a real VMU */
    if(!vmu_is_vmu(dev))
        return MAPLE_EINVALID;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32_t *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CLOCK;
    send_buf[1] = 0;

    dev->frame.cmd = MAPLE_COMMAND_BREAD;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = vmu_get_datetime_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the VMU to accept it */
    if(genwait_wait(&dev->frame, "vmu_get_datetime", 10000, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_RESPONDED) {
            /* It's probably never coming back, so just unlock the frame */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vmu_get_datetime: timeout to unit %c%c\n",
                   dev->port + 'A', dev->unit + '0');
            *unix = -1;
            return MAPLE_ETIMEOUT;
        }
    }

    if(dev->frame.state != MAPLE_FRAME_RESPONDED) {
        dbglog(DBG_ERROR, "vmu_get_datetime: incorrect state for unit %c%c (%d)\n",
               dev->port + 'A', dev->unit + '0', dev->frame.state);
        dev->frame.state = MAPLE_FRAME_VACANT;
        *unix = -1;
        return MAPLE_EFAIL;
    }

    /* Copy out the response */
    resp = (maple_response_t *)dev->frame.recv_buf;
    send_buf = (uint32_t *)resp->data;

    if(resp->response != MAPLE_RESPONSE_DATATRF
            || send_buf[0] != MAPLE_FUNC_CLOCK) {
        rv = MAPLE_EFAIL;
        *unix = -1;
        dbglog(DBG_ERROR, "vmu_get_datetime failed: %s(%d)/%08lx\r\n",
               maple_perror(resp->response), resp->response, send_buf[0]);
    }
    else {
        rv = MAPLE_EOK;
        struct tm btime = { 0 };
        vmu_datetime_to_tm((const vmu_datetime_t*)(send_buf + 1), &btime);
        *unix = mktime(&btime);
    }

    maple_frame_unlock(&dev->frame);

    return rv;
}
