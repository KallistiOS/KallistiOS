/* KallistiOS ##version##

   mintr.c
   Copyright (C) 2002, 2004 Nick Kochakian
   Copyright (C) 2024 Paul Cercueil

   Distributed under the terms of the KOS license.
*/

/* Modem interrupt handlers and interrupt setup
   The G2 interrupt setup code was taken from the LAN adapter code in KOS */
#include <stdlib.h>
#include <dc/asic.h>
#include <arch/rtc.h>

#include <kos/oneshot_timer.h>
#include "dc/modem/modem.h"
#include "mintern.h"

/* Forward declarations */
static void modemIntResetTimeoutTimer(void);
static void modemIntShutdownTimeoutTimer(void);
static void modemIntSetupTimeoutTimer(unsigned int timeout_secs,
                                      void (*callbackCode)(void *));
static void modemIntStartTimeoutTimer(void);

static oneshot_timer_t *oneshot;

/* This controls the code that's executed during a modem generated interrupt */
void (*modemCallbackCode)(void) = NULL;

void mintDelayCallback(void *d) {
    (void)d;

    modemIntResetTimeoutTimer();
    modemIntShutdownTimeoutTimer();

    modemCfg.flags &= ~MODEM_CFG_FLAG_CONNECTING;
    modemCfg.flags |= MODEM_CFG_FLAG_CONNECTED;

    /* Reset the EQM configuration data */
    modemCfg.eqm.flags       = 0;
    modemCfg.eqm.attempts    = 0;
    modemCfg.eqm.counter     = 0;
    modemCfg.eqm.lastCounter = 0;
    modemCfg.eqm.auxCounter  = 0;

    /* Setup the connection interrupts */
    modemIntSetupConnectionInterrupts(0);

    modemDataClearBuffers();

    modemDataUpdateTXBuffer();
    modemDataUpdateRXBuffer();

    if(modemCfg.eventHandler)
        modemCfg.eventHandler(MODEM_EVENT_CONNECTED);
}

/* Aborts any connection that is currently being made */
void modemConnectionAbort(void) {
    modemIntResetControlCode();
    modemDropLine();

    /* Stop the timer if it was being used */
    modemIntShutdownTimeoutTimer();

    if(modemCfg.eventHandler)
        modemCfg.eventHandler(MODEM_EVENT_CONNECTION_FAILED);
}

void mintInitialConnectionTimeoutCallback(void *d) {
    (void)d;

    /* If the modem is in the state MODEM_STATE_CONNECTING and the timeout
       counter goes over a certain value, then it's assumed that nothing useful
       is going to happen and the connection should be aborted */

    /* It only takes about 10 seconds to connect from this state, so this
       should be a long enough time to wait before aborting */

    if(modemCfg.actual.state == MODEM_STATE_CONNECTING)
        modemConnectionAbort();
}

/* This is called after a timer interrupt is generated after a period of one
   second after the answer tone is detected. This is also called by answering
   modes when a connection should be established. */
void modemConnectionAnswerCallback(void *d) {
    (void)d;

    modemIntShutdownTimeoutTimer();

    /* Establish a connection */
    modemEstablishConnection();
    modemCfg.actual.state = MODEM_STATE_CONNECTING;

    /* Reset the state of the signal flag */
    modemCfg.flags &= ~MODEM_CFG_FLAG_SIGNAL;

    /* Set up the counter so that after a period of time, the
       connection can be aborted if no progress is being made */
    modemIntSetupTimeoutTimer(25, mintInitialConnectionTimeoutCallback);
    modemIntStartTimeoutTimer();
}

/* Internal. Called when the connection was dropped by the remote modem. */
void modemDisconnected(void) {
    modemDropLine();

    if(modemCfg.eventHandler)
        modemCfg.eventHandler(MODEM_EVENT_DISCONNECTED);
}

/* Conditions for dropping the line while connected:
   Rate renegotiation requested cleardown (ABCODE), CONF changes to a cleardown
   code, EQM is way too high and the remote modem doesn't respond to retrains,
   or RLSD turns off. */

/* This is an internal helper function for the clock counter updates that are
   used in the EQM and retrain handlers. The primary purpose of this is to
   ensure that an overflow is avoided if the timer sits counting for a while
   without an interrupt happening and having the difference of when the count
   started to when it ended, which has the potential to be large, being added
   to a smaller 8-bit variable. The result of the addition of the two values
   is checked to see if it fits inside of the signed 8-bit data range. If it
   does, then it's stored into the counter, otherwise the value is capped at
   either end. */
