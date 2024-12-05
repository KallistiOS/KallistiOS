/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/audio.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <adx/adx.h>
#include <adx/snddrv.h>
#include <dc/sound/sfxmgr.h>
#include <dc/sound/sound.h>
#include <dc/sound/stream.h>

#include "audio.h"
#include "assert.h"
#include "scene.h"
#include "save.h"

// Both go from 0-24
// So we initialize to roughly 50%
static uint8_t music_volume_ = 12;
static uint8_t sfx_volume_   = 120; // NOTE: This internally goes from 0-240, but we divide/multiply by 10 on
                                    // the getters / setters so it's consistent with music volume.

static sfxhnd_t sfx_menu_move_;
static sfxhnd_t sfx_menu_select_;
static sfxhnd_t sfx_menu_error_;
static sfxhnd_t sfx_coin_;
static sfxhnd_t sfx_slowdown_;
static sfxhnd_t sfx_slowdown_back_;
static sfxhnd_t sfx_teleport_;
static sfxhnd_t sfx_gameover_;

void init_audio(void) {
    snd_stream_init();
    sfx_menu_move_     = snd_sfx_load("/rd/audio/menu_move.wav");
    sfx_menu_select_   = snd_sfx_load("/rd/audio/menu_select.wav");
    sfx_menu_error_    = snd_sfx_load("/rd/audio/menu_error.wav");
    sfx_coin_          = snd_sfx_load("/rd/audio/coin.wav");
    sfx_teleport_      = snd_sfx_load("/rd/audio/teleport.wav");
    sfx_gameover_      = snd_sfx_load("/rd/audio/gameover.wav");
    sfx_slowdown_      = snd_sfx_load("/rd/audio/slowdown.wav");
    sfx_slowdown_back_ = snd_sfx_load("/rd/audio/slowdown_back.wav");
}

void deinit_audio(void) {
    snd_stream_shutdown();
    snd_sfx_unload_all();
}

///// Music //////

static enum Song : uint8_t { NIL_SONG, MENU_SONG, GAME_SONG, CREDITS_SONG } current_song = NIL_SONG;

static void play_song(void) {
    adx_stop();
    if (music_volume_ == 0) return;
    int result = -1;
    switch (current_song) {
        case NIL_SONG:
            break;

        case MENU_SONG:
            result = adx_dec("/rd/audio/menu_song.adx", 1);
            assert_msg(result == 1, "menu_song.adx could not be loaded");
            break;

        case GAME_SONG:
            result = adx_dec("/rd/audio/gamescene.adx", 1);
            assert_msg(result == 1, "gamescene.adx could not be loaded");
            break;

        case CREDITS_SONG:
            result = adx_dec("/rd/audio/credits.adx", 1);
            assert_msg(result == 1, "credits.adx could not be loaded");
            break;

        default:
            break;
    }
}

static void update_music_volume(void) {
    assert_msg(music_volume_ < 25 && music_volume_ >= 0, "Music volume is not between 0 and 24");

    // First, set the volume to 0
    for (int i = 0; i < 25; i++) {
        snddrv_volume_down();
    }

    if (!music_volume_ == 0) {
        // Then, set the volume to the current music volume
        for (int i = 0; i < music_volume_; i++) {
            snddrv_volume_up();
        }
    }
}

void update_song(void) {
    if (music_volume_ == 0) return;

    const enum Song prev_song = current_song;
    switch (get_current_scene()) {
        case RAYLOGO:
            current_song = NIL_SONG;
            break;
        case LOADING:
        case MAINMENU:
        case SHOP:
        case UNLOCKABLES:
        case OPTIONS:
            current_song = MENU_SONG;
            break;
        case GAME:
            current_song = GAME_SONG;
            break;
        case CREDITS:
            current_song = CREDITS_SONG;
            break;
    }

    if (prev_song != current_song) {
        play_song();

        // Wait for the song to start
        while (snddrv.drv_status != SNDDRV_STATUS_STREAMING) {
            thd_sleep(1);
        }

        // Set the music volume to music_volume_
        if (snddrv.drv_status != SNDDRV_STATUS_NULL) {
            update_music_volume();
        }
    }
}

uint8_t get_music_volume(void) {
    return music_volume_;
}

void set_music_volume(uint8_t volume) {
    music_volume_ = volume;
    update_music_volume();
}

void increment_music_volume(void) {
    const uint8_t prev_music_volume = music_volume_;

    if (music_volume_ < 24) {
        music_volume_++;
        snddrv_volume_up();
    }

    if (prev_music_volume == 0) {
        update_song(); // If the music volume was 0, we need to start playing the song
        adx_resume();
    }
}

void decrement_music_volume(void) {
    if (music_volume_ > 0) {
        music_volume_--;
        snddrv_volume_down();
    }

    if (music_volume_ <= 0) {
        adx_pause();
    }
}

///// SFX //////

uint8_t get_sfx_volume(void) {
    return (sfx_volume_ / 10);
}

void set_sfx_volume(uint8_t volume) {
    sfx_volume_ = volume * 10;
}

void increment_sfx_volume(void) {
    if (sfx_volume_ < 240) {
        sfx_volume_ += 10;
    }
}

void decrement_sfx_volume(void) {
    if (sfx_volume_ > 0) {
        sfx_volume_ -= 10;
    }
}

static constexpr int pan_center_ = 128;

void play_sfx_menu_move(void) {
    snd_sfx_play_chn(4, sfx_menu_move_, sfx_volume_, pan_center_);
}

void play_sfx_menu_select(void) {
    snd_sfx_play_chn(4, sfx_menu_select_, sfx_volume_, pan_center_);
}

void play_sfx_menu_error(void) {
    snd_sfx_play_chn(4, sfx_menu_error_, sfx_volume_, pan_center_);
}

void play_sfx_coin(void) {
    snd_sfx_play_chn(2, sfx_coin_, sfx_volume_, pan_center_);
}

void play_sfx_slowdown_activate(void) {
    snd_sfx_play_chn(4, sfx_slowdown_, sfx_volume_, pan_center_);
}

void play_sfx_slowdown_deactivate(void) {
    snd_sfx_play_chn(4, sfx_slowdown_back_, sfx_volume_, pan_center_);
}

void play_sfx_game_over(void) {
    snd_sfx_play_chn(4, sfx_gameover_, sfx_volume_, pan_center_);
}

void play_sfx_teleport(void) {
    snd_sfx_play_chn(4, sfx_teleport_, sfx_volume_, pan_center_);
}