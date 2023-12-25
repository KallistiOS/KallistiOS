/* KallistiOS ##version##

   mdata.c
   Copyright (C) 2002, 2004 Nick Kochakian

   Distributed under the terms of the KOS license.
*/

/*
   The receive buffer is filled as data comes into the modem's RX FIFO buffer.
   If the receive buffer is full, an overflow in this buffer only occurs
   when some data needs to be taken out of the hardware RX FIFO buffer when
   it reaches the point of nearly overflowing itself.

   Data that is sent is first directed to the modem's TX FIFO buffer, but
   if that becomes full, then it begins being stored in this transmit buffer
   and is eventually moved into the modem's TX FIFO buffer. If this transmit
   buffer becomes full, then nothing else can be written, and the application
   needs to wait for the modem to start emptying its own buffer.
*/

#include <stdlib.h>
#include <string.h>
#include <kos/dbglog.h>
#include <kos/thread.h>
#include <assert.h>
#include <dc/modem/modem.h>
#include "mintern.h"

/* The maximum size of the MDP's TX and RX FIFO buffers are 128 bytes */
#define MODEM_DATA_LIMIT 128

/* Default buffer sizes. This should be sufficient for most things */
#define MODEM_TX_BUFFER_SIZE 1024
#define MODEM_RX_BUFFER_SIZE 1024

CHAIN_BUFFER *rxBuffer = NULL;
CHAIN_BUFFER *txBuffer = NULL;

unsigned char rxBufferLockFlag = 0;
unsigned char txBufferLockFlag = 0;

/* Flag locking and unlocking functions */

/* Checks for a lock set on a flag and sets (locks) the flag if it's not
   already set. Interrupts are disabled for a short period of time during the
   process. Returns a non-zero value if the lock was successfully set and the
   calling process can proceed, otherwise the process should drop out from
   the function that it tried to lock the flag from. */
int modemDataLockFlag(unsigned char *flag) {
    int irqState;
    int locked = 0;

    /* Disable interrupts */
    irqState = irq_disable();

    if(!*flag) {
        *flag  = 1;
        locked = 1;
    }

    /* Restore interrupts */
    irq_restore(irqState);

    return locked;
}

/* Unlocks a flag that was locked with the modemDataLockFlag function */
void modemDataUnlockFlag(unsigned char *flag) {
    int irqState;

    /* Disable interrupts */
    irqState = irq_disable();

    assert(*flag);
    *flag = 0;

    /* Restore interrupts */
    irq_restore(irqState);
}

/* Create the RX and TX buffers with the defined sizes. */
void modemDataSetupBuffers(void) {
    if(!rxBuffer) {
        rxBuffer         = createChainBuffer(MODEM_RX_BUFFER_SIZE);
        rxBufferLockFlag = 0;
    }

    if(!txBuffer) {
        txBuffer         = createChainBuffer(MODEM_TX_BUFFER_SIZE);
        txBufferLockFlag = 0;
    }
}

/* Clears all data from the RX and TX buffers and makes sure they're ready for
   use */
void modemDataClearBuffers(void) {
    if(rxBuffer)
        clearChainBuffer(rxBuffer);

    if(txBuffer)
        clearChainBuffer(txBuffer);

    rxBufferLockFlag = 0;
    txBufferLockFlag = 0;
}

void modemDataDestroyBuffers(void) {
    if(rxBuffer) {
        destroyChainBuffer(rxBuffer);
        rxBuffer = NULL;
    }

    if(txBuffer) {
        destroyChainBuffer(txBuffer);
        txBuffer = NULL;
    }
}

/* Internal function. It's assumed that this is being called from a function
   that already has a valid lock on the receive buffer. Returns the number of
   bytes that were written to the local RX FIFO buffer. */
