/* KallistiOS ##version##

   modem.c
   Copyright (C)2002, 2004 Nick Kochakian
   Copyright (C) 2024 Paul Cercueil

   Distributed under the terms of the KOS license.
*/

#include <kos.h>
#include <dc/g2bus.h>
#include <stdlib.h>
#include <assert.h>
#include "dc/modem/modem.h"
#include "mintern.h"

#define MODEM_TIMEOUT_MS 215

void modemHardReset(void);
void modemSoftReset(void);
void modemConfigurationReset(void); /* Configures the modem. It's assumed that
                                       the modem has already been reset. */

MODEM_CFG modemCfg = {
    0, 0,
    { 0, 0, 0 },
    NULL,
    { 0, MODEM_STATE_NULL, 0 },
    { 0, 0 },
    { 0, 0, 0, 0, 0, 0 }
};

/* Internal flags shared to keep track of parts of the driver's state */
unsigned char modemInternalFlags = 0;

/* Converts a protocol/speed constant to a value that can be used with the
   CONF register to configure the modem. A value of zero is equal to an
   undefined speed.

   This only covers configuration codes for v.17, v.22, v.22 bis, v.32, and
   v.32 bis protocols. */
const unsigned char modemSpeedConfig[7][15] = {
    /* V.17 protocol */
    /* 1200  2400  4800  7200  9600  12000 14400 16800 19200 21600 */
    {
        0,    0,    0,    0xB8, 0xB4, 0xB2, 0xB1, 0,    0,    0,

        /* 24000 26400 28000 31200 33600 */
        0,    0,    0,    0,    0
    },

    /* V.22 protocol */
    /* 1200  2400  4800  7200  9600  12000 14400 16800 19200 21600 */
    {
        0x52, 0,    0,    0,    0,    0,    0,    0,    0,    0,

        /* 24000 26400 28000 31200 33600 */
        0,    0,    0,    0,    0
    },

    /* V.22 bis protocol */
    /* 1200  2400  4800  7200  9600  12000 14400 16800 19200 21600 */
    {
        0x82, 0x84, 0,    0,    0,    0,    0,    0,    0,    0,

        /* 24000 26400 28000 31200 33600 */
        0,    0,    0,    0,    0
    },

    /* V.32 protocol */
    /* 1200  2400  4800  7200  9600  12000 14400 16800 19200 21600 */
    {
        0,    0,    0x71, 0,    0x75, 0,    0,    0,    0,    0,

        /* 24000 26400 28000 31200 33600 */
        0,    0,    0,    0,    0
    },

    /* V.32 bis protocol */
    /* 1200  2400  4800  7200  9600  12000 14400 16800 19200 21600 */
    {
        0,    0,    0,    0x78, 0,    0x72, 0x76, 0,    0,    0,

        /* 24000 26400 28000 31200 33600 */
        0,    0,    0,    0,    0
    },
};

/* Converts the value found in the SPEED register (0Eh:4-0) to a numerical
   value in bits per second (bps.) */
const unsigned short modemBPSConstants[31] = {
    300,   600,   1200,  2400,  4800,  9600,  12000, 14400, 7200,  16800,
    19200, 21600, 24000, 26400, 28800, 31200, 33600, 0,     0,     0,
    0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    0
};

/* Converts a MODEM_SPEED_* constant into a value in bits per second */
__inline static unsigned short speedCodeToBPS(unsigned char speed) {
    if(speed == MODEM_SPEED_1200)
        return 1200;

    return (unsigned short)speed * 2400;
}

/* Sets the members of a MODEM_CINFO structure based on the defined protocol
   and speed values */
void modemCreateCInfoFromPS(const unsigned char protocol,
                            const unsigned char speed, MODEM_CINFO *cInfo) {
    assert(speed <= 15);

    cInfo->protocol = protocol;
    cInfo->speed    = speed;
    cInfo->bps      = speedCodeToBPS(cInfo->speed);
}

/* Sets the members of a MODEM_CINFO structure based on a configuration code
   from CONF */
