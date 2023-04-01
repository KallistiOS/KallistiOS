/* KallistiOS ##version##

   dc/maple.h
   Copyright (C) 2002 Megan Potter
   Copyright (C) 2015 Lawrence Sebald

   This new driver's design is based loosely on the LinuxDC maple
   bus driver.
*/

/** \file   dc/maple.h
    \brief  Maple Bus driver interface.

    This file provides support for accessing the Maple bus on the Dreamcast.
    Maple is the bus that all of your controllers and memory cards and the like
    connect to, so this is one of those types of things that are quite important
    to know how to use.

    Each peripheral device registers their driver within this system, and can be
    accessed through the functions here. Most of the drivers have their own
    functionality that is implemented in their header files, as well.

    \author Megan Potter
    \author Lawrence Sebald

    \see    dc/maple/controller.h
    \see    dc/maple/dreameye.h
    \see    dc/maple/keyboard.h
    \see    dc/maple/mouse.h
    \see    dc/maple/purupuru.h
    \see    dc/maple/sip.h
    \see    dc/maple/vmu.h
*/

#ifndef __DC_MAPLE_H
#define __DC_MAPLE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <sys/queue.h>

/** \brief  Enable Maple DMA debugging.

    Changing this to a 1 will add massive amounts of processing time to the
    maple system in general, but it can help in verifying DMA errors. In
    general, for most purposes this should stay disabled.
*/
#define MAPLE_DMA_DEBUG 0

/** \brief  Enable Maple IRQ debugging.

    Changing this to a 1 will turn on intra-interrupt debugging messages, which
    may cause issues if you're using dcload rather than a raw serial debug
    terminal. You probably will never have a good reason to enable this, so keep
    it disabled for normal use.
*/
#define MAPLE_IRQ_DEBUG 0

/** \defgroup maple_regs            Maple Bus register locations

    These are various registers related to the Maple Bus. In general, you
    probably won't ever need to mess with these directly.

    @{
*/
#define MAPLE_BASE      0xa05f6c00          /**< \brief Maple register base */
#define MAPLE_DMAADDR   (MAPLE_BASE+0x04)   /**< \brief DMA address register */
#define MAPLE_RESET2    (MAPLE_BASE+0x10)   /**< \brief Reset register #2 */
#define MAPLE_ENABLE    (MAPLE_BASE+0x14)   /**< \brief Enable register */
#define MAPLE_STATE     (MAPLE_BASE+0x18)   /**< \brief Status register */
#define MAPLE_SPEED     (MAPLE_BASE+0x80)   /**< \brief Speed register */
#define MAPLE_RESET1    (MAPLE_BASE+0x8c)   /**< \brief Reset register #1 */
/** @} */

/** \defgroup maple_reg_values      Values to write to Maple Bus registers

    These are the values that are written to registers to get them to do their
    thing.

    @{
*/
#define MAPLE_RESET2_MAGIC      0               /**< \brief 2nd reset value */
#define MAPLE_ENABLE_ENABLED    1               /**< \brief Enable Maple */
#define MAPLE_ENABLE_DISABLED   0               /**< \brief Disable Maple */
#define MAPLE_STATE_IDLE        0               /**< \brief Idle state */
#define MAPLE_STATE_DMA         1               /**< \brief DMA in-progress */
#define MAPLE_SPEED_2MBPS       0               /**< \brief 2Mbps bus speed */
#define MAPLE_SPEED_TIMEOUT(n)  ((n) << 16)     /**< \brief Bus timeout macro */

#ifndef _arch_sub_naomi
#define MAPLE_RESET1_MAGIC      0x6155404f      /**< \brief First reset value */
#else
#define MAPLE_RESET1_MAGIC      0x6155405f
#endif

/** @} */

