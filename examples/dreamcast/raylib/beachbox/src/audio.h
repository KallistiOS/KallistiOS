/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/audio.h
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#ifndef BBOX_AUDIO_H
#define BBOX_AUDIO_H

#include <stdint.h>

// Initializes the game's audio
void init_audio(void);

// Deinitializes the game's audio
void deinit_audio(void);

// Changes songs when you switch scenes
void update_song(void);

// Returns the music volume
uint8_t get_music_volume(void);

// Sets the music volume to the given value
void set_music_volume(uint8_t volume);

// Increases music volume by 1
void increment_music_volume(void);

// Decreases music volume by 1
void decrement_music_volume(void);

// Returns the sound effect volume
uint8_t get_sfx_volume(void);

// Sets the sound effect volume to the given value
void set_sfx_volume(uint8_t volume);

// Increases sfx volume by 1
void increment_sfx_volume(void);

// Decreases sfx volume by 1
void decrement_sfx_volume(void);

// Plays the sound effect for menu movement
void play_sfx_menu_move(void);

// Plays the sound effect for 'clicking' a button
void play_sfx_menu_select(void);

// Plays the sound effect for 'clicking' a disabled button
void play_sfx_menu_error(void);

// Plays the sound effect for picking up a coin
void play_sfx_coin(void);

// Plays the sound effect for activating the slowdown power
void play_sfx_slowdown_activate(void);

// Plays the sound effect for deactivating the slowdown power
void play_sfx_slowdown_deactivate(void);

// Plays the sound effect for jumping
void play_sfx_game_over(void);

// Plays the sound effect for teleporting
void play_sfx_teleport(void);

#endif