void mInternUpdateClockCounter(char *counter, int32 toAdd) {
    /* Add the counter value to the clock difference */
    toAdd += *counter;

    /* Check the result */
    if(toAdd < -128)
        toAdd = -128;
    else if(toAdd > 127)
        toAdd = 127;

    /* Store the value in the counter */
    *counter = (char)toAdd;
}

/* Used by modemConnectedUpdate. Returns a non-zero value when
   modemDisconnected should be called. */
int modemConnectedEQMUpdate(void) {
    time_t        curClock;
    unsigned char flag = 0; /* Set if the MDP reported a high EQM value */

    if(modemRead(REGLOC(0xB)) & 0x1) { /* Check EQMAT */
        modemClearBits(REGLOC(0xB), 0x1);

        /* Don't do anything during a retrain */
        if(!(modemCfg.eqm.flags & EQM_FLAG_RETRAIN)) {
            /* Read the EQM value, but only check the second byte */
            if((unsigned char)(dspRead16(MAKE_DSP_ADDR(0x20C)) >> 8) >= 124)
                flag = 1;

            modemCfg.eqm.flags |= EQM_FLAG_EQMAT;
        }
    }

    /* Don't do anything during a retrain */
    if(!(modemCfg.eqm.flags & EQM_FLAG_RETRAIN)) {
        /* High EQM clock updater. Updates the primary high counter if the high
           EQM flag is set and the aux counter to switch from high to low if the
           EQM is flagged as being high but the MDP doesn't set EQMAT again after
           a few seconds of it first being detected. */
        if(modemCfg.eqm.flags & EQM_FLAG_HIGH) {
            /* Check the time */
            curClock = rtc_unix_secs();

            if(curClock >= modemCfg.eqm.nextClock) {
                if(modemCfg.eqm.flags & EQM_FLAG_EQMAT) {
                    /* EQM high counter update */
                    modemCfg.eqm.auxCounter = 0; /* Reset the high EQM timeout counter */
                    mInternUpdateClockCounter(&modemCfg.eqm.counter,
                                              (curClock - modemCfg.eqm.nextClock) + 1);

                    modemCfg.eqm.flags &= ~EQM_FLAG_EQMAT;

                    /* Wait for about 5 seconds before doing anything about the
                       high EQM value */
                    if(modemCfg.eqm.counter >= 5) {
                        /* Reset the last counter, but keep the current counter so
                           a high to low countdown can be done after the retrain
                           finishes */
                        modemCfg.eqm.lastCounter = 0;

                        /* If the number of consecutive retrain attempts is too
                           high, then the connection is probably bad */
                        if(modemCfg.eqm.attempts >= 2)
                            return 1; /* Abort */

                        modemCfg.eqm.attempts++;
                        modemCfg.eqm.flags &= ~EQM_FLAG_HIGH;

                        /* Start the retrain */
                        modemCfg.eqm.flags |= EQM_FLAG_RETRAIN;

                        modemSetBits(REGLOC(0x8), 0x2); /* Set RTRN */

                        /* Clear SPEED and the value stored for it */
                        modemClearBits(REGLOC(0xE), 0x1F);
                        modemCfg.registers.speed = 0;

                        /* Reset the retrain counter */
                        modemCfg.eqm.auxCounter = 0;
                        modemCfg.eqm.nextClock  = rtc_unix_secs();
                    }
                }
                else {
                    /* EQM high aux counter update */
                    mInternUpdateClockCounter(&modemCfg.eqm.auxCounter,
                                              (curClock - modemCfg.eqm.nextClock) + 1);

                    if(modemCfg.eqm.auxCounter >= 2) {
                        /* The high EQM counter hasn't updated for about two seconds
                           now, so it probably means that the EQM value isn't going
                           to go high anytime soon. */

                        /* Switch from high to low */
                        modemCfg.eqm.lastCounter = modemCfg.eqm.counter;
                        modemCfg.eqm.counter     = 4;
                        modemCfg.eqm.nextClock   = rtc_unix_secs() + 1;

                        modemCfg.eqm.flags &= ~EQM_FLAG_HIGH;
                    }
                }
            }
        }

        /* This part acts as the initiator for triggering the high EQM event
           state. The next part is the low EQM count down, which is performed
           when the EQM is not considered high. */
        if(!(modemCfg.eqm.flags & EQM_FLAG_RETRAIN)) {
            if(flag) { /* The EQM value was found to be high during this call */
                if(!(modemCfg.eqm.flags & EQM_FLAG_HIGH)) {
                    /* This is a new instance of the EQM value being too high */
                    modemCfg.eqm.nextClock  = rtc_unix_secs() + 1;
                    modemCfg.eqm.counter    = modemCfg.eqm.lastCounter;
                    modemCfg.eqm.auxCounter = 0; /* Reset the high to low timeout counter */
                    modemCfg.eqm.flags |= EQM_FLAG_HIGH;
                    modemCfg.eqm.flags &= ~EQM_FLAG_EQMAT; /* Clear this for the new high EQM state */
                }
            }
            else {
                /* The EQM value is no longer being reported high by the MDP */
                if(modemCfg.eqm.counter > 0 &&
                        !(modemCfg.eqm.flags & EQM_FLAG_HIGH)) {
                    /* Check the timer */
                    curClock = rtc_unix_secs();

                    if(curClock >= modemCfg.eqm.nextClock) {
                        /* Update the counter */
                        mInternUpdateClockCounter(&modemCfg.eqm.counter,
                                                  -((curClock - modemCfg.eqm.nextClock) + 1));

                        if(modemCfg.eqm.counter > 0)
                            modemCfg.eqm.nextClock = curClock + 1; /* Keep counting */
                        else {
                            /* The EQM value has been low for a period of time */
                            modemCfg.eqm.attempts    = 0;
                            modemCfg.eqm.lastCounter = 0;
                        }
                    }
                }
            }
        }
    }

    return 0;
}