/** \defgroup maple_cmds            Maple commands and responses

    These are all either commands or responses to commands sent to or from Maple
    in normal operation.

    @{
*/
#define MAPLE_RESPONSE_FILEERR      -5  /**< \brief File error */
#define MAPLE_RESPONSE_AGAIN        -4  /**< \brief Try again later */
#define MAPLE_RESPONSE_BADCMD       -3  /**< \brief Bad command sent */
#define MAPLE_RESPONSE_BADFUNC      -2  /**< \brief Bad function code */
#define MAPLE_RESPONSE_NONE         -1  /**< \brief No response */
#define MAPLE_COMMAND_DEVINFO       1   /**< \brief Device info request */
#define MAPLE_COMMAND_ALLINFO       2   /**< \brief All info request */
#define MAPLE_COMMAND_RESET         3   /**< \brief Reset device request */
#define MAPLE_COMMAND_KILL          4   /**< \brief Kill device request */
#define MAPLE_RESPONSE_DEVINFO      5   /**< \brief Device info response */
#define MAPLE_RESPONSE_ALLINFO      6   /**< \brief All info response */
#define MAPLE_RESPONSE_OK           7   /**< \brief Command completed ok */
#define MAPLE_RESPONSE_DATATRF      8   /**< \brief Data transfer */
#define MAPLE_COMMAND_GETCOND       9   /**< \brief Get condition request */
#define MAPLE_COMMAND_GETMINFO      10  /**< \brief Get memory information */
#define MAPLE_COMMAND_BREAD         11  /**< \brief Block read */
#define MAPLE_COMMAND_BWRITE        12  /**< \brief Block write */
#define MAPLE_COMMAND_BSYNC         13  /**< \brief Block sync */
#define MAPLE_COMMAND_SETCOND       14  /**< \brief Set condition request */
#define MAPLE_COMMAND_MICCONTROL    15  /**< \brief Microphone control */
#define MAPLE_COMMAND_CAMCONTROL    17  /**< \brief Camera control */
/** @} */

/** \defgroup maple_functions       Maple device function codes

    This is the list of maple device types (function codes). Each device must
    have at least one function to actually do anything.

    @{
*/

/* Function codes; most sources claim that these numbers are little
   endian, and for all I know, they might be; but since it's a bitmask
   it doesn't really make much different. We'll just reverse our constants
   from the "big-endian" version. */
#define MAPLE_FUNC_PURUPURU     0x00010000  /**< \brief Jump pack */
#define MAPLE_FUNC_MOUSE        0x00020000  /**< \brief Mouse */
#define MAPLE_FUNC_CAMERA       0x00080000  /**< \brief Camera (Dreameye) */
#define MAPLE_FUNC_CONTROLLER   0x01000000  /**< \brief Controller */
#define MAPLE_FUNC_MEMCARD      0x02000000  /**< \brief Memory card */
#define MAPLE_FUNC_LCD          0x04000000  /**< \brief LCD screen */
#define MAPLE_FUNC_CLOCK        0x08000000  /**< \brief Clock */
#define MAPLE_FUNC_MICROPHONE   0x10000000  /**< \brief Microphone */
#define MAPLE_FUNC_ARGUN        0x20000000  /**< \brief AR gun? */
#define MAPLE_FUNC_KEYBOARD     0x40000000  /**< \brief Keyboard */
#define MAPLE_FUNC_LIGHTGUN     0x80000000  /**< \brief Lightgun */
/** @} */

/* \cond */
/* Pre-define list/queue types */
struct maple_frame;
TAILQ_HEAD(maple_frame_queue, maple_frame);

struct maple_driver;
LIST_HEAD(maple_driver_list, maple_driver);
/* \endcond */

/** \brief  Maple frame to be queued for transport.

    Internal representation of a frame to be queued up for sending.

    \headerfile dc/maple.h
*/
typedef struct maple_frame {
    /** \brief  Send queue handle. NOT A FUNCTION! */
    TAILQ_ENTRY(maple_frame)    frameq;

    int                 cmd;        /**< \brief Command (see \ref maple_cmds) */
    int                 dst_port;   /**< \brief Destination port */
    int                 dst_unit;   /**< \brief Destination unit */
    int                 length;     /**< \brief Data transfer length in 32-bit words */
    volatile int        state;      /**< \brief Has this frame been sent / responded to? */
    volatile int        queued;     /**< \brief Are we on the queue? */

    void                *send_buf;  /**< \brief The data which will be sent (if any) */
    uint8               *recv_buf;  /**< \brief Points into recv_buf_arr, but 32-byte aligned */

    struct maple_device *dev;       /**< \brief Does this belong to a device? */

    void (*callback)(struct maple_frame *);     /**< \brief Response callback */

#if MAPLE_DMA_DEBUG
    uint8   recv_buf_arr[1024 + 1024 + 32]; /**< \brief Response receive area */
#else
    uint8   recv_buf_arr[1024 + 32];        /**< \brief Response receive area */
#endif
} maple_frame_t;

