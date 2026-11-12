/* KallistiOS ##version##

   stream_lr_test.c

   Plays a 440 Hz tone in the LEFT channel only (right channel is silent)
   through snd_stream with a 32-byte aligned 16-bit stereo buffer. The tone
   must be heard in the left ear/speaker. If it comes out of the right one,
   the stereo separation in snd_stream is swapping the channels.

   Press START to exit.
*/

#include <stdint.h>
#include <kos.h>
#include <dc/sound/stream.h>

/* 44000 Hz makes 440 Hz exactly 100 frames per period, so no libm is needed
   and the tone is phase-continuous across callbacks. */
#define FREQ        44000
#define PERIOD      100
#define BUF_BYTES   SND_STREAM_BUFFER_MAX_PCM16

static int16_t tone[PERIOD];
static int16_t buf[BUF_BYTES / 2] __attribute__((aligned(32)));
static unsigned phase;

static void *get_data(snd_stream_hnd_t hnd, int req_bytes, int *got_bytes) {
    int frames, i;

    (void)hnd;

    if(req_bytes > BUF_BYTES)
        req_bytes = BUF_BYTES;

    frames = req_bytes / 4; /* 16-bit stereo: 4 bytes per frame */

    for(i = 0; i < frames; i++) {
        buf[i * 2] = tone[phase];  /* left  */
        buf[i * 2 + 1] = 0;        /* right */
        phase = (phase + 1) % PERIOD;
    }

    *got_bytes = frames * 4;
    return buf;
}

int main(int argc, char **argv) {
    snd_stream_hnd_t hnd;
    maple_device_t *cont;
    cont_state_t *st;
    int i;

    (void)argc;
    (void)argv;

    /* Integer triangle wave, about half of full scale. Timbre isn't important,
       only that it is periodic at 440 Hz. */
    for(i = 0; i < PERIOD; i++) {
        int v = (i < PERIOD / 2) ? i : PERIOD - i; /* 0..50..0 */
        tone[i] = (int16_t)((v - PERIOD / 4) * 600);
    }

    vid_set_mode(DM_640x480, PM_RGB565);
    bfont_draw_str(vram_s + 20 * 640 + 20, 640, 1, "Tone should play in the LEFT ear only.");
    bfont_draw_str(vram_s + 44 * 640 + 20, 640, 1, "Press START to exit.");

    /* The stereo split scratch buffer is sized by snd_stream_init_ex(), and
       snd_stream_init() only sizes it for SND_STREAM_BUFFER_MAX (32 KiB per
       channel), which is too small for a SND_STREAM_BUFFER_MAX_PCM16 stream
       that refills up to 64 KiB per channel. */
    if(snd_stream_init_ex(2, BUF_BYTES) < 0) {
        printf("snd_stream_init failed\n");
        return -1;
    }

    hnd = snd_stream_alloc(get_data, SND_STREAM_BUFFER_MAX_PCM16);
    if(hnd == SND_STREAM_INVALID) {
        printf("snd_stream_alloc failed\n");
        snd_stream_shutdown();
        return -1;
    }

    snd_stream_start(hnd, FREQ, 1);

    for(;;) {
        snd_stream_poll(hnd);
        thd_sleep(20);

        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        if(cont && (st = (cont_state_t *)maple_dev_status(cont)) &&
           (st->buttons & CONT_START))
            break;
    }

    snd_stream_stop(hnd);
    snd_stream_destroy(hnd);
    snd_stream_shutdown();
    return 0;
}