/* A handler for when a connection has been established */
void modemConnectedUpdate(void) {
    unsigned char data;
    time_t        curClock;

    /* Check ABCODE for errors */
    data = modemRead(REGLOC(0x14));

    if(data) {
        /* Clear ABCODE */
        modemWrite(REGLOC(0x14), 0);

        /* Check the error code to see if it's anything important */
        if(data == 0x96) {
            /* Received rate sequence called for a cleardown */
            modemDisconnected();
            return;
        }
    }

    /* Check for a change in SPEED */
    data = modemRead(REGLOC(0xE)) & 0x1F;

    if(modemCfg.registers.speed != data) {
        /* SPEED has changed */
        modemCfg.registers.speed = data;
        modemCfg.actual.speed    = modemBPSConstants[modemCfg.registers.speed];

        /* If the retrain flag was set, this signals that it has completed */
        modemCfg.eqm.flags &= ~EQM_FLAG_RETRAIN;
    }

    /* Check to see if the CONF register has changed */
    data = modemRead(REGLOC(0x12));

    if(modemCfg.registers.conf != data) {
        /* CONF has changed */
        modemCfg.registers.conf = data;
        modemCreateCInfoFromCC(modemCfg.registers.conf, &modemCfg.cInfo);

        /* Drop the line if CONF is a cleardown code */
        if(data == 0xE0 || data == 0x90 || data == 0xC0 || data == 0x70) {
            modemDisconnected();
            return;
        }
    }

    if(modemCfg.flags & MODEM_CFG_FLAG_CONNECTED) {
        /* Update the retain counter if a retrain is currently taking place */
        if(modemCfg.eqm.flags & EQM_FLAG_RETRAIN) {
            curClock = rtc_unix_secs();

            if(curClock >= modemCfg.eqm.nextClock) {
                mInternUpdateClockCounter(&modemCfg.eqm.auxCounter,
                                          (curClock - modemCfg.eqm.nextClock) + 1);

                if(modemCfg.eqm.auxCounter >= 30) {
                    /* The retrain has been taking place for twenty seconds.
                       That's too long. The modem was probably disconnected, and
                       the MDP will be stuck in an infinite waiting state if
                       nothing is done. */
                    modemDisconnected();
                    return;
                }

                modemCfg.eqm.nextClock = rtc_unix_secs() + 1;
            }
        }

        if(modemRead(REGLOC(0xE)) & 0x80) { /* RTDET */
            /* A retrain was detected from the remote modem */

            /* Make sure that this isn't done more than once per actual
               retrain */
            if(!(modemCfg.eqm.flags & EQM_FLAG_RETRAIN)) {
                modemCfg.eqm.flags |= EQM_FLAG_RETRAIN;

                /* Clear SPEED and the value stored for it */
                modemClearBits(REGLOC(0xE), 0x1F);
                modemCfg.registers.speed = 0;

                /* If the EQM was currently being detected as high, then clear
                   the high flag but save the current counter so that if the EQM
                   does go high again, the counter can resume where it left off. */
                if(modemCfg.eqm.flags & EQM_FLAG_HIGH) {
                    modemCfg.eqm.flags &= ~EQM_FLAG_HIGH;
                    modemCfg.eqm.lastCounter = modemCfg.eqm.counter;
                    modemCfg.eqm.counter     = 0;
                }

                /* Reset the retrain counter */
                modemCfg.eqm.auxCounter = 0;
                modemCfg.eqm.nextClock  = rtc_unix_secs() + 1;
            }
        }

        if(modemConnectedEQMUpdate()) {
            modemDisconnected();
            return;
        }

        modemDataUpdateRXBuffer();
        modemDataUpdateTXBuffer();
    }
}