/** \defgroup maple_frame_states    States that frames can be in
    @{
*/
#define MAPLE_FRAME_VACANT      0   /**< \brief Ready to be used */
#define MAPLE_FRAME_UNSENT      1   /**< \brief Ready to be sent */
#define MAPLE_FRAME_SENT        2   /**< \brief Frame has been sent, but no response yet */
#define MAPLE_FRAME_RESPONDED   3   /**< \brief Frame has a response */
/** @} */

/** \brief  Maple device info structure.

    This structure is used by the hardware to deliver the response to the device
    info request.

    \headerfile dc/maple.h
*/
typedef struct maple_devinfo {
    uint32  functions;              /**< \brief Function codes supported */
    uint32  function_data[3];       /**< \brief Additional data per function */
    uint8   area_code;              /**< \brief Region code */
    uint8   connector_direction;    /**< \brief ? */
    char    product_name[30];       /**< \brief Name of device */
    char    product_license[60];    /**< \brief License statement */
    uint16  standby_power;          /**< \brief Power consumption (standby) */
    uint16  max_power;              /**< \brief Power consumption (max) */
} maple_devinfo_t;

/** \brief  Maple response frame structure.

    This structure is used to deliver the actual response to a request placed.
    The data field is where all the interesting stuff will be.

    \headerfile dc/maple.h
*/
typedef struct maple_response {
    int8    response;   /**< \brief Response */
    uint8   dst_addr;   /**< \brief Destination address */
    uint8   src_addr;   /**< \brief Source address */
    uint8   data_len;   /**< \brief Data length (in 32-bit words) */
    uint8   data[];     /**< \brief Data (if any) */
} maple_response_t;

/** \brief  One maple device.

    Note that we duplicate the port/unit info which is normally somewhat
    implicit so that we can pass around a pointer to a particular device struct.

    \headerfile dc/maple.h
*/
typedef struct maple_device {
    /* Public */
    int             valid;  /**< \brief Is this a valid device? */
    int             port;   /**< \brief Maple bus port connected to */
    int             unit;   /**< \brief Unit number, off of the port */
    maple_devinfo_t info;   /**< \brief Device info struct */

    /* Private */
    int                     dev_mask;       /**< \brief Device-present mask for unit 0's */
    maple_frame_t           frame;          /**< \brief One rx/tx frame */
    struct maple_driver     *drv;           /**< \brief Driver which handles this device */

    volatile int            status_valid;   /**< \brief Have we got our first status update? */
    uint8                   status[1024];   /**< \brief Status buffer (for pollable devices) */
} maple_device_t;

#define MAPLE_PORT_COUNT    4   /**< \brief Number of ports on the bus */
#define MAPLE_UNIT_COUNT    6   /**< \brief Max number of units per port */

/** \brief  Internal representation of a Maple port.

    each maple port can contain up to 6 devices, the first one of which is
    always the port itself.

    \headerfile dc/maple.h
*/
typedef struct maple_port {
    int             port;                       /**< \brief Port ID */
    maple_device_t  units[MAPLE_UNIT_COUNT];    /**< \brief Pointers to active units */
} maple_port_t;

/** \brief  A maple device driver.

    Anything which is added to this list is capable of handling one or more
    maple device types. When a device of the given type is connected (includes
    startup "connection"), the driver is invoked. This same process happens for
    disconnection, response receipt, and on a periodic interval (for normal
    updates).

    \headerfile dc/maple.h
*/
typedef struct maple_driver {
    /** \brief  Driver list handle. NOT A FUNCTION! */
    LIST_ENTRY(maple_driver)    drv_list;

    uint32      functions;  /**< \brief One or more MAPLE_FUNCs ORed together */
    const char  *name;      /**< \brief The driver name */

    /* Callbacks, to be filled in by the driver */

    /** \brief  Periodic polling callback.

        This callback will be called to update the status of connected devices
        periodically.

        \param  drv         This structure for the driver.
    */
    void (*periodic)(struct maple_driver *drv);

    /** \brief  Device attached callback.

        This callback will be called when a new device of this driver is
        connected to the system.

        \param  drv         This structure for the driver.
        \param  dev         The device that was connected.
        \return             0 on success, <0 on error.
    */
    int (*attach)(struct maple_driver *drv, maple_device_t *dev);

    /** \brief  Device detached callback.

        This callback will be called when a device of this driver is disconnected
        from the system.

        \param  drv         This structure for the driver.
        \param  dev         The device that was detached.
    */
    void (*detach)(struct maple_driver *drv, maple_device_t *dev);
} maple_driver_t;