void modemCreateCInfoFromCC(const unsigned char cc, MODEM_CINFO *cInfo) {
    /*
       v.17     -> 0xB1 - 0xB2, 0xB4, and 0xB8
       v.22     -> 0x52
       v.22 bis -> 0x82 and 0x84
       v.32     -> 0x71 and 0x75
       v.32 bis -> 0x72, 0x76, and 0x78
       v.34     -> 0xC1 - 0xCE
    */

    if((cc >= 0xB1 && cc <= 0xB4) || cc == 0xB8) {
        /* v.17 */
        cInfo->protocol = MODEM_PROTOCOL_V17;

        if(cc != 0xB8) {
            /* 0xB1 - 0xB4 = 14400 - 9600 bps */
            cInfo->speed = MODEM_SPEED_14400 - ((cc - 0xB0) >> 1); /* cc = [0, 2] */
        }
        else {
            /* 0xB8 = 7200 bps */
            cInfo->speed = MODEM_SPEED_7200;
        }
    }
    else if(cc == 0x52) {
        /* v.22 */
        cInfo->protocol = MODEM_PROTOCOL_V22;
        cInfo->speed    = MODEM_SPEED_1200;
    }
    else if(cc == 0x82 || cc == 0x84) {
        /* v.22 bis */
        cInfo->protocol = MODEM_PROTOCOL_V22BIS;
        cInfo->speed    = (cc == 0x82) ? MODEM_SPEED_1200 :
                          MODEM_SPEED_2400;
    }
    else if(cc == 0x71 || cc == 0x75) {
        /* v.32 */
        cInfo->protocol = MODEM_PROTOCOL_V32;
        cInfo->speed    = (cc == 0x71) ? MODEM_SPEED_4800 :
                          MODEM_SPEED_9600;
    }
    else if(cc == 0x72 || cc == 0x76 || cc == 0x78) {
        /* v.32 bis */
        cInfo->protocol = MODEM_PROTOCOL_V32BIS;

        if(cc == 0x72)
            cInfo->speed = MODEM_SPEED_12000;
        else if(cc == 0x76)
            cInfo->speed = MODEM_SPEED_14400;
        else
            cInfo->speed = MODEM_SPEED_7200;
    }
    else if(cc >= 0xC1 && cc <= 0xCE) {
        /* v.34 */
        cInfo->protocol = MODEM_PROTOCOL_V34;
        cInfo->speed    = cc - 0xC0;
    }
    else {
        cInfo->protocol = 0;
        cInfo->speed    = 0;
    }

    /* Translate the speed code into bits per second */
    cInfo->bps = speedCodeToBPS(cInfo->speed);
}

/* Gets the closest possible configuration code based on the values specified
   by the protocol and data rate */
unsigned char modemGetCCFromProtocolAndDataRate(const unsigned char protocol,
        const unsigned short bps) {
    unsigned char cc = 0;

    switch(protocol) {
        case MODEM_PROTOCOL_V17:

            if(bps <= 7200)  /* Min */
                cc = 0xB8;   /* 7200 bps */
            else if(bps >= 14400)  /* Max */
                cc = 0xB1;     /* 14400 bps */
            else {
                if(bps == 9600)
                    cc = 0xB4;
                else
                    cc = 0xB2; /* 12000 bps */
            }

            break;

        case MODEM_PROTOCOL_V22:
            cc = 0x52; /* 1200 bps */
            break;

        case MODEM_PROTOCOL_V22BIS:

            if(bps <= 1200)
                cc = 0x82;
            else
                cc = 0x84; /* 2400 bps */

            break;

        case MODEM_PROTOCOL_V32:

            if(bps <= 4800)
                cc = 0x71;
            else
                cc = 0x75; /* 9600 bps */

            break;

        case MODEM_PROTOCOL_V32BIS:

            if(bps <= 7200)
                cc = 0x78;
            else if(bps >= 14400)
                cc = 0x76;
            else
                cc = 0x72; /* 12000 bps */

            break;

        case MODEM_PROTOCOL_V34:
            cc = (bps / 2400) + 0xC0;

            if(cc < 0xC1)
                cc = 0xC1;
            else if(cc > 0xCE)
                cc = 0xCE;

            break;
    }

    assert(cc);

    return cc;
}

/* Verifys if the controller data returned as a result of a reset is valid
   or not. Returns zero if the results are invalid other a non zero number
   is returned. */
__inline static int verifyControllerData(unsigned short *ctrlInfo) {
    /* RAM1 and RAM2 Checksums */
    if((ctrlInfo[0] == 0xEA3C || ctrlInfo[0] == 0x451) &&
            (ctrlInfo[1] == 0x5536 || ctrlInfo[1] == 0x49A5)) {
        /* ROM1 and ROM2 Checksums */
        if(ctrlInfo[2] == 0x5F4C &&
                (ctrlInfo[3] == 0x3835 || ctrlInfo[3] == 0x3836)) {
            /* Timer/ROM/RAM */
            if(ctrlInfo[4] == 0x801 || ctrlInfo[4] == 0xDE00) {
                /* Part Number and Revision Level */
                if((ctrlInfo[5] == 0x3730 || ctrlInfo[5] == 0x3731) &&
                        ctrlInfo[6] == 0x4241)
                    return 1;
            }
        }
    }

    return 0;
}

/* Does the same thing as verifyControllerData but checks to see if the DSP
   values from the DSP self test are valid. */
