/* KallistiOS ##version##

   modem.h
   Copyright (C) 2002, 2004 Nick Kochakian

   Distributed under the terms of the KOS license.

*/

/** \file    dc/modem/modem.h
    \brief   Definitions to use the Dreamcast modem.
    \ingroup modem

    This file contains functions and constants to be used with the Dreamcast
    modem driver.

    \author Nick Kochakian
*/

#ifndef __DC_MODEM_MODEM_H
#define __DC_MODEM_MODEM_H

#include "mconst.h"

/** \defgroup   modem           Modem
    \brief      Driver for the Dreamcast's built-in 56k Modem
    \ingroup    networking_drivers
*/

/** \defgroup   modem_modes     Modes of Operation
    \brief                      Operating modes for modem
    \ingroup    modem

    This group defines the modes that the Dreamcast modem can be in at any given
    point in time.

    @{
*/
/** \brief  Connect to a remote modem. */
#define MODEM_MODE_REMOTE 0

/** \brief  Answer a call when a ring is detected. */
#define MODEM_MODE_ANSWER 1

/** \brief  Modem not in use. Do not attempt to set this mode yourself! */
#define MODEM_MODE_NULL   255
/** @} */

/** \defgroup   modem_v22bis    V.22bis Modes
    \brief                      Modem V.22bis Modes
    \ingroup    modem
    @{
*/
/** \brief  1200bps, V.22bis */
#define MODEM_SPEED_V22BIS_1200 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V22BIS, MODEM_SPEED_1200)

/** \brief  2400bps, V.22bis */
#define MODEM_SPEED_V22BIS_2400 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V22BIS, MODEM_SPEED_2400)
/** @} */

/** \defgroup   modem_v22       V.22 Modes
    \brief                      Modem V.22 Modes
    \ingroup    modem 
    @{
*/
/** \brief  1200bps, V.22 */
#define MODEM_SPEED_V22_1200 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V22, MODEM_SPEED_1200)
/** @} */

/** \defgroup   modem_v32       V.32 Modes
    \brief                      Modem V.32 Modes
    \ingroup    modem
    @{
*/
/** \brief  4800bps, V.32 */
#define MODEM_SPEED_V32_4800 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V32, MODEM_SPEED_4800)

/** \brief  9600bps, V.32 */
#define MODEM_SPEED_V32_9600 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V32, MODEM_SPEED_9600)
/** @} */

/** \defgroup   modem_v32bis    V.32bis Modes
    \brief                      Modem V.32 Modes
    \ingroup    modem
    @{
*/
/** \brief  7200bps, V.32bis */
#define MODEM_SPEED_V32BIS_7200  MODEM_MAKE_SPEED(MODEM_PROTOCOL_V32BIS, MODEM_SPEED_7200)

/** \brief  12000bps, V.32bis */
#define MODEM_SPEED_V32BIS_12000 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V32BIS, MODEM_SPEED_12000)

/** \brief  14400bps, V.32bis */
#define MODEM_SPEED_V32BIS_14400 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V32BIS, MODEM_SPEED_14400)
/** @} */

/** \defgroup   modem_v8        V.8 Modes
    \brief                      Modem V.8 Modes
    \ingroup    modem
    @{
*/
/** \brief  2400bps, V.8 */
#define MODEM_SPEED_V8_2400  MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_2400)

/** \brief  4800bps, V.8 */
#define MODEM_SPEED_V8_4800  MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_4800)

/** \brief  7200bps, V.8 */
#define MODEM_SPEED_V8_7200  MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_7200)

/** \brief  9600bps, V.8 */
#define MODEM_SPEED_V8_9600  MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_9600)

/** \brief  12000bps, V.8 */
#define MODEM_SPEED_V8_12000 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_12000)

/** \brief  14400bps, V.8 */
#define MODEM_SPEED_V8_14400 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_14400)

/** \brief  16800bps, V.8 */
#define MODEM_SPEED_V8_16800 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_16800)

/** \brief  19200bps, V.8 */
#define MODEM_SPEED_V8_19200 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_19200)

/** \brief  21600bps, V.8 */
#define MODEM_SPEED_V8_21600 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_21600)

/** \brief  24000bps, V.8 */
#define MODEM_SPEED_V8_24000 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_24000)

/** \brief  26400bps, V.8 */
#define MODEM_SPEED_V8_26400 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_26400)

/** \brief  28000bps, V.8 */
#define MODEM_SPEED_V8_28000 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_28000)

/** \brief  31200bps, V.8 */
#define MODEM_SPEED_V8_31200 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_31200)

