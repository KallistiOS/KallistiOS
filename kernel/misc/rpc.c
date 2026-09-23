/* KallistiOS ##version##

   rpc.c
   Copyright (C) 2026 Paul Cercueil
*/

#include <kos/dbglog.h>
#include <kos/genwait.h>
#include <kos/mutex.h>
#include <kos/rpc.h>
#include <kos/thread.h>
#include <kos/worker_thread.h>

#include <errno.h>
#include <stdint.h>

typedef struct rpc_cb_data {
    rpc_cb_t cb;
    void *data;
} rpc_cb_data_t;

static rpc_cb_data_t rpc_cb_data[256];

static kthread_worker_t *worker;
static kthread_worker_t *notifier;

static uint16_t next_cmd_id;

static mutex_t rpc_lock = MUTEX_INITIALIZER;

static kthread_attr_t rpc_worker_thread_attr = {
    .prio = 9, /* slightly below default */
    .label = "[rpc]",
};

static inline void * rpc_id_to_addr(uint16_t id) {
    /* Generate a fake address that will be used with genwait, so that
     * clients can wait on a particular command ID. */
    return (void *)(0xFFFF0000 + id);
}

static void rpc_notifier(void *d) {
    rpc_t *rpc = d;

    rpc->rpc_notify();
}

static void rpc_worker(void *d) {
    rpc_queue_t queue;
    rpc_t *rpc = d;
    rpc_cmd_t cmd;
    int ret;

    /* Read the queue structure */
    rpc->rpc_read(&queue, rpc->inbound, sizeof(queue));

    while(queue.head != queue.tail) {
        /* Read one command */
        rpc->rpc_read(&cmd, &queue.addr[queue.tail], sizeof(cmd));

        if(!rpc_cb_data[cmd.cmd].cb) {
            dbglog(DBG_WARNING, "No handler for RPC command %hhu\n", cmd.cmd);
        } else {
            ret = rpc_cb_data[cmd.cmd].cb(&cmd, rpc_cb_data[cmd.cmd].data);

            /* The command is not async? send the response */
            if(!(cmd.flags & RPC_CMD_FLAG_ASYNC)) {
                rpc_cmd_t resp = {
                    .cmd = RPC_CMD_RESP,
                    .flags = RPC_CMD_FLAG_ASYNC,
                    .params = {
                        [0] = cmd.id,
                        [1] = ret,
                    },
                };

                rpc_execute(rpc, &resp);
            }
        }

        if(++queue.tail == queue.size)
            queue.tail = 0;
    }

    /* Update to the new TAIL */
    rpc->rpc_write((void *)rpc->inbound + offsetof(rpc_queue_t, tail),
                   &queue.tail, sizeof(queue.tail));
}

static int rpc_has_response(const rpc_cmd_t *cmd, void *d) {
    (void)d;

    genwait_wake_all_err(rpc_id_to_addr(cmd->params[0]), (int)cmd->params[1]);

    return 0;
}

int rpc_init(rpc_t *rpc) {
    if(!rpc || !rpc->rpc_read || !rpc->rpc_write || !rpc->rpc_notify) {
        errno = EINVAL;
        return -1;
    }

    worker = thd_worker_create_ex(&rpc_worker_thread_attr, rpc_worker, rpc);
    if(!worker)
        return -1;

    notifier = thd_worker_create(rpc_notifier, rpc);
    if(!notifier) {
        thd_worker_destroy(worker);
        return -1;
    }

    rpc_register(RPC_CMD_RESP, rpc_has_response, rpc);

    return 0;
}

void rpc_shutdown(void) {
    thd_worker_destroy(notifier);
    thd_worker_destroy(worker);
}

int rpc_execute(rpc_t *rpc, rpc_cmd_t *cmd) {
    rpc_queue_t queue;
    int ret = 0;

    mutex_lock(&rpc_lock);

    /* Get a command ID */
    cmd->id = next_cmd_id++;

    /* Read the queue structure */
    rpc->rpc_read(&queue, rpc->outbound, sizeof(queue));

    /* Write our command at the current HEAD */
    rpc->rpc_write(&queue.addr[queue.head], cmd, sizeof(*cmd));

    if(++queue.head == queue.size)
        queue.head = 0;

    /* Update to the new HEAD */
    rpc->rpc_write((void *)rpc->outbound + offsetof(rpc_queue_t, head),
                   &queue.head, sizeof(queue.head));

    mutex_unlock(&rpc_lock);

    /* Notify the remote processor that a new message has been euqueued */
    thd_worker_wakeup(notifier);

    if(!(cmd->flags & RPC_CMD_FLAG_ASYNC)) {
        /* Wait until we have a response */
        genwait_wait(rpc_id_to_addr(cmd->id), "RPC", 0);

        ret = thd_get_current()->thd_errno;
    }

    return ret;
}

void rpc_process_inbound(rpc_t *rpc) {
    (void)rpc;

    thd_worker_wakeup(worker);
}

void rpc_register(rpc_code_t code, rpc_cb_t cb, void *cb_data) {
    rpc_cb_data[code] = (rpc_cb_data_t){ cb, cb_data };
}