__inline static int verifyDSPData(unsigned short *dspInfo) {
    /* EC Checksum */
    if(dspInfo[0] == 0xF083 || dspInfo[0] == 0xA577) {
        /* Multiplier Checksum */
        if(dspInfo[1] == 0x46EE) {
            /* RAM and ROM Checksums */
            if((dspInfo[2] == 0xFA || dspInfo[2] == 0xB482) &&
                    (dspInfo[3] == 0xA09 || dspInfo[3] == 0x3836)) {
                /* Part Number and Revision Level */
                if((dspInfo[4] == 0x3730 || dspInfo[4] == 0x3631) &&
                        dspInfo[5] == 0x2041)
                    return 1;
            }
        }
    }

    return 0;
}

/* Initiates a modem self test and determines if a modem is present or not.
   This only needs to be called ONCE during the modemInit function. */
int modemSelfTest(void) {
    int            i;
    unsigned int   ms;
    unsigned short ctrlInfo[7];
    unsigned short dspInfo[6];

#define readValue16(ah, al) ((modemRead(ah) << 8) | (modemRead(al)))

    /*
       Data is read in this order. Expected values are also listed.
       The modem that this was tested on got all of the expected values that
       appear first.

       Controller info
       ---------------
       RAM1 Checksum  = 0xEA3C or 0x451
       RAM2 Checksum  = 0x5536 or 0x49A5
       ROM1 Checksum  = 0x5F4C
       ROM2 Checksum  = 0x3835 or 0x3836
       Timer/ROM/RAM  = 0x801 or 0xDE00
       Part Number    = 0x3730 or 0x3731
       Revision Level = 0x4241

       DSP info
       --------
       EC Checksum         = 0xF083 or 0xA577
       Multiplier Checksum = 0x46EE
       RAM Checksum        = 0xFA or 0xB482
       ROM Checksum        = 0xA09 or 0x3836
       Part Number         = 0x3730 or 0x3631
       Revision Level      = 0x2041

       For detection, it should be safe to assume that if either the
       controller test data or the DSP test data match there's a modem
       present. If both do not match, or more likely a timeout occurs, then
       there's no modem present.
    */

    /* Soft reset */
    modemSetBits(REGLOC(0x1A), 0x80);
    modemSetBits(REGLOC(0x1F), 0x1);

    /* It seems to work better if the registers are read two times rather
       than waiting for the two RAM checksums to appear as the manual
       suggests. */

    for(i = 0; i < 2; i++) {
        /* Controller info */
        ctrlInfo[0] = readValue16(REGLOC(0x1D), REGLOC(0x1C));
        ctrlInfo[1] = readValue16(REGLOC(0x1B), REGLOC(0x1A));
        ctrlInfo[2] = readValue16(REGLOC(0x19), REGLOC(0x18));
        ctrlInfo[3] = readValue16(REGLOC(0x17), REGLOC(0x16));
        ctrlInfo[4] = readValue16(REGLOC(0x15), REGLOC(0x14));
        ctrlInfo[5] = readValue16(REGLOC(0x13), REGLOC(0x12));
        ctrlInfo[6] = readValue16(REGLOC(0x11), REGLOC(0x0));

        /* This resets bit 3 in register 0x1E */
        modemRead(REGLOC(0x10));

        for(ms = 0; ms < MODEM_TIMEOUT_MS; ms++) {
            if(modemRead(REGLOC(0x1E)) & 0x8)
                break;

            thd_sleep(1);
        }

        if(ms == MODEM_TIMEOUT_MS) {
            /* Too late */
            return 0;
        }
    }

    /* Now for the DSP self test */

    /* DSP info */
    dspInfo[0] = readValue16(REGLOC(0x1B), REGLOC(0x1A));
    dspInfo[1] = readValue16(REGLOC(0x19), REGLOC(0x18));
    dspInfo[2] = readValue16(REGLOC(0x17), REGLOC(0x16));
    dspInfo[3] = readValue16(REGLOC(0x15), REGLOC(0x14));
    dspInfo[4] = readValue16(REGLOC(0x13), REGLOC(0x12));
    dspInfo[5] = readValue16(REGLOC(0x11), REGLOC(0x0));

    /* Wait for a bit */
    for(ms = 0; ms < MODEM_TIMEOUT_MS; ms++) {
        if(!(modemRead(REGLOC(0x1F)) & 0x1))
            break;

        thd_sleep(1);
    }

    thd_sleep(10); /* Wait for 10ms before accessing the MDP again */

    if(ms == MODEM_TIMEOUT_MS) {
        /* No modem, I guess. It timed out, so that might mean something. */
        return 0;
    }

    if(verifyControllerData(ctrlInfo) || verifyDSPData(dspInfo))
        return 1; /* Modem detected */

    /* No modem detected */
    return 0;
}

