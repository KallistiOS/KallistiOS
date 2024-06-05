/* KallistiOS ##version##

   mintern.h
   Copyright (C)2002, 2004 Nick Kochakian

   Distributed under the terms of the KOS license.

*/

#ifndef __MINTERN_H
#define __MINTERN_H

#include <time.h>
#include <arch/irq.h>
#include <dc/g2bus.h>
#include "chainbuf.h"

#ifndef REGLOC
#define REGLOC(x) (0xA0600400 + (x) * 4)
#endif

#define MAKE_DSP_ADDR(x) ((x) & 0xFF), (((x) >> 8) & 0xF)

#ifndef G2_8BP_RST
#define G2_8BP_RST 0xA0600480
#endif

#define modemWrite(a, d)     g2_write_8(a, d)
#define modemRead(a)         g2_read_8(a)
#define modemSetBits(a, d)   g2_write_8(a, g2_read_8(a) | (d))
#define modemClearBits(a, d) g2_write_8(a, g2_read_8(a) & ~(d))

/* DSP memory access macros */
#define dspSetBits8(DSPAddr, bits) dspWrite8(DSPAddr, dspRead8(DSPAddr) | (bits))
#define dspClearBits8(DSPAddr, bits) dspWrite8(DSPAddr, dspRead8(DSPAddr) & ~(bits))

/* Internal flag bits */
#define MODEM_INTERNAL_FLAG_INIT_ATEXIT       0x1 /* Set when atexit from modem_init is called */
#define MODEM_INTERNAL_FLAG_INT_INIT_ATEXIT   0x2 /* Set when atexit from modemIntInit is called */
#define MODEM_INTERNAL_FLAG_TIMER_HANDLER_SET 0x4 /* The timeout callback has been registered */
#define MODEM_INTERNAL_FLAG_TIMER_RUNNING     0x8 /* The timeout timer is running */

/* Configuration flag bits */
#define MODEM_CFG_FLAG_ORIGINATE       0x1  /* Set if the MDP is originating, clear when the MDP is answering */
#define MODEM_CFG_FLAG_SIGNAL          0x2  /* Some non-auto modes wait for certain signals to be detected before setting DATA and RTS */
#define MODEM_CFG_FLAG_AUTOMODE        0x4  /* Set if the MDP's automode functionality is being used */
#define MODEM_CFG_FLAG_CONNECTING      0x8  /* Set if the modem is currently waiting for or attempting a connection */
#define MODEM_CFG_FLAG_CONNECTED       0x10 /* Set or cleared if the modem is no longer connecting. Determines if there is actually a connection or not. */
#define MODEM_CFG_FLAG_DISABLE_DIALING 0x20 /* Set if dialing should not be allowed */
#define MODEM_CFG_FLAG_DIALING         0x40 /* Set if the modem is currently dialing a number */

/* EQM related flags */
#define EQM_FLAG_HIGH    0x1 /* Set if the EQM value was found to be way too high */
#define EQM_FLAG_RETRAIN 0x2 /* Set if a retrain is currently in progress */
#define EQM_FLAG_EQMAT   0x4 /* The MDP has recently set EQMAT */

enum {
    MODEM_STATE_NULL = 0,     /* Not doing anything specific */
    MODEM_STATE_RING_WAIT,    /* Waiting for a ring (answering modes only) */
    MODEM_STATE_CONNECT_WAIT, /* Waiting for a handshake signal */
    MODEM_STATE_CONNECTING,   /* Connecting to a remote computer */
    MODEM_STATE_DELAY         /* Set if the modem is doing something but should
                                 wait for a period of time before resuming
                                 operation */
};

typedef struct {
    unsigned char  protocol;    /* MODEM_PROTOCOL_* */
    unsigned char  speed;       /* MODEM_SPEED_* */
    unsigned short bps;
} MODEM_CINFO;

typedef struct {
    unsigned char inited;
    unsigned char flags;     /* Modem configuration flags */
    MODEM_CINFO   cInfo;

    MODEMEVENTHANDLERPROC eventHandler;

    struct {
        unsigned long speed;      /* Actual connection speed */
        int           state;      /* State constant used by the interrupt handlers */
        unsigned char phaseState; /* Flags for monitoring the phase and phase state
                                 of the connection that's being initiated */
    } actual;

    struct {
        unsigned char conf;  /* Last known value of the CONF register */
        unsigned char speed; /* Last known value of the SPEED register */
    } registers;

    struct {
        unsigned char flags;    /* EQM_FLAG_* */
        unsigned char attempts; /* The number of consecutive retrain attempts */
        time_t        nextClock;
        char          counter;
        char          lastCounter;
        char          auxCounter;
    } eqm;
} MODEM_CFG;

extern MODEM_CFG            modemCfg;
extern const unsigned short modemBPSConstants[31];
extern int                  __isLessThan(int x, int y);
extern unsigned char        modemInternalFlags;

/* From mintr.c */
void modemIntInit(void);
void modemIntShutdown(void);
void modemIntSetupConnectionInterrupts(int clear);
void modemIntSetupProtocolInterrupts(int clear);
void modemIntConfigModem(void);
void modemIntSetHandler(int protocol, int mode);
void modemIntResetControlCode(void);
void modemInternalSetupDialingInts(int clear);

/* From modem.c */
void           modem_reset(void);
void           modemCreateCInfoFromCC(const unsigned char cc, MODEM_CINFO *cInfo);
void           modemDropLine(void);
void           modemEstablishConnection(void);
int            dspWrite8(unsigned char meaddl, unsigned char meaddh,
                         unsigned char medal);
int            dspWrite16(unsigned char meaddl, unsigned char meaddh,
                          unsigned short medal);
int            dspWrite16x2(unsigned char meaddl1, unsigned char meaddh1,
                            unsigned char meaddl2, unsigned char meaddh2,
                            unsigned short medal);
unsigned char  dspRead8(unsigned char meaddl, unsigned char meaddh);
unsigned short dspRead16(unsigned char meaddl, unsigned char meaddh);
unsigned short dspRead16x2(unsigned char meaddl1, unsigned char meaddh1,
                           unsigned char meaddl2, unsigned char meaddh2);

/* From mdata.c */
void modemDataSetupBuffers(void);
void modemDataClearBuffers(void);
void modemDataDestroyBuffers(void);
void modemDataUpdateRXBuffer(void);
void modemDataUpdateTXBuffer(void);
void modemDataHandleDialingData(void);

#endif
