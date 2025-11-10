#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <arch/arch.h>
#include <dc/biosfont.h>
#include <dc/video.h>
#include <kos/dbglog.h>
#include <kos/fs.h>
#include <kos/thread.h>

#include "compact_chip_log.h"
#include "impl.h"


#define PEEK_BUFFER(TYPE, PTR) (*((TYPE*)(PTR)));
#define READ_AND_SEEK_BUFFER(TYPE, PTR) ({ \
    TYPE temp = *((TYPE*)PTR);             \
    PTR += sizeof(TYPE);                   \
    temp;                                  \
});
#define MIN_VALUE(VALUE_1, VALUE_2) ({ \
    __typeof(VALUE_1) v1 = VALUE_1;    \
    __typeof(VALUE_2) v2 = VALUE_2;    \
    v1 < v2 ? v1 : v2;                 \
})


extern void streamaica_init(size_t channels, uint32_t sample_rate);
extern int streamaica_refill(int16_t** buffers, int samples);
extern void streamaica_pad_with_silence();
extern void streamaica_start();
extern uint32_t streamaica_get_elapsed_ms();
extern void streamaica_update_panning(uint8_t* pan_values);
extern void streamaica_poll();
extern void streamaica_finalize();

extern void panthd_initialize(uint32_t sample_rate);
extern void panthd_enqueue(uint32_t samplestamp, uint8_t* pan_values);
extern void panthd_start();
extern void panthd_finalize();


static uint32_t next_elapsed = 0;

static void calc_time_progress(uint32_t timestamp, int* seconds, int* minutes) {
    uint32_t m = timestamp / 60000;
    uint32_t s = (timestamp - (m * 60000)) / 1000;
    *seconds = (int)s;
    *minutes = (int)m;
}

static void print_progress(uint32_t elapsed, size_t duration) {
    const size_t x = 8;
    const size_t y = 64;
    const int bar_size = 30;

    if (elapsed < next_elapsed) return;
    next_elapsed += 500;

    float percent = (float)elapsed / (float)duration;
    int progress = (int)(bar_size * percent);

    if (progress > bar_size) progress = bar_size;

    int es, em;
    calc_time_progress(elapsed, &es, &em);
    int ds, dm;
    calc_time_progress((uint32_t)duration, &ds, &dm);

    bfont_draw_str_fmt(
        &vram_s[x + (y * vid_mode->width)],
        vid_mode->width,
        1,
        "Playback: [%.*s%.*s]\n           %02i:%02i / %02i:%02i",
        progress,
        "##############################",
        bar_size - progress,
        "------------------------------",
        em, es, dm, ds
    );
}

static const CCL_FileHeader* ccl_read(const char* filename, size_t* out_data_length) {
    file_t file = fs_open(filename, O_RDONLY);
    if (file == FILEHND_INVALID) {
        dbglog(DBG_ERROR, "cannot open %s\n", filename);
        return NULL;
    }

    CCL_FileHeader* ccl = fs_mmap(file);

    *out_data_length = fs_total(file) - (long)sizeof(CCL_FileHeader);

    uint8_t signature[sizeof(ccl->signature)] = {'C', 'C', 'L', 'V'};
    if (memcmp(ccl->signature, signature, sizeof(signature)) != 0) {
        dbglog(DBG_ERROR, "invalid file signature\n");
        return NULL;
    }

    uint8_t chip_identifier[sizeof(ccl->chip_identifier)] = {'y', 'm', '2', '6', '1', '2', '\0', '\0'};
    if (memcmp(ccl->chip_identifier, chip_identifier, sizeof(chip_identifier)) != 0) {
        dbglog(DBG_ERROR, "unknown chip identifier %.*s\n", sizeof(ccl->chip_identifier), ccl->chip_identifier);
        return NULL;
    }

    // work only with ROMDISK filesystem
    fs_close(file);

    return ccl;
}