/* Modem initialization
   Does a self test to detect the presence of a modem, then
   resets the modem and sets up the default interrupt handler
   and buffers if one was found

   Returns a non-zero value on success */
int modem_init(void) {
    if(modemCfg.inited)
        return 1;

    modemHardReset();

    if(!modemSelfTest())
        return 0;

    /* Only need to call atexit once */
    if(!(modemInternalFlags & MODEM_INTERNAL_FLAG_INIT_ATEXIT)) {
        atexit(modem_shutdown);
        modemInternalFlags |= MODEM_INTERNAL_FLAG_INIT_ATEXIT;
    }

    modemDataSetupBuffers();

    modemIntInit();
    modemConfigurationReset();

    modemCfg.eventHandler = NULL;
    modemCfg.inited       = 1;

    return 1;
}

void modem_shutdown(void) {
    if(!modemCfg.inited)
        return;

    modemHardReset();
    modemDataDestroyBuffers();

    modemCfg.inited = 0;
}

void modemHardReset(void) {
    /* This zeroes out all of the modem's registers */
    modemWrite(G2_8BP_RST, 0);

    timer_spin_sleep(25); /* A slight delay just to be safe */

    /* This sets the modem's registers to their default settings */
    modemWrite(G2_8BP_RST, 1);

    /* Wait for a little while so the modem has time to reset itself
       completely */
    timer_spin_sleep(150);
}

void modemSoftReset(void) {
    modemSetBits(REGLOC(0x1A), 0x80); /* Set SFRES */
    modemSetBits(REGLOC(0x1F), 0x1);  /* Set NEWC */

    while(modemRead(REGLOC(0x1F)) & 0x1);  /* Wait for NEWC to clear */

    /* Wait a minimum of 10ms before using the MDP again */
    timer_spin_sleep(100);
}

void modemConfigurationReset(void) {
    /* Reset the configuration */
    modemCfg.flags = 0;

    /* Configure modem interrupt events */
    modemIntConfigModem();
}

void modem_reset(void) {
    modemSoftReset();
    modemConfigurationReset();
    modemDataClearBuffers();
}

/* Sets the v.8 configuration registers and the v.34 bit mask to their maximum
   values */
void modemSetV8ModeMax(void) {
    unsigned short mask;

    /* Read and set the v.34 mask first */
    mask = dspRead16x2(MAKE_DSP_ADDR(0x382), MAKE_DSP_ADDR(0x383));

    /* Setting all the bits except the last two tells the remote modem (if
       v.34 is used) that every speed that's possible under this protocol
       is available */
    mask |= 0x3FFF;

    /* Write the mask back */
    dspWrite16x2(MAKE_DSP_ADDR(0x382), MAKE_DSP_ADDR(0x383), mask);

    /* Set the v.8 configuration register to configure the v.34 full duplex
       mode for it's highest setting. The other configuration registers are
       set to their maximums by default. */
    dspWrite8(MAKE_DSP_ADDR(0x309), 0xCE);
}

/* Sets the v.8 configuration registers and v.34 bit mask to closely correspond
   to the specified maximum data rate */
void modemSetV8ModeDesired(const unsigned short bps) {
    unsigned short mask;
    int            i;

    assert(bps >= 2400 && bps <= 33600);

    /* Set the v.34 bit mask first */
    mask = dspRead16x2(MAKE_DSP_ADDR(0x382), MAKE_DSP_ADDR(0x383));

    /* Clear bits 0 - 13 */
    mask &= ~0x3FFF;

    /* Set bits, starting at bit 0, until the corresponding bit for the
       specified data rate is set. Setting the bits in this way prevents
       the remote modem from trying to renegotiate the connection speed,
       which puts garbage into the receive buffers on both ends. */
    i = 0;

    while(i <= 13 && 2400 * (i + 1) <= bps) {
        mask |= 1 << i;
        i++;
    }

    /* Write the mask back */
    dspWrite16x2(MAKE_DSP_ADDR(0x382), MAKE_DSP_ADDR(0x383), mask);

    /* Now set the v.8 configuration registers */
    dspWrite8(MAKE_DSP_ADDR(0x309),
              modemGetCCFromProtocolAndDataRate(MODEM_PROTOCOL_V34, bps));
    dspWrite8(MAKE_DSP_ADDR(0x30B),
              modemGetCCFromProtocolAndDataRate(MODEM_PROTOCOL_V32BIS, bps));
    dspWrite8(MAKE_DSP_ADDR(0x30C),
              modemGetCCFromProtocolAndDataRate(MODEM_PROTOCOL_V22BIS, bps));
    dspWrite8(MAKE_DSP_ADDR(0x30D),
              modemGetCCFromProtocolAndDataRate(MODEM_PROTOCOL_V17, bps));
}

