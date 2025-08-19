/* KallistiOS ##version##

   dc/maple/dreameye.h
   Copyright (C) 2005, 2009, 2010 Lawrence Sebald

*/

/** \file    dc/maple/dreameye.h
    \brief   Definitions for using the Dreameye Camera device.
    \ingroup peripherals_camera

    This file contains the definitions needed to access the Maple Camera type
    device (aka, the Dreameye). Currently, this driver allows you to download
    the still pictures that are saved on the camera and delete them. It does not
    allow you to use the camera for video input currently.

    \author Lawrence Sebald
*/

#ifndef __DC_MAPLE_DREAMEYE_H
#define __DC_MAPLE_DREAMEYE_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <dc/maple.h>

/** \defgroup peripherals_camera    Camera
    \brief                          Maple driver for the DreamEye peripheral
    \ingroup                        peripherals

    @{
*/

/** \brief  Dreameye status structure.

    This structure contains information about the status of the Camera device
    and can be fetched with maple_dev_status(). You should not change any of
    this information, it should all be considered read-only. Most of the fields
    in here are related to image transfers, and messing with them during a
    transfer could screw things up.

    \headerfile dc/maple/dreameye.h
*/
typedef struct dreameye_state {
    /** \brief  The number of images on the device. */
    int             image_count;

    /** \brief  Is the image_count field valid? */
    int             image_count_valid;

    /** \brief  The number of transfer operations required for the selected
                image. */
    int             transfer_count;

    /** \brief  Is an image transferring now? */
    int             img_transferring;

    /** \brief  Storage for image data. */
    uint8          *img_buf;

    /** \brief  The size of the image in bytes. */
    int             img_size;

    /** \brief  The image number currently being transferred. */
    uint8           img_number;
} dreameye_state_t;

/** \brief  Get the number of images on the device.

    This constant is used with the MAPLE_COMMAND_GETCOND command to fetch the
    number of images on the device.
*/
#define DREAMEYE_GETCOND_NUM_IMAGES     0x81

/** \brief  Get the number of transfers to copy an image.

     This constant is used with the MAPLE_COMMAND_GETCOND command to fetch the
     number of times a transfer command must be sent to get the image specified.
*/
#define DREAMEYE_GETCOND_TRANSFER_COUNT 0x83

/** \brief  Get an image from the device.

    This subcommand is used with the MAPLE_COMMAND_CAMCONTROL command to fetch
    part of image data from the specified image.
*/
#define DREAMEYE_SUBCOMMAND_IMAGEREQ    0x04

/** \brief  Erase an image from the device.

    This subcommand is used with the MAPLE_COMMAND_CAMCONTROL command to remove
    an image from the device.
*/
#define DREAMEYE_SUBCOMMAND_ERASE       0x05

/** \brief  Error return command.

     This subcommand is used by the dreameye with the MAPLE_COMMAND_CAMCONTROL
     command to indicate an error occurred in a subcommand.
*/
#define DREAMEYE_SUBCOMMAND_ERROR       0xFF

/** \brief  Continue transferring an image. */
#define DREAMEYE_IMAGEREQ_CONTINUE      0x00

/** \brief  Start transferring an image from its start. */
#define DREAMEYE_IMAGEREQ_START         0x40

/** \brief  Get the number of images on the Dreameye.

    This function fetches the number of saved images on the specified Dreameye
    device. It can be sent to any of the subdevices of the MAPLE_FUNC_CONTROLLER
    root device of the Dreameye. When the response comes from the device, the
    image_count field of the dreameye_state_t for the specified device will have
    the number of images on the device, and image_count_valid will be set to 1.

    \param  dev             The device to query.
    \param  block           Set to 1 to wait for the Dreameye to respond.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_ETIMEOUT  The command timed out while blocking.
    \retval MAPLE_EAGAIN    Could not send the command to the device, try again.
*/
int dreameye_get_image_count(maple_device_t *dev, int block);

/** \brief  Transfer an image from the Dreameye.

    This function fetches a single image from the specified Dreameye device.
    This function will block, and can take a little while to execute. You must
    use the first subdevice of the MAPLE_FUNC_CONTROLLER root device of the
    Dreameye as the dev parameter.

    \param  dev             The device to get an image from.
    \param  image           The image number to download.
    \param  data            A pointer to a buffer to store things in. This
                            will be allocated by the function and you are
                            responsible for freeing the data when you are done.
    \param  img_sz          A pointer to storage for the size of the image, in
                            bytes.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EFAIL     On error.
*/
int dreameye_get_image(maple_device_t *dev, uint8 image, uint8 **data,
                       int *img_sz);

/** \brief  Erase an image from the Dreameye.

    This function erases the specified image from the Dreameye device. This
    command can be sent to any of the subdevices of the MAPLE_FUNC_CONTROLLER
    root device of the Dreameye.

    \param  dev             The device to erase from.
    \param  image           The image number to erase (0xFF to erase all).
    \param  block           Set to 1 to wait for the Dreameye to respond.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    Couldn't send the command, try again.
    \retval MAPLE_ETIMEOUT  Timeout on blocking.
    \retval MAPLE_EINVALID  Invalid image number specified.
*/
int dreameye_erase_image(maple_device_t *dev, uint8 image, int block);

/* \cond */
/* Init / Shutdown */
void dreameye_init(void);
void dreameye_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_MAPLE_DREAMEYE_H */
