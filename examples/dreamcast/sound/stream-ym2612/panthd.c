#include <assert.h>
#include <string.h>

#include <kos/mutex.h>
#include <kos/thread.h>
#include <kos/timer.h>


#define QUEUE_LENGTH 16


extern void streamaica_update_panning(uint8_t* pan_values);
extern uint32_t streamaica_get_elapsed_ms();


typedef struct {
    uint8_t values[6];
    uint32_t timestamp;
} Pan;


static Pan queue[QUEUE_LENGTH];
static volatile size_t start_index;
static size_t end_index;
static const Pan* last_entry;
static mutex_t mutex;
static kthread_t* thread;
static uint64_t rate;


static void* routine(void* param) {
    (void)param;

    while (true) {
        mutex_lock(&mutex);
        uint32_t now = streamaica_get_elapsed_ms();
        uint32_t next_timestamp = now + 1;

        uint32_t start = start_index;
        Pan* queue_ptr = &queue[start];
        const Pan* queue_end = &queue[end_index];

        for (; queue_ptr < queue_end; queue_ptr++) {
            next_timestamp = queue_ptr->timestamp;
            if (next_timestamp > now) {
                break;
            }

            streamaica_update_panning(queue_ptr->values);
        }

        size_t new_start = (size_t)(queue_ptr - queue);
        if (new_start != start) start_index = new_start;

        mutex_unlock(&mutex);

        now = streamaica_get_elapsed_ms();
        if (next_timestamp > now) {
            uint32_t ms = next_timestamp - now;
            thd_sleep(ms);
        } else {
            thd_pass();
        }
    }

    __unreachable();
    return NULL;
}


void panthd_initialize(uint32_t sample_rate) {
    thread = NULL;
    rate = sample_rate;

    start_index = 0;
    end_index = 0;

    queue[0] = (Pan){.timestamp = UINT32_MAX, .values = {128, 128, 128, 128, 128, 128}};
    last_entry = &queue[0];

    mutex_init(&mutex, MUTEX_TYPE_NORMAL);
}

void panthd_enqueue(uint32_t samplestamp, uint8_t* pan_values) {
    if (memcmp(last_entry->values, pan_values, sizeof(last_entry->values)) == 0) {
        // nothing to do
        return;
    }

    mutex_lock(&mutex);

    Pan* entry;
    uint32_t timestamp = (uint32_t)((samplestamp * 1000ULL) / rate);

    if (last_entry->timestamp == timestamp) {
        entry = (Pan*)last_entry;
        goto L_enqueue;
    }

    size_t start = start_index;
    size_t end = end_index;

    /*if (start == end && start != 0) {
        start = 0;
        end = 0;
    } else */if (end >= QUEUE_LENGTH) {
        if (start == 0 && end >= QUEUE_LENGTH) {
            start = 1;
            end = QUEUE_LENGTH - 1;
        }

        size_t count = end - start;
        memmove(queue, &queue[start], count * sizeof(Pan));

        start_index = 0;
        end = count;
    }

    entry = &queue[end];
    last_entry = entry;
    end_index = end + 1;

L_enqueue:
    entry->timestamp = timestamp;
    memcpy(entry->values, pan_values, sizeof(entry->values));

    mutex_unlock(&mutex);
}

void panthd_start() {
    thread = thd_create(false, routine, NULL);
}

void panthd_finalize() {
    mutex_lock(&mutex);
    thd_destroy(thread);
    mutex_unlock(&mutex);
    mutex_destroy(&mutex);
}
