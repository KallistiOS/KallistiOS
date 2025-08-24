/* KallistiOS ##version##

   dc/maple/purupuru.h
   Copyright (C) 2003 Megan Potter
   Copyright (C) 2005, 2010 Lawrence Sebald
   Copyright (C) 2025 Donald Haase

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
    \author Donald Haase
*/

#ifndef __DC_MAPLE_PURUPURU_H
#define __DC_MAPLE_PURUPURU_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdbool.h>
#include <stdint.h>
#include <dc/maple.h>

/** \defgroup peripherals_rumble    Rumble Pack
    \brief                          Maple driver for vibration pack peripherals
    \ingroup                        peripherals

    @{
*/

/** \brief  Effect generation structure.

    This structure is used for convenience to send an effect to the jump pack.
    The members in the structure note general explanations of their use as well
    as some limitations and suggestions. There shouldn't be a need to use the
    raw accessor with the new fully specified members.
*/
typedef union purupuru_effect  {
    /** \brief Access the raw 32-bit value to be sent to the puru */
    uint32_t raw;
    /** \brief Deprecated old structure which has been inverted now to union with raw. */
    struct {
        uint8_t special     __depr("Please see purupuru_effect_t which has new members.");
        uint8_t effect1     __depr("Please see purupuru_effect_t which has new members.");
        uint8_t effect2     __depr("Please see purupuru_effect_t which has new members.");
        uint8_t duration    __depr("Please see purupuru_effect_t which has new members.");
    };
    struct {
        /** \brief Continuous Vibration. When set vibration will continue until stopped */
        bool    cont    : 1;
        /** \brief Reserved. Always 0s */
        uint8_t res     : 3;
        /** \brief Motor number. 0 will cause an error. 1 is the typical setting. */
        uint8_t motor   : 4;

        /** \brief Backward direction (- direction) intensity setting bits. 0 stops vibration. */
        uint8_t bpow    : 3;
        /** \brief Divergent vibration. The rumble will get stronger until it stops. */
        bool    div     : 1;
        /** \brief Forward direction (+ direction) intensity setting bits. 0 stops vibration. */
        uint8_t fpow    : 3;
        /** \brief Convergent vibration. The rumble will get weaker until it stops. */
        bool    conv    : 1;

        /** \brief Vibration frequency. for most purupuru 4-59. */
        uint8_t freq;
        /** \brief Vibration inclination period. */
        uint8_t inc;
    };
} purupuru_effect_t;

 /* Compat */
static inline uint32_t __depr("Please see purupuru_effect_t for modern equivalent.") PURUPURU_EFFECT2_UINTENSITY(uint8_t x) {return (x << 4);}
static inline uint32_t __depr("Please see purupuru_effect_t for modern equivalent.") PURUPURU_EFFECT2_LINTENSITY(uint8_t x) {return (x);}
static inline uint32_t __depr("Please see purupuru_effect_t for modern equivalent.") PURUPURU_EFFECT1_INTENSITY(uint8_t x)  {return (x << 4);}

static const uint8_t PURUPURU_EFFECT2_DECAY     __depr("Please see purupuru_effect_t for modern equivalent.") = (8 << 4);
static const uint8_t PURUPURU_EFFECT2_PULSE     __depr("Please see purupuru_effect_t for modern equivalent.") = (8);
static const uint8_t PURUPURU_EFFECT1_PULSE     __depr("Please see purupuru_effect_t for modern equivalent.") = (8 << 4);
static const uint8_t PURUPURU_EFFECT1_POWERSAVE __depr("Please see purupuru_effect_t for modern equivalent.") = (15);
static const uint8_t PURUPURU_SPECIAL_MOTOR1    __depr("Please see purupuru_effect_t for modern equivalent.") = (1 << 4);
static const uint8_t PURUPURU_SPECIAL_MOTOR2    __depr("Please see purupuru_effect_t for modern equivalent.") = (1 << 7);
static const uint8_t PURUPURU_SPECIAL_PULSE     __depr("Please see purupuru_effect_t for modern equivalent.") = (1);

/** \brief  Send an effect to a jump pack.

    This function sends an effect created with the purupuru_effect_t structure
    to a jump pack to be executed.

    \param  dev             The device to send the command to.
    \param  effect          The effect to send.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent. Try again later.
    \retval MAPLE_EINVALID  The command is not being sent due to invalid input.
*/
int purupuru_rumble(maple_device_t *dev, const purupuru_effect_t *effect);

/** \brief  Send a raw effect to a jump pack.

    This function sends an effect to a jump pack to be executed. This is for if
    you want to bypass KOS-based error checking. This is not recommended except
    for testing purposes.

    \param  dev             The device to send the command to.
    \param  effect          The effect to send.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent. Try again later.
*/
int purupuru_rumble_raw(maple_device_t *dev, uint32_t effect);

/* \cond */
/* Init / Shutdown */
void purupuru_init(void);
void purupuru_shutdown(void);
/* \endcond */

/** @} */

__END_DECLS

#endif  /* __DC_MAPLE_PURUPURU_H */