/** \brief  Maple state structure.

    We put everything in here to keep from polluting the global namespace too
    much.

    \headerfile dc/maple.h
*/
typedef struct maple_state_str {
    /** \brief  Maple device driver list. Do not manipulate directly! */
    struct maple_driver_list    driver_list;

    /** \brief  Maple frame submission queue. Do not manipulate directly! */
    struct maple_frame_queue    frame_queue;

    /** \brief  Maple device info structure */
    maple_port_t                ports[MAPLE_PORT_COUNT];

    /** \brief  DMA interrupt counter */
    volatile int                dma_cntr;

    /** \brief  VBlank interrupt counter */
    volatile int                vbl_cntr;

    /** \brief  DMA send buffer */
    uint8                       *dma_buffer;

    /** \brief  Is a DMA running now? */
    volatile int                dma_in_progress;

    /** \brief  Next port that will be auto-detected */
    int                         detect_port_next;

    /** \brief  Next unit which will be auto-detected */
    int                         detect_unit_next;

    /** \brief  Did the detect wrap? */
    volatile int                detect_wrapped;

    /** \brief  Our vblank handler handle */
    int                         vbl_handle;

    /** \brief  The port to read for lightgun status, if any. */
    int                         gun_port;

    /** \brief  The horizontal position of the lightgun signal. */
    int                         gun_x;

    /** \brief  The vertical position of the lightgun signal. */
    int                         gun_y;
} maple_state_t;

/** \brief  Maple DMA buffer size.

    Increase if you do a _LOT_ of maple stuff on every periodic interrupt.
*/
#define MAPLE_DMA_SIZE 16384

/* Maple memory read/write functions; these are just hooks in case
   we need to do something else later */
/** \brief  Maple memory read macro. */
#define maple_read(A) ( *((vuint32*)(A)) )

/** \brief  Maple memory write macro. */
#define maple_write(A, V) ( *((vuint32*)(A)) = (V) )

/* Return codes from maple access functions */
/** \defgroup maple_func_rvs        Return values from Maple functions
    @{
*/
#define MAPLE_EOK       0   /**< \brief No error */
#define MAPLE_EFAIL     -1  /**< \brief Command failed */
#define MAPLE_EAGAIN    -2  /**< \brief Try again later */
#define MAPLE_EINVALID  -3  /**< \brief Invalid command */
#define MAPLE_ENOTSUPP  -4  /**< \brief Command not suppoted by device */
#define MAPLE_ETIMEOUT  -5  /**< \brief Command timed out */
/** @} */

/**************************************************************************/
/* maple_globals.c */

/** \brief  Global state info.

    Do not manipulate this state yourself, as it will likely break things if you
    do so.
*/
extern maple_state_t maple_state;

/**************************************************************************/
/* maple_utils.c */

/** \brief  Enable the Maple bus.

    This will be done for you autmatically at init time, and there's probably
    not many reasons to be doing this during runtime.
*/
void maple_bus_enable();

/** \brief  Disable the Maple bus.

    There's really not many good reasons to be mucking with this at runtime.
*/
void maple_bus_disable();

/** \brief  Start a Maple DMA.

    This stuff will all be handled internally, so there's probably no reason to
    be doing this yourself.
*/
void maple_dma_start();

/** \brief  Stop a Maple DMA.

    This stuff will all be handled internally, so there's probably no reason to
    be doing this yourself.
*/
void maple_dma_stop();

/** \brief  Is a Maple DMA in progress?

    \return                 Non-zero if a DMA is in progress.
*/
int maple_dma_in_progress();

/** \brief  Set the Maple DMA address.

    Once again, you should not muck around with this in your programs.
*/
void maple_dma_addr(void *ptr);

/** \brief  Return a "maple address" for a port, unit pair.

    \param  port            The port to build the address for.
    \param  unit            The unit to build the address for.
    \return                 The Maple address of the pair.
*/
uint8 maple_addr(int port, int unit);

/** \brief  Decompose a "maple address" into a port, unit pair.

    WARNING: This function will not work with multi-cast addresses!

    \param  addr            The input address.
    \param  port            Output space for the port of the address.
    \param  unit            Output space for the unit of the address.
*/
void maple_raddr(uint8 addr, int * port, int * unit);

/** \brief  Return a string with the capabilities of a given function code.

    This function is not re-entrant, and thus NOT THREAD SAFE.

    \param  functions       The list of function codes.
    \return                 A string containting the capabilities.
*/
const char * maple_pcaps(uint32 functions);

