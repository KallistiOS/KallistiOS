/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/audio.c
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#include <dc/sound/sfxmgr.h>
#include <dc/sound/sound.h>
#include <dc/sound/stream.h>
#include <kos/thread.h>
#include <wav/sndwav.h>

#include "audio.h"
#include "assert.h"
#include "scene.h"
#include "save.h"

static uint8_t music_volume_ = 255;
static uint8_t sfx_volume_   = 255;

static sfxhnd_t         sfx_menu_move_;
static sfxhnd_t         sfx_menu_select_;
static sfxhnd_t         sfx_menu_error_;
static sfxhnd_t         sfx_coin_;
static sfxhnd_t         sfx_slowdown_;
static sfxhnd_t         sfx_slowdown_back_;
static sfxhnd_t         sfx_teleport_;
static sfxhnd_t         sfx_gameover_;
static wav_stream_hnd_t song_menu_;
static wav_stream_hnd_t song_game_;
static wav_stream_hnd_t song_credits_;

#define LOOP 1
void init_audio(void) {
    snd_stream_init();
    wav_init();
    sfx_menu_move_     = snd_sfx_load("/rd/audio/menu_move.wav");
    sfx_menu_select_   = snd_sfx_load("/rd/audio/menu_select.wav");
    sfx_menu_error_    = snd_sfx_load("/rd/audio/menu_error.wav");
    sfx_coin_          = snd_sfx_load("/rd/audio/coin.wav");
    sfx_teleport_      = snd_sfx_load("/rd/audio/teleport.wav");
    sfx_gameover_      = snd_sfx_load("/rd/audio/gameover.wav");
    sfx_slowdown_      = snd_sfx_load("/rd/audio/slowdown.wav");
    sfx_slowdown_back_ = snd_sfx_load("/rd/audio/slowdown_back.wav");

    song_menu_    = wav_create("/rd/audio/song_menu.wav", LOOP);
    song_game_    = wav_create("/rd/audio/song_game.wav", LOOP);
    song_credits_ = wav_create("/rd/audio/song_credits.wav", LOOP);
}
#undef LOOP

void deinit_audio(void) {
    snd_stream_shutdown();
    snd_sfx_unload_all();
    wav_destroy(song_menu_);
    wav_destroy(song_game_);
    wav_destroy(song_credits_);
    wav_shutdown();
}

///// Music //////

static wav_stream_hnd_t *current_song_ = nullptr;

static void update_music_volume(void) {
    assert_msg(music_volume_ <= 255 && music_volume_ >= 0, "Music volume is not between 0 and 255");
    wav_volume(*current_song_, music_volume_);
}

static void play_song(void) {
    if (current_song_ == nullptr) return;
    if (music_volume_ == 0) return;

    update_music_volume();
    wav_play(*current_song_);
}

void update_song(void) {
    if (music_volume_ == 0) return;

    const wav_stream_hnd_t *prev_song = current_song_;

    switch (get_current_scene()) {
        case RAYLOGO:
            current_song_ = nullptr;
            break;
        case LOADING:
        case MAINMENU:
        case SHOP:
        case UNLOCKABLES:
        case OPTIONS:
            current_song_ = &song_menu_;
            break;
        case GAME:
            current_song_ = &song_game_;
            break;
        case CREDITS:
            current_song_ = &song_credits_;
            break;
    }

    if (prev_song != current_song_) {
        wav_stop(*prev_song);
        play_song();
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
    if (music_volume_ < 255) {
        music_volume_ += 17;
        update_music_volume();
    }
}

void decrement_music_volume(void) {
    if (music_volume_ > 0) {
        music_volume_ -= 17;
        update_music_volume();
    }
}

///// SFX //////

uint8_t get_sfx_volume(void) {
    return sfx_volume_;
}

void set_sfx_volume(uint8_t volume) {
    sfx_volume_ = volume;
}

void increment_sfx_volume(void) {
    if (sfx_volume_ < 255) {
        sfx_volume_ += 17;
    }
}

void decrement_sfx_volume(void) {
    if (sfx_volume_ > 0) {
        sfx_volume_ -= 17;
    }
}

#define PAN_CENTER 128
void play_sfx_menu_move(void) {
    snd_sfx_play_chn(6, sfx_menu_move_, sfx_volume_, PAN_CENTER);
}

void play_sfx_menu_select(void) {
    snd_sfx_play_chn(4, sfx_menu_select_, sfx_volume_, PAN_CENTER);
}

void play_sfx_menu_error(void) {
    snd_sfx_play_chn(4, sfx_menu_error_, sfx_volume_, PAN_CENTER);
}

void play_sfx_coin(void) {
    snd_sfx_play_chn(0, sfx_coin_, sfx_volume_, PAN_CENTER);
}

void play_sfx_slowdown_activate(void) {
    snd_sfx_play_chn(4, sfx_slowdown_, sfx_volume_, PAN_CENTER);
}

void play_sfx_slowdown_deactivate(void) {
    snd_sfx_play_chn(4, sfx_slowdown_back_, sfx_volume_, PAN_CENTER);
}

void play_sfx_game_over(void) {
    snd_sfx_play_chn(4, sfx_gameover_, sfx_volume_, PAN_CENTER);
}

void play_sfx_teleport(void) {
    snd_sfx_play_chn(6, sfx_teleport_, sfx_volume_, PAN_CENTER);
}
#undef PAN_CENTER