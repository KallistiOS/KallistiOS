/* KallistiOS ##version##

   dc/maple/purupuru.h
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2005, 2010 Lawrence Sebald

*/

/** \file    dc/maple/purupuru.h
    \brief   Definitions for using the Puru Puru (Jump) Pack.
    \ingroup peripherals_rumble

    This file contains the definitions needed to access maple jump pack devices.
    Puru Puru was Sega's internal name for the device, hence why its referred to
    in this way here.

    This driver is largely based off of information provided by Kamjin on the
    DCEmulation forums. See
    http://dcemulation.org/phpBB/viewtopic.php?f=29&t=48462 if you're interested
    in the original documentation.

    Also, its important to note that not all Jump Packs are created equal. Some
    of the stuff in here does not do what it seems like it should on many
    devices. The "decay" setting, for instance, does not seem to work on Sega
    Puru Purus, and actually makes most (if not all) effects do absolutely
    nothing. Basically, its all a big guess-and-test game to get things to work
    the way you might like. Don't be surprised if you manage to set up something
    that does absolutely nothing on the first try.

    \author Lawrence Sebald
*/

#ifndef __DC_MAPLE_PURUPURU_H
#define __DC_MAPLE_PURUPURU_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <dc/maple.h>

/**
 * The following macros are retained for backwards compatibility.
 *  
 * Their original documentation can be found in the corresponding bitfields
 * in the purupuru_effect_t structure.
 * 
 *  */
#define PURUPURU_EFFECT2_UINTENSITY(x) (x << 4)
#define PURUPURU_EFFECT2_LINTENSITY(x) (x)
#define PURUPURU_EFFECT2_DECAY         (8 << 4)
#define PURUPURU_EFFECT2_PULSE         (8)
#define PURUPURU_EFFECT1_INTENSITY(x)  (x << 4)
#define PURUPURU_EFFECT1_PULSE         (8 << 4)
#define PURUPURU_EFFECT1_POWERSAVE     (15)
#define PURUPURU_SPECIAL_MOTOR1        (1 << 4)
#define PURUPURU_SPECIAL_MOTOR2        (1 << 7)
#define PURUPURU_SPECIAL_PULSE         (1)


/** \defgroup peripherals_rumble    Rumble Pack
    \brief                          Maple driver for vibration pack peripherals
    \ingroup                        peripherals

    @{
*/

/** \brief  Effect generation structure.

    This structure is used for convenience to send an effect to the jump pack.
    This, along with the various macros in this file can give a slightly better
    idea of the effect being generated than using the raw values.
*/
typedef union purupuru_effect  {
    uint32_t raw;
    struct {
        /** \brief  Special effects field. */
        uint8 special;

        /** \brief  1st effect field. */
        uint8 effect1;
        
        /** \brief  2nd effect field. */
        uint8 effect2;
        
        /** \brief  The duration of the effect. No idea on units... */
        uint8 duration;
    };
      struct {
    /* Special Effects and motor select. The normal purupuru packs will
only have one motor. Selecting MOTOR2 for these is probably not
a good idea. The PULSE setting here supposably creates a sharp
pulse effect, when ORed with the special field. */

    /** \brief  Yet another pulse effect.
        This supposedly creates a sharp pulse effect.
    */
    uint32_t special_pulse : 1;
    uint32_t : 3; // unused

    /** \brief  Select motor #1.

        Most jump packs only have one motor, but on things that do have more
       than one motor (like PS1->Dreamcast controller adapters that support
       rumble), this selects the first motor.
    */
    uint32_t special_motor1 : 1;
    uint32_t : 2; // unused

    /** \brief  Select motor #2.

        Most jump packs only have one motor, but on things that do have more
       than one motor (like PS1->Dreamcast controller adapters that support
       rumble), this selects the second motor.
    */
    uint32_t special_motor2 : 1;

    /** \brief  Ignore this command.

        Valid value 15 (0xF).

        Most jump packs will ignore commands with this set in effect1,
       apparently.
    */
    uint32_t fx1_powersave : 4;

    /** \brief  Upper nibble of effect1.

        This value works with the lower nibble of the effect2 field to
        increase the intensity of the rumble effect.
        Valid values are 0-7.

        \see    rumble_fields_t.fx2_lintensity
    */
    uint32_t fx1_intensity : 3;

    /** \brief  Give a pulse effect to the rumble.

        This probably should be used with rumble_fields_t.fx1_pulse as well.

        \see    rumble_fields_t.fx2_pulse
    */
    uint32_t fx1_pulse : 1;

    /** \brief  Lower-nibble of effect2.

        This value works with the upper nibble of the effect1
        field to increase the intensity of the rumble effect.
        Valid values are 0-7.

        \see    rumble_fields_t.fx1_intensity
    */
    uint32_t fx2_lintensity : 3;

    /** \brief  Give a pulse effect to the rumble.

        This probably should be used with rumble_fields_t.fx1_pulse as well.

        \see    rumble_fields_t.fx1_intensity
    */
    uint32_t fx2_pulse : 1;

    /** \brief  Upper-nibble of effect2.

        This apparently lowers the rumble's intensity somewhat.
        Valid values are 0-7.
    */
    uint32_t fx2_uintensity : 3;

    /* OR these in with your effect2 value if you feel so inclined.
       if you or the PULSE effect in here, you probably should also
       do so with the effect1 one below. */

    /** \brief  Give a decay effect to the rumble on some packs. */
    uint32_t fx2_decay : 1;

    /* these last bits are covered by duration in the first anonymous struct,
       so not named here */
    uint32_t :8;  
  };
} purupuru_effect_t;


/** \brief  Send an effect to a jump pack.

    This function sends an effect created with the purupuru_effect_t structure
    to a jump pack to be executed.

    \param  dev             The device to send the command to.
    \param  effect          The effect to send.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent. Try again later.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
*/
int purupuru_rumble(maple_device_t *dev, purupuru_effect_t *effect);

/** \brief  Send a raw effect to a jump pack.

    This function sends an effect to a jump pack to be executed. This is for if
    you (for some reason) don't want to use purupuru_effect_t to build the
    effect up.

    \param  dev             The device to send the command to.
    \param  effect          The effect to send.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent. Try again later.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
*/
int purupuru_rumble_raw(maple_device_t *dev, uint32 effect);

/* \cond */
/* Init / Shutdown */
void purupuru_init(void);
void purupuru_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_MAPLE_PURUPURU_H */