/** \brief  Return a string representing the maple response code.

    \param  response        The response code returned from the function.
    \return                 A string containing a textual respresentation of the
                            response code.
*/
const char * maple_perror(int response);

/** \brief  Determine if a given device is valid.
    \param  p               The port to check.
    \param  u               The unit to check.
    \return                 Non-zero if the device is valid.
*/
int maple_dev_valid(int p, int u);

/** \brief  Enable light gun mode for this frame.

    This function enables light gun processing for the current frame of data.
    Light gun mode will automatically be disabled when the data comes back for
    this frame.

    \param  port            The port to enable light gun mode on.
    \return                 MAPLE_EOK on success, MAPLE_EFAIL on error.
*/
int maple_gun_enable(int port);

/** \brief  Disable light gun mode.

    There is probably very little reason to call this function. Light gun mode
    is ordinarily disabled and is automatically disabled after the data has been
    read from the device. The only reason to call this function is if you call
    the maple_gun_enable() function, and then change your mind during the same
    frame.
*/
void maple_gun_disable(void);

/** \brief  Read the light gun position values.

    This function fetches the gun position values from the video hardware and
    returns them via the parameters. These values are not normalized before
    returning.

    \param  x               Storage for the horizontal position of the gun.
    \param  y               Storage for the vertical position of the gun.

    \note   The values returned from this function are the raw H and V counter
            values from the video hardware where the gun registered its
            position. The values, however, need a bit of massaging before they
            correspond nicely to screen values. The y value is particularly odd
            in interlaced modes due to the fact that you really have half as
            many physical lines on the screen as you might expect.
*/
void maple_gun_read_pos(int *x, int *y);

#if MAPLE_DMA_DEBUG
/* Debugging help */

/** \brief  Setup a sentinel for debugging DMA issues.
    \param  buffer          The buffer to add the sentinel to.
    \param  bufsize         The size of the data in the buffer.
*/
void maple_sentinel_setup(void * buffer, int bufsize);

/** \brief  Verify the presence of the sentine.
    \param  bufname         A string to recognize the buffer by.
    \param  buffer          The buffer to check.
    \param  bufsize         The size of the buffer.
*/
void maple_sentinel_verify(const char * bufname, void * buffer, int bufsize);
#endif

/**************************************************************************/
/* maple_queue.c */

/** \brief  Send all queued frames. */
void maple_queue_flush();

/** \brief  Submit a frame for queueing.

    This will generally be called inside the periodic interrupt; however, if you
    need to do something asynchronously (e.g., VMU access) then it might cause
    some problems. In this case, the function will automatically do locking by
    disabling interrupts temporarily. In any case, the callback will be done
    inside an IRQ context.

    \param  frame           The frame to queue up.
    \retval 0               On success.
    \retval -1              If the frame is already queued.
*/
int maple_queue_frame(maple_frame_t *frame);

/** \brief  Remove a used frame from the queue.

    This will be done automatically when the frame is consumed.

    \param  frame           The frame to remove from the queue.
    \retval 0               On success.
    \retval -1              If the frame is not queued.
*/
int maple_queue_remove(maple_frame_t *frame);

/** \brief  Initialize a new frame to prepare it to be placed on the queue.

    You should call this before you fill in the frame data.

    \param  frame           The frame to initialize.
*/
void maple_frame_init(maple_frame_t *frame);

/** \brief  Lock a frame so that someone else can't use it in the mean time.
    \retval 0               On success.
    \retval -1              If the frame is already locked.
*/
int maple_frame_lock(maple_frame_t *frame);

/** \brief  Unlock a frame. */
void maple_frame_unlock(maple_frame_t *frame);

/**************************************************************************/
/* maple_driver.c */

/** \brief  Register a maple device driver.

    This should be done before calling maple_init().

    \retval 0               On success (no error conditions defined).
*/
int maple_driver_reg(maple_driver_t *driver);

/** \brief  Unregister a maple device driver.
    \retval 0               On success (no error conditions defined).
*/
int maple_driver_unreg(maple_driver_t *driver);

/** \brief  Attach a maple device to a driver, if possible.
    \param  det             The detection frame.
    \retval 0               On success.
    \retval -1              If no driver is available.
*/
int maple_driver_attach(maple_frame_t *det);