/*******************************
 * Initial connection handlers *
 *******************************/

/* These internal functions are used for monitoring signals related to the
   state of the initial handshake, and are only used when the MDP is NOT in
   automode.

   The conditions for calling any of the modemConnectionUpdate functions are:
   - The AUTOMODE flag must NOT be set
   - The SIGNAL flag must NOT be set
   - The current state must be MODEM_STATE_CONNECTING
   - The function that's called corresponds to the protocol in the cInfo
     structure
*/

/* Called by the other modemConnectionUpdate functions when the signal flag
   should be set */
void modemConnectionUpdateSignalFlag(void) {
    if(!(modemCfg.flags & MODEM_CFG_FLAG_SIGNAL)) {
        modemSetBits(REGLOC(0x9), 0x4); /* Set DATA */
        /* RTS is set after RLSD turns on */

        modemCfg.flags |= MODEM_CFG_FLAG_SIGNAL;
    }
}

/* For connections that use v.32/v.32 bis */
void modemConnectionUpdateV32(void) {
    if(modemCfg.flags & MODEM_CFG_FLAG_ORIGINATE) {
        /* Originating */
        if(modemRead(REGLOC(0xC)) & 0x40)  /* Wait for ACDET */
            modemConnectionUpdateSignalFlag();
    }
    else {
        /* Answering */
        if(modemRead(REGLOC(0xC)) & 0x80)  /* Wait for AADET */
            modemConnectionUpdateSignalFlag();
    }
}

/* Returns a non-zero value if an error was detected during the initial
   connection sequence that cannot be recovered from */
int modemConnectionErrorHandler(void) {
    unsigned char data;
    int           abort = 0;

    data = modemRead(REGLOC(0x14));

    if(data) {
        /* Clear the error */
        modemWrite(REGLOC(0x14), 0);

        if(data == 0x24) {
            /* Error 0x24: Timed out when waiting for first phase reversal.
               This seems to happen when the line is closed while the connection
               is being established. Abort and go back to waiting at this
               point. */
            abort = 1;
        }

        /* Unless an abort is about to happen, errors that can be recovered from
           reset the timeout counter */
        if(!abort)
            oneshot_timer_reset(oneshot);
    }

    /* Phase state monitoring */
    if(modemCfg.cInfo.protocol == MODEM_PROTOCOL_V8) {
        data = modemRead(REGLOC(0x16)); /* Read SECRXB, which contains the
                                          current phase and phase state numbers
                                          for the handshake */

        /* Check to see if anything has changed */
        if((data & 0xE0) != (modemCfg.actual.phaseState & 0xE0)) {
            /* Phase change */
            if((data & 0xE0) < (modemCfg.actual.phaseState & 0xE0))
                oneshot_timer_reset(oneshot); /* Phase fallback, reset the timeout counter */

            /* Update both the phase and the phase state */
            modemCfg.actual.phaseState = data;
        }

        if((data & 0x1F) != (modemCfg.actual.phaseState & 0x1F)) {
            /* Phase state change */
            if((data & 0x1F) < (modemCfg.actual.phaseState & 0x1F))
                oneshot_timer_reset(oneshot); /* Phase state fallback, reset the timeout counter */

            /* Update the phase state */
            modemCfg.actual.phaseState &= ~0x1F;
            modemCfg.actual.phaseState |= data & 0x1F;
        }
    }

    return abort;
}

