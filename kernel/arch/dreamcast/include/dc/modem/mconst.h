/* KallistiOS ##version##

   mconst.h
   Copyright (C) 2002, 2004 Nick Kochakian

   Distributed under the terms of the KOS license.

*/

/** \file    dc/modem/mconst.h
    \brief   Constants used in the modem driver.
    \ingroup modem

    This file contains constants that are used for the modem driver. You should
    not ever need to include this file directly, as the main modem driver header
    file includes it automatically.

    Generally, you will not need to use the stuff in this file yourself at all,
    as the main modem header file defines many useful combinations for you.

    \author Nick Kochakian
*/

/* Modem constants are defined here. Automatically included by modem.h */
#ifndef __MODEM_MCONST_H
#define __MODEM_MCONST_H

/* Each speed constant contains information about the data rate in bps and the
   protocol that's being used. The first 4 bits identify the the speed that's
   being used, and the last 4 bits identify the protocol.

   And don't try to create your own custom speeds from these, you'll cause
   something very bad to happen. Only use the MODEM_SPEED_* constants defined
   in modem.h! */

/** \defgroup   modem_speeds    Speed Values
    \brief                      Modem speed values
    \ingroup    modem

    This group defines the available speed values that are able to be used with
    the Dreamcast's modem. The actual speed value consists of one of these in
    the lower 4 bits and one of the protocols in the upper 4 bits. Don't try to
    use any speeds not defined here, as bad things may happen.

    It should be fairly obvious from the names what the speeds are (they're all
    expressed in bits per second).

    @{
*/
#define MODEM_SPEED_AUTO  0x0
#define MODEM_SPEED_1200  0x0
#define MODEM_SPEED_2400  0x1
#define MODEM_SPEED_4800  0x2
#define MODEM_SPEED_7200  0x3
#define MODEM_SPEED_9600  0x4
#define MODEM_SPEED_12000 0x5
#define MODEM_SPEED_14400 0x6
#define MODEM_SPEED_16800 0x7
#define MODEM_SPEED_19200 0x8
#define MODEM_SPEED_21600 0x9
#define MODEM_SPEED_24000 0xA
#define MODEM_SPEED_26400 0xB
#define MODEM_SPEED_28000 0xC
#define MODEM_SPEED_31200 0xD
#define MODEM_SPEED_33600 0xE
/** @} */

/** \defgroup   modem_protocols Protocol Values
    \brief                      Modem protocol values
    \ingroup    modem

    This group defines the available protocol values that are able to be used
    with the Dreamcast's modem. The actual speed value consists of one of these
    in the upper 4 bits and one of the speeds in the lower 4 bits. Don't try to
    use any protocols not defined here, as bad things may happen.

    It should be fairly obvious from the names what the protocols that will be
    used are.

    @{
*/
#define MODEM_PROTOCOL_V17    0x0
#define MODEM_PROTOCOL_V22    0x1
#define MODEM_PROTOCOL_V22BIS 0x2
#define MODEM_PROTOCOL_V32    0x3
#define MODEM_PROTOCOL_V32BIS 0x4
#define MODEM_PROTOCOL_V34    0x5
#define MODEM_PROTOCOL_V8     0x6
/** @} */

/** \brief   Extract the protocol from a full speed/protocol value.
    \ingroup modem

    \param  x           The speed/protocol value to look at.
    \return             The protocol in use.
    \see                modem_protocols
*/
#define MODEM_SPEED_GET_PROTOCOL(x) ((modem_speed_t)(x) >> 4)

/** \brief   Extract the speed from a full speed/protocol value.
    \ingroup modem

    \param  x           The speed/protocol value to look at.
    \return             The speed in use.
    \see                modem_speeds
*/
#define MODEM_SPEED_GET_SPEED(x)    ((modem_speed_t)(x) & 0xF)

/** \brief   Combine a protocol and speed into a single value.
    \ingroup modem

    \param  p           The protocol to use.
    \param  s           The speed to use.
    \return             The full speed/protocol value.
    \see                modem_protocols
    \see                modem_speeds
*/
#define MODEM_MAKE_SPEED(p, s)      ((modem_speed_t)((((p) & 0xF) << 4) | ((s) & 0xF)))

/** \brief   Modem speed/protocol value type. 
    \ingroup modem
 */
typedef unsigned char modem_speed_t;

#endif