/** \brief  33600bps, V.8 */
#define MODEM_SPEED_V8_33600 MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_33600)

/** \brief  Automatically set speed, V.8 */
#define MODEM_SPEED_V8_AUTO  MODEM_MAKE_SPEED(MODEM_PROTOCOL_V8, MODEM_SPEED_1200)
/** @} */

/** \brief   Modem Event Types.
    \ingroup modem

    These are the events that a modem event handler should be expected to
    receive at any given point in time.
*/
typedef enum {
    /** \brief  The modem tried to establish a connection, but failed.*/
    MODEM_EVENT_CONNECTION_FAILED = 0,

    /** \brief  A connection has been established. */
    MODEM_EVENT_CONNECTED,

    /** \brief  The remote modem dropped the connection. */
    MODEM_EVENT_DISCONNECTED,

    /** \brief  New data has entered the previously empty receive buffer. */
    MODEM_EVENT_RX_NOT_EMPTY,

    /** \brief  The receive buffer overflowed and was cleared. */
    MODEM_EVENT_OVERFLOW,

    /** \brief  The transmit buffer has been emptied. */
    MODEM_EVENT_TX_EMPTY
} modemEvent_t;

/** \brief   Type of a modem event handling function. 
    \ingroup modem
 */
typedef void (*MODEMEVENTHANDLERPROC)(modemEvent_t event);

/* From modem.c */
/** \brief   Initialize the modem.
    \ingroup modem

    This function initializes the modem for use.

    \retval 0               On failure.
    \retval 1               On success.
*/
int           modem_init(void);

/** \brief   Shut down the modem.
    \ingroup modem

    This function shuts down the modem after it has been initialized, resetting
    all of the registers to their defaults.
*/
void          modem_shutdown(void);

/** \brief   Set the modem up for the specified mode.
    \ingroup modem

    This function sets up the modem's registers for the specified mode and speed
    combination.

    \param  mode            The mode to use.
    \param  speed           The speed to use.
    \see    modem_modes
    \see    modem_v22bis
    \see    modem_v22
    \see    modem_v32
    \see    modem_v32bis
    \see    modem_v8
*/
int           modem_set_mode(int mode, modem_speed_t speed);

/** \brief   Wait for the modem to detect a dialtone.
    \ingroup modem

    This function waits for a dialtone to be detected on the modem.

    \param  ms_timeout      The number of milliseconds to wait, in multiples of 100.
    \retval 0               If a dialtone is detected before timeout.
    \retval -1              If no dialtone is detected.
*/
int           modem_wait_dialtone(int ms_timeout);

/** \brief   Dial the specified number on the modem.
    \ingroup modem

    \param  digits          The number to dial, as a string.
    \retval 0               On failure.
    \retval 1               On success.
*/
int           modem_dial(const char *digits);

/** \brief   Set the event handler for the modem.
    \ingroup modem

    This function sets up an event handler for when things happen on the modem.

    \param  eventHandler    The function to call when an event occurs.
*/
void          modem_set_event_handler(MODEMEVENTHANDLERPROC eventHandler);

/** \brief   Disconnect the modem.
    \ingroup modem

    This function instructs the modem to disconnect from the remote modem.
*/
void          modem_disconnect(void);

/** \brief   Check if the modem is connecting.
    \ingroup modem

    \return                 0 if the modem is not currently connecting, non-zero
                            otherwise.
*/
int           modem_is_connecting(void);

/** \brief   Check if the modem is connected.
    \ingroup modem

    \return                 0 if the modem is not currently connected, non-zero
                            otherwise.
*/
int           modem_is_connected(void);

/** \brief   Get the connection rate that the modem is connected at.
    \ingroup modem

    \return                 The connection rate in bits per second.
*/
unsigned long modem_get_connection_rate(void);

/* From mdata.c */
/** \brief   Read data from the modem buffers.
    \ingroup modem

    \param  data            The buffer to read into.
    \param  size            The maximum number of bytes to read.
    \return                 The actual number of bytes read.
*/
int modem_read_data(unsigned char *data, int size);

/** \brief   Write data to the modem buffers.
    \ingroup modem

    \param  data            The buffer to write from.
    \param  size            The maximum number of bytes to write.
    \return                 The actual number of bytes written.
*/
int modem_write_data(unsigned char *data, int size);

/** \brief   Check if the modem has data waiting to be read.
    \ingroup modem

    \return                 0 if no data available, non-zero otherwise.
*/
int modem_has_data(void);

#endif