/* Changes modes of operation for the modem */
int modem_set_mode(int mode, modem_speed_t speed) {
    if(mode != MODEM_MODE_REMOTE && mode != MODEM_MODE_ANSWER)
        return 0;

    modem_reset();

    /* The REMOTE and ANSWER modes need to start off with RA (Relay A) not
       active. Make sure it's disabled. */
    if(modemRead(REGLOC(0x7)) & 0x2)
        modemClearBits(REGLOC(0x7), 0x2);

    /* Enable the transmitter FIFO */
    modemSetBits(REGLOC(0x4), 0x10); /* Set FIFOEN */

    /* Set the character size to 8 bits */
    modemSetBits(REGLOC(0x6), 0x3); /* Set both WDSZ bits */

    /* Disable parity and use one stop bit */
    modemClearBits(REGLOC(0x6), 0xC); /* Clear PEN and STB */

    /* TDBE, Transmit Data Buffer Empty, MUST be set before setting ASYN..
       Hopefully this is not a problem since the modem was recently reset. */
    assert(modemRead(REGLOC(0x1E)) & 0x8);

    /* To enter ASYNC mode, TPDM must be set and RTS must be clear */

    /* Turn Transmitter Parallel Data Mode on */
    modemSetBits(REGLOC(0x8), 0x40); /* Set TPDM */

    modemClearBits(REGLOC(0x8), 0x1); /* Clear RTS */

    /* Set the ASYN (ASYNC) bit */
    modemSetBits(REGLOC(0x8), 0x80);

    switch(mode) {
        case MODEM_MODE_REMOTE:
            /* This procedure is described on page 117 of the PDF. */

            /* Enable the tone detectors */
            modemSetBits(REGLOC(0x2), 0x80); /* Set TDE */

            /* Open the line */
            modemSetBits(REGLOC(0x7), 0x2); /* Set RA */

            /* Disable the DATA bit so that the hardware interrupts will
               work as expected. It also doesn't hurt to clear the DTR
               bit. */
            modemClearBits(REGLOC(0x9), 0x5); /* Clear DATA and DTR */

            /* Set the dialing mode */
            modemWrite(REGLOC(0x12), 0x81); /* Sets CONF */

            /* Notify the modem that the configuration has changed */
            modemSetBits(REGLOC(0x1F), 0x1); /* Set NEWC */

            /* Delay at least 4ms */
            timer_spin_sleep(10);

            /* Dial using DTMF tones, and set the modem in origination
               mode */
            modemSetBits(REGLOC(0x9), 0x30); /* Set DTMF and ORG */

            /* Notify the modem that the configuration has changed */
            modemSetBits(REGLOC(0x1F), 0x1); /* Set NEWC */

            /* The modem is originating. Set the origination flag to
               indicate this. */
            modemCfg.flags |= MODEM_CFG_FLAG_ORIGINATE;
            break;

        case MODEM_MODE_ANSWER:
            /* Clear ORG. This mode answers and does not originate */
            modemClearBits(REGLOC(0x9), 0x10);

            modemSetBits(REGLOC(0x1F), 0x1); /* Set NEWC */

            /* Clear the DATA and DTR bits. The modem is waiting for a
               call, not answering one at this point. */
            modemClearBits(REGLOC(0x9), 0x5);

            /* Set the configuration for v.23 receive mode at 1200 bps.
               This will be used for caller ID later. The desired mode
               is set before a connection is attempted. */
            modemWrite(REGLOC(0x12), 0xA4);

            /* Set NEWC */
            modemSetBits(REGLOC(0x1F), 0x1);
            break;
    }

    modemCreateCInfoFromPS(MODEM_SPEED_GET_PROTOCOL(speed),
                           MODEM_SPEED_GET_SPEED(speed), &modemCfg.cInfo);

    if(modemCfg.cInfo.protocol == MODEM_PROTOCOL_V8) {
        if(mode == MODEM_MODE_REMOTE) {
            /* Configure for origination mode */
            dspWrite8(MAKE_DSP_ADDR(0x312), 0xA1); /* v.23 full duplex mode */
            dspWrite8(MAKE_DSP_ADDR(0x313), 0xA1); /* v.23 half duplex mode */
        }
        else {
            /* Configure for answer mode */
            dspWrite8(MAKE_DSP_ADDR(0x312), 0xA4); /* v.23 full duplex mode */
            dspWrite8(MAKE_DSP_ADDR(0x313), 0xA4); /* v.23 half duplex mode */
        }

        if(modemCfg.cInfo.speed == MODEM_SPEED_AUTO) {
            modemSetV8ModeMax(); /* Set the maximum values, the highest possible speed is used */

            /* Set the automode flag to indicate that the MDP's automode feature
               will be used when a connection is established */
            modemCfg.flags |= MODEM_CFG_FLAG_AUTOMODE;
        }
        else {
            /* Set the desired speed that the user specified */
            modemSetV8ModeDesired(modemCfg.cInfo.bps);
        }
    }

    modemIntSetHandler(modemCfg.cInfo.protocol, mode);

    /* Setup interrupts that are protocol specific */
    modemIntSetupProtocolInterrupts(0);

    return 1;
}

