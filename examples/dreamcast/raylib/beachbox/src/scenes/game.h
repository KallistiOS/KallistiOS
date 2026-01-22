/* KallistiOS ##version##
   examples/dreamcast/raylib/beachbox/src/scenes/game.h
   Copyright (C) 2024 Agustín Bellagamba
   Copyright (C) 2024 Cypress
*/

#ifndef BBOX_GAME_H
#define BBOX_GAME_H

#include <stdint.h>

// Initializes the game scene
void init_game_scene(void);

// Updates the game scene
void update_game_scene(void);

// Draws the game scene
void draw_game_scene(void);

// Ends the current run
void end_game(void);

// Returns true if the game is over
bool is_game_over(void);

// Returns true if the game is paused
bool is_game_paused(void);

// Toggles the game pause
void toggle_pause(void);

// Returns true if a new high score was reached
bool is_new_high_score(void);

// Returns the coins earned in the current run
uint16_t get_current_coins(void);

// Adds a coin to the current run's score
void add_coin(void);

#endif