static int32_t ccl_interpret(uint8_t* ccl_data, uint8_t* ccl_data_end, uint8_t** advance) {
    CCL_Packet pkt;
    CCL_PacketPeek pkt_hdr;
    CCL_PacketStreamUpdateWide pkt_upwide;
    CCL_PacketStreamUpdateNibble pkt_upnibble;
    CCL_PacketPortPack_0_1 pkt_pack01;
    CCL_PacketPortPack_2_3 pkt_pack23;
    CCL_PacketPortPack_0_1_2_3 pkt_pack0123;
    CCL_PacketPortWrite pkt_portwrite;

    while (ccl_data <= ccl_data_end) {
        pkt_hdr = PEEK_BUFFER(CCL_PacketPeek, ccl_data);

        switch (pkt_hdr.type) {
            case CCL_CMD_end_of_file:
                ccl_data += sizeof(CCL_Packet);
                *advance = ccl_data;
                return -1;
            case CCL_CMD_stream_update:
                pkt = READ_AND_SEEK_BUFFER(CCL_Packet, ccl_data);
                *advance = ccl_data;
                return pkt.stream_updates;
            case CCL_CMD_stream_update_nibble:
                pkt_upnibble = READ_AND_SEEK_BUFFER(CCL_PacketStreamUpdateNibble, ccl_data);
                *advance = ccl_data;
                return pkt_upnibble.stream_updates;
            case CCL_CMD_stream_update_wide:
                pkt_upwide = READ_AND_SEEK_BUFFER(CCL_PacketStreamUpdateWide, ccl_data);
                *advance = ccl_data;
                return pkt_upwide.stream_updates;
            case CCL_CMD_port_write:
                pkt = READ_AND_SEEK_BUFFER(CCL_Packet, ccl_data);
                for (uint8_t i = 0; i < pkt.port_writes; i++) {
                    pkt_portwrite = READ_AND_SEEK_BUFFER(CCL_PacketPortWrite, ccl_data);
                    ym2612_w(pkt_portwrite.offset, pkt_portwrite.data);
                }
                break;
            case CCL_CMD_port_write_pack_0_1:
                pkt_pack01 = READ_AND_SEEK_BUFFER(CCL_PacketPortPack_0_1, ccl_data);
                ym2612_w(0, pkt_pack01.data[0]);
                ym2612_w(1, pkt_pack01.data[1]);
                continue;
            case CCL_CMD_port_write_pack_2_3:
                pkt_pack23 = READ_AND_SEEK_BUFFER(CCL_PacketPortPack_2_3, ccl_data);
                ym2612_w(2, pkt_pack23.data[0]);
                ym2612_w(3, pkt_pack23.data[1]);
                continue;
            case CCL_CMD_port_write_pack_0_1_2_3:
                pkt_pack0123 = READ_AND_SEEK_BUFFER(CCL_PacketPortPack_0_1_2_3, ccl_data);
                ym2612_w(0, pkt_pack0123.data[0]);
                ym2612_w(1, pkt_pack0123.data[1]);
                ym2612_w(2, pkt_pack0123.data[2]);
                ym2612_w(3, pkt_pack0123.data[3]);
                continue;
        }
    }

    *advance = ccl_data;
    return -1;
}