int modemDataInternalHandleReceivedData(void) {
    int           freeSpace;
    int           offset;
    unsigned char data[MODEM_DATA_LIMIT];
    unsigned char wasEmpty;

    wasEmpty = chainBufferContainsData(rxBuffer) ? 0 : 1;

    /* Check for an overflow */
    if(modemRead(REGLOC(0xA)) & 0x8) { /* Check OE */
        /* Critical: Clear the receive buffer that's in system RAM */
        clearChainBuffer(rxBuffer);

        if(modemCfg.eventHandler)
            modemCfg.eventHandler(MODEM_EVENT_OVERFLOW);

        /* Clear OE */
        modemClearBits(REGLOC(0xA), 0x8);
    }

    /* Copy data from the MDP's RX FIFO buffer to the local RX FIFO buffer if
       any data exists */
    freeSpace = getChainBufferFreeSpace(rxBuffer);
    offset    = 0;

    if(freeSpace > MODEM_DATA_LIMIT)
        freeSpace = MODEM_DATA_LIMIT;

    while((modemRead(REGLOC(0xC)) & 0x2) && offset < freeSpace) { /* Checks RXFNE */
        data[offset] = modemRead(REGLOC(0x0)); /* Read a byte */
        offset++;
    }

    /* If anything was read, then store it in the chain buffer */
    if(offset > 0) {
        writeToChainBuffer(rxBuffer, data, offset);

        if(wasEmpty && modemCfg.eventHandler)
            modemCfg.eventHandler(MODEM_EVENT_RX_NOT_EMPTY);
    }

    return offset;
}

/* Internal function. It's assumed that this is being called from a function
   that already has a valid lock on the transmit buffer. */
void modemDataInternalHandleOutgoingData(void) {
    int           bufferLength;
    int           counter;
    unsigned char data;

    /* Don't need to do anything if the local TX FIFO buffer is empty */
    bufferLength = getChainBufferLength(txBuffer);

    if(bufferLength <= 0)
        return;

    /* CTS needs to be set before any data can be copied into TBUFFER */
    while(!(modemRead(REGLOC(0xF)) & 0x20)) {
        thd_pass();
    }

    /* Copy data from the local TX FIFO buffer into the MDP's TX FIFO buffer
        while there's data in the local buffer and there's space in the MDP's
        buffer */
    counter = 0;

    while((modemRead(REGLOC(0x0D)) & 0x2) && bufferLength > 0 &&
            counter < MODEM_DATA_LIMIT) { /* Checks TXFNF */
        /* Read a byte from the chain buffer */
        if(readFromChainBuffer(txBuffer, &data, 1) != 1)
            assert(0); /* This should never happen */

        /* Write the byte to TBUFFER */
        modemWrite(REGLOC(0x10), data);

        bufferLength--;
        counter++;
    }

    /* If the buffer was emptied then generate the corresponding event
        if the event handler is set */
    if(bufferLength <= 0 && modemCfg.eventHandler)
        modemCfg.eventHandler(MODEM_EVENT_TX_EMPTY);

}

/* Internal. Called by the interrupt service routine to update the receive
   buffers. */
void modemDataUpdateRXBuffer(void) {
    if(!rxBuffer)
        return;

    if(modemDataLockFlag(&rxBufferLockFlag)) {
        modemDataInternalHandleReceivedData();
        modemDataUnlockFlag(&rxBufferLockFlag);
    }
}

/* Internal. Called by the interrupt service routine to update the transmission
   buffers. */
void modemDataUpdateTXBuffer(void) {
    if(!txBuffer)
        return;

    if(modemDataLockFlag(&txBufferLockFlag)) {
        modemDataInternalHandleOutgoingData();
        modemDataUnlockFlag(&txBufferLockFlag);
    }
}

int modem_read_data(unsigned char *data, int size) {
    int bytesRead = 0;
    int bufferLength;

    if(!rxBuffer || !(modemCfg.flags & MODEM_CFG_FLAG_CONNECTED))
        return 0;

    if(modemDataLockFlag(&rxBufferLockFlag)) {
        /* Make sure OE isn't set before reading. Don't read from the local
           buffer if OE is set, since the buffer will be cleared soon after. */
        if(!(modemRead(REGLOC(0xA)) & 0x8)) {
            bufferLength = getChainBufferLength(rxBuffer);

            if(bufferLength > 0)
                bytesRead = readFromChainBuffer(rxBuffer, data,
                                                (size < bufferLength) ? size :
                                                bufferLength);
        }

        /* Update the local RX FIFO buffer if there's data waiting on the MDP */
        bufferLength = modemDataInternalHandleReceivedData();

        /* If there's still room left in the destination and there's data in the
           buffer then read as much as possible into the destination */
        size -= bytesRead;

        if(size > 0 && bufferLength > 0)
            bytesRead += readFromChainBuffer(rxBuffer, &data[bytesRead],
                                             (size < bufferLength) ? size :
                                             bufferLength);

        modemDataUnlockFlag(&rxBufferLockFlag);
    }

    return bytesRead;
}