/* Connection handler for non-56k modes */
void modemConnection(void) {
    /* Check for any errors generated by the MDP */
    if(modemConnectionErrorHandler()) {
        modemConnectionAbort();
        return; /* Abort */
    }

    switch(modemCfg.actual.state) {
        case MODEM_STATE_RING_WAIT:

            /* For answering modes only. Waits for a ring before
               attempting a connection. */
            if(modemRead(REGLOC(0xF)) & 0x8)
                modemConnectionAnswerCallback(NULL);

            break;

        case MODEM_STATE_CONNECT_WAIT: /* Only used by originating modes */

            /* Was the handshake signal detected? */
            if(modemRead(REGLOC(0xB)) & 0x10) { /* Wait until ATV25 is set */
                /* Need to wait for one second before establishing a
                   connection */
                modemCfg.actual.state = MODEM_STATE_DELAY;

                /* Disable dialing */
                modemCfg.flags |= MODEM_CFG_FLAG_DISABLE_DIALING;

                /* If TDBIE is set at this point it needs to be cleared
                   otherwise the system could lock up. It's pretty bad if
                   it needs to be reset now because it indicates that the
                   handshake started while the modem was still dialing. */
                if(modemRead(REGLOC(0x1E)) & 0x20)
                    modemClearBits(REGLOC(0x1E), 0x20); /* Clear TDBIE */

                /* Setup the timeout timer to delay for 1 second before
                   calling the callback function */
                modemIntSetupTimeoutTimer(1, modemConnectionAnswerCallback);
                modemIntStartTimeoutTimer();
            }

            break;

        case MODEM_STATE_CONNECTING:

            /* Non-automode signal monitoring for the v.32 protocols */
            if(!(modemCfg.flags & MODEM_CFG_FLAG_AUTOMODE) &&
                    !(modemCfg.flags & MODEM_CFG_FLAG_SIGNAL) &&
                    (modemCfg.cInfo.protocol == MODEM_PROTOCOL_V32 ||
                     modemCfg.cInfo.protocol == MODEM_PROTOCOL_V32BIS))
                modemConnectionUpdateV32();

            /* Has the connection completed yet? */
            if(modemRead(REGLOC(0xF)) & 0x80) { /* Wait for RLSD */
                /* Stop the timeout timer that's currently running */
                modemIntShutdownTimeoutTimer();

                if(!(modemRead(REGLOC(0x8)) & 0x1))
                    modemSetBits(REGLOC(0x8), 0x1); /* Set RTS */

                /* Disable any of the protocol specific interrupts that
                   were set before the connection was established */
                modemIntSetupProtocolInterrupts(1);

                /* Update CONF and SPEED */
                modemCfg.registers.conf  = modemRead(REGLOC(0x12));
                modemCfg.registers.speed = modemRead(REGLOC(0xE)) & 0x1F;

                /* Update cInfo to reflect the final connection
                   configuration */
                modemCreateCInfoFromCC(modemCfg.registers.conf,
                                       &modemCfg.cInfo);

                /* Get the connection speed */
                modemCfg.actual.speed = modemBPSConstants[modemCfg.registers.speed];

                /* Delay for 5 seconds before actually going into a
                   connected state */
                modemCfg.actual.state = MODEM_STATE_NULL;
                modemCallbackCode     = modemConnectedUpdate;

                modemIntSetupTimeoutTimer(5, mintDelayCallback);
                modemIntStartTimeoutTimer();
            }

            break;
    }

    /* Handle dialing */
    if(modemCfg.flags & MODEM_CFG_FLAG_DIALING) {
        /* If the interrupt was caused by the transmission buffer being empty,
           then clear the interrupt by writing the next byte of data to the
           MDP */
        if(modemRead(REGLOC(0x1E)) & 0x80)
            modemDataHandleDialingData();
    }
}

static void modemCallback(uint32 code, void *data) {
    (void)code;
    (void)data;

    if(modemCallbackCode != NULL)
        modemCallbackCode();

    /* Acknowledge the interrupt */
    if(modemRead(REGLOC(0x1F)) & 0x80)    /* Check NSIA */
        modemClearBits(REGLOC(0x1F), 0x8); /* Clear NEWS if NSIA was set */
}