/* A dial tone is detected if TONEA, TONEB, and TONEC are set */
int modem_dialtone_detected(void) {
    return (modemRead(REGLOC(0xB)) & 0xE0) == 0xE0;
}

/* Waits for a dialtone. Timeout is specified as multiples of
   100ms with the minimum value being 100. */
int modem_wait_dialtone(int timeout) {
    while(timeout > 0) {
        if(modem_dialtone_detected())
            return 0;

        thd_sleep(100);
        timeout -= 100;
    }

    return -1;
}

void modem_set_event_handler(MODEMEVENTHANDLERPROC eventHandler) {
    modemCfg.eventHandler = eventHandler;
}

/* For internal use only */
void modemDropLine(void) {
    /* Make sure the interrupt handler doesn't doing anything else */
    modemIntResetControlCode();

    /* Only do this if either DATA or DTR are currently set */
    if(modemRead(REGLOC(0x9)) & 0x5)
        modemClearBits(REGLOC(0x9), 0x5); /* Reset DTR and DATA */

    /* Set NEWC if CONF contains a v.34 cleardown code */
    if(modemRead(REGLOC(0x12)) == 0xC0)
        modemSetBits(REGLOC(0x1F), 0x1);  /* Set NEWC */

    /* Disable any connection specific interrupts */
    modemIntSetupConnectionInterrupts(1);

    /* Close the line if it's open */
    if(modemRead(REGLOC(0x7)) & 0x2)
        modemClearBits(REGLOC(0x7), 0x2);

    /* The modem's not connected anymore */
    modemCfg.flags &= ~(MODEM_CFG_FLAG_CONNECTING |
                        MODEM_CFG_FLAG_CONNECTED);

    /* Reset the origination bit so any attempts to dial will fail */
    modemCfg.flags &= ~MODEM_CFG_FLAG_ORIGINATE;
}

void modem_disconnect(void) {
    unsigned int i;

    if(!(modemCfg.flags & MODEM_CFG_FLAG_CONNECTED))
        return;

    /* The control code doesn't need to do anything else now */
    modemIntResetControlCode();

    /* Any disconnection attempt will abort and forcefully terminate the
       connection if nothing happens after 5 seconds of starting to attempt
       a disconnection */

    switch(modemCfg.cInfo.protocol) {
        case MODEM_PROTOCOL_V32:
        case MODEM_PROTOCOL_V32BIS:
            /* In v.32 mode, the remote modem will detect a cleardown
               sequence and both modems will drop the carrier at the
               end */
            modemWrite(REGLOC(0x12), 0x70);
            modemSetBits(REGLOC(0x15), 0x4); /* Set RREN */

            /* Wait for a disconnection or a timeout */
            for(i = 0; i < 500; i++) {
                if(!(modemRead(REGLOC(0xF)) & 0x80))
                    break;

                thd_sleep(10);
            }
            break;

        case MODEM_PROTOCOL_V34:
            modemWrite(REGLOC(0x12), 0xC0);
            modemSetBits(REGLOC(0x15), 0x4); /* Set RREN */

            for(i = 0; i < 500; i++) {
                if(!(modemRead(REGLOC(0xF)) & 0x80) ||
                    modemRead(REGLOC(0x14)) == 0x96) {
                    break;
                }

                thd_sleep(10);
            }
            break;

        default:
            modemClearBits(REGLOC(0x9), 0x5); /* Reset DTR and DATA */

            /* Wait for 2 seconds just because */
            thd_sleep(2000);

            break;
    }

    modemDropLine();
}

