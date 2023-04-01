/* KallistiOS ##version##

   aica_comm.h
   Copyright (C) 2000-2002 Megan Potter

   Structure and constant definitions for the SH-4/AICA interface. This file is
   included from both the ARM and SH-4 sides of the fence.
*/

#ifndef __DC_SOUND_AICA_COMM_H
#define __DC_SOUND_AICA_COMM_H

#ifndef __ARCH_TYPES_H
typedef unsigned long uint8;
typedef unsigned long uint32;
#endif

/* Command queue; one of these for passing data from the SH-4 to the
   AICA, and another for the other direction. If a command is written
   to the queue and it is longer than the amount of space between the
   head point and the queue size, the command will wrap around to
   the beginning (i.e., queue commands _can_ be split up). */
typedef struct aica_queue {
    uint32      head;       /* Insertion point offset (in bytes) */
    uint32      tail;       /* Removal point offset (in bytes) */
    uint32      size;       /* Queue size (in bytes) */
    uint32      valid;      /* 1 if the queue structs are valid */
    uint32      process_ok; /* 1 if it's ok to process the data */
    uint32      data;       /* Pointer to queue data buffer */
} aica_queue_t;

/* Command queue struct for commanding the AICA from the SH-4 */
typedef struct aica_cmd {
    uint32      size;       /* Command data size in dwords */
    uint32      cmd;        /* Command ID */
    uint32      timestamp;  /* When to execute the command (0 == now) */
    uint32      cmd_id;     /* Command ID, for cmd/response pairs, or channel id */
    uint32      misc[4];    /* Misc Parameters / Padding */
    uint8       cmd_data[]; /* Command data */
} aica_cmd_t;

/* Maximum command size -- 256 dwords */
#define AICA_CMD_MAX_SIZE   256

/* This is the cmd_data for AICA_CMD_CHAN. Make this 16 dwords long
   for two aica bus queues. */
typedef struct aica_channel {
    uint32      cmd;        /* Command ID */
    uint32      base;       /* Sample base in RAM */
    uint32      type;       /* (8/16bit/ADPCM) */
    uint32      length;     /* Sample length */
    uint32      loop;       /* Sample looping */
    uint32      loopstart;  /* Sample loop start */
    uint32      loopend;    /* Sample loop end */
    uint32      freq;       /* Frequency */
    uint32      vol;        /* Volume 0-255 */
    uint32      pan;        /* Pan 0-255 */
    uint32      pos;        /* Sample playback pos */
    uint32      pad[5];     /* Padding */
} aica_channel_t;

/* Declare an aica_cmd_t big enough to hold an aica_channel_t
   using temp name T, aica_cmd_t name CMDR, and aica_channel_t name CHANR */
#define AICA_CMDSTR_CHANNEL(T, CMDR, CHANR) \
    uint8   T[sizeof(aica_cmd_t) + sizeof(aica_channel_t)]; \
    aica_cmd_t  * CMDR = (aica_cmd_t *)T; \
    aica_channel_t  * CHANR = (aica_channel_t *)(CMDR->cmd_data);
#define AICA_CMDSTR_CHANNEL_SIZE    ((sizeof(aica_cmd_t) + sizeof(aica_channel_t))/4)

/* Command values (for aica_cmd_t) */
#define AICA_CMD_NONE       0x00000000  /* No command (dummy packet)    */
#define AICA_CMD_PING       0x00000001  /* Check for signs of life  */
#define AICA_CMD_CHAN       0x00000002  /* Perform a wavetable action   */
#define AICA_CMD_SYNC_CLOCK 0x00000003  /* Reset the millisecond clock  */

/* Response values (for aica_cmd_t) */
#define AICA_RESP_NONE      0x00000000
#define AICA_RESP_PONG      0x00000001  /* Response to CMD_PING             */
#define AICA_RESP_DBGPRINT  0x00000002  /* Entire payload is a null-terminated string   */

/* Command values (for aica_channel_t commands) */
#define AICA_CH_CMD_MASK    0x0000000f

#define AICA_CH_CMD_NONE    0x00000000
#define AICA_CH_CMD_START   0x00000001
#define AICA_CH_CMD_STOP    0x00000002
#define AICA_CH_CMD_UPDATE  0x00000003

/* Start values */
#define AICA_CH_START_MASK  0x00300000

#define AICA_CH_START_DELAY 0x00100000 /* Set params, but delay key-on */
#define AICA_CH_START_SYNC  0x00200000 /* Set key-on for all selected channels */

/* Update values */
#define AICA_CH_UPDATE_MASK 0x000ff000

#define AICA_CH_UPDATE_SET_FREQ 0x00001000 /* frequency     */
#define AICA_CH_UPDATE_SET_VOL  0x00002000 /* volume        */
#define AICA_CH_UPDATE_SET_PAN  0x00004000 /* panning       */

/* Sample types */
#define AICA_SM_8BIT    1
#define AICA_SM_16BIT   0
#define AICA_SM_ADPCM   2

#endif /* !__DC_SOUND_AICA_COMM_H */
