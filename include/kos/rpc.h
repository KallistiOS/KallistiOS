/* KallistiOS ##version##

   include/kos/rpc.h
   Copyright (C) 2026 Paul Cercueil
*/

/** \file    kos/rpc.h
    \brief   Remote procedure call support

    This file contains an API that can be used for sending RPC messages to a
    remote processor and handling RPC messages from a remote processor.

    \author Paul Cercueil
*/
#ifndef __KOS_RPC_H
#define __KOS_RPC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/regfield.h>
#include <stdint.h>

/** \brief  RPC command code.

    A RPC command code identifies a command that can be executed on the remote
    processor.

    \headerfile kos/rpc.h
*/
typedef unsigned char rpc_code_t;

/** \brief  Special command code that represents a response to a command
    received from the remote. */
#define RPC_CMD_RESP    0xff

/** \brief  RPC command structure.

    This structure packs the details of a remote procedure call. */
typedef struct rpc_cmd {
    rpc_code_t cmd;             /**< \brief Command code */
    uint8_t flags;              /**< \brief Command flags */
    uint16_t id;                /**< \brief Command ID (leave it blank) */
    uint32_t params[3];         /**< \brief Command parameters (12 bytes total) */
} rpc_cmd_t;

/** \name   RPC command flags
    \brief  Flags supported for the rpc_cmd_t.flags field. */
#define RPC_CMD_FLAG_ASYNC      BIT(0)  /**< \brief Command is asynchronous */

/** \name   Command queue
    \brief  RPC command queue structure.

    The RPC mechanism has two of these; one used for passing the data
    from the remote processor to the local processor, and the second
    is used for the other way.
*/
typedef struct rpc_cmd_queue {
    uint32_t head;              /**< \brief Insertion point offset (in commands) */
    uint32_t tail;              /**< \brief Removal point offset (in commands) */
    uint32_t size;              /**< \brief Queue size (in commands) */
    rpc_cmd_t *addr;            /**< \brief Addresses of the commands buffer */
} rpc_queue_t;

/** \name   RPC data structure.

    This structure should be populated by the user program, before being passed
    to rpc_init(). */
typedef struct rpc {
    /** \brief  Pointer to the inbound queue */
    rpc_queue_t *inbound;

    /** \brief  Pointer to the outbound queue */
    rpc_queue_t *outbound;

    /** \brief  RPC callback to read from queue

        \param  dst         A pointer to the local destination buffer
        \param  rpc_src     The source address in the remote address space
        \param  len         The length of the transfer
    */
    void (*rpc_read)(void *dst, const void *rpc_src, size_t len);

    /** \brief  RPC callback to write to the queue

        \param  rpc_dst     The destination address in the remote address space
        \param  src         A pointer to the local source buffer
        \param  len         The length of the transfer
    */
    void (*rpc_write)(void *rpc_dst, const void *src, size_t len);

    /** \brief  RPC callback to wake up the remote processor

        This function will be called to notify the remote processor that one
        or more commands have been added to its inbound queue. */
    void (*rpc_notify)(void);
} rpc_t;

/** \brief  Initialize the RPC mechanism.

    \param  rpc             A pointer to an (initialized) rpc structure

    \retval 0               On success.
    \retval -1              If the RPC structure hasn't been properly
                            initialized.
*/
int rpc_init(rpc_t *rpc);

/** \brief  Shutdown the RPC mechanism. */
void rpc_shutdown(void);

/** \brief  Execute a command on the remote processor

    This function enqueues a command onto the RPC's outbound queue.

    \param  rpc             A pointer to the rpc structure
    \param  cmd             A pointer to the command to be sent

    \returns                For asynchronous commands, 0 is returned.
                            Otherwise, the return value of the remote function
                            is returned.
*/
int rpc_execute(rpc_t *rpc, rpc_cmd_t *cmd);

/** \brief  Request processing of the inbound queue

    This function should be called when any new inbound commands should be
    processed.

    \param  rpc             A pointer to the rpc structure
*/
void rpc_process_inbound(rpc_t *rpc);

/** \brief  Callback type for rpc_register(). */
typedef int (*rpc_cb_t)(const rpc_cmd_t *cmd, void *cb_data);

/** \brief  Assign a function to a given command code.

    This function should be used to register a function that will be then
    called when receiving a given command, identified by its code, in the
    inbound queue.

    \param  code            The command code identifier
    \param  cb              The callback function to call
    \param  cb_data         A user pointer to pass to the callback
*/
void rpc_register(rpc_code_t code, rpc_cb_t cb, void *cb_data);

/** \brief  Unregister a function for a given command code.

    \param  code            The command code identifier
*/
static inline void rpc_unregister(rpc_code_t code) {
    rpc_register(code, NULL, NULL);
}

__END_DECLS

#endif /* __KOS_RPC_H */
