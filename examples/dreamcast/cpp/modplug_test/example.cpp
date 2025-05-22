#include <kos.h>
#include <modplug/stdafx.h>
#include <modplug/sndfile.h>

uint16_t sound_buffer[65536] = {0};
CSoundFile *soundfile;

void *mod_callback(snd_stream_hnd_t hnd, int len, int * actual) {
    int res;

    res = soundfile->Read(sound_buffer, len) * 4/*samplesize*/;

    //printf("res: %i, len: %i\n",res,len);
    if(res < len) {
        soundfile->SetCurrentPos(0);
        soundfile->Read(&sound_buffer[res], len - res);
    }

    *actual = len;

    return sound_buffer;
}

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;
    uint8_t *mod_buffer;
    uint32_t hnd;
    char filename[] = "/rd/test.s3m";

    printf("modplug_test beginning\n");

    snd_stream_init();

    hnd = fs_open(filename, O_RDONLY);

    if(!hnd) {
        printf("Error reading %s\n", filename);
        return 0;
    }

    printf("Filesize: %i\n", fs_total(hnd));
    mod_buffer = (uint8_t *)malloc(fs_total(hnd));

    if(!mod_buffer) {
        printf("Not enough memory\n");
        return 0;
    }

    printf("Memory allocated\n");

    if((size_t)fs_read(hnd, mod_buffer, fs_total(hnd)) != fs_total(hnd)) {
        printf("Read error\n");
        free(mod_buffer);
        return 0;
    }

    printf("File read\n");

    soundfile = new CSoundFile;

    if(!soundfile) {
        printf("Not enough memory\n");
        free(mod_buffer);
        return 0;
    }

    printf("CSoundFile created\n");

    if(!soundfile->Create(mod_buffer, fs_total(hnd))) {
        printf("Mod not loaded\n");
        free(mod_buffer);
        delete soundfile;
        return 0;
    }

    printf("Mod loaded\n");
    soundfile->SetWaveConfig(44100, 16, 2);
    printf("Type: %li\n", soundfile->GetType());
    printf("Title: %s\n", soundfile->GetTitle());

    /*fs_close(hnd);
    free(mod_buffer);*/

    snd_stream_hnd_t shnd = snd_stream_alloc(mod_callback, SND_STREAM_BUFFER_MAX);
    snd_stream_start(shnd, 44100, 1);

    while(1) {
        /* Check key status */
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if(cont) {
            state = (cont_state_t *)maple_dev_status(cont);

            if(state && state->buttons & CONT_START)
                break;
        }

        snd_stream_poll(shnd);

        timer_spin_sleep(10);
    }

    delete soundfile;

    snd_stream_destroy(shnd);

    snd_stream_shutdown();

    return 0;
}
