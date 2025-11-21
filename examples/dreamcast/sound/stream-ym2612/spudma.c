
#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#include <dc/spu.h>
#include <dc/sq.h>
#include <kos/thread.h>


#define QUEUE_LENGTH 16


typedef struct {
    bool queued;
    int16_t* from;
    uint32_t dest;
    uint32_t length;
} RequestDMA;


static volatile __aligned(32) size_t dma_in_progress = 0;
static RequestDMA dma_queue[QUEUE_LENGTH] = {};


static void chain(void* data) {
    (void)data;

    RequestDMA* next = NULL;

    for (size_t i = 0; i < QUEUE_LENGTH; i++) {
        if (dma_queue[i].queued) {
            next = &dma_queue[i];
            break;
        }
    }

    dma_in_progress--;

    if (!next) return;
    next->queued = false;

    int rs = spu_dma_transfer(next->from, next->dest, next->length, 0, chain, NULL);
    if (rs != 0) {
        // this never should happen
        assert_msg(rs < 0, "dma chaining failed");
    }
}


void spu_dma_transfer_enqueue(void* from, uintptr_t dest, size_t length) {
    for (size_t i = 0; i < QUEUE_LENGTH; i++) {
        if (dma_queue[i].queued) continue;
        dma_queue[i] = (RequestDMA){.queued = true, .from = from, .dest = dest, .length = length};
        return;
    }

    assert_msg(false, "dma queue is full");
}

void spu_dma_transfer_flush() {
    if (dma_in_progress > 0) return;

    RequestDMA* first = NULL;
    size_t count = 0;

    for (size_t i = 0; i < QUEUE_LENGTH; i++) {
        if (dma_queue[i].queued) {
            if (!first) first = &dma_queue[i];
            count++;
        }
    }

    if (!first) return;

    dma_in_progress += count;
    first->queued = false;

    while (true) {
        int rs = spu_dma_transfer(first->from, first->dest, first->length, 0, chain, NULL);
        if (rs == 0) {
            return;
        }
        assert_msg(errno == EINPROGRESS, "dma failed");
    }
}

void spu_dma_transfer_wait() {
    while (dma_in_progress > 0) {
        thd_sleep(2);
    }
}