void modemIntInit(void) {
    /* Only need to call atexit once */
    if(!(modemInternalFlags & MODEM_INTERNAL_FLAG_INT_INIT_ATEXIT)) {
        atexit(modemIntShutdown);
        modemInternalFlags |= MODEM_INTERNAL_FLAG_INT_INIT_ATEXIT;
    }

    /* Set the default IRQ handler */
    modemCallbackCode = NULL;
    asic_evt_set_handler(ASIC_EVT_EXP_8BIT, modemCallback, NULL);
    asic_evt_enable(ASIC_EVT_EXP_8BIT, ASIC_IRQB);

    /* Oneshot timer will be configured later. */
    oneshot = oneshot_timer_create(NULL, NULL, 0);
}

void modemIntShutdown(void) {
    asic_evt_disable(ASIC_EVT_EXP_8BIT, ASIC_IRQB);
    asic_evt_remove_handler(ASIC_EVT_EXP_8BIT);

    oneshot_timer_destroy(oneshot);
}

#define dspSetClear8(addr, mask, clear)\
    {\
        data = dspRead8(addr);\
        if(!(clear))\
            data |= mask;\
        else\
            data &= ~mask;\
        dspWrite8(addr, data);\
    }

/* Sets or clears (if clear is non-zero) interrupts that only need to be
   enabled once a connection is established or disabled once the connection is
   dropped */
void modemIntSetupConnectionInterrupts(int clear) {
    unsigned char data;

    /* Clear OE if it's set */
    if(modemRead(REGLOC(0xA)) & 0x8)
        modemClearBits(REGLOC(0xA), 0x8);

    /* Setup interrupts for these events in register 0x1:
       RXHF - RX FIFO half full (01h:1)
       TXHF - TX FIFO half full (01h:2) */
    dspSetClear8(MAKE_DSP_ADDR(0x247), 0x6, clear);

    /* RXFNE (0Ch:1) */
    dspSetClear8(MAKE_DSP_ADDR(0x244), 0x2, clear);

    /* Setup interrupts for these events in register 0xA:
       OE - Overrun error (0Ah:3) */
    dspSetClear8(MAKE_DSP_ADDR(0x246), 0x8, clear);

    /* Clear EQMAT if it's set */
    if(modemRead(REGLOC(0xB)) & 0x1)
        modemClearBits(REGLOC(0xB), 0x1);

    /* Setup interrupts for EQMAT */
    dspSetClear8(MAKE_DSP_ADDR(0x245), 0x1, clear);
}

/* Similar to modemIntSetupConnectionInterrupts in that it sets up temporary
   interrupts, but they're disabled after a connection has been established */
void modemIntSetupProtocolInterrupts(int clear) {
    unsigned char data;

    /* Setup protocol specific interrupts that are only set temporarily */
    if(modemCfg.cInfo.protocol == MODEM_PROTOCOL_V8) {
        /* Enable interrupts for SECRXB to monitor the handshake. This will be
           disabled after the connection has completed. */
        dspWrite8(MAKE_DSP_ADDR(0x370), clear ? 0 : 0xFF);
    }
    else if(modemCfg.cInfo.protocol == MODEM_PROTOCOL_V32 ||
            modemCfg.cInfo.protocol == MODEM_PROTOCOL_V32BIS) {
        if(!(modemCfg.flags & MODEM_CFG_FLAG_AUTOMODE)) {
            if(modemCfg.flags & MODEM_CFG_FLAG_ORIGINATE) {
                /* Enable interrupts for these events in register 0xC:
                   ACDET - Indicates that a V.32 bis/V.32 AC sequence has
                           been detected (0Ch:6) */
                dspSetClear8(MAKE_DSP_ADDR(0x244), 0x40, clear);
            }
            else {
                /* Enable AADET in register 0xC */
                dspSetClear8(MAKE_DSP_ADDR(0x244), 0x80, clear);
            }
        }
    }
}

/* Communicates with the modem's DSP to mask out some events on which a
   hardware interrupt should be generated. This also disables the default
   IRQ on completion of a DSP read or write. */
