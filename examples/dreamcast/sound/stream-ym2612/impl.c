#include "impl.h"

#include <stddef.h>

#define FM_BUFFER_SIZE 128


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

//
// foward references to fm2612.c file
//
#define BUILD_YM2612 1
static UINT8 IsVGMInit = 1;
static void ym2612_update_req(void* param);

#include "chips/fm2612.c"

#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#define AICA_PAN_FULL_LEFT 0
#define AICA_PAN_TOTAL_CENTER 128
#define AICA_PAN_FULL_RIGHT 255

static void* instance;
static UINT8 instance_flags = 0x00;


static void ym2612_update_req(void* param) {
    void** chip_ptr = (void**)param;

    stream_sample_t* stub[0x02] = {NULL, NULL};
    ym2612_update_one(*chip_ptr, stub, 0);
}

static inline uint8_t ym2612_calc_aica_pan(unsigned int* OPN_PAN_CH) {
    //
    // stereo registers have 1bit of resolution
    // enabled=INT_MAX disabled=INT_MIN
    //
    unsigned int L = OPN_PAN_CH[0];
    unsigned int R = OPN_PAN_CH[1];

    if (L && R) {
        return AICA_PAN_TOTAL_CENTER;
    } else if (L) {
        return AICA_PAN_FULL_LEFT;
    } else if (R) {
        return AICA_PAN_FULL_RIGHT;
    } else {
        return AICA_PAN_TOTAL_CENTER;
    }
}


void ym2612_stream_update(int16_t* buffer, int samples) {
    FMSAMPLE fm_buffer_l[FM_BUFFER_SIZE];
    FMSAMPLE fm_buffer_r[FM_BUFFER_SIZE];

    FMSAMPLE* fm_buffer[2] = {fm_buffer_l, fm_buffer_r};

    int16_t* buffer_l = buffer + (samples * 0);
    int16_t* buffer_r = buffer + (samples * 1);

    while (samples > 0) {
        int required = samples < FM_BUFFER_SIZE ? samples : FM_BUFFER_SIZE;
        ym2612_update_one(instance, fm_buffer, required);

        for (int i = 0; i < required; i++) {
            *buffer_l++ = (int16_t)fm_buffer[0][i];
            *buffer_r++ = (int16_t)fm_buffer[1][i];
        }
        samples -= required;
    }
}