/** \brief  Detach an attached maple device.
    \param  p               The port of the device to detach.
    \param  u               The unit of the device to detach.
    \retval 0               On success.
    \retval -1              If the device wasn't valid.
*/
int maple_driver_detach(int p, int u);

/** \brief  For each device which the given driver controls, call the callback.
    \param  drv             The driver to loop through devices of.
    \param  callback        The function to call. The parameter is the device
                            that it is being called on. It should return 0 on
                            success, and <0 on failure.
    \retval 0               On success.
    \retval -1              If any callbacks return <0.
*/
int maple_driver_foreach(maple_driver_t *drv, int (*callback)(maple_device_t *));

/**************************************************************************/
/* maple_irq.c */

/** \brief  Called on every VBL (~60fps).
    \param  code            The ASIC event code.
*/
void maple_vbl_irq_hnd(uint32 code);

/** \brief  Called after a Maple DMA send / receive pair completes.
    \param  code            The ASIC event code.
*/
void maple_dma_irq_hnd(uint32 code);

/**************************************************************************/
/* maple_enum.c */

/** \brief  Return the number of connected devices.
    \return                 The number of devices connected.
*/
int maple_enum_count();

/** \brief  Get a raw device info struct for the given device.
    \param  p               The port to look up.
    \param  u               The unit to look up.
    \return                 The device at that address, or NULL if no device is
                            there.
*/
maple_device_t * maple_enum_dev(int p, int u);

/** \brief  Get the Nth device of the requested type (where N is zero-indexed).
    \param  n               The index to look up.
    \param  func            The function code to look for.
    \return                 The device found, if any. NULL otherwise.
*/
maple_device_t * maple_enum_type(int n, uint32 func);

/** \brief  Return the Nth device that is of the requested type and supports the
            list of capabilities given.

    Note, this only currently makes sense for controllers, since some devices
    don't necessarily use the function data in the same manner that controllers
    do (and controllers are the only devices where we have a list of what all
    the bits mean at the moment).

    \param  n               The index to look up.
    \param  func            The function code to look for.
    \param  cap             Capabilities bits to look for.
    \return                 The device found, if any. NULL otherwise.
*/
maple_device_t * maple_enum_type_ex(int n, uint32 func, uint32 cap);

/** \brief  Get the status struct for the requested maple device.

    This function will wait until the status is valid before returning.
    You should cast to the appropriate type you're expecting.

    \param  dev             The device to look up.
    \return                 The device's status.
*/
void * maple_dev_status(maple_device_t *dev);

/**************************************************************************/
/* maple_init.c */

/** \brief  Initialize Maple.
    \return                 0 on success, <0 on failure.
*/
int maple_init();

/** \brief  Shutdown Maple. */
void maple_shutdown();

/** \brief  Wait for the initial bus scan to complete. */
void maple_wait_scan();

/**************************************************************************/
/* Convienence macros */

/* A "foreach" loop to scan all maple devices of a given type. It is used
   like this:

   MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, st)
    if (st->buttons & CONT_START)
        return -1;
   MAPLE_FOREACH_END()

   The peripheral index can be obtained with __i, and the raw device struct
   with __dev. The code inside the loop is guaranteed to be inside a block
   (i.e., { code })
 */

/** \brief  Begin a foreach loop over Maple devices.

    This macro (along with the MAPLE_FOREACH_END() one) implements a simple
    foreach-style loop over the given type of devices. Essentially, it grabs the
    status of the device, and leaves it to you to figure out what to do with it.

    The most common use of this would be to look for input on any controller.

    \param  TYPE            The function code of devices to look at.
    \param  VARTYPE         The type to cast the return value of
                            maple_dev_status() to.
    \param  VAR             The name of the result of maple_dev_status().
*/
#define MAPLE_FOREACH_BEGIN(TYPE, VARTYPE, VAR) \
    do { \
        maple_device_t  * __dev; \
        VARTYPE * VAR; \
        int __i; \
        \
        __i = 0; \
        while ( (__dev = maple_enum_type(__i, TYPE)) ) { \
            VAR = (VARTYPE *)maple_dev_status(__dev); \
            do {

/** \brief  End a foreach loop over Maple devices.

    Each MAPLE_FOREACH_BEGIN() must be paired with one of these after the loop
    body.
*/
#define MAPLE_FOREACH_END() \
    } while(0); \
    __i++; \
    } \
    } while(0);

__END_DECLS

#endif /* __DC_MAPLE_H */