void modemIntConfigModem(void) {
    modemIntResetControlCode();

    /* Disable memory access interrupts by setting bit 6 */
    dspSetBits8(MAKE_DSP_ADDR(0x89), 0x40);

    /* Enable interrupts for these events in register 0xF:
       RLSD - Set when the MDP has finished the connection sequence
       RI   - Ring indicator
       DSR  - Data set ready */
    dspSetBits8(MAKE_DSP_ADDR(0x241), 0x98);

    /* Enable interrupts for these events in register 0xB:
       TONEA \
       TONEB  | Tone detectors used for detecting dial tones, etc.
       TONEC /
       ATV25  - Answer tone detection */
    dspSetBits8(MAKE_DSP_ADDR(0x245), 0xF0);

    /* Enable interrupts for these events in register 0xE:
       SPEED - Speed indication (0Eh:0-4)
       RTDET - Retrain detected */
    dspSetBits8(MAKE_DSP_ADDR(0x242), 0x9F);

    /* Enable ABCODE interrupts for register 0x14 */
    dspWrite8(MAKE_DSP_ADDR(0x38A), 0xFF);

    /* TXFNF (0Dh:1) */
    dspSetBits8(MAKE_DSP_ADDR(0x243), 0x2);

    /* Set default values for some of the variables that the interrupts
       use */
    modemCfg.actual.state = MODEM_STATE_NULL;

    /* Set NSIE and NEWS */
    modemSetBits(REGLOC(0x1F), 0x18);
}

/* Sets the interrupt handler and state based on the specified mode of
   operation */
void modemIntSetHandler(int protocol, int mode) {
    (void)protocol;

    modemCfg.flags &= ~MODEM_CFG_FLAG_CONNECTED;
    modemCfg.flags |= MODEM_CFG_FLAG_CONNECTING;

    modemCfg.actual.phaseState = 0;
    modemCallbackCode          = modemConnection;

    switch(mode) {
        case MODEM_MODE_REMOTE:
            modemCfg.actual.state = MODEM_STATE_CONNECT_WAIT;
            break;

        case MODEM_MODE_ANSWER:
            modemCfg.actual.state = MODEM_STATE_RING_WAIT;
            break;
    }
}

void modemIntResetControlCode(void) {
    /* Reset the interrupt handler's control code */
    modemCallbackCode = NULL;
}

static void modemIntSetupTimeoutTimer(unsigned int timeout_secs,
                                      void (*callbackCode)(void *)) {
    oneshot_timer_stop(oneshot);
    oneshot_timer_setup(oneshot, callbackCode, NULL, timeout_secs * 1000);

    /* It doesn't matter if this is called more than once consecutively even
       if the timer is still running */
    modemInternalFlags |= MODEM_INTERNAL_FLAG_TIMER_HANDLER_SET;
    modemInternalFlags &= ~MODEM_INTERNAL_FLAG_TIMER_RUNNING;
}

static void modemIntStartTimeoutTimer(void) {
    if(!(modemInternalFlags & MODEM_INTERNAL_FLAG_TIMER_RUNNING)) {
        modemInternalFlags |= MODEM_INTERNAL_FLAG_TIMER_RUNNING;
        oneshot_timer_start(oneshot);
    }
}

static void modemIntResetTimeoutTimer(void) {
    if(modemInternalFlags & MODEM_INTERNAL_FLAG_TIMER_RUNNING) {
        oneshot_timer_stop(oneshot);
        modemInternalFlags &= ~MODEM_INTERNAL_FLAG_TIMER_RUNNING;
    }
}

static void modemIntShutdownTimeoutTimer(void) {
    /* Stop the timer if it's still running */
    modemIntResetTimeoutTimer();

    if(modemInternalFlags & MODEM_INTERNAL_FLAG_TIMER_HANDLER_SET) {
        modemInternalFlags &= ~MODEM_INTERNAL_FLAG_TIMER_HANDLER_SET;
    }
}

/********************************************************************/
/* Internal dialing interrupt functions */

/* Setup or clear the transmission buffer interrupts. These are used when to
   control the flow of data to the MDP while dialing. */
void modemInternalSetupDialingInts(int clear) {
    if(!clear)
        modemSetBits(REGLOC(0x1E), 0x20); /* Set TDBIE */
    else
        modemClearBits(REGLOC(0x1E), 0x20); /* Clear TDBIE */
}