void ym2612_stream_update_ex(int16_t** buffer, int buffer_offset, int length, uint8_t* panning) {
    //
    // Multi-channel version of ym2612_update_one() function
    // outputs are the 6 channels of this chip
    //
    YM2612* F2612 = (YM2612*)instance;
    FM_OPN* OPN = &F2612->OPN;
    INT32* out_fm = OPN->out_fm;
    int i;
    INT32 dacout;
    FM_CH* cch[6];
    INT32 pan[6];

    cch[0] = &F2612->CH[0];
    cch[1] = &F2612->CH[1];
    cch[2] = &F2612->CH[2];
    cch[3] = &F2612->CH[3];
    cch[4] = &F2612->CH[4];
    cch[5] = &F2612->CH[5];

    if (!F2612->MuteDAC)
        dacout = F2612->dacout;
    else
        dacout = 0;

    refresh_fc_eg_chan(OPN, cch[0]);
    refresh_fc_eg_chan(OPN, cch[1]);
    if ((OPN->ST.mode & 0xc0)) {
        /* 3SLOT MODE */
        if (cch[2]->SLOT[SLOT1].Incr == -1) {
            refresh_fc_eg_slot(OPN, &cch[2]->SLOT[SLOT1], (int)OPN->SL3.fc[1], OPN->SL3.kcode[1]);
            refresh_fc_eg_slot(OPN, &cch[2]->SLOT[SLOT2], (int)OPN->SL3.fc[2], OPN->SL3.kcode[2]);
            refresh_fc_eg_slot(OPN, &cch[2]->SLOT[SLOT3], (int)OPN->SL3.fc[0], OPN->SL3.kcode[0]);
            refresh_fc_eg_slot(OPN, &cch[2]->SLOT[SLOT4], (int)cch[2]->fc, cch[2]->kcode);
        }
    } else
        refresh_fc_eg_chan(OPN, cch[2]);
    refresh_fc_eg_chan(OPN, cch[3]);
    refresh_fc_eg_chan(OPN, cch[4]);
    refresh_fc_eg_chan(OPN, cch[5]);
    if (!length) {
        update_ssg_eg_channel(&cch[0]->SLOT[SLOT1]);
        update_ssg_eg_channel(&cch[1]->SLOT[SLOT1]);
        update_ssg_eg_channel(&cch[2]->SLOT[SLOT1]);
        update_ssg_eg_channel(&cch[3]->SLOT[SLOT1]);
        update_ssg_eg_channel(&cch[4]->SLOT[SLOT1]);
        update_ssg_eg_channel(&cch[5]->SLOT[SLOT1]);
    }

    /* 6-channel panning values for AICA */
    panning[0] = ym2612_calc_aica_pan(&OPN->pan[0]);
    panning[1] = ym2612_calc_aica_pan(&OPN->pan[2]);
    panning[2] = ym2612_calc_aica_pan(&OPN->pan[4]);
    panning[3] = ym2612_calc_aica_pan(&OPN->pan[6]);
    panning[4] = ym2612_calc_aica_pan(&OPN->pan[8]);
    panning[5] = ym2612_calc_aica_pan(&OPN->pan[10]);

    /* 6-channel panning mask */
    pan[0] = (int)(OPN->pan[0] | OPN->pan[1]);
    pan[1] = (int)(OPN->pan[2] | OPN->pan[3]);
    pan[2] = (int)(OPN->pan[4] | OPN->pan[5]);
    pan[3] = (int)(OPN->pan[6] | OPN->pan[7]);
    pan[4] = (int)(OPN->pan[8] | OPN->pan[9]);
    pan[5] = (int)(OPN->pan[10] | OPN->pan[11]);

    /* buffering */
    for (i = 0; i < length; i++) {
        /* clear outputs */
        out_fm[0] = 0;
        out_fm[1] = 0;
        out_fm[2] = 0;
        out_fm[3] = 0;
        out_fm[4] = 0;
        out_fm[5] = 0;

        /* update SSG-EG output */
        update_ssg_eg_channel(&cch[0]->SLOT[SLOT1]);
        update_ssg_eg_channel(&cch[1]->SLOT[SLOT1]);
        update_ssg_eg_channel(&cch[2]->SLOT[SLOT1]);
        update_ssg_eg_channel(&cch[3]->SLOT[SLOT1]);
        update_ssg_eg_channel(&cch[4]->SLOT[SLOT1]);
        update_ssg_eg_channel(&cch[5]->SLOT[SLOT1]);

        /* calculate FM */
        if (!F2612->dac_test) {
            chan_calc(F2612, OPN, cch[0]);
            chan_calc(F2612, OPN, cch[1]);
            chan_calc(F2612, OPN, cch[2]);
            chan_calc(F2612, OPN, cch[3]);
            chan_calc(F2612, OPN, cch[4]);
            if (F2612->dacen)
                *cch[5]->connect4 += dacout;
            else
                chan_calc(F2612, OPN, cch[5]);
        } else {
            out_fm[0] = out_fm[1] = dacout;
            out_fm[2] = out_fm[3] = dacout;
            out_fm[5] = dacout;
        }

        /* advance LFO */
        advance_lfo(OPN);

        /* advance envelope generator */
        OPN->eg_timer += OPN->eg_timer_add;
        while (OPN->eg_timer >= OPN->eg_timer_overflow) {
            OPN->eg_timer -= OPN->eg_timer_overflow;
            OPN->eg_cnt++;

            advance_eg_channel(OPN, &cch[0]->SLOT[SLOT1]);
            advance_eg_channel(OPN, &cch[1]->SLOT[SLOT1]);
            advance_eg_channel(OPN, &cch[2]->SLOT[SLOT1]);
            advance_eg_channel(OPN, &cch[3]->SLOT[SLOT1]);
            advance_eg_channel(OPN, &cch[4]->SLOT[SLOT1]);
            advance_eg_channel(OPN, &cch[5]->SLOT[SLOT1]);
        }

        if (out_fm[0] > 8192)
            out_fm[0] = 8192;
        else if (out_fm[0] < -8192)
            out_fm[0] = -8192;
        if (out_fm[1] > 8192)
            out_fm[1] = 8192;
        else if (out_fm[1] < -8192)
            out_fm[1] = -8192;
        if (out_fm[2] > 8192)
            out_fm[2] = 8192;
        else if (out_fm[2] < -8192)
            out_fm[2] = -8192;
        if (out_fm[3] > 8192)
            out_fm[3] = 8192;
        else if (out_fm[3] < -8192)
            out_fm[3] = -8192;
        if (out_fm[4] > 8192)
            out_fm[4] = 8192;
        else if (out_fm[4] < -8192)
            out_fm[4] = -8192;
        if (out_fm[5] > 8192)
            out_fm[5] = 8192;
        else if (out_fm[5] < -8192)
            out_fm[5] = -8192;

        /* 6-channels output */
        buffer[0][buffer_offset + i] = (int16_t)(pan[0] & out_fm[0]);
        buffer[1][buffer_offset + i] = (int16_t)(pan[1] & out_fm[1]);
        buffer[2][buffer_offset + i] = (int16_t)(pan[2] & out_fm[2]);
        buffer[3][buffer_offset + i] = (int16_t)(pan[3] & out_fm[3]);
        buffer[4][buffer_offset + i] = (int16_t)(pan[4] & out_fm[4]);
        buffer[5][buffer_offset + i] = (int16_t)(pan[5] & out_fm[5]);

        /* CSM mode: if CSM Key ON has occured, CSM Key OFF need to be sent       */
        /* only if Timer A does not overflow again (i.e CSM Key ON not set again) */
        OPN->SL3.key_csm <<= 1;

        /* timer A control */
        // INTERNAL_TIMER_A( &OPN->ST , cch[2] )
        {
            if (OPN->ST.TAC && (OPN->ST.timer_handler == 0))
                if ((OPN->ST.TAC -= (int)(OPN->ST.freqbase * 4096)) <= 0) {
                    TimerAOver(&OPN->ST);
                    // CSM mode total level latch and auto key on
                    if (OPN->ST.mode & 0x80)
                        CSMKeyControll(OPN, cch[2]);
                }
        }

        if (OPN->SL3.key_csm & 2) {
            /* CSM Mode Key OFF (verified by Nemesis on real hardware) */
            FM_KEYOFF_CSM(cch[2], SLOT1);
            FM_KEYOFF_CSM(cch[2], SLOT2);
            FM_KEYOFF_CSM(cch[2], SLOT3);
            FM_KEYOFF_CSM(cch[2], SLOT4);
            OPN->SL3.key_csm = 0;
        }
    }
}

int device_start_ym2612(int clock) {
    clock &= 0x3fffffff;

    int rate = clock / 72;
    if (!(instance_flags & 0x04)) {
        rate /= 2;
    }

    instance = ym2612_init(&instance, clock, rate, NULL, NULL);

    return rate;
}

void device_stop_ym2612() {
    ym2612_shutdown(instance);
}

void device_reset_ym2612() {
    ym2612_reset_chip(instance);
}

void ym2612_w(offs_t offset, UINT8 data) {
    ym2612_write(instance, offset & 3, data);
}

void ym2612_set_options(UINT8 Flags) {
    instance_flags = Flags;
    ym2612_setoptions(Flags);
}

void ym2612_set_mute_mask(UINT32 MuteMask) {
    ym2612_set_mutemask(instance, MuteMask);
}