/* For internal use only */
void modemEstablishConnection(void) {
    unsigned char mSpeedConfig = 0;

    if(modemCfg.cInfo.protocol != MODEM_PROTOCOL_V8) {
        mSpeedConfig = modemSpeedConfig[modemCfg.cInfo.protocol][modemCfg.cInfo.speed];
        assert(mSpeedConfig != 0);
    }
    else
        mSpeedConfig = 0xAA; /* Establish a connection using a v.8

                               handshake */

    /* Set the connection mode first */
    modemWrite(REGLOC(0x12), mSpeedConfig); /* Sets CONF */

    /* Notify the MDP of the configuration change */
    modemSetBits(REGLOC(0x1F), 0x1); /* Set NEWC */

    /* If the modem is not originating then open the line so the incoming call
       can be answered */
    if(!(modemCfg.flags & MODEM_CFG_FLAG_ORIGINATE)) {
        modemSetBits(REGLOC(0x7), 0x2); /* Set RA */
        timer_spin_sleep(10);
    }

    /* Note that in all modes of operation RTS will be turned on as soon
       as DTR is set. This is allowed according to the hardware documents,
       and it makes life easier. */

    modemSetBits(REGLOC(0x9), 0x1); /* Set DTR */

    if(modemCfg.flags & MODEM_CFG_FLAG_AUTOMODE) {
        /* Have the MDP use v.8 automode */
        modemSetBits(REGLOC(0x15), 0x8); /* Set AUTO */
    }
    else {
        if(modemCfg.cInfo.protocol != MODEM_PROTOCOL_V32 &&
                modemCfg.cInfo.protocol != MODEM_PROTOCOL_V32BIS) {
            /* Set DATA and RTS */
            modemSetBits(REGLOC(0x9), 0x4);
            modemSetBits(REGLOC(0x8), 0x1);
        }
    }
}

int dspWrite8(unsigned char meaddl, unsigned char meaddh,
              unsigned char medal) {
    unsigned char data;

    /* Set the MEMW bit. Tell the DSP that data will be written. */
    modemSetBits(REGLOC(0x1D), 0x20);

    /* Read register 0x1D, and then modify its data so that the last 5 bits
       of the memory access address are set properly, then write it back. */
    data = modemRead(REGLOC(0x1D)) & ~0x4F; /* Clear bit 6 and bits 0-3 */

    /* Set bit 6 and bits 0-3 based on the value of meaddh */
    data |= ((meaddh & 0x10) << 2) | (meaddh & 0xF);

    /* Write data back to register 0x1D */
    modemWrite(REGLOC(0x1D), data);

    /* Write the first 8 bits of the DSP RAM address */
    modemWrite(REGLOC(0x1C), meaddl);

    /* Write the data that will be written to the specified address into
       MEDAL */
    modemWrite(REGLOC(0x18), medal);

    /* Set MEMACC. This tells the DSP that the data is ready to be written */
    modemSetBits(REGLOC(0x1D), 0x80);

    /* MEACC is cleared and NEWS is set when the write completes */
    /* Put some sort of timeout here instead */
    while(modemRead(REGLOC(0x1D)) & 0x80);

    /* Clear MEMW now because only 8 bits of data is being written */
    modemClearBits(REGLOC(0x1D), 0x20);

    return 1;
}

int dspWrite16(unsigned char meaddl, unsigned char meaddh,
               unsigned short medal) {
    unsigned char data;

    /* Set the MEMW bit. Tell the DSP that data will be written. */
    modemSetBits(REGLOC(0x1D), 0x20);

    /* Read register 0x1D, and then modify its data so that the last 5 bits
       of the memory access address are set properly, then write it back. */
    data = modemRead(REGLOC(0x1D)) & ~0x4F; /* Clear bit 6 and bits 0-3 */

    /* Set bit 6 and bits 0-3 based on the value of meaddh */
    data |= ((meaddh & 0x10) << 2) | (meaddh & 0xF);

    /* Write data back to register 0x1D */
    modemWrite(REGLOC(0x1D), data);

    /* Write the first 8 bits of the DSP RAM address */
    modemWrite(REGLOC(0x1C), meaddl);

    /* Write the first 8-bits of data into MEDAL */
    modemWrite(REGLOC(0x18), medal & 0xFF);

    /* Write the last 8-bits of data into MEDAM */
    modemWrite(REGLOC(0x19), medal >> 8);

    /* Set MEMACC. This tells the DSP that the data is ready to be written */
    modemSetBits(REGLOC(0x1D), 0x80);

    /* Wait for the write to complete */
    while(modemRead(REGLOC(0x1D)) & 0x80);

    /* Clear MEMW to tell the MDP that no more data is to be written */
    modemClearBits(REGLOC(0x1D), 0x20);

    return 1;
}