int main(int argc, char** argv) {
    dbglog_set_level(DBG_INFO);

    //
    // Loop and fade-out params
    //
    uint32_t loop_count = 2;
    int32_t fade_milliseconds = 10000;


#if __OPTIMIZE__ < 1
    dbglog(DBG_WARNING, "YM2612 interpreter is quite expensive, compile least with -O1 to avoid suttering.\n");
#endif


    // open compact chip log file
    size_t data_length;
    const CCL_FileHeader* ccl = ccl_read("/rd/track05.ccl", &data_length);
    if (!ccl) return 1;

    dbglog(
        DBG_INFO,
        "CCL: chip=%.*s clock=%lu\nStream: freq=%luhz loop_start=%lu samples=%lu channels=6\n",
        sizeof(ccl->chip_identifier), ccl->chip_identifier, ccl->chip_clock,
        ccl->sample_rate, ccl->loop_start_samples, ccl->pcm_samples
    );

    uint8_t* ccl_data = sizeof(CCL_FileHeader) + (uint8_t*)ccl;
    uint8_t* ccl_data_end = ccl_data + data_length;

    // initialize AICA streams
    uint8_t panning[6];
    bool start_streams = true;
    streamaica_init(6, ccl->sample_rate);
    panthd_initialize(ccl->sample_rate);

    // initialize ym2612
    ym2612_set_options(ccl->chip_flags);
    device_start_ym2612((int)ccl->chip_clock);
    device_reset_ym2612();
    ym2612_set_mute_mask(0x00000000);

    // initialize ym2612 buffers
    int buffers_length = 16384;
    int buffers_used = 0;
    int16_t* buffers[6] = {
        memalign(32, (size_t)buffers_length * sizeof(int16_t)),
        memalign(32, (size_t)buffers_length * sizeof(int16_t)),
        memalign(32, (size_t)buffers_length * sizeof(int16_t)),
        memalign(32, (size_t)buffers_length * sizeof(int16_t)),
        memalign(32, (size_t)buffers_length * sizeof(int16_t)),
        memalign(32, (size_t)buffers_length * sizeof(int16_t))
    };

    int32_t samples_to_read;
    int required;
    uint8_t* ccl_data_next = ccl_data;
    int32_t fade_samples = ((int32_t)ccl->sample_rate * fade_milliseconds) / 1000;
    int32_t fade_samples_progress = fade_samples;
    uint32_t stream_duration = (uint32_t)((ccl->pcm_samples * 1000ULL) / ccl->sample_rate);
    uint32_t samplestamp = 0;
    bool fade_active = false;
    bool run = true;

    stream_duration *= loop_count;
    stream_duration += fade_milliseconds;
    if (loop_count > 0) loop_count--;

L_run_interpreter:
    samples_to_read = ccl_interpret(ccl_data_next, ccl_data_end, &ccl_data_next);

    if (samples_to_read == -1) {
        bool loop_again;
        if (loop_count > 0) {
            loop_count--;
            loop_again = true;
        } else if (fade_samples_progress > 0) {
            if (!fade_active) fade_active = true;
            loop_again = true;
        } else {
            dbglog(DBG_INFO, "interpreter completed\n");
            loop_again = false;
            run = false;
        }

        if (loop_again) {
            ccl_data_next = &ccl_data[ccl->loop_offset];
            goto L_run_interpreter;
        }
    } else if (samples_to_read < 0) {
        dbglog(DBG_ERROR, "playback failed\n");
        run = false;
    }

L_stream_update:
    required = MIN_VALUE(samples_to_read, buffers_length - buffers_used);
    samples_to_read -= required;

    ym2612_stream_update_ex(buffers, buffers_used, required, panning);

    panthd_enqueue(samplestamp, panning);
    samplestamp += (uint32_t)required;

    if (fade_active) {
        if (required > fade_samples_progress) {
            // no more fetching fade-out completed
            run = false;
            samples_to_read = 0;
            required = fade_samples_progress;
        }

        for (int32_t i = 0; i < required; i++) {
            double percent = fade_samples_progress / (double)fade_samples;

            for (int32_t c = 0; c < 6; c++) {
                int16_t* sample = &buffers[c][buffers_used];
                *sample = (int16_t)(*sample * percent);
            }

            // advance to the next sample of each channel
            buffers_used++;
            fade_samples_progress--;
        }
    } else {
        buffers_used += required;
    }

    if (buffers_used < buffers_length && run) {
        goto L_run_interpreter;
    }

    // refill stream buffers
    int written;
    while ((written = streamaica_refill(buffers, buffers_used)) < 1) {
        // start streams (if unstarted)
        if (start_streams) {
            start_streams = false;
            streamaica_start();
            panthd_start();
        }

        // wait until all samples can be written
        streamaica_poll();
        print_progress(streamaica_get_elapsed_ms(), stream_duration);
        thd_sleep(1);
    }

    if (written > 0) {
        int no_written = buffers_length - written;
        for (size_t i = 0; i < 6; i++) {
            memmove(buffers[i], buffers[i] + written, no_written * sizeof(int16_t));
        }
        buffers_used = no_written;
    }

    // synthesize more samples
    if (run) {
        if (samples_to_read > 0)
            goto L_stream_update;
        else
            goto L_run_interpreter;
    }

    // wait until all samples are played (not precise, always waits a bit more)
    while (streamaica_get_elapsed_ms() <= stream_duration) {
        streamaica_pad_with_silence();
        streamaica_poll();
        thd_sleep(100);
        print_progress(streamaica_get_elapsed_ms(), stream_duration);
    }

    // stop everything
    // free(ccl);
    for (size_t i = 0; i < 6; i++) free(buffers[i]);
    device_stop_ym2612();
    panthd_finalize();
    streamaica_finalize();

    dbglog(DBG_INFO, "playback completed\n");
    arch_menu();

    return 0;
}
