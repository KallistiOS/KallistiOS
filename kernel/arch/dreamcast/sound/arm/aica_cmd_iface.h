/* KallistiOS ##version##

   aica_cmd_iface.h
   (c)2000-2002 Megan Potter

   Definitions for the SH-4/AICA interface. This file is meant to be
   included from both the ARM and SH-4 sides of the fence.
*/

#ifndef __ARM_AICA_CMD_IFACE_H
#define __ARM_AICA_CMD_IFACE_H

#include "aica_comm.h"

/* This is where our SH-4/AICA comm variables go... */

/* 0x000000 - 0x010000 are reserved for the program */

/* Location of the SH-4 to AICA queue; commands from here will be
   periodically processed by the AICA and then removed from the queue. */
#define AICA_MEM_CMD_QUEUE  0x010000    /* 32K */

/* Location of the AICA to SH-4 queue; commands from here will be
   periodically processed by the SH-4 and then removed from the queue. */
#define AICA_MEM_RESP_QUEUE 0x018000    /* 32K */

/* This is the channel base, which holds status structs for all the
   channels. This is READ-ONLY from the SH-4 side. */
#define AICA_MEM_CHANNELS   0x020000    /* 64 * 16*4 = 4K */

/* The clock value (in milliseconds) */
#define AICA_MEM_CLOCK      0x021000    /* 4 bytes */

/* 0x021004 - 0x030000 are reserved for future expansion */

/* Open ram for sample data */
#define AICA_RAM_START      0x030000
#define AICA_RAM_END        0x200000

/* Quick access to the AICA channels */
#define AICA_CHANNEL(x)     (AICA_MEM_CHANNELS + (x) * sizeof(aica_channel_t))

/* Channels status register bits */
#define AICA_CHANNEL_KEYONEX   0x8000
#define AICA_CHANNEL_KEYONB    0x4000

#endif  /* __ARM_AICA_CMD_IFACE_H */