int modem_write_data(unsigned char *data, int size) {
    int freeSpace;
    int bytesWritten = 0;

    if(!txBuffer || !(modemCfg.flags & MODEM_CFG_FLAG_CONNECTED))
        return 0;

    if(modemDataLockFlag(&txBufferLockFlag)) {
        /* Write data from the source to the local buffer if there's any free
           space */
        freeSpace = getChainBufferFreeSpace(txBuffer);

        if(freeSpace > 0) {
            if(size > freeSpace)
                size = freeSpace;

            if(writeToChainBuffer(txBuffer, data, size))
                assert(0); /* This should never happen */

            bytesWritten = size;
        }

        /* Send data to the MDP if there's any waiting in the local buffer */
        modemDataInternalHandleOutgoingData();

        modemDataUnlockFlag(&txBufferLockFlag);
    }

    return bytesWritten;
}

/* If the modem has data waiting to be read a non zero value is returned,
   otherwise zero is returned. */
int modem_has_data(void) {
    return chainBufferContainsData(rxBuffer);
}

/********************************************************************/
/* Modem dialing functions */

/* Translate a DTMF symbol to an output word for the modem. */
static char dtmf_trans(int sym) {
    static char trans[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '*', '#', 'A', 'B', 'C', 'D'
    };

    int i;

    for(i = 0; i < 16; i++) {
        if(trans[i] == sym)
            return i;
    }

    return -1;
}

int modem_dial(const char *digits) {
    int  i, length;
    int  primeBuffer = 0;
    int  result      = 0;
    char w;

    /* Dialing only works for originating modes. Also checks if dialing has been
       disabled or if the modem is already dialing. And finally, dialing won't
       be done if the modem is connected. */
    if(!(modemCfg.flags & MODEM_CFG_FLAG_ORIGINATE) ||
            (modemCfg.flags & (MODEM_CFG_FLAG_DISABLE_DIALING |
                               MODEM_CFG_FLAG_DIALING |
                               MODEM_CFG_FLAG_CONNECTED)))
        return 0;

    /* Get a lock on the transmit buffer */
    if(modemDataLockFlag(&txBufferLockFlag)) {
        /* Translate all of the characters in digits and put them into the
           transmit buffer */
        length = strlen(digits);

        if(length > 0) {
            /* Debug: The length of free space in the transmit buffer should be
                      greater than or equal to the length of the input digits */
            assert(getChainBufferFreeSpace(txBuffer) >= length);

            for(i = 0; i < length; i++) {
                w = dtmf_trans(digits[i]);

                if(w < 0)
                    dbglog(DBG_ERROR, "modem_dial: unknown DTMF symbol '%c'\n", digits[i]);
                else
                    writeToChainBuffer(txBuffer, (unsigned char *)&w, 1);
            }

            primeBuffer = 1;

            /* Get the byte to prime the MDP's transmission buffer with */
            length = readFromChainBuffer(txBuffer, &w, 1);
            assert(length == 1); /* Should have read one byte */

            /* Get the number of bytes still in the transmission buffer */
            length = getChainBufferLength(txBuffer);
        }

        modemDataUnlockFlag(&txBufferLockFlag);

        if(primeBuffer) {
            /* If there's any data left in the local transmission buffer, set the
               dialing flag and interrupts so the rest of the data is sent to the
               MDP's transmission buffer when an interrupt occurs */
            if(length > 1) {
                modemCfg.flags |= MODEM_CFG_FLAG_DIALING;
                modemInternalSetupDialingInts(0);
            }

            /* Send the first byte to the MDP */
            modemWrite(REGLOC(0x10), w);

            result = 1;
        }
    }

    return result;
}

void modemDataHandleDialingData(void) {
    char data;
    int  length;

    /* Get a lock on the transmission buffer */
    if(modemDataLockFlag(&txBufferLockFlag)) {
        /* Get the byte to send to the MDP */
        length = readFromChainBuffer(txBuffer, &data, 1);
        assert(length == 1); /* Should have read one byte */

        /* If this is going to be the last byte of data sent to the MDP then
           the dialing interrupts need to be cleared before that happens */
        if(getChainBufferLength(txBuffer) <= 0) {
            /* Clear the dialing flag and interrupts */
            modemCfg.flags &= ~MODEM_CFG_FLAG_DIALING;
            modemInternalSetupDialingInts(1);
        }

        modemDataUnlockFlag(&txBufferLockFlag);

        /* Send the data to the MDP */
        modemWrite(REGLOC(0x10), data);
    }
    else
        assert(0); /* This should not happen */
}
