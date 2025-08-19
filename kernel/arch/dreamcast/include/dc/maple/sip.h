/* KallistiOS ##version##

   dc/maple/sip.h
   Copyright (C) 2005, 2008, 2010, 2013 Lawrence Sebald

*/

/** \file    dc/maple/sip.h
    \brief   Definitions for using the Sound Input Peripheral.
    \ingroup peripherals_mic

    This file contains the definitions needed to access the Maple microphone
    type device (the Seaman mic). Many thanks go out to ZeZu who pointed me
    toward what some of the commands actually do in the original version of this
    driver.

    As a note, the device itself is actually referred to by the system as the
    Sound Input Peripheral, so hence why this driver is named as it is.

    \author Lawrence Sebald
*/

#ifndef __DC_MAPLE_SIP_H
#define __DC_MAPLE_SIP_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <sys/types.h>
#include <dc/maple.h>

/** \defgroup peripherals_mic   Microphone
    \brief                      Maple driver for microphone input devices
    \ingroup                    peripherals

    @{
*/

/** \brief  Type for a microphone sample callback.

    This is the signature that is required for a function to accept samples
    from the microphone as it is sampling. This function will be called about
    once per frame, and in an interrupt context (so it should be pretty quick
    to execute). Basically, all you should do in one of these is copy the
    samples out to your own buffer -- do not do any processing on the samples
    in your callback other than to copy them out!

    \param  dev             The device the samples are coming from.
    \param  samples         Pointer to the sample buffer.
    \param  len             The number of bytes in the sample buffer.

    \headerfile dc/maple/sip.h
*/
typedef void (*sip_sample_cb)(maple_device_t *dev, uint8 *samples, size_t len);

/** \brief  SIP status structure.

    This structure contains information about the status of the microphone
    device and can be fetched with maple_dev_status(). You should not modify
    any of the values in here, it is all "read-only" to your programs. Modifying
    any of this, especially while the microphone is sampling could really screw
    things up.

    \headerfile dc/maple/sip.h
*/
typedef struct  sip_state {
    /** \brief  The gain value for the microphone amp. */
    int             amp_gain;

    /** \brief  The type of samples that are being recorded. */
    int             sample_type;

    /** \brief  What frequency are we sampling at? */
    int             frequency;

    /** \brief  Is the mic currently sampling? */
    int             is_sampling;

    /** \brief  Sampling callback. */
    sip_sample_cb   callback;
} sip_state_t;

/** \brief  Get recorded samples from the microphone device.

    This subcommand is used with the MAPLE_COMMAND_MICCONTROL command to fetch
    samples from the microphone.
*/
#define SIP_SUBCOMMAND_GET_SAMPLES 0x01

/** \brief  Start and stop sampling.

    This subcommand is used with the MAPLE_COMMAND_MICCONTROL command to start
    and stop sampling on the microphone.
*/
#define SIP_SUBCOMMAND_BASIC_CTRL  0x02

/** \brief  Minimum microphone gain. */
#define SIP_MIN_GAIN     0x00

/** \brief  Default microphone gain. */
#define SIP_DEFAULT_GAIN 0x0F

/** \brief  Maximum microphone gain. */
#define SIP_MAX_GAIN     0x1F

/** \brief  Set the microphone's gain value.

    This function sets the gain value of the specified microphone device to
    the value given. This should only be called prior to sampling so as to keep
    the amplification constant throughout the sampling process, but can be
    changed on the fly if you really want to.

    \param  dev             The microphone device to set gain on.
    \param  g               The value to set as the gain.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EINVALID  If g is out of range.
    \see    SIP_MIN_GAIN
    \see    SIP_DEFAULT_GAIN
    \see    SIP_MAX_GAIN
*/
int sip_set_gain(maple_device_t *dev, unsigned int g);

/* Sample types. These two values are the only defined types of samples that
   the SIP can output. 16-bit signed is your standard 16-bit signed samples,
   where 8-bit ulaw is obvously encoded as ulaw. */

/** \brief  Record 16-bit signed integer samples. */
#define SIP_SAMPLE_16BIT_SIGNED 0x00

/** \brief  Record 8-bit ulaw samples. */
#define SIP_SAMPLE_8BIT_ULAW    0x01

/** \brief  Set the sample type to be recorded by the microphone.

    This function sets the sample type that the microphone will return. The
    default value for this is 16-bit signed integer samples. You must call this
    prior to sip_start_sampling() if you wish to change it from the default.

    \param  dev             The microphone device to set sample type on.
    \param  type            The type of samples requested.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EINVALID  If type is invalid.
    \retval MAPLE_EFAIL     If the microphone is sampling.
    \see    SIP_SAMPLE_16BIT_SIGNED
    \see    SIP_SAMPLE_8BIT_ULAW
*/
int sip_set_sample_type(maple_device_t *dev, unsigned int type);

/* Sampling frequencies. The SIP supports sampling at either 8kHz or 11.025 kHz.
   One of these values should be passed to the sip_set_frequency function. */
/** \brief  Record samples at 11.025kHz. */
#define SIP_SAMPLE_11KHZ 0x00

/** \brief  Record samples at 8kHz. */
#define SIP_SAMPLE_8KHZ  0x01

/** \brief  Set the sample frequency to be recorded by the microphone.

    This function sets the sample frequency that the microphone will record. The
    default value for this is about 11.025kHz samples. You must call this prior
    to sip_start_sampling() if you wish to change it from the default.

    \param  dev             The microphone device to set sample type on.
    \param  freq            The type of samples requested.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EINVALID  If freq is invalid.
    \retval MAPLE_EFAIL     If the microphone is sampling.
    \see    SIP_SAMPLE_11KHZ
    \see    SIP_SAMPLE_8KHZ
*/
int sip_set_frequency(maple_device_t *dev, unsigned int freq);

/** \brief  Start sampling on a microphone.

    This function informs a microphone it should start recording samples.

    \param  dev             The device to start sampling on.
    \param  cb              A callback to call when samples are ready.
    \param  block           Set to 1 to wait for the SIP to start sampling.
                            Otherwise check the is_sampling member of the status
                            for dev to know when it has started.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent, try again later.
    \retval MAPLE_EFAIL     If the microphone is already sampling or the
                            callback function is NULL.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
*/
int sip_start_sampling(maple_device_t *dev, sip_sample_cb cb, int block);

/** \brief  Stop sampling on a microphone.

    This function informs a microphone it should stop recording samples.

    \param  dev             The device to stop sampling on.
    \param  block           Set to 1 to wait for the SIP to stop sampling.
                            Otherwise check the is_sampling member of the status
                            for dev to know when it has finished.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent, try again later.
    \retval MAPLE_EFAIL     If the microphone is not sampling.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
*/
int sip_stop_sampling(maple_device_t *dev, int block);

/* \cond */
/* Init / Shutdown */
void sip_init(void);
void sip_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_MAPLE_SIP_H */