int dspWrite16x2(unsigned char meaddl1, unsigned char meaddh1,
                 unsigned char meaddl2, unsigned char meaddh2,
                 unsigned short medal) {
    unsigned char data;

    /************************************
     * Write the first byte into memory *
     ************************************/

    /* Set the MEMW bit. Tell the DSP that data will be written. */
    modemSetBits(REGLOC(0x1D), 0x20);

    /* Read register 0x1D, and then modify its data so that the last 5 bits
       of the memory access address are set properly, then write it back. */
    data = modemRead(REGLOC(0x1D)) & ~0x4F; /* Clear bit 6 and bits 0-3 */

    /* Set bit 6 and bits 0-3 based on the value of meaddh1 */
    data |= ((meaddh1 & 0x10) << 2) | (meaddh1 & 0xF);

    /* Write data back to register 0x1D */
    modemWrite(REGLOC(0x1D), data);

    /* Write the first 8 bits of the DSP RAM address */
    modemWrite(REGLOC(0x1C), meaddl1);

    /* Write the data that will be written to the specified address into
       MEDAL */
    modemWrite(REGLOC(0x18), medal & 0xFF);

    /* Set MEMACC. This tells the DSP that the data is ready to be written */
    modemSetBits(REGLOC(0x1D), 0x80);

    /* Put some sort of timeout here instead */
    while(modemRead(REGLOC(0x1D)) & 0x80);

    /*************************************
     * Write the second byte into memory *
     *************************************/

    /* Set the MEMW bit. Tell the DSP that data will be written. */
    modemSetBits(REGLOC(0x1D), 0x20);

    /* Read register 0x1D, and then modify its data so that the last 5 bits
       of the memory access address are set properly, then write it back. */
    data = modemRead(REGLOC(0x1D)) & ~0x4F; /* Clear bit 6 and bits 0-3 */

    /* Set bit 6 and bits 0-3 based on the value of meaddh2 */
    data |= ((meaddh2 & 0x10) << 2) | (meaddh2 & 0xF);

    /* Write data back to register 0x1D */
    modemWrite(REGLOC(0x1D), data);

    /* Write the first 8 bits of the DSP RAM address */
    modemWrite(REGLOC(0x1C), meaddl2);

    /* Write the data that will be written to the specified address into
       MEDAL */
    modemWrite(REGLOC(0x18), medal >> 8);

    /* Set MEMACC. This tells the DSP that the data is ready to be written */
    modemSetBits(REGLOC(0x1D), 0x80);

    while(modemRead(REGLOC(0x1D)) & 0x80);

    /* Clear MEMW because the 2 bytes of data have been written */
    modemClearBits(REGLOC(0x1D), 0x20);

    return 1;
}

unsigned char dspRead8(unsigned char meaddl, unsigned char meaddh) {
    unsigned char data;

    /* Clear the MEMW bit since data is being read instead of written */
    modemClearBits(REGLOC(0x1D), 0x20);

    /* Read register 0x1D, and then modify its data so that the last 5 bits
       of the memory access address are set properly, then write it back. */
    data = modemRead(REGLOC(0x1D)) & ~0x4F; /* Clear bit 6 and bits 0-3 */

    /* Set bit 6 and bits 0-3 based on the value of meaddh */
    data |= ((meaddh & 0x10) << 2) | (meaddh & 0xF);

    /* Write data back to register 0x1D */
    modemWrite(REGLOC(0x1D), data);

    /* Write the first 8 bits of the memory address in MEADDL */
    modemWrite(REGLOC(0x1C), meaddl);

    /* Set the MEACC bit so that the DSP reads the specified address */
    modemSetBits(REGLOC(0x1D), 0x80);

    /* Wait for the MEACC bit to be cleared */
    /* Put a timeout or something here instead */
    while(modemRead(REGLOC(0x1D)) & 0x80);

    /* Return the data that's in the MEDAL register */
    return modemRead(REGLOC(0x18));
}

/* Does a 16-bit read from a single address using method 4. This is the same
   as dspRead8, but is doesn't ignore the MEDAM register. */
unsigned short dspRead16(unsigned char meaddl, unsigned char meaddh) {
    unsigned short value;

    value = (unsigned short)dspRead8(meaddl, meaddh);

    /* Read the MEDAM register, it contains the last 8 bits of the word */
    value |= ((unsigned short)modemRead(REGLOC(0x19))) << 8;

    return value;
}

/* Does two 8-bit reads from two different locations in DSP RAM and combines
   the two values into one 16-bit value. The first address is the low byte of
   the word and the second address is the high byte of the word. */
unsigned short dspRead16x2(unsigned char meaddl1, unsigned char meaddh1,
                           unsigned char meaddl2, unsigned char meaddh2) {
    unsigned short value;

    value = (unsigned short)dspRead8(meaddl1, meaddh1);
    value |= ((unsigned short)dspRead8(meaddl2, meaddh2)) << 8;

    return value;
}

/* Status functions */
int modem_is_connecting(void) {
    return (modemCfg.flags & MODEM_CFG_FLAG_CONNECTING) ? 1 : 0;
}

int modem_is_connected(void) {
    return (modemCfg.flags & MODEM_CFG_FLAG_CONNECTED) ? 1 : 0;
}

unsigned long modem_get_connection_rate(void) {
    return modemCfg.actual.speed;
